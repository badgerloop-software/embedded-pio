#include "canmanager.h"


void CANManager::readISR() {
    // TODO: read CAN message and enqueue data
} 

CANManager::CANManager(CAN_TypeDef* canPort, CAN_PINS pins, int frequency, uint8_t rx) : canBus(canPort, pins), messageQueue() {
    // Attach RX pin and handler function to interrupt
    attachInterrupt(digitalPinToInterrupt(rx), [this]() { this->readISR(); }, RISING || FALLING);   //attaches an interrupt to handler (readISR) using lambda function and calls it on rising or falling edge
    this->canBus.enableMBInterrupts();                                                              //enables mailbox interrupts

    // Begin canBus and set its frequency
    this->canBus.begin();
    this->canBus.setBaudRate(frequency);
}

CANManager::~CANManager() {
    //Clear interrupt handlers
    this->canBus.disableMBInterrupts();
}

int CANManager::sendMessage(int messageID, void* data, int length, int timeout) {
    // TODO: use a Ticker to handle the timeout
    unsigned long start = millis();
    // TODO: make sure to runQueue in between attempts to send.
    boolean retValue = false;
    CAN_message_t CAN_message;
    CAN_message.id = messageID;
    CAN_message.len = length;
    for (int i = 0; i < length; i++) {
        CAN_message.buf[i] = ((uint8_t*)data)[i];
    }
    while (!(retValue = this->canBus.write(CAN_message)) && millis() - start < timeout){
        this->runQueue(1);
    }
    return retValue;
}

void CANManager::runQueue(int duration) {
    // TODO: use Ticker to handle the duration
    unsigned long start = millis();

    // TODO: dequeue a CAN message, and call readHandler to deal with the CAN message accordingly
    while (millis() - start < duration){
        if (!this->messageQueue.empty()) {
            CAN_data msg = this->messageQueue.front();
            this->messageQueue.pop();
            this->readHandler(msg);                 //virtual void readHandler needed in header maybe?
        }
    }   
}