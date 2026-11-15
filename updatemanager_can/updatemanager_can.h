#ifndef UPDATEMANAGERCAN_H
#define UPDATEMANAGERCAN_H

#include <zephyr/storage/flash_map.h>

void acceptCanPayload();
void stopCanPayload();

void canUpdateTask(void * p1, void * p2, void * p3);

#if FIXED_PARTITION_EXISTS(payload_partition)
void canUpdateSendTask(void * p1, void * p2, void * p3);
#endif

#endif
