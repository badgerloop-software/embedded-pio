#include "coordinator.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define EVT_ETH_ACTIVE     BIT(0)
#define EVT_PAYLOAD_READY  BIT(1)
#define EVT_CAN_TRIGGER    BIT(2)
#define EVT_CAN_ACTIVE     BIT(3)
#define EVT_CAN_DONE       BIT(4)

K_EVENT_DEFINE(updateEvents);

static atomic_t payloadBoard = ATOMIC_INIT(CAN_BREAKOUT_BOARD);
static atomic_t canTargetBoard = ATOMIC_INIT(CAN_BREAKOUT_BOARD);

void ethUpdateStarted() {
    k_event_post(&updateEvents, EVT_ETH_ACTIVE);
}

void ethUpdateFinished() {
    k_event_clear(&updateEvents, EVT_ETH_ACTIVE);
}

void signalPayloadReady(board_id_t boardId) {
    atomic_set(&payloadBoard, boardId);
    k_event_post(&updateEvents, EVT_PAYLOAD_READY);
}

bool canTransferActive() {
    return (k_event_wait(&updateEvents, EVT_CAN_ACTIVE, false, K_NO_WAIT) != 0);
}

board_id_t waitForPayload() {
    k_event_wait(&updateEvents, EVT_PAYLOAD_READY, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_PAYLOAD_READY);
    return (board_id_t)atomic_get(&payloadBoard);
}

void beginCanSend(board_id_t boardId) {
    atomic_set(&canTargetBoard, boardId);
    k_event_post(&updateEvents, EVT_CAN_ACTIVE | EVT_CAN_TRIGGER);
}

void waitCanSendDone() {
    k_event_wait(&updateEvents, EVT_CAN_DONE, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_CAN_DONE);
}

board_id_t waitCanTrigger() {
    k_event_wait(&updateEvents, EVT_CAN_TRIGGER, false, K_FOREVER);
    k_event_clear(&updateEvents, EVT_CAN_TRIGGER);
    return (board_id_t)atomic_get(&canTargetBoard);
}

void canSendComplete() {
    k_event_clear(&updateEvents, EVT_CAN_ACTIVE);
    k_event_post(&updateEvents, EVT_CAN_DONE);
}
