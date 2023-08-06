#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "thing.h"

static void (*reset)() = NULL;
static long (*getTime)() = NULL;
static bool (*initializeRadio)(RadioAddress) = NULL;
static char *(*loadThingId)() = NULL;
static char *(*loadRegistrationCode)() = NULL;
static bool (*configureRadio)() = NULL;
static bool (*changeRadioAddress)(RadioAddress, bool) = NULL;
static void (*loadThingInfo)(ThingInfo *) = NULL;
static void (*saveThingInfo)(ThingInfo *) = NULL;
static void (*configureThingProtocols)() = NULL;
static void (*sendRadioData)(RadioAddress, uint8_t[], int) = NULL;
static int (*receiveRadioData)(uint8_t[], int) = NULL;

static ThingInfo thingInfo = {NULL, NONE, NULL, NULL, NULL};
static RadioAddress currentRadioAddress = {0x00, 0x00, 0xff};

static uint8_t messages[MAX_SIZE_PROTOCOL_DATA * 2];
static int messagesLength = 0;

static const uint8_t dacServiceAddress[] = DAC_SERVICE_ADDRESS;
static const uint8_t dacClientAddress[] = DAC_CLIENT_ADDRESS;

static ActionProtocolRegistration *actionProtocolRegistrations = NULL;
static LanNotificationAndRexInfo *lanNotificationAndRexInfos = NULL;

static DataProtocolRegistration *dataProtocolRegistrations = NULL;
static DaqState *daqStates = NULL;

#define DEFAULT_RADIO_DATA_RECEIVING_INTERVAL 1000

static long radioDataReceivingInterval = DEFAULT_RADIO_DATA_RECEIVING_INTERVAL;
static long lastRadioDataReceivingTime;
static uint8_t receivedRadioData[MAX_SIZE_PROTOCOL_DATA];

void registerResetter(void (*_reset)()) {
	reset = _reset;
}

void registerTimer(long (*_getTime)()) {
	getTime = _getTime;
}

void registerRadioInitializer(bool (*_initializeRadio)(RadioAddress address)) {
	initializeRadio = _initializeRadio;
}

void registerThingIdLoader(char *(*_loadThingId)()) {
	loadThingId = _loadThingId;
}

void registerRegistrationCodeLoader(char *(*_loadRegistrationCode)()) {
	loadRegistrationCode = _loadRegistrationCode;
}

void registerRadioConfigurer(bool (*_configureRadio)()) {
	configureRadio = _configureRadio;
}

void registerRadioAddressChanger(bool (*_changeRadioAddress)(RadioAddress address, bool savePersistently)) {
	changeRadioAddress = _changeRadioAddress;
}

void registerThingInfoLoader(void (*_loadThingInfo)(ThingInfo *thingInfo)) {
	loadThingInfo = _loadThingInfo;
}

void registerThingInfoSaver(void (*_saveThingInfo)(ThingInfo *thingInfo)) {
	saveThingInfo = _saveThingInfo;
}

void registerThingProtocolsConfigurer(void (*_configureThingProtocols)()) {
	configureThingProtocols = _configureThingProtocols;
}

void registerRadioDataSender(void (*_sendRadioData)(RadioAddress address, uint8_t data[], int dataSize)) {
	sendRadioData = _sendRadioData;
}

void registerRadioDataReceiver(int (*_receiveRadioData)(uint8_t buff[], int buffSize)) {
	receiveRadioData = _receiveRadioData;
}

void registerActionProtocol(ProtocolName name,
			int8_t (*processProtocol)(Protocol *), bool isQueryProtocol) {
	ActionProtocolRegistration *newestRegistration = malloc(sizeof(ActionProtocolRegistration));
	newestRegistration->name = name;
	newestRegistration->processProtocol = processProtocol;
	newestRegistration->isQueryProtocol = isQueryProtocol;
	newestRegistration->next = NULL;

	if(actionProtocolRegistrations) {
		ActionProtocolRegistration *last = actionProtocolRegistrations;
		while(last->next) {
			last = last->next;
		}

		last->next = newestRegistration;
	}
	else {
		actionProtocolRegistrations = newestRegistration;
	}
}

bool unregisterActionProtocol(ProtocolName name) {
	if(!actionProtocolRegistrations)
		return false;

	ActionProtocolRegistration *current = actionProtocolRegistrations;
	ActionProtocolRegistration *previous = NULL;
	while(current) {
		if(current->name.ns[0] != name.ns[0] ||
			current->name.ns[1] != name.ns[1] ||
			current->name.localName != name.localName) {
			previous = current;
			current = current->next;
			continue;
		}

		if(previous) {
			previous->next = current->next;
		}
		else {
			actionProtocolRegistrations = current->next;
		}

		free(current);
		return true;
	}

	return false;
}

void registerDataProtocol(ProtocolName name, int8_t (*acquireData)(Protocol *),
			long samplingInterval) {
	DataProtocolRegistration *newestRegistration = malloc(sizeof(DataProtocolRegistration));
	newestRegistration->name = name;
	newestRegistration->aquireData = acquireData;
	newestRegistration->samplingInterval = samplingInterval;
	newestRegistration->next = NULL;

	if(dataProtocolRegistrations) {
		DataProtocolRegistration *last = dataProtocolRegistrations;
		while(last->next) {
			last = last->next;
		}

		last->next = newestRegistration;
	} else {
		dataProtocolRegistrations = newestRegistration;
	}
}

bool unregisterDataProtocol(ProtocolName name) {
	if(!dataProtocolRegistrations)
		return false;

	DataProtocolRegistration *current = dataProtocolRegistrations;
	DataProtocolRegistration *previous = NULL;
	while(current) {
		if(current->name.ns[0] != name.ns[0] ||
			current->name.ns[1] != name.ns[1] ||
			current->name.localName != name.localName) {
			previous = current;
			current = current->next;
			continue;
		}

		if(previous) {
			previous->next = current->next;
		} else {
			dataProtocolRegistrations = current->next;
		}

		free(current);
		return true;
	}

	return false;
}

DaqState *getDaqState(ProtocolName name) {
	if (!daqStates) {
		DaqState *daqState = malloc(sizeof(DaqState));
		daqState->name = name;
		daqState->lastDaqTime = 0;

		daqStates = daqState;

		return daqState;
	} else {
		DaqState *current = daqStates;
		while(true) {
			if (current->name.ns[0] == name.ns[0] &&
					current->name.ns[1] == name.ns[1] &&
						current->name.localName == name.localName)
				return current;

			if(current->next)
				current = current->next;
			else
				break;
		}

		DaqState *daqState = malloc(sizeof(DaqState));
		daqState->name = name;
		daqState->lastDaqTime = 0;
		current->next = daqState;

		return daqState;
	}
}

void unregisterThingHooks() {
	reset = NULL;
	initializeRadio = NULL;
	configureRadio = NULL;
	changeRadioAddress = NULL;
	loadThingId = NULL;
	loadRegistrationCode = NULL;
	loadThingInfo = NULL;
	saveThingInfo = NULL;
	configureThingProtocols = NULL;
	sendRadioData = NULL;
	receiveRadioData = NULL;
}

void chooseUplinkAddress(RadioAddress chosen) {
	if (thingInfo.uplinkChannelBegin == thingInfo.uplinkChannelEnd) {
		chosen[0] = thingInfo.uplinkAddressHighByte;
		chosen[1] = thingInfo.uplinkAddressLowByte;
		chosen[2] = thingInfo.uplinkChannelBegin;

		return;
	}

	int uplinkChannels = (thingInfo.uplinkChannelEnd - thingInfo.uplinkChannelEnd) + 1;
	int chosenChannelIndex;
#ifdef ARDUINO
	chosenChannelIndex = random(uplinkChannels);
#else
	srand((unsigned)getTime());
	chosenChannelIndex = rand() % uplinkChannels;
#endif
	chosen[0] = thingInfo.uplinkAddressHighByte;
	chosen[1] = thingInfo.uplinkAddressHighByte;
	chosen[2] = thingInfo.uplinkChannelEnd + chosenChannelIndex;
}

void sendAndRelease(RadioAddress to, ProtocolData *pData) {
	sendRadioData(to, pData->data, pData->dataSize);
	releaseProtocolData(pData);
}

int introduce(char *thingId, char *registrationCode) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter introduce."));
#else
	DEBUG_OUT("enter introduce.");
#endif	
	Protocol introduction = createProtocol(NAME_TUXP_PROTOCOL_INTRODUCTION);
	
	if(addStringAttribute(&introduction, NAME_ATTRIBUTE_THING_ID_TUXP_PROTOCOL_INTRODUCTION,
			thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;
	
	if (addBytesAttribute(&introduction, NAME_ATTRIBUTE_ADDRESS_TUXP_PROTOCOL_INTRODUCTION,
			dacClientAddress, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if (setText(&introduction, registrationCode) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;
	
	ProtocolData pData = {NULL, 0};
	if (translateAndRelease(&introduction, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;
	
	thingInfo.dacState = INTRODUCTING;

#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("Sending introduction protocol to DAC service...."));
#else
	DEBUG_OUT("Sending introduction protocol to DAC service....");
#endif
	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

int isConfigured(char *thingId) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter isConfigured."));
#else
	DEBUG_OUT("enter isConfigured.");
#endif

	Protocol isConfigured = createProtocol(NAME_TUXP_PROTOCOL_IS_CONFIGURED);
	if(addBytesAttribute(&isConfigured, NAME_ATTRIBUTE_ADDRESS_TUXP_PROTOCOL_IS_CONFIGURED,
				dacClientAddress, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if(setText(&isConfigured, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData = {NULL, 0};
	if(translateAndRelease(&isConfigured, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;

	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

bool checkHooks() {
	if (loadThingInfo == NULL ||
			saveThingInfo == NULL ||
			loadThingId == NULL ||
			loadRegistrationCode == NULL ||
			initializeRadio == NULL ||
			configureRadio == NULL ||
			changeRadioAddress == NULL ||
			configureThingProtocols == NULL ||
			sendRadioData== NULL ||
			receiveRadioData == NULL ||
			reset == NULL ||
			getTime == NULL) {
		return false;
	}

	return true;
}

bool isAddressSame(RadioAddress address1, RadioAddress address2) {
	for (int i = 0; i < SIZE_RADIO_ADDRESS; i++) {
		if (address1[i] != address2[i])
			return false;
	}

	return true;
}

int doDac() {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter doDac."));
#else
	DEBUG_OUT("enter doDac.");
#endif

	if (!isAddressSame(currentRadioAddress, dacClientAddress)) {
		if (!changeRadioAddress(dacClientAddress, false))
			return debugErrorAndReturn("doDac", THING_ERROR_CHANGE_RADIO_ADDRESS);

		memcpy(currentRadioAddress, dacClientAddress, 3);
	}

	if (thingInfo.dacState == INITIAL) {
		int result = introduce(thingInfo.thingId, loadRegistrationCode());
		if(result != 0) {
			thingInfo.dacState = INITIAL;
			return debugErrorAndReturn("doDac", THING_ERROR_DAC_INTRODUCTION);
		}
	} else if (thingInfo.dacState == ALLOCATED) {
		int result = isConfigured(thingInfo.thingId);
		if(result != 0) {
			return debugErrorAndReturn("doDac", THING_ERROR_DAC_IS_CONFIGURED);
		}
	} else {
		return debugErrorAndReturn("doDac", THING_ERROR_INVALID_DAC_STATE);
	}

	return 0;
}

void cleanMessages() {
	messagesLength = 0;
}

int toBeAThing() {
	if (!checkHooks())
		return debugErrorAndReturn("toBeAThing", THING_ERROR_LACK_OF_HOOKS);

	cleanMessages();
	loadThingInfo(&thingInfo);

	if (!initializeRadio(currentRadioAddress)) {
		return debugErrorAndReturn("toBeAThing", THING_ERROR_INITIALIZE_RADIO);
	}
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("Radio has initialized."));
#else
	DEBUG_OUT("Radio has initialized.");
#endif

	if (thingInfo.dacState == CONFIGURED) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
		Serial.println(F("Thing has configured."));
#else
		DEBUG_OUT("Thing has configured.");
#endif

		if (!isAddressSame(currentRadioAddress, thingInfo.address)) {
			if (!changeRadioAddress(thingInfo.address, true))
				return debugErrorAndReturn("toBeAThing", THING_ERROR_CONFIGURE_RADIO);
			memcpy(currentRadioAddress, thingInfo.address, 3);
		}

		configureThingProtocols();

#if defined(ARDUINO) && defined(ENABLE_DEBUG)
		Serial.println(F("I'm a thing now!!!"));
#else
		DEBUG_OUT("I'm a thing now!!!");
#endif

		return 0;
	} else {
		if (thingInfo.thingId == NULL) {
			thingInfo.thingId = loadThingId();

			if(!configureRadio())
				return debugErrorAndReturn("toBeAThing", THING_ERROR_CONFIGURE_RADIO);

#if defined(ARDUINO) && defined(ENABLE_DEBUG)
			Serial.println(F("Radio has configured."));
#else
			DEBUG_OUT("Radio has configured.");
#endif

			thingInfo.dacState = INITIAL;
			
			saveThingInfo(&thingInfo);
		}

		return doDac();
	}
}

bool amIAThing() {
	return thingInfo.dacState == CONFIGURED;
}

void resetThing() {
	loadThingInfo(&thingInfo);

	thingInfo.address = NULL;
	thingInfo.uplinkChannelBegin = -1;
	thingInfo.uplinkChannelEnd = -1;
	thingInfo.uplinkAddressHighByte = 0xff;
	thingInfo.uplinkAddressLowByte = 0xff;
	thingInfo.dacState = INITIAL;

	saveThingInfo(&thingInfo);
}

void getCurrentRadioAddress(RadioAddress address) {
	for (int i = 0; i < SIZE_RADIO_ADDRESS; i++)
		address[i] = currentRadioAddress[i];
}

uint8_t getLanId() {
	return currentRadioAddress[1];
}

int findProtocolStartPosition() {
	for (int i = 0; i < messagesLength - 1; i++) {
		if (messages[i] == FLAG_DOC_BEGINNING_END) {
			if (i - 1 >= 0 && messages[i - 1] == FLAG_ESCAPE)
				continue;

			if (messages[i + 1] == FLAG_DOC_BEGINNING_END)
				continue;

			return i;
		}
	}

	return -1;
}

int findProtocolEndPosition(int startPosition) {
	for(int i = startPosition + 1; i < messagesLength; i++) {
		if(i - 1 >= 0 && messages[i - 1] == FLAG_ESCAPE)
			continue;

		if(messages[i] == FLAG_DOC_BEGINNING_END) {
			return i;
		}
	}

	return -1;
}

int processProtocol(uint8_t data[], int size) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processProtocol."));
#else
	DEBUG_OUT("enter processProtocol.");
#endif

	ProtocolData pData = {data, size};
	if (isLanExecution(&pData)) {
		TinyId requestId;
		Protocol action = createEmptyProtocol();
		if(parseLanExecution(&pData, requestId, &action) != 0) {
			releaseProtocol(&action);
			return TUXP_ERROR_FAILED_TO_PARSE_PROTOCOL;
		}

		ActionProtocolRegistration *registration = getActionProtocolRegistration(action.name);
		if(!registration) {
			releaseProtocol(&action);
		 	return TUXP_ERROR_UNKNOWN_PROTOCOL_NAME;
		}

		if(!registration->processProtocol) {
			releaseProtocol(&action);
			return TUXP_ERROR_NO_REGISTRATED_PROCESSOR;
		}

		int8_t errorNumber = registration->processProtocol(&action);
		releaseProtocol(&action);

		if (registration->isQueryProtocol)
			return 0;

		ProtocolData pDataLanAnswer = {NULL, 0};
		if (errorNumber == 0) {
			LanAnswer answer = createLanResonse(requestId);
			if (translateLanAnswer(&answer, &pDataLanAnswer) != 0) {
				releaseProtocolData(&pDataLanAnswer);
				return TUXP_ERROR_FAILED_TO_TRANSLATE_ANSWER;
			}
		} else {
			LanAnswer answer = createLanError(requestId, errorNumber);

			if(translateLanAnswer(&answer, &pDataLanAnswer) != 0) {
				releaseProtocolData(&pDataLanAnswer);
				return TUXP_ERROR_FAILED_TO_TRANSLATE_ANSWER;
			}
		}

		RadioAddress chosen;
		chooseUplinkAddress(chosen);
		sendAndRelease(chosen, &pDataLanAnswer);
		return 0;
	} else {
		Protocol protocol;
		int result = parseProtocol(&pData, &protocol);
		if (result != 0) {
			releaseProtocol(&protocol);
			return result;
		}

		ActionProtocolRegistration *registration = getActionProtocolRegistration(protocol.name);
		if (!registration) {
			releaseProtocol(&protocol);
			return TUXP_ERROR_UNKNOWN_PROTOCOL_NAME;
		}

		if (!registration->processProtocol) {
			releaseProtocol(&protocol);
			return TUXP_ERROR_NO_REGISTRATED_PROCESSOR;
		}

		registration->processProtocol(&protocol);
		releaseProtocol(&protocol);

		return 0;
	}
}

int processAsAThing(uint8_t data[], int size) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processAsAThing."));
#else
	DEBUG_OUT("enter processAsAThing.");
#endif

	return processProtocol(data, size);
}

int allocated(int uplinkChannelBegin, int uplinkChannelEnd, uint8_t uplinkAddressHighByte,
	uint8_t uplinkAddressLowByte, uint8_t *allocatedAddress) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter allocated."));
#else
	DEBUG_OUT("enter allocated.");
#endif

	thingInfo.uplinkChannelBegin = uplinkChannelBegin;
	thingInfo.uplinkChannelEnd = uplinkChannelEnd;
	thingInfo.uplinkAddressHighByte = uplinkAddressHighByte;
	thingInfo.uplinkAddressLowByte = uplinkAddressLowByte;

	thingInfo.address = malloc(sizeof(uint8_t) * allocatedAddress[0]);
	if (!thingInfo.address)
		return TUXP_ERROR_OUT_OF_MEMEORY;
	memcpy(thingInfo.address, allocatedAddress + 1, allocatedAddress[0]);

	thingInfo.dacState = ALLOCATED;

	saveThingInfo(&thingInfo);

	Protocol allocated = createProtocol(NAME_TUXP_PROTOCOL_ALLOCATED);
	if (setText(&allocated, thingInfo.thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData = {NULL, 0};
	if(translateAndRelease(&allocated, &pData) != 0) {
		releaseProtocolData(&pData);
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;
	}

	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

int processAllocation(Protocol *allocation) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processAllocation."));
#else
	DEBUG_OUT("enter processAllocation.");
#endif

	int uplinkChannelBegin;
	if (!getIntAttributeValue(allocation, NAME_ATTRIBUTE_UPLINK_CHANNEL_BEGIN_TUXP_PROTOCOL_ALLOCATION, &uplinkChannelBegin))
		return TUXP_ERROR_LACK_OF_ALLOCATION_PARAMETERS;

	int uplinkChannelEnd;
	if(!getIntAttributeValue(allocation, NAME_ATTRIBUTE_UPLINK_CHANNEL_END_TUXP_PROTOCOL_ALLOCATION, &uplinkChannelEnd))
		return TUXP_ERROR_LACK_OF_ALLOCATION_PARAMETERS;

	uint8_t *uplinkAddress = getBytesAttributeValue(allocation, NAME_ATTRIBUTE_UPLINK_ADDRESS_TUXP_PROTOCOL_ALLOCATION);
	if (!uplinkAddress || uplinkAddress[0] != 0x02)
		return TUXP_ERROR_LACK_OF_ALLOCATION_PARAMETERS;

	uint8_t *allocatedAddress = getBytesAttributeValue(allocation, NAME_ATTRIBUTE_ALLOCATED_ADDRESS_TUXP_PROTOCOL_ALLOCATION);
	if (!allocatedAddress)
		return TUXP_ERROR_LACK_OF_ALLOCATION_PARAMETERS;

	if (allocatedAddress[0] != SIZE_RADIO_ADDRESS)
		return TUXP_ERROR_ILLEGAL_ALLOCATED_ADDRESS;

	return allocated(uplinkChannelBegin, uplinkChannelEnd,
		uplinkAddress[1], uplinkAddress[2], allocatedAddress);
}

void processNotConfigured() {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processNotConfigured."));
#else
	DEBUG_OUT("enter processNotConfigured.");
#endif

	thingInfo.uplinkChannelBegin = -1;
	thingInfo.uplinkChannelEnd = -1;
	thingInfo.uplinkAddressHighByte = 0xff;
	thingInfo.uplinkAddressLowByte = 0xff;
	thingInfo.address = NULL;
	thingInfo.dacState = INITIAL;

	saveThingInfo(&thingInfo);

	reset();
}

bool processConfigured() {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processConfigured."));
#else
	DEBUG_OUT("enter processConfigured.");
#endif

	cleanMessages();

	if (!isAddressSame(currentRadioAddress, thingInfo.address) &&
			!changeRadioAddress(thingInfo.address, true)) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
		Serial.println(F("Can't to be a thing because can't change radio address."));
#else
		DEBUG_OUT("Can't to be a thing because can't change radio address.");
#endif
		return false;
	}

	memcpy(currentRadioAddress, thingInfo.address, 3);

	configureThingProtocols();

	thingInfo.dacState = CONFIGURED;
	saveThingInfo(&thingInfo);

#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("I'm a thing now!!!"));
#else
	DEBUG_OUT("I'm a thing now!!!");
#endif

	return true;
}

int processDac(uint8_t data[], int size) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processDac."));
#else
	DEBUG_OUT("enter processDac.");
#endif

	ProtocolData pData = {data, size};

	if (thingInfo.dacState == INTRODUCTING) {
		if (!isProtocol(&pData, NAME_TUXP_PROTOCOL_ALLOCATION)) {
			return debugErrorAndReturn("processDac", TUXP_ERROR_NOT_SUITABLE_DAC_PROTOCOL);
		}

		Protocol allocation = createEmptyProtocol();

		int result = parseProtocol(&pData, &allocation);
		if (result != 0) {
			releaseProtocol(&allocation);
			return debugErrorAndReturn("processDac", TUXP_ERROR_FAILED_TO_PARSE_PROTOCOL);
		}

		result = processAllocation(&allocation);
		releaseProtocol(&allocation);
		if (result != 0)
			return debugErrorDetailAndReturn("processDac", THING_ERROR_DAC_ALLOCATION, result);

		return 0;
	} else if (thingInfo.dacState == ALLOCATED) {
		if (isBareProtocol(&pData, NAME_TUXP_PROTOCOL_NOT_CONFIGURED)) {
			processNotConfigured();
			return 0;
		} else if (isBareProtocol(&pData, NAME_TUXP_PROTOCOL_CONFIGURED)) {
			if (!processConfigured())
				return debugErrorAndReturn("processDac", THING_ERROR_CHANGE_RADIO_ADDRESS);

			return 0;
		} else {
			return debugErrorAndReturn("processDac", TUXP_ERROR_NOT_SUITABLE_DAC_PROTOCOL);
		}
	} else {
		return TUXP_ERROR_INVALID_DAC_STATE;
	}

	return 0;
}

int processReceivedData(uint8_t data[], int dataSize) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter processReceivedData."));
#else
	DEBUG_OUT("enter processReceivedData.");
#endif

	if(dataSize > MAX_SIZE_PROTOCOL_DATA * 2)
		return TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE;

	if(messagesLength + dataSize > MAX_SIZE_PROTOCOL_DATA * 2) {
		cleanMessages();
		return TUXP_ERROR_MESSAGES_BUFF_OVERFLOW;
	}

	memcpy(messages + messagesLength, data, dataSize);
	messagesLength += dataSize;

	int protocolStartPosition = findProtocolStartPosition();
	if(protocolStartPosition == -1) {
		cleanMessages();
		return TUXP_ERROR_ABANDON_MALFORMED_DATA;
	}

	int protocolEndPosition = findProtocolEndPosition(protocolStartPosition);
	if(protocolEndPosition == -1)
		return TUXP_ERROR_WAITING_DATA;

#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("Found a protocol. Process it."));
#else
	DEBUG_OUT("Found a protocol. Process it.");
#endif
	
	int protocolSize = protocolEndPosition - protocolStartPosition + 1;
	uint8_t protocolData[MAX_SIZE_PROTOCOL_DATA];
	memcpy(protocolData, messages + protocolStartPosition, protocolSize);

	if(protocolEndPosition == messagesLength - 1) {
		cleanMessages();
	} else {
		messagesLength = messagesLength - (protocolEndPosition + 1);
		memcpy(messages, messages + protocolEndPosition + 1, messagesLength);
	}

	int result;
	if (thingInfo.dacState == CONFIGURED) {
		result = processAsAThing(protocolData, protocolSize);
	} else {
		result = processDac(protocolData, protocolSize);
	}

	return result;
}

int notify(TinyId requestId, Protocol *event) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter notify."));
#else
	DEBUG_OUT("enter notify.");
#endif
	if (!amIAThing())
		return debugErrorAndReturn("notify", THING_ERROR_NOT_A_THING_YET);

	ProtocolData pData = {NULL, 0};
	int result = translateLanNotification(requestId,  event, false, &pData);
	if (result != 0) {
		releaseProtocolData(&pData);
		return debugErrorDetailAndReturn("notify", THING_ERROR_PROTOCOL_TRANSLATION, result);
	}

	RadioAddress chosen;
	chooseUplinkAddress(chosen);
	sendAndRelease(chosen, &pData);

	return 0;
}

int notifyWithAck(TinyId requestId, Protocol *event) {
	return -1;
}

int report(TinyId requestId, Protocol *data) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter report."));
#else
	DEBUG_OUT("enter report.");
#endif

	if(!amIAThing())
		return debugErrorAndReturn("report", THING_ERROR_NOT_A_THING_YET);

	ProtocolData pData = {NULL, 0};
	int result = translateLanReport(requestId, data, false, &pData);
	if(result != 0) {
		releaseProtocolData(&pData);
		return debugErrorDetailAndReturn("report", THING_ERROR_PROTOCOL_TRANSLATION, result);
	}

	RadioAddress chosen;
	chooseUplinkAddress(chosen);
	sendAndRelease(chosen, &pData);

	return 0;
}

int reportWithAck(TinyId requestId, Protocol *data) {
	return -1;
}

long getNextRexTime(int lanId, long elapsedTime) {
	return -1;
}


ActionProtocolRegistration *getActionProtocolRegistration(ProtocolName name) {
	ActionProtocolRegistration *current = actionProtocolRegistrations;
	while(current) {
		if(current->name.ns[0] == name.ns[0] &&
			current->name.ns[1] == name.ns[1] &&
			current->name.localName == name.localName) {

			return current;
		}

		current = current->next;
	}

	return NULL;
}

void setRadioDataReceivingInterval(long interval) {
	radioDataReceivingInterval = interval;
}

int receiveAndProcessRadioData() {
	long currentTime = getTime();
	if(lastRadioDataReceivingTime != 0 &&
		(currentTime - lastRadioDataReceivingTime) < radioDataReceivingInterval) {
		return 0;
	}

	int receivedRadioDataSize = receiveRadioData(receivedRadioData, MAX_SIZE_PROTOCOL_DATA);
	lastRadioDataReceivingTime = currentTime;

	if(receivedRadioDataSize == 0)
		return 0;

	return processReceivedData(receivedRadioData, receivedRadioDataSize);
}

int doDaq() {
	if (!dataProtocolRegistrations)
		return 0;

	DataProtocolRegistration *current = dataProtocolRegistrations;
	while (current) {
		DaqState *daqState = getDaqState(current->name);

		long currentTime = getTime();
		if (daqState->lastDaqTime != 0 &&
				(currentTime - daqState->lastDaqTime) < current->samplingInterval)
			continue;

		Protocol protocol = createProtocol(current->name);
		int result = current->aquireData(&protocol);
		if (result != 0) {
			return debugErrorDetailAndReturn("doDaq", THING_ERROR_AQUIRE_DATA, result);
		}

		daqState->lastDaqTime = currentTime;

		TinyId requestId;
		result = makeTinyId(getLanId(), REQUEST, currentTime, requestId);
		if(result != 0)
			return debugErrorDetailAndReturn("doDaq", THING_ERROR_MAKE_TINY_ID, result);

		report(requestId, &protocol);
		releaseProtocol(&protocol);

		current = current->next;
	}

	return 0;
}

int doWorksAThingShouldDo() {
	int result = receiveAndProcessRadioData();
	if(result != 0) {
		return debugErrorDetailAndReturn("doWorksAThingShouldDo",
			THING_ERROR_PROCESS_RECEIVED_RADIO_DATA, result);
	}

	if (!amIAThing())
		return 0;

	result = doDaq();
	if(result != 0) {
		return debugErrorDetailAndReturn("doWorksAThingShouldDo", THING_ERROR_DO_DAQ, result);
	}

	return 0;
}
