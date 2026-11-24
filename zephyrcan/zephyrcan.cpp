#include "zephyrcan.h"
#include <errno.h>
#include <string.h>
#include <zephyr/sys/atomic.h>

#ifdef USING_CAN_FD_AND_BITRATE_SWITCHING
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF | CAN_FRAME_BRS;
#elif defined(USING_CAN_FD)
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF;
#else
    static const uint8_t _sendMessageFlag = 0;
#endif

#define RX_STACK_SIZE  2048
#define RX_PRIORITY    7
#define RX_STACK_COUNT 4

K_THREAD_STACK_ARRAY_DEFINE(zephyrcan_rx_stacks, RX_STACK_COUNT, RX_STACK_SIZE);

static atomic_t zephyrcanRxStackUsed = ATOMIC_INIT(0);

static int claimRxStack() {
    for (int i = 0; i < RX_STACK_COUNT; i++) {
        if (!atomic_test_and_set_bit(&zephyrcanRxStackUsed, i)) {
            return i;
        }
    }

    return -1;
}

static void releaseRxStack(int index) {
    atomic_clear_bit(&zephyrcanRxStackUsed, index);
}

ZephyrCAN::ZephyrCAN(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    _canDevice(canDevice), _canStatus(CAN_OK), _rxStackIndex(-1)
{
    k_msgq_init(&_rxMsgQ, (char *)_rxMsgQBuffer, sizeof(struct can_frame), CAN_RX_QUEUE_DEPTH);

    if (device_is_ready(_canDevice))
    {
        size_t i = 0;

        while (_canStatus == CAN_OK && i < targetIDListSize) {

            struct can_filter filter = {
                .id = targetIDList[i],
                .mask = CAN_STD_ID_MASK, 
                .flags = 0,
            };

            int response = can_add_rx_filter_msgq(_canDevice, &_rxMsgQ, &filter);
            
            if(response < 0)
            {
                _canStatus = FAILED_TO_ADD_CAN_CALLBACK;
            }
            else
            {
                _filterIDList.push_back(response);
            }

            i++;
        }

        if (_canStatus == CAN_OK) {
            int response = can_set_bitrate(canDevice, frequency);

            if (response < 0 && response != -EBUSY) {
                _canStatus = FAILED_TO_SET_CAN_BITRATE;
            }
        }
    }
    else
    {
        _canStatus = DEVICE_NOT_READY;
    }
}

ZephyrCAN::~ZephyrCAN() {
    if (_rxStackIndex >= 0) {
        k_thread_abort(&_rxTask);
        releaseRxStack(_rxStackIndex);
    }

    for (auto const id : _filterIDList) {
        can_remove_rx_filter(_canDevice, id);
    }
}

void ZephyrCAN::rxTaskBridge(void *p1, void *p2, void *p3) {
    static_cast<ZephyrCAN *>(p1)->rxLoop();
}

void ZephyrCAN::rxLoop() {
    struct can_frame frame;
    while (1) {
        if (k_msgq_get(&_rxMsgQ, &frame, K_FOREVER) == 0) {
            readHandler(&frame);
        }
    }
}

CanErrorCode ZephyrCAN::begin() {
    if (_canStatus != CAN_OK || _rxStackIndex >= 0) {
        return _canStatus;
    }

    int response = can_start(_canDevice);

    if (response < 0 && response != -EALREADY) {
        _canStatus = FAILED_TO_START_CAN;
        return _canStatus;
    }

    _rxStackIndex = claimRxStack();

    if (_rxStackIndex < 0) {
        _canStatus = FAILED_TO_ALLOCATE_RX_STACK;
        return _canStatus;
    }

    k_thread_create(&_rxTask, zephyrcan_rx_stacks[_rxStackIndex],
                    K_THREAD_STACK_SIZEOF(zephyrcan_rx_stacks[_rxStackIndex]),
                    rxTaskBridge, this, NULL, NULL,
                    RX_PRIORITY, 0, K_NO_WAIT);

    return _canStatus;
}

CanErrorCode ZephyrCAN::canStatus() const{
    return _canStatus;
}

int ZephyrCAN::sendMessage(uint32_t messageID, const uint8_t * data, uint8_t length, int timeout)
{
    struct can_frame frame;

    if (length > sizeof(frame.data))
    {
        length = sizeof(frame.data);
    }

    frame.id = messageID;
    frame.flags = _sendMessageFlag;
    frame.dlc = can_bytes_to_dlc(length);
    memcpy(frame.data, data, length);

    return can_send(_canDevice, &frame, K_MSEC(timeout), nullptr, nullptr);
}
