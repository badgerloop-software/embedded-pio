#include "canmanager.h"


void CANManager::readISR() {
    // TODO: read CAN message and enqueue data
} 

CANManager::CANManager(CAN_TypeDef* canPort, CAN_PINS pins, int frequency) : canBus(canPort, pins), messageQueue() {
    // TODO: attach RX pin and handler function to interrupt
    
    // TODO: begin canBus and set its frequency

}

CANManager::~CANManager() {

}

int CANManager::sendMessage(int messageID, void* data, int length, int timeout) {
    // TODO: use a Ticker to handle the timeout

    // TODO: make sure to runQueue in between attempts to send.
    return 0;
}

void CANManager::runQueue(int duration) {
    // TODO: use Ticker to handle the duration

    // TODO: dequeue a CAN message, and call readHandler to deal with the CAN message accordingly

}