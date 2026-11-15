#include "coordinator.h"
#include <zephyr/kernel.h>

#define EVT_ETH_ACTIVE     BIT(0)
#define EVT_PAYLOAD_READY  BIT(1)
#define EVT_CAN_TRIGGER    BIT(2)
#define EVT_CAN_ACTIVE     BIT(3)
#define EVT_CAN_DONE       BIT(4)

K_EVENT_DEFINE(updateEvents);

void ethUpdateStarted() {
    k_event_post(&updateEvents, EVT_ETH_ACTIVE);
}

void ethUpdateFinished() {
    k_event_clear(&updateEvents, EVT_ETH_ACTIVE);
}

void signalPayloadReady() {
    k_event_post(&updateEvents, EVT_PAYLOAD_READY);
}

bool canTransferActive() {
    return (k_event_wait(&updateEvents, EVT_CAN_ACTIVE, false, K_NO_WAIT) != 0);
}

void waitForPayload() {
    k_event_wait(&updateEvents, EVT_PAYLOAD_READY, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_PAYLOAD_READY);
}

void beginCanSend() {
    k_event_post(&updateEvents, EVT_CAN_ACTIVE | EVT_CAN_TRIGGER);
}

void waitCanSendDone() {
    k_event_wait(&updateEvents, EVT_CAN_DONE, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_CAN_DONE);
}

void waitCanTrigger() {
    k_event_wait(&updateEvents, EVT_CAN_TRIGGER, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_CAN_TRIGGER);
}

void canSendComplete() {
    k_event_clear(&updateEvents, EVT_CAN_ACTIVE);
    k_event_post(&updateEvents, EVT_CAN_DONE);
}
