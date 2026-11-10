#ifndef UPDCAN_H
#define UPDCAN_H

#include "zephyrcan.h"

#define UPDATE_ID   0x00
#define ACK_ID      0x01
#define UPDATE_QUEUE_DEPTH  8

static const uint32_t CAN_FREQUENCY = 250000;

class UpdCAN : public ZephyrCAN {
    public:
        UpdCAN(const struct device *canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency = CAN_FREQUENCY);
        //Use it as a **readNextMessage**
        void readHandler(struct can_frame * msg);
        int sendUpdateData();
        int sendACK(uint8_t code);
        int readNextMessage(struct can_frame * msg, k_timeout_t timeout);
        int droppedFrames() const;
    
    private:
        struct k_msgq _updateCanMsgq;
        struct can_frame _updateCanMsgqBuffer[UPDATE_QUEUE_DEPTH];
        int _droppedFrames;
};

#endif
