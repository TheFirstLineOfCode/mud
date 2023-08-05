#include "radio_module_adaptation.h"

#include <stdint.h>
#include <stdbool.h>

#include <Arduino.h>

#include <thing.h>
#include <mcu_board_adaptation.h>

#if defined(ARDUINO_UNO) && defined(USE_SOFTWARE_SERIAL)
extern SoftwareSerial softwareSerial;
#endif

static uint8_t radioConfigs[5];

void configureSerialUart() {
  SerialUart.begin(9600);
  while (!SerialUart)
    delay(200);
}

int readDataFromUart(uint8_t buff[], int buffSize) {
  int available = SerialUart.available();
  if (available == 0)
    return 0;

  return SerialUart.readBytes(buff, buffSize);
}

int receiveRadioDataImpl(uint8_t buff[], int buffSize) {
	int dataSize = readDataFromUart(buff, buffSize);
	if (dataSize > 0)
		printToSerialPort("Radio data received. Data: ", buff, dataSize);
	
	return dataSize;
}

bool isOk(uint8_t response[], int responseSize) {
  if (responseSize < 4)
    return false;

  int firstByteOfOkIndex = responseSize - 4;
  return response[firstByteOfOkIndex] == 0x4f &&
    response[firstByteOfOkIndex + 1] == 0x4b &&
    response[firstByteOfOkIndex + 2] == 0x0d &&
    response[firstByteOfOkIndex + 3] == 0x0a;
}

int executeRadioConfigurationCommand(uint8_t command[], int size, uint8_t response[], int responseMaxSize) {
  digitalWrite(LORA_CHIP_MD0_PIN, HIGH);
  digitalWrite(LORA_CHIP_MD1_PIN, HIGH);

  delay(1000);

  SerialUart.write(command, size);
  SerialUart.flush();

  delay(1000);
  
  int responseSize = readDataFromUart(response, responseMaxSize);

  digitalWrite(LORA_CHIP_MD1_PIN, LOW);
  digitalWrite(LORA_CHIP_MD0_PIN, LOW);

  delay(1000);
  
  return responseSize;
}

void resetRadio() {
  uint8_t resetRadioCommand[] = {0xc9, 0xc9, 0xc9};
  uint8_t response[16];

  int responseSize = executeRadioConfigurationCommand(resetRadioCommand, 3,  response, 16);

  printToSerialPort("Response for LoRa chip resetting: ", response, responseSize);
}

bool readRadioConfigs() {
  uint8_t readConfigsCommand[] = {0xc1, 0xc1, 0xc1};

  uint8_t response[16];
  int responseSize = executeRadioConfigurationCommand(readConfigsCommand, 3,  response, 16);
  printToSerialPort("Response for configs reading: ", response, responseSize);

  if (responseSize < 6) {
    return false;
  } else {
    int configsFirstByteIndex = responseSize - 6;
    if ((response[configsFirstByteIndex] & 0xff) != 0xc0 && (response[configsFirstByteIndex] & 0xff) != 0xc2)
      return false;
    
    for (int i = 0; i < 5; i++)
      radioConfigs[i] = response[configsFirstByteIndex + i +  1];
  }

  return true;
}

bool initializeRadioImpl(uint8_t address[]) {
  pinMode(LORA_CHIP_AUX_PIN, INPUT);
  pinMode(LORA_CHIP_MD0_PIN, OUTPUT);
  pinMode(LORA_CHIP_MD1_PIN, OUTPUT);

  configureSerialUart();

  delay(1000);
  
  if (!readRadioConfigs()) {
#ifdef ENABLE_DEBUG
    Serial.println(F("Error: Can't read radio configs."));
#endif

    return false;
  }
    
  address[0] = radioConfigs[0];
  address[1] = radioConfigs[1];
  address[2] = radioConfigs[3];

  return true;
}

bool configureRadioToP2pTransmissionMode() {
  uint8_t configureToP2pTransmissionCommand[] = {0xc0, radioConfigs[0], radioConfigs[1], radioConfigs[2],
      radioConfigs[3], radioConfigs[4] | 0x80};
  uint8_t response[16];
  int responseSize = executeRadioConfigurationCommand(configureToP2pTransmissionCommand, 6, response, 16);
  printToSerialPort("Response for P2P transmission mode setting: ", response, responseSize);

  return isOk(response, responseSize);
}

bool configureRadioImpl() {
  if ((radioConfigs[4] & 0x80) != 0x80) {
#ifdef ENABLE_DEBUG
    Serial.println(F("Lora chip isn't in P2P transmission mode. Configure it to P2P transmission mode."));
#endif

    if (!configureRadioToP2pTransmissionMode()) {
#ifdef ENABLE_DEBUG
      Serial.println(F("Error: Can't configure LoRa chip to P2P transmission mode."));
#endif
      return false;
    }

    radioConfigs[4] = radioConfigs[4] | 0x80;
  } else {
    printToSerialPort("Current radio configs: ", radioConfigs, 5);
  }

  return true;
}

bool changeRadioAddressImpl(RadioAddress address, bool savePersistently) {
  uint8_t commandByte = 0xc2;
  if (savePersistently)
    commandByte = 0xc0;

  uint8_t changeAddressCommand[] = {commandByte, address[0], address[1], radioConfigs[2], address[2], radioConfigs[4]};  
  uint8_t response[16];
  int responseSize = executeRadioConfigurationCommand(changeAddressCommand, 6, response, 16);
  printToSerialPort("Response for address changing: ", response, responseSize);

  bool result = isOk(response, responseSize);
  if (result)
    printToSerialPort("Radio address has changed to: ", address, SIZE_RADIO_ADDRESS);

  return result;
}

void sendRadioDataImpl(RadioAddress address, uint8_t data[], int dataSize) {
  uint8_t finalDataBeSent[3 + dataSize];
  finalDataBeSent[0] = address[0];
  finalDataBeSent[1] = address[1];
  finalDataBeSent[2] = address[2];

  for (int i = 0; i < dataSize; i++)
    finalDataBeSent[3 + i] = data[i];
  
  printToSerialPort("Send data to peer. Data: ", finalDataBeSent, 3 + dataSize);
  SerialUart.write(finalDataBeSent, 3 + dataSize);
}

void configureRadioModule() {
  registerRadioInitializer(initializeRadioImpl);
  registerRadioConfigurer(configureRadioImpl);
  registerRadioAddressChanger(changeRadioAddressImpl);
  registerRadioDataSender(sendRadioDataImpl);
  registerRadioDataReceiver(receiveRadioDataImpl);
}
