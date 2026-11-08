#ifndef ZEPHYRCAN_H
#define ZEPHYRCAN_H

#include <stdint.h>
#include <stddef.h>
#include <vector>

using namespace std;

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

#define CAN_RX_QUEUE_DEPTH 16

typedef enum {
    CAN_OK = 0,
    FAILED_TO_ADD_CAN_CALLBACK = 1,
    FAILED_TO_START_CAN = 2,
    FAILED_TO_SET_CAN_BITRATE = 3,
    DEVICE_NOT_READY = 4,
} CanErrorCode;

class ZephyrCAN {
    public:
        ZephyrCAN(const struct device *canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency);
        ~ZephyrCAN();

        virtual void readHandler(struct can_frame * msg) = 0;
        int sendMessage(uint32_t messageID, const uint8_t * data, uint8_t length, int timeout = 10);
        
        CanErrorCode canStatus() const;
        CanErrorCode begin();

    private:
        void rxLoop();
        static void rxTaskBridge(void * p1, void * p2, void * p3);

        const struct device *const _canDevice;
        CanErrorCode _canStatus;
        vector<int> _filterIDList;
        struct k_msgq _rxMsgQ;
        struct can_frame _rxMsgQBuffer[CAN_RX_QUEUE_DEPTH];
        struct k_thread _rxTask;
};

#endif
