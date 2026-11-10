#include "updatemanager_can.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
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

                if (writeToBackup(data, toWrite, false) < 0) {
                    LOG_ERR("writeToBackup failed at %u bytes", (unsigned)(imageLen - remaining));
                    updateCanBus.sendACK(ACK_ERR);
                } else {
                    remaining -= toWrite;
                    updateCanBus.sendACK(ACK_OK);
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
