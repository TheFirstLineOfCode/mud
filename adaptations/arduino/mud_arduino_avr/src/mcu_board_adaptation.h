#ifndef MUD_BOARD_ADAPTATION_H
#define MUD_BOARD_ADAPTATION_H

#include <HardwareSerial.h>

#include "mud_configuration.h"

#ifdef ARDUINO_MICRO

#define SerialUart Serial1

#endif

#ifdef ARDUINO_UNO

#ifdef USE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
#define SerialUart softwareSerial
#else
#define SerialUart Serial
#endif

#endif

void configureMcuBoard(const char *modelName);
void printToSerialPort(char title[], uint8_t message[], int size);
void resetAll();

#endif
