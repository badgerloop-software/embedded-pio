#ifndef __CAN_MANAGER_H__
#define __CAN_MANAGER_H__

#include <memory> // need this for shared_ptr
#include <queue>
#include "STM32_CAN.h"

#define DEFAULT_CAN_FREQ 250000     // Match Elcon charger

typedef struct {
    uint32_t id;
    uint8_t buf[8];  
    uint8_t len;
} CAN_data;

class CANManager {
    private:
        std::queue<CAN_data> messageQueue; // CAN messages we read
        STM32_CAN canBus;                  // object to interface with CAN
        
        // Interrupt Service Routine/Handler to read CAN message and put the data in a queue
        void readISR(); 

    public:
        /* Constructor initializing bus and all manager functions
         *
         * canPort: choose from CAN1, CAN2, CAN3 (NOTE: see STM32_CAN library for more details)
         * pins: choose from DEF, ALT1, ALT2
         * frequency: Baud rate of can bus
         * rx: digital pin number to attach interrupt to
         */
        CANManager(CAN_TypeDef* canPort, CAN_PINS pins, uint8_t rx, int frequency = DEFAULT_CAN_FREQ);

        // Destructor stopping manager and freeing resources
        ~CANManager();

        /* Reads input message and does any logic handling needed
         * Intended to be implemented by class extension per board
         */
        virtual void readHandler(CAN_data msg) = 0;

        /* Send a message over CAN
         *
         * messageID: CAN ID to use to identify the signal
         * data: Payload array
         * length: Size of data in bytes
         * timeout: in milliseconds
         */ 
        int sendMessage(int messageID, void* data, int length, int timeout = 10);

        /* Processes CAN (read) messages stored in messageQueue for a set duration. 
         * THIS IS THE FUNCTION TO CALL FOR PROCESSING CAN READ MESSAGES
         * 
         * duration: time in milliseconds
         */
        void runQueue(int duration);
};


#endif