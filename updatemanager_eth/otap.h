#ifndef OTAP_H
#define OTAP_H

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>

#include <stdint.h>
#include <stddef.h>

#include "coordinator.h"

int initSwapping(board_id_t boardId);
int writeToBackup(uint8_t * newData, size_t	len, bool flush);
void setWriteToBackupDone();
int setWriteToPayloadDone();

#endif
