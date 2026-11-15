#include "updatemanager_can.h"
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <flashimage.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include "updcan.h"

LOG_MODULE_REGISTER(canupd, LOG_LEVEL_INF);

static const struct device * const canDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const uint32_t CAN_DEVICE_FREQUENCY = 250000;

static const int COMMAND_SIZE = 1;

#ifdef USING_CAN_FD_AND_BITRATE_SWITCHING
    static const int CAN_PAYLOAD_SIZE = 64;
#elif defined(USING_CAN_FD)
    static const int CAN_PAYLOAD_SIZE = 64;
#else
    static const int CAN_PAYLOAD_SIZE = 8;
#endif

static const int BUFFER_SIZE = CAN_PAYLOAD_SIZE - COMMAND_SIZE;
static const int DATA_ALIGNMENT_BYTES = CONFIG_IMG_BLOCK_BUF_SIZE;

static const uint8_t ACK_OK  = 0x00;
static const uint8_t ACK_ERR = 0x01;

static const uint32_t UPDATE_ID_LIST[] = {UPDATE_ID};

typedef enum {
    REQUEST_START_UPDATE = 0,
    FILE_SIZE = 1,
    SENDING_FILE = 2,
    FILE_SENT = 3,
} command_t;

atomic_t canUpdateFlag = ATOMIC_INIT(0);

void acceptCanPayload() {
    atomic_set(&canUpdateFlag, 1);
}

void stopCanPayload() {
    atomic_set(&canUpdateFlag, 0);
}

void canUpdateTask(void * p1, void * p2, void * p3) {
    uint32_t imageLen = 0;
    size_t remaining = 0;
    struct can_frame canFrame;
    uint8_t dataBuffer[DATA_ALIGNMENT_BYTES + BUFFER_SIZE];
    int dataBufferSize = 0;
    
    UpdCAN updateCanBus(canDev, UPDATE_ID_LIST, sizeof(UPDATE_ID_LIST)/sizeof(UPDATE_ID_LIST[0]), CAN_DEVICE_FREQUENCY);

    if (updateCanBus.begin() != CAN_OK) {
        LOG_ERR("CAN init failed: %d", updateCanBus.canStatus());
        return;
    }

    if (!boot_is_img_confirmed()) {
        int rc = boot_write_img_confirmed();
        if (rc) {
            LOG_ERR("Image confirm failed: %d", rc);
        } else {
            LOG_INF("Image confirmed - update is now permanent");
        }
    }

    while (1) {
        updateCanBus.readNextMessage(&canFrame, K_FOREVER);

        uint8_t command = canFrame.data[0];
        uint8_t * data = canFrame.data + COMMAND_SIZE;
        size_t dataLen = BUFFER_SIZE;

        switch (command) {
            case REQUEST_START_UPDATE: {
                updateCanBus.sendACK(ACK_OK);
                break;
            }
            case FILE_SIZE: {
                imageLen = sys_get_le32(data);
                remaining = imageLen;
                LOG_INF("Update request, image %u bytes", (unsigned)imageLen);
                initSwapping();
                updateCanBus.sendACK(ACK_OK);
                break;
            }
            case SENDING_FILE: {
                size_t toWrite = MIN(dataLen, remaining);
                memcpy(dataBuffer[dataBufferSize], data, toWrite);
                dataBufferSize += toWrite;

                if (dataBufferSize >= DATA_ALIGNMENT_BYTES || dataBufferSize == remaining) 
                {
                    if (writeToBackup(data, toWrite, false) < 0) {
                        LOG_ERR("writeToBackup failed at %u bytes", (unsigned)(imageLen - remaining));
                        updateCanBus.sendACK(ACK_ERR);
                    } else {
                        remaining -= dataBufferSize;
                        updateCanBus.sendACK(ACK_OK);
                    }
                }
                break;
            }
            case FILE_SENT: {
                LOG_INF("Image received (%u bytes), swapping", (unsigned)imageLen);
                writeToBackup(NULL, 0, true);
                setWriteToBackupDone();
                updateCanBus.sendACK(ACK_OK);
                k_msleep(1000);
                sys_reboot(SYS_REBOOT_COLD);
                break;
            }
            default:
                break;
        }
    }
}

#if FIXED_PARTITION_EXISTS(payload_partition)

static const uint32_t PAYLOAD_MAGIC = 0x60BA6E25; //GOBADGERS

struct payload_meta {
    uint32_t magic;
    uint32_t length;
    uint32_t crc32;
};

static const uint32_t ACK_ID_LIST[] = {ACK_ID};
static const int ACK_TIMEOUT_MS = 1000;
static const int CRC_CHUNK_SIZE = 256;

static int verifyPayload(const struct flash_area * payloadArea, struct payload_meta * meta) {
    if (flash_area_read(payloadArea, 0, meta, sizeof(struct payload_meta)) != 0) {
        return -1;
    }

    if (meta->magic != PAYLOAD_MAGIC) {
        LOG_ERR("Magic number incorrect");
        return -1;
    }

    uint8_t chunk[CRC_CHUNK_SIZE];
    uint32_t crc = 0;
    size_t remaining = meta->length;
    off_t offset = sizeof(struct payload_meta);

    while (remaining > 0) {
        size_t toRead = MIN((size_t)CRC_CHUNK_SIZE, remaining);

        if (flash_area_read(payloadArea, offset, chunk, toRead) != 0) {
            return -1;
        }

        crc = crc32_ieee_update(crc, chunk, toRead);
        offset += toRead;
        remaining -= toRead;
    }

    return (crc == meta->crc32) ? 0 : -1;
}

static int sendCommandFrame(UpdCAN & sendBus, uint8_t command, const uint8_t * data, uint8_t length) {
    uint8_t frameBuffer[CAN_PAYLOAD_SIZE];

    frameBuffer[0] = command;
    if (length > 0) {
        memcpy(frameBuffer + COMMAND_SIZE, data, MIN(length, BUFFER_SIZE));
    }

    return sendBus.sendMessage(UPDATE_ID, frameBuffer, COMMAND_SIZE + length);
}

static int waitForAck(UpdCAN & sendBus) {
    struct can_frame ackFrame;

    if (sendBus.readNextMessage(&ackFrame, K_MSEC(ACK_TIMEOUT_MS)) != 0) {
        return -1;
    }

    return (ackFrame.data[0] == ACK_OK) ? 0 : -1;
}

void canUpdateSendTask(void * p1, void * p2, void * p3) {
    const struct flash_area * payloadArea;
    struct payload_meta meta;

    if (flash_area_open(FIXED_PARTITION_ID(payload_partition), &payloadArea) != 0) {
        LOG_ERR("Failed to open payload partition");
        return;
    }

    if (verifyPayload(payloadArea, &meta) != 0) {
        LOG_ERR("Payload slot has no valid image");
        flash_area_close(payloadArea);
        return;
    }

    UpdCAN sendBus(canDev, ACK_ID_LIST, sizeof(ACK_ID_LIST)/sizeof(ACK_ID_LIST[0]), CAN_DEVICE_FREQUENCY);

    if (sendBus.begin() != CAN_OK) {
        LOG_ERR("CAN init failed: %d", sendBus.canStatus());
        flash_area_close(payloadArea);
        return;
    }

    if (sendCommandFrame(sendBus, REQUEST_START_UPDATE, NULL, 0) < 0 || waitForAck(sendBus) != 0) {
        LOG_ERR("Start handshake failed");
        flash_area_close(payloadArea);
        return;
    }

    uint8_t lengthBuffer[4];
    sys_put_le32(meta.length, lengthBuffer);

    if (sendCommandFrame(sendBus, FILE_SIZE, lengthBuffer, sizeof(lengthBuffer)) < 0 || waitForAck(sendBus) != 0) {
        LOG_ERR("File size handshake failed");
        flash_area_close(payloadArea);
        return;
    }

    uint8_t chunk[BUFFER_SIZE];
    size_t remaining = meta.length;
    off_t offset = sizeof(struct payload_meta);

    while (remaining > 0) {
        size_t toSend = MIN((size_t)BUFFER_SIZE, remaining);

        if (flash_area_read(payloadArea, offset, chunk, toSend) != 0) {
            LOG_ERR("Payload read failed at %u bytes", (unsigned)(meta.length - remaining));
            flash_area_close(payloadArea);
            return;
        }

        if (sendCommandFrame(sendBus, SENDING_FILE, chunk, toSend) < 0 || waitForAck(sendBus) != 0) {
            LOG_ERR("Send failed at %u bytes", (unsigned)(meta.length - remaining));
            flash_area_close(payloadArea);
            return;
        }

        offset += toSend;
        remaining -= toSend;
    }

    if (sendCommandFrame(sendBus, FILE_SENT, NULL, 0) < 0 || waitForAck(sendBus) != 0) {
        LOG_ERR("Final handshake failed");
        flash_area_close(payloadArea);
        return;
    }

    LOG_INF("Payload sent over CAN (%u bytes)", (unsigned)meta.length);
    flash_area_close(payloadArea);
}
#endif
