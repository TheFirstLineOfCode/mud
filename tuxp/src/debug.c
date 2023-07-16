#include <stddef.h>
#include <stdio.h>

#include "debug.h"

#define NO_CAUSE_INFO_FOUND 0

static void (*debugOutput)(const char out[]) = NULL;

void setDebugOutputter(void (*_debugOutput)(const char out[])) {
	debugOutput = _debugOutput;
}

void debugOut(const char out[]) {
	if(!debugOutput)
		return;

	debugOutput(out);
}

void noDebugOut() {}

int debugErrorAndReturn(const char funName[], int errorNumber) {
	return debugErrorDetailAndReturn(funName, errorNumber, NO_CAUSE_INFO_FOUND);
}

int debugErrorDetailAndReturn(const char funName[], int errorNumber, int errorNumberOfCause) {
	if(!debugOut)
		return;

	char buff[128];
	if (errorNumberOfCause == NO_CAUSE_INFO_FOUND) {
		sprintf(buff, "Error - Function name: %s. Error number: %d.", funName, errorNumber);
	} else {
		sprintf(buff, "Error - Function name: %s. Error number: %d. Error number of cause: %d.",
			funName, errorNumber, errorNumberOfCause);
	}

	DEBUG_OUT(buff);

	return errorNumber;
}
