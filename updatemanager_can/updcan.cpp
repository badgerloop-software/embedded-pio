#include "updcan.h"
#include <zephyr/logging/log.h>

UpdCAN::UpdCAN(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    ZephyrCAN(canDevice, targetIDList, targetIDListSize, frequency) {
    
    _droppedFrames = 0;
    k_msgq_init(&_updateCanMsgq, (char *)_updateCanMsgqBuffer, 
                sizeof(struct can_frame), UPDATE_QUEUE_DEPTH);
}

void UpdCAN::readHandler(struct can_frame * msg) {
    //If a message arrives here just pretend its for the update
    //In an ideal world the filters should only let update IDs through
    //Sequential processing to make the file transfer simple.

    if (k_msgq_put(&_updateCanMsgq, msg, K_NO_WAIT) != 0) {
        _droppedFrames++;
    }
}

int UpdCAN::droppedFrames() const { 
    return _droppedFrames; 
}

int UpdCAN::readNextMessage(struct can_frame * msg, k_timeout_t timeout) {
    return k_msgq_get(&_updateCanMsgq, msg, timeout);
}

int UpdCAN::sendUpdateData() {
    return 0;
}

int UpdCAN::sendACK(uint8_t code){
    return sendMessage(ACK_ID, &code, sizeof(uint8_t));
}
