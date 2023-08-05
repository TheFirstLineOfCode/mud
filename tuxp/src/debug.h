#ifndef MUD_DEBUG_H
#define MUD_DEBUG_H

#include <stdbool.h>

#if defined(ARDUINO)

#include <Arduino.h>

#endif

#ifdef _WIN32

#define ENABLE_DEBUG 1

#else

#include "mcu_board_adaptation.h"

#endif


#ifdef ENABLE_DEBUG

#define DEBUG_OUT(out) debugOut(out)

#else

#define DEBUG_OUT(out) noDebugOut()

#endif


void setDebugOutputter(void (*debugOutput)(const char out[]));
void debugOut(const char out[]);
void noDebugOut();
int debugErrorAndReturn(const char funName[], int errorNumber);
int debugErrorDetailAndReturn(const char funName[], int errorNumber, int errorNumberOfCause);

#endif
