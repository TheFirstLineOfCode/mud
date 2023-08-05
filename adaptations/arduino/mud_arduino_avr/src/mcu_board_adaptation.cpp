#include <ArduinoUniqueID.h>
#include <EEPROM.h>

#include <tuxp.h>
#include <thing.h>
#include <debug.h>
#include <radio_module_adaptation.h>

#include "mcu_board_adaptation.h"

#if defined(ARDUINO_UNO) && defined(USE_SOFTWARE_SERIAL)
SoftwareSerial softwareSerial(SOFTWARE_SERIAL_RX_PIN, SOFTWARE_SERIAL_TX_PIN);
#endif

static char *modelName;

void debugOutputImpl(const char out[]) {
	Serial.println(out);
}

void printToSerialPort(char title[], uint8_t message[], int size) {
#if defined(ENABLE_DEBUG)

  Serial.print(title);

  if (size == 0) {
    Serial.println(F("0 bytes. {}."));
  } else {
    Serial.print(size);
    Serial.print(F(" bytes. {"));
    for (int i = 0; i < size; i++) {
      Serial.print(message[i]);
      if (i == size - 1)
        break;

     Serial.print(F(","));
    }
	
    Serial.println(F("}."));
  }
  
#endif
}

void configureSerial() {
  Serial.begin(9600);
  while (!Serial) {
    delay(200);
  }
}

void initializeEepRom(int eepRomLength) {
  for (int i = 0; i < eepRomLength - 3; i++)
    EEPROM.write(i, 0);

  EEPROM.write(eepRomLength - 3, 0xfd);
  EEPROM.write(eepRomLength - 2, 0xfe);
  EEPROM.write(eepRomLength - 1, 0xff);
}

void (* resetFunc) () = 0;

void resetImpl() {
  resetFunc();
}

long getTimeImpl() {
	return millis();
}

int readAddressFromEepRom(uint8_t *address, int position) {
    for (int i = 0; i < SIZE_RADIO_ADDRESS; i++)
      *(address + i) = EEPROM.read(position + i);
      
    return position + SIZE_RADIO_ADDRESS;
}

uint8_t getDacStateByteValue(DacState dacState) {
  if (dacState == NONE)
    return 0;
  else if (dacState == INITIAL)
    return 1;
  else if (dacState == INTRODUCTING)
    return 2;
  else if (dacState == ALLOCATING)
    return 3;
  else if (dacState == ALLOCATED)
    return 4;
  else if (dacState == CONFIGURED)
    return 5;
  else
    return 0;
}

DacState getDacStateByByte(uint8_t iDacState) {
  if (iDacState == 0)
    return NONE;
  else if (iDacState == 1)
    return INITIAL;
  else if (iDacState == 2)
    return INTRODUCTING;
  else if (iDacState == 3)
    return ALLOCATING;
  else if (iDacState == 4)
    return ALLOCATED;
  else if (iDacState == 5)
    return CONFIGURED;
  else
    return NONE;
}

void debugOutThingInfo(ThingInfo *thingInfo) {
#ifdef ENABLE_DEBUG
  char buffer[64];
  if (thingInfo->dacState == ALLOCATED ||
        thingInfo->dacState == CONFIGURED) {
    sprintf(buffer, "Thing ID: %s. DAC state: %d. Address: {%d, %d, %d}.",
			    thingInfo->thingId, getDacStateByteValue(thingInfo->dacState),
            thingInfo->address[0], thingInfo->address[1], thingInfo->address[2]);
  } else {
  	sprintf(buffer, "Thing ID: %s. DAC state: %d.",
			    thingInfo->thingId == NULL ? "NULL" : thingInfo->thingId, getDacStateByteValue(thingInfo->dacState));
  }
  debugOut(buffer);
#endif
}

void loadThingInfoImpl(ThingInfo *thingInfo) {
  uint8_t thingIdSize = EEPROM.read(0);

  if (thingIdSize == 0) {
#ifdef ENABLE_DEBUG
    Serial.println(F("No thing info in storage. Use initial value."));
#endif

    thingInfo->thingId = NULL;
    thingInfo->dacState = NONE;
    thingInfo->address = NULL;
    thingInfo->uplinkChannelBegin = -1;
	thingInfo->uplinkChannelEnd = -1;
    thingInfo->uplinkAddressHighByte = 0xff;
	thingInfo->uplinkAddressLowByte = 0xff;
  } else {
    int position = 1;
    thingInfo->thingId = malloc(sizeof(char) * (thingIdSize + 1));
    for (int i = 0; i < thingIdSize; i++) {
      *((thingInfo->thingId) + i) = EEPROM.read(i + 1);
    }
    *((thingInfo->thingId) + thingIdSize) = '\0';    
    position += thingIdSize;

    uint8_t iDacState = EEPROM.read(position);
    thingInfo->dacState = getDacStateByByte(iDacState);
    position++;
    
    if (thingInfo->dacState == ALLOCATED ||
          thingInfo->dacState == CONFIGURED) {
	  EEPROM.get(position, thingInfo->uplinkChannelBegin);
	  position += 2;
	  
	  EEPROM.get(position, thingInfo->uplinkChannelEnd);
	  position += 2;
	  
	  thingInfo->uplinkAddressHighByte = EEPROM.read(position);
	  position++;
	  thingInfo->uplinkAddressLowByte = EEPROM.read(position);
	  position++;
	  
      thingInfo->address = malloc(SIZE_RADIO_ADDRESS);
      readAddressFromEepRom(thingInfo->address, position);
    }
  }
  
  #ifdef ENABLE_DEBUG
    Serial.println(F("ThingInfo loaded."));
  #endif
  debugOutThingInfo(thingInfo);
}

int writeAddressToEepRom(uint8_t *address, int position) {
  for (int i = 0; i < SIZE_RADIO_ADDRESS; i++)
    EEPROM.write(position + i , *(address + i));
    
  return position + SIZE_RADIO_ADDRESS;  
}

void saveThingInfoImpl(ThingInfo *thingInfo) {
  if (thingInfo->thingId == NULL) {
#ifdef ENABLE_DEBUG
    Serial.println(F("NULL thing ID. Can't save thing info."));
#endif
    return;
  }

  int thingIdSize = strlen(thingInfo->thingId);
  EEPROM.write(0, thingIdSize);
  for (int i = 0; i < thingIdSize; i++) {
    EEPROM.write(i + 1, *((thingInfo->thingId) + i));
  }

  int position = 1 + thingIdSize;
  EEPROM.write(position, getDacStateByteValue(thingInfo->dacState));
  position++;

  if (thingInfo->dacState == ALLOCATED ||
        thingInfo->dacState == CONFIGURED) {
	EEPROM.put(position, thingInfo->uplinkChannelBegin);
	position += 2;
	EEPROM.put(position, thingInfo->uplinkChannelEnd);
	position += 2;
	
	EEPROM.write(position, thingInfo->uplinkAddressHighByte);
	position++;
	EEPROM.write(position, thingInfo->uplinkAddressLowByte);
	position++;
	
    position = writeAddressToEepRom(thingInfo->address, position);
  }
  
  #ifdef ENABLE_DEBUG
    Serial.println(F("ThingInfo saved."));
  #endif

  debugOutThingInfo(thingInfo);
}

void resetAll() {
  EEPROM.write(EEPROM.length() - 1, 0);
}

void configureMcuBoard(const char *_modelName) {
  int modelNameLength = strlen(_modelName);
  modelName = malloc(sizeof(char) * (modelNameLength + 1));
  strcpy(modelName, _modelName);
  
#ifdef ENABLE_DEBUG
  configureSerial();
  Serial.println("Serial has configured.");
  setDebugOutputter(debugOutputImpl);
#endif
  
  registerThingInfoLoader(loadThingInfoImpl);
  registerThingInfoSaver(saveThingInfoImpl);
  registerResetter(resetImpl);
  registerTimer(getTimeImpl);
  
  uint16_t eepRomLength = EEPROM.length();
  uint8_t lastByteOfEepRom = EEPROM.read(eepRomLength - 1);
  uint8_t nextToLastByteOfEepRom = EEPROM.read(eepRomLength - 2);
  uint8_t theThirdToLastByteOfEepRom = EEPROM.read(eepRomLength - 3);
  
  if (lastByteOfEepRom != 0xff ||
        nextToLastByteOfEepRom != 0xfe ||
          theThirdToLastByteOfEepRom != 0xfd) {
#ifdef ENABLE_DEBUG
    Serial.println(F("EEPROM not initialized. Initlize it now."));
#endif
    initializeEepRom(eepRomLength);
  }
}
