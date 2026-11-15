#ifndef UPDATEMANAGERETHERNET_H
#define UPDATEMANAGERETHERNET_H

#include <stdint.h>

void acceptEthernetPayload();
void stopEthernetPayload();

void ethernetUpdateTask(void * p1, void * p2, void * p3);

#endif
