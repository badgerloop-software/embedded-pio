#include "canmanager.h"


void CANManager::readISR() {
    // Read CAN message and enqueue data
    CAN_message_t CAN_message;
    bool check = this->canBus.read(CAN_message);  //returns true if message is read successfully
    
    if (check){
        CAN_data toQueue= {CAN_message.id, *CAN_message.buf, CAN_message.len}; //Dereferencing CAN_message.buf to convert it to uint8_t from uint8_t*
        this->messageQueue.push(toQueue);
    }
} 

CANManager::CANManager(CAN_TypeDef* canPort, CAN_PINS pins, uint8_t rx, int frequency) : canBus(canPort, pins), messageQueue() {
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

    //Empties queue
    while (!this->messageQueue.empty()) {
        this->messageQueue.pop();
    }
}

int CANManager::sendMessage(int messageID, void* data, int length, int timeout) {
    // Storing the time when the function is called
    unsigned long start = millis();
    
    bool retValue = false;                  //return value of write function. False by default

    // Create a CAN message that is going to be written
    CAN_message_t CAN_message;                
    CAN_message.id = messageID;
    CAN_message.len = length;

    // Copy the data to the buffer
    for (int i = 0; i < length; i++) {
        CAN_message.buf[i] = ((uint8_t*)data)[i];
    }

    // Keep trying to write the message until it is successful or timeout
    while (!(retValue = this->canBus.write(CAN_message)) && millis() - start < timeout){
        this->runQueue(1);
    }

    return retValue;  //returns true if message is written successfully
}

void CANManager::runQueue(int duration) {
    // Storing the time when the function is called
    unsigned long start = millis();

    // dequeues messageQueue until the duration is reached
    while (millis() - start < duration){
        if (!this->messageQueue.empty()) {
            CAN_data msg = this->messageQueue.front();
            this->messageQueue.pop();
            this->readHandler(msg);    //calls readHandler function to process the message              
        }
    }   
}