#ifndef UPDATEMANAGERCAN_H
#define UPDATEMANAGERCAN_H

void acceptCanPayload();
void stopCanPayload();

void canUpdateTask(void * p1, void * p2, void * p3);

#endif
