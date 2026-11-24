#include "otap.h"
#include <stdint.h>
#include <string.h>
#include <zephyr/storage/stream_flash.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(OTAP_LOG, LOG_LEVEL_INF);

struct flash_img_context ctx;

static board_id_t targetBoard = CAN_BREAKOUT_BOARD;
static const struct flash_area * payloadArea;
static struct stream_flash_ctx payloadStream;
static uint8_t payloadBuffer[CONFIG_IMG_BLOCK_BUF_SIZE];
static uint8_t payloadHeader[PAYLOAD_HEADER_SIZE];
static uint32_t payloadLength = 0;
static uint32_t payloadCrc = 0;

static int initPayloadSlot()
{
    int err;
    uint8_t areaID = PARTITION_ID(payload_partition);

    err = boot_erase_img_bank(areaID);
    if (err) {
        LOG_ERR("Failed to erase payload partition: %d", err);
        return err;
    }

    err = flash_area_open(areaID, &payloadArea);
    if (err) {
        LOG_ERR("Failed to open payload partition: %d", err);
        return err;
    }

    err = stream_flash_init(&payloadStream, flash_area_get_device(payloadArea),
                            payloadBuffer, sizeof(payloadBuffer),
                            payloadArea->fa_off + PAYLOAD_HEADER_SIZE,
                            payloadArea->fa_size - PAYLOAD_HEADER_SIZE, NULL);
    if (err) {
        LOG_ERR("stream_flash_init failed: %d", err);
        return err;
    }

    payloadLength = 0;
    payloadCrc = 0;
    return 0;
}

int initSwapping(board_id_t boardId)
{
    int err;

    targetBoard = boardId;

    if (targetBoard != CAN_BREAKOUT_BOARD) {
        return initPayloadSlot();
    }

    uint8_t areaID = flash_img_get_upload_slot();

    err = flash_img_init_id(&ctx,
                            areaID);
    if (err) {
        LOG_ERR("flash_img_init_id failed: %d", err);
        return err;
    }

    err = boot_erase_img_bank(areaID);
    if (err) {
        LOG_ERR("Failed to erase image bank: %d", err);
        return err;
    }

    return 0;
}


int writeToBackup(uint8_t * newData, size_t len, bool flush){

    if (targetBoard != CAN_BREAKOUT_BOARD) {
        int err = stream_flash_buffered_write(&payloadStream, newData, len, flush);

        if (err == 0 && len > 0) {
            payloadCrc = crc32_ieee_update(payloadCrc, newData, len);
            payloadLength += len;
        }

        return err;
    }

    size_t bytesWritten = flash_img_bytes_written(&ctx);

    if (bytesWritten + len > ctx.flash_area->fa_size) {
        return -1;
    }

    return flash_img_buffered_write(&ctx, newData, len, flush);

}

// TODO: this should have a return value to indicate success or failure
void setWriteToBackupDone(){

   int err = boot_request_upgrade(BOOT_UPGRADE_TEST);

    if (err == -1) {
        LOG_ERR("Failed to request upgrade: %d", err);
    }

// rturrn err;
// TODO: make sure all memmory is freed

}

int setWriteToPayloadDone()
{
    struct payload_meta meta = {
        .magic = PAYLOAD_MAGIC,
        .length = payloadLength,
        .crc32 = payloadCrc,
    };

    memset(payloadHeader, 0xFF, sizeof(payloadHeader));
    memcpy(payloadHeader, &meta, sizeof(meta));

    int err = flash_area_write(payloadArea, 0, payloadHeader, sizeof(payloadHeader));
    if (err) {
        LOG_ERR("Failed to write payload meta: %d", err);
    }

    return err;
}
