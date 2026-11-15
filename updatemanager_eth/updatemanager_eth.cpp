#include "updatemanager_eth.h"
#include "zephyrethernet.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <otap.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include "coordinator.h"

LOG_MODULE_REGISTER(ethupd, LOG_LEVEL_INF);

static const int COMMAND_SIZE = 1;
static const int BUFFER_SIZE = CONFIG_IMG_BLOCK_BUF_SIZE;
static const int ETHERNET_PAYLOAD_SIZE = COMMAND_SIZE + BUFFER_SIZE;

static const char PC_IP[] = "192.168.1.2";

static const uint8_t ACK_OK  = 0x00;
static const uint8_t ACK_ERR = 0x01;

typedef enum {
    REQUEST_START_UPDATE = 0,
    REQUEST_BLOCK_SIZE = 1,
    FILE_SIZE = 2,
    SENDING_FILE = 3,
    FILE_SENT = 4,
} command_t;

atomic_t ethUpdateFlag = ATOMIC_INIT(0);

void acceptEthernetPayload() {
    atomic_set(&ethUpdateFlag, 1);
}

void stopEthernetPayload() {
    atomic_set(&ethUpdateFlag, 0);
}

void ethernetUpdateTask(void * p1, void * p2, void * p3) {
    uint8_t ethBuffer[ETHERNET_PAYLOAD_SIZE];
    size_t bytesRead;
    uint32_t imageLen = 0;
    size_t remaining = 0;
    board_id_t targetBoard = CAN_BREAKOUT_BOARD;

    ZephyrEthernet ze(PC_IP);
    
#if !FIXED_PARTITION_EXISTS(payload_partition)
    LOG_WRN("payload_partition not defined, this will cause error in updating other boards");
#endif

    if (ze.initEthernetDevice(true) != ETH_OK) {
        LOG_ERR("Etherenet init failed: %d", ze.ethernetStatus());
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
        ze.getNextPacket(ethBuffer, sizeof(ethBuffer), &bytesRead);

        if (bytesRead < COMMAND_SIZE) {
            continue;
        }

        uint8_t command = ethBuffer[0];
        uint8_t * data = ethBuffer + COMMAND_SIZE;
        size_t dataLen = bytesRead - COMMAND_SIZE;

        switch (command) {
            case REQUEST_START_UPDATE: {
                if (canTransferActive()) {
                    LOG_WRN("Update denied, CAN transfer in progress");
                    ze.sendPacket(&ACK_ERR, 1);
                    break;
                }
                if (dataLen < 1 || data[0] >= BOARD_COUNT) {
                    LOG_ERR("Update denied, invalid board id");
                    ze.sendPacket(&ACK_ERR, 1);
                    break;
                }
                targetBoard = (board_id_t)data[0];
                LOG_INF("Update start, board %u", (unsigned)targetBoard);
                ze.sendPacket(&ACK_OK, 1);
                break;
            }
            case REQUEST_BLOCK_SIZE: {
                uint8_t reply[4];
                sys_put_le32((uint32_t)BUFFER_SIZE, reply);
                ze.sendPacket(reply, sizeof(reply));
                break;
            }
            case FILE_SIZE: {
                if (canTransferActive()) {
                    LOG_WRN("Update denied, CAN transfer in progress");
                    ze.sendPacket(&ACK_ERR, 1);
                    break;
                }
                if (dataLen >= 4) {
                    imageLen = sys_get_le32(data);
                    remaining = imageLen;
                    LOG_INF("Update request, image %u bytes", (unsigned)imageLen);
                    ethUpdateStarted();
                    if (initSwapping(targetBoard) != 0) {
                        ethUpdateFinished();
                        ze.sendPacket(&ACK_ERR, 1);
                    } else {
                        ze.sendPacket(&ACK_OK, 1);
                    }
                }
                break;
            }
            case SENDING_FILE: {
                size_t toWrite = MIN(dataLen, remaining);

                if (writeToBackup(data, toWrite, false) < 0) {
                    LOG_ERR("writeToBackup failed at %u bytes", (unsigned)(imageLen - remaining));
                    ze.sendPacket(&ACK_ERR, 1);
                } else {
                    remaining -= toWrite;
                    ze.sendPacket(&ACK_OK, 1);
                }
                break;
            }
            case FILE_SENT: {
                int flushResult = writeToBackup(NULL, 0, true);
                ethUpdateFinished();

                if (flushResult < 0) {
                    LOG_ERR("Final flush failed: %d", flushResult);
                    ze.sendPacket(&ACK_ERR, 1);
                } else if (targetBoard == CAN_BREAKOUT_BOARD) {
                    LOG_INF("Image received (%u bytes), swapping", (unsigned)imageLen);
                    setWriteToBackupDone();
                    ze.sendPacket(&ACK_OK, 1);
                    k_msleep(1000);
                    sys_reboot(SYS_REBOOT_COLD);
                } else if (setWriteToPayloadDone() != 0) {
                    ze.sendPacket(&ACK_ERR, 1);
                } else {
                    LOG_INF("Payload received (%u bytes) for board %u, forwarding over CAN", (unsigned)imageLen, (unsigned)targetBoard);
                    ze.sendPacket(&ACK_OK, 1);
                    signalPayloadReady(targetBoard);
                }

                targetBoard = CAN_BREAKOUT_BOARD;
                break;
            }
            default:
                break;
        }
    }
}
