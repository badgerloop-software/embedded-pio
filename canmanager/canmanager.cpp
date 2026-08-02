#include "canmanager.h"

CANManager::CANManager(CAN_TypeDef* canPort, CAN_PINS pins, int frequency) : canBus(canPort, pins) {
    this->canBus.begin();
    this->frequency = frequency;
    this->canBus.setBaudRate(frequency);
    this->canBus.setAutoBusOffRecovery(true);
}

bool CANManager::sendMessage(int messageID, void* data, int length, int timeout) {
    bool retValue = false;

    CAN_message_t CAN_message;
    CAN_message.id = messageID;
    CAN_message.len = length;

    for (int i = 0; i < length; i++) {
        CAN_message.buf[i] = ((uint8_t*)data)[i];
    }

    unsigned long start = millis();

    while (!(retValue = this->canBus.write(CAN_message)) && millis() - start < timeout){
        this->runQueue(1);
    }

    return retValue;
}

void CANManager::runQueue(int duration) {
    CAN_message_t msg;

    unsigned long start = millis();

    while (millis() - start < duration){
        if (this->canBus.read(msg)) {
            this->readHandler(msg);
        }
    }
}

void CANManager::reset() {
    this->canBus.end();
    this->canBus.begin();
    this->canBus.setBaudRate(this->frequency);
    this->canBus.setAutoBusOffRecovery(true);
}
