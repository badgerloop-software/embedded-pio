#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <zephyr/toolchain.h>

typedef enum {
    CAN_BREAKOUT_BOARD = 0,
    PDC = 1,
} board_id_t;

static const uint32_t BOARD_CAN_ID_LIST[] = {0xFF, 0xAA};
static const size_t BOARD_COUNT = sizeof(BOARD_CAN_ID_LIST) / sizeof(BOARD_CAN_ID_LIST[0]);

static const uint32_t PAYLOAD_MAGIC = 0x60BA6E25; //GOBADGERS
static const size_t PAYLOAD_HEADER_SIZE = CONFIG_IMG_BLOCK_BUF_SIZE;

struct payload_meta {
    uint32_t magic;
    uint32_t length;
    uint32_t crc32;
};

BUILD_ASSERT(sizeof(struct payload_meta) <= CONFIG_IMG_BLOCK_BUF_SIZE, "payload_meta must fit in the payload header");

#ifdef __cplusplus
extern "C" {
#endif

void ethUpdateStarted();
void ethUpdateFinished();
void signalPayloadReady(board_id_t boardId);
bool canTransferActive();

board_id_t waitForPayload();
void beginCanSend(board_id_t boardId);
void waitCanSendDone();

board_id_t waitCanTrigger();
void canSendComplete();

#ifdef __cplusplus
}
#endif

#endif
