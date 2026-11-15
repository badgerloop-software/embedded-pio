#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ethUpdateStarted();
void ethUpdateFinished();
void signalPayloadReady();
bool canTransferActive();

void waitForPayload();
void beginCanSend();
void waitCanSendDone();

void waitCanTrigger();
void canSendComplete();

#ifdef __cplusplus
}
#endif

#endif
