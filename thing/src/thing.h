#ifndef MUD_THING_H
#define MUD_THING_H

#include <stdint.h>
#include <stdbool.h>

#include "things_tiny_id.h"
#include "protocols.h"

#define THING_ERROR_LACK_OF_HOOKS -1
#define THING_ERROR_INITIALIZE_RADIO -2
#define THING_ERROR_CONFIGURE_RADIO -3
#define THING_ERROR_CHANGE_RADIO_ADDRESS -4
#define THING_ERROR_DAC_INTRODUCTION -5
#define THING_ERROR_INIT_PROTOCOL_ATTRIBUTES -6
#define THING_ERROR_SET_PROTOCOL_ATTRIBUTE -7
#define THING_ERROR_SET_PROTOCOL_TEXT -8
#define THING_ERROR_PROTOCOL_TRANSLATION -9
#define THING_ERROR_DAC_ALLOCATION -10
#define THING_ERROR_DAC_IS_CONFIGURED -11
#define THING_ERROR_DAC_NOT_CONFIGURED -12
#define THING_ERROR_DAC_CONFIGURED -13
#define THING_ERROR_INVALID_DAC_STATE -14
#define THING_ERROR_NOT_A_THING_YET -15
#define THING_ERROR_PROCESS_RECEIVED_RADIO_DATA -16
#define THING_ERROR_DO_DAQ -17
#define THING_ERROR_AQUIRE_DATA -18
#define THING_ERROR_MAKE_TINY_ID -19

#define SIZE_RADIO_ADDRESS 3
#define DAC_SERVICE_ADDRESS {0xef, 0xef, 0x1f}
#define DAC_CLIENT_ADDRESS {0xef, 0xee, 0x1f}

typedef uint8_t RadioAddress[SIZE_RADIO_ADDRESS];

typedef enum DacState {
	NONE,
	INITIAL,
	INTRODUCTING,
	ALLOCATING,
	ALLOCATED,
	CONFIGURED
} DacState;

typedef struct ThingInfo {
	char *thingId;
	DacState dacState;
	int uplinkChannelBegin;
	int uplinkChannelEnd;
	uint8_t uplinkAddressHighByte;
	uint8_t uplinkAddressLowByte;
	uint8_t *address;
} ThingInfo;

typedef struct ActionProtocolRegistration {
	ProtocolName name;
	int8_t (*processProtocol)(Protocol *);
	bool isQueryProtocol;
	struct ActionProtocolRegistration *next;
} ActionProtocolRegistration;

typedef struct DataProtocolRegistration {
	ProtocolName name;
	int8_t (*aquireData)(Protocol *);
	long samplingInterval;
	struct DataProtocolRegistration *next;
} DataProtocolRegistration;

typedef struct DaqState {
	ProtocolName name;
	long lastDaqTime;
	struct DaqState *next;
} DaqState;


typedef struct LanNotificationAndRexInfo {
	ProtocolData *lanNotificationData;
	uint8_t rexTimes;
	long nextRexTime;
} LanNotificationAndRexInfo;

void registerResetter(void (*reset)());
void registerTimer(long (*getTime)());
void registerRadioInitializer(bool (*initializeRadio)(RadioAddress address));
void registerThingIdLoader(char *(*loadThingId)());
void registerRegistrationCodeLoader(char *(*loadRegistrationCode)());
void registerRadioConfigurer(bool (*configureRadio)());
void registerRadioAddressChanger(bool (*changeRadioAddress)(RadioAddress address, bool savePersistently));
void registerThingInfoLoader(void (*loadThingInfo)(ThingInfo *thingInfo));
void registerThingInfoSaver(void (*saveThingInfo)(ThingInfo *thingInfo));
void registerThingProtocolsConfigurer(void (*configureThingProtocols)());
void registerRadioDataSender(void (*sendRadioData)(RadioAddress address, uint8_t data[], int dataSize));
void registerRadioDataReceiver(int (*receiveRadioData)(uint8_t buff[], int buffSize));
void unregisterThingHooks();

void registerActionProtocol(ProtocolName name,
	int8_t (*processProtocol)(Protocol *), bool isQueryProtocol);
bool unregisterActionProtocol(ProtocolName name);
ActionProtocolRegistration *getActionProtocolRegistration(ProtocolName name);

void registerDataProtocol(ProtocolName name, int8_t (*acquireData)(Protocol *),
	long samplingInterval);
bool unregisterDataProtocol(ProtocolName name);
DaqState *getDaqState(ProtocolName name);

int toBeAThing();
bool amIAThing();
void resetThing();
void getCurrentRadioAddress(RadioAddress address);
uint8_t getLanId();
int processReceivedData(uint8_t data[], int dataSize);
int doDaq();
void sendAndRelease(RadioAddress to, ProtocolData *pData);
int notify(TinyId requestId, Protocol *event);
int notifyWithAck(TinyId requestId, Protocol *event);
int report(TinyId requestId, Protocol *data);
int reportWithAck(TinyId requestId, Protocol *data);
long getNextRexTime(int lanId, long elapsedTime);
void setRadioDataReceivingInterval(long ms);
int doWorksAThingShouldDo();

#endif
