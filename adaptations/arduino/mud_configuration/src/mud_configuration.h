#ifndef MUD_CONFIGURATION_H
#define MUD_CONFIGURATION_H

#define ENABLE_DEBUG 1

// For my two Arduino Micro boards.
#define ARDUINO_MICRO 1

// HLT
#define LORA_CHIP_MD0_PIN 2
#define LORA_CHIP_MD1_PIN 3
#define LORA_CHIP_AUX_PIN 4

/*// STR-01
#define LORA_CHIP_MD0_PIN 2
#define LORA_CHIP_MD1_PIN 3
#define LORA_CHIP_AUX_PIN 4

// SL-02
#define LORA_CHIP_AUX_PIN 10
#define LORA_CHIP_MD0_PIN 11
#define LORA_CHIP_MD1_PIN 12*/

/*// For my Arduino UNO R3 board.
#define ARDUINO_UNO 1
// #define USE_SOFTWARE_SERIAL 1
#define LORA_CHIP_MD0_PIN 3
#define LORA_CHIP_MD1_PIN 4
#define LORA_CHIP_AUX_PIN 5

#if defined(ARDUINO_UNO) && defined(USE_SOFTWARE_SERIAL)
// For my Arduino UNO R3 board.
#define SOFTWARE_SERIAL_RX_PIN 11
#define SOFTWARE_SERIAL_TX_PIN 12
#endif*/

#endif
