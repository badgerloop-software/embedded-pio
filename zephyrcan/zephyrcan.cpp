#include "zephyrcan.h"

#ifdef USING_CAN_FD_AND_BITRATE_SWITCHING
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF | CAN_FRAME_BRS;
#elif defined(USING_CAN_FD)
    static const uint8_t _sendMessageFlag = CAN_FRAME_FDF;
#else
    static const uint8_t _sendMessageFlag = 0;
#endif

#define RX_STACK_SIZE 2048
#define RX_PRIORITY   7

K_THREAD_STACK_DEFINE(zephyrcan_rx_stack, RX_STACK_SIZE);

ZephyrCAN::ZephyrCAN(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    _canDevice(canDevice), _canStatus(CAN_OK)
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

        if (_canStatus == CAN_OK && can_set_bitrate(canDevice, frequency) < 0) {
            _canStatus = FAILED_TO_SET_CAN_BITRATE;
        }
    }
    else
    {
        _canStatus = DEVICE_NOT_READY;
    }
}

ZephyrCAN::~ZephyrCAN() {
    k_thread_abort(&_rxTask);
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
    if (_canStatus == CAN_OK && can_start(_canDevice) < 0) {
        _canStatus = FAILED_TO_START_CAN;
    }
    else {
        k_thread_create(&_rxTask, zephyrcan_rx_stack,
                        K_THREAD_STACK_SIZEOF(zephyrcan_rx_stack),
                        rxTaskBridge, this, NULL, NULL,
                        RX_PRIORITY, 0, K_NO_WAIT);
    }

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
