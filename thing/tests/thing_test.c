#include <string.h>

#include "unity.h"

#include "debug.h"
#include "tacp.h"
#include "thing.h"

static const ProtocolName NAME_PROTOCOL_FLASH = {{0xf7, 0x01}, 0x00};
#define NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH 0x01

static const char *thingId = "SL-LE01-C980AFE9";
static ThingInfo thingInfoInStorage = {NULL, NONE, NULL, NULL, NULL};
static int resetTimes = 0;
static DacState dacState = INITIAL;
static const uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
static const uint8_t dacClientAddress[] = {0xef, 0xee, 0x1f};
static const uint8_t configuredNodeAddress[] = {0x00, 0x01, 0x17};
static const uint8_t gatewayUplinkAddress[] = {0x00, 0xef, 0x17};
static uint8_t nodeAddress[] = {0x00, 0x00, 0x00};

static bool flashProcessed = false;
static int lanAnswerTimes = 0;

void resetImpl() {}

bool changeRadioAddressImpl(RadioAddress address, bool savePersistently) {
	memcpy(nodeAddress, address, SIZE_RADIO_ADDRESS);
	return true;
}

bool initializeRadioImpl(RadioAddress address) {
	return true;
}

char *generateThingIdImpl() {
	return "SL-LE01-C980AFE9";
}

bool configureRadioImpl() {
	return true;
}

int8_t processFlash(Protocol *protocol) {
	int repeat;
	TEST_ASSERT_TRUE(getIntAttributeValue(protocol, NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH, &repeat));

	if (repeat == 5) {
		flashProcessed = true;
		return 0;
	}
	
	if (repeat != 14)
		TEST_FAIL_MESSAGE("Invalid repeat value for flash action.");

	return -1;
}

void configureThingProtocolsImpl() {
	ProtocolName flashProtocolName = {{0xf7, 0x01}, 0x00};
	registerActionProtocol(flashProtocolName, processFlash, false);
}

void loadThingInfoImpl(ThingInfo *thingInfo) {
	thingInfo->thingId = thingInfoInStorage.thingId;
	thingInfo->dacState = thingInfoInStorage.dacState;
	thingInfo->address = thingInfoInStorage.address;
	thingInfo->gatewayUplinkAddress = thingInfoInStorage.gatewayUplinkAddress;
	thingInfo->gatewayDownlinkAddress = thingInfoInStorage.gatewayDownlinkAddress;
}

void saveThingInfoImpl(ThingInfo *thingInfo) {
	thingInfoInStorage.thingId = thingInfo->thingId;
	thingInfoInStorage.address = thingInfo->address;
	thingInfoInStorage.gatewayUplinkAddress = thingInfo->gatewayUplinkAddress;
	thingInfoInStorage.gatewayDownlinkAddress = thingInfo->gatewayDownlinkAddress;
	thingInfoInStorage.dacState = thingInfo->dacState;
}

void sendToGatewayMock1(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	if (dacState == INITIAL) {
		TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);

		TEST_ASSERT_EQUAL_INT(29, dataSize);
		uint8_t expectedIntroductionData[] = {
			0xff,
				0xf8, 0x03, 0x00, 0x01, 0x80,
					0x01, 0xfb, 0xef, 0xee, 0x1f, 0xfe,
					0x53, 0x4c, 0x2d, 0x4c, 0x45, 0x30, 0x31, 0x2d, 0x43, 0x39, 0x38, 0x30, 0x41, 0x46, 0x45, 0x39,
			0xff
		};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedIntroductionData, data, dataSize);

		ProtocolData pDataIntroduction = {data, dataSize};
		TEST_ASSERT_TRUE(isProtocol(&pDataIntroduction, NAME_TACP_PROTOCOL_INTRODUCTION));
		
		Protocol introduction = createEmptyProtocol();
		TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataIntroduction, &introduction));
		
		uint8_t *actualAddress = getBytesAttributeValue(&introduction, NAME_ATTRIBUTE_ADDRESS_TACP_PROTOCOL_INTRODUCTION);
		TEST_ASSERT_NOT_NULL(actualAddress);
		TEST_ASSERT_EQUAL_UINT8(0x03, actualAddress[0]);
		TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, actualAddress + 1, 3);

		char *text = getText(&introduction);
		TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, text, strlen(thingId));

		Protocol allocation = createProtocol(NAME_TACP_PROTOCOL_ALLOCATION);

		uint8_t gatewayUplinkAddress[] = {0x00, 0xef, 0x17};
		addBytesAttribute(&allocation,
			NAME_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS_TACP_PROTOCOL_ALLOCATION, gatewayUplinkAddress, 3);

		uint8_t gatewayDownlinkAddress[] = {0x00, 0x00, 0x17};
		addBytesAttribute(&allocation,
			NAME_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS_TACP_PROTOCOL_ALLOCATION, gatewayDownlinkAddress, 3);

		uint8_t allocatedAddress[] = {0x00, 0x01, 0x17};
		addBytesAttribute(&allocation,
			NAME_ATTRIBUTE_ALLOCATED_ADDRESS_TACP_PROTOCOL_ALLOCATION, allocatedAddress, 3);

		ProtocolData pDataAllocation;
		TEST_ASSERT_EQUAL_INT(0, translateAndRelease(&allocation, &pDataAllocation));

		dacState = ALLOCATING;

		TEST_ASSERT_EQUAL(0, processReceivedData(pDataAllocation.data, pDataAllocation.dataSize));
		releaseProtocolData(&pDataAllocation);
	} else if (dacState == ALLOCATING) {
		ProtocolData pDataAllocated = {data, dataSize};
		TEST_ASSERT_TRUE(isProtocol(&pDataAllocated, NAME_TACP_PROTOCOL_ALLOCATED));

		Protocol allocated = createEmptyProtocol();
		TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataAllocated, &allocated));

		char *text = getText(&allocated);
		TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, text, strlen(thingId));
	}
}

void sendToGatewayMock2(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	ProtocolData pDataIsConfigured = {data, dataSize};
	TEST_ASSERT_TRUE(isProtocol(&pDataIsConfigured, NAME_TACP_PROTOCOL_IS_CONFIGURED));

	Protocol isConfigured = createEmptyProtocol();
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataIsConfigured, &isConfigured));

	uint8_t *nodeAddress = getBytesAttributeValue(&isConfigured, NAME_ATTRIBUTE_ADDRESS_TACP_PROTOCOL_IS_CONFIGURED);
	uint8_t expectedNodeAddress[] = {0x03, 0xef, 0xee, 0x1f};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedNodeAddress, nodeAddress, 4);

	char *nodeThingId = getText(&isConfigured);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, nodeThingId, strlen(thingId));

	Protocol notConfigured = createProtocol(NAME_TACP_PROTOCOL_NOT_CONFIGURED);
	ProtocolData pDataNotConfigured;
	TEST_ASSERT_EQUAL_INT(0,translateAndRelease(&notConfigured, &pDataNotConfigured));

	dacState = INITIAL;

	TEST_ASSERT_EQUAL(0, processReceivedData(pDataNotConfigured.data, pDataNotConfigured.dataSize));
	releaseProtocolData(&pDataNotConfigured);
}

void sendToGatewayMock3(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	sendToGatewayMock1(address, data, dataSize);
	if(dacState == ALLOCATING) {
		Protocol configured = createProtocol(NAME_TACP_PROTOCOL_CONFIGURED);

		ProtocolData pDataConfigured;
		TEST_ASSERT_EQUAL_INT(0, translateAndRelease(&configured, &pDataConfigured));

		dacState = INITIAL;

		TEST_ASSERT_EQUAL(0, processReceivedData(pDataConfigured.data, pDataConfigured.dataSize));
		releaseProtocolData(&pDataConfigured);
	}
}

void sendToGatewayMock4(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(gatewayUplinkAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(configuredNodeAddress, nodeAddress, 3);

	ProtocolData pData = {data, dataSize};
	TEST_ASSERT_TRUE(isLanAnswer(&pData));

	lanAnswerTimes++;

	LanAnswer answer;
	TEST_ASSERT_EQUAL(0, parseLanAnswer(&pData, &answer));

	if (lanAnswerTimes == 1) {
		TEST_ASSERT_TRUE(isResponseTinyId(answer.traceId));
	} else if (lanAnswerTimes == 2) {
		TEST_ASSERT_TRUE(isErrorTinyId(answer.traceId));
		TEST_ASSERT_EQUAL_INT8(-1, answer.errorNumber);
	} else {
		TEST_FAIL_MESSAGE("Too many LAN answers.");
	}
}

int receiveRadioDataImpl(uint8_t buffer[], int bufferSize) {
	return 0;
}

void registerThingHooks() {
	registerRadioInitializer(initializeRadioImpl);
	registerThingIdGenerator(generateThingIdImpl);
	registerRadioConfigurer(configureRadioImpl);
	registerRadioAddressChanger(changeRadioAddressImpl);
	registerThingInfoLoader(loadThingInfoImpl);
	registerThingInfoSaver(saveThingInfoImpl);
	registerResetter(resetImpl);
	registerRadioDataReceiver(receiveRadioDataImpl);
	registerThingProtocolsConfigurer(configureThingProtocolsImpl);
}

void debutOutputImpl(const char *message) {
	printf("%s\r\n", message);
}

void setUp() {
	setDebugOutputter(debutOutputImpl);

	registerThingHooks();

	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);

	if(thingInfo.dacState == NONE) {
		registerRadioDataSender(sendToGatewayMock1);
	} else if(thingInfo.dacState == INITIAL) {
		if (resetTimes == 2) {
			registerRadioDataSender(sendToGatewayMock3);
		} else {
			char message[128];
			sprintf(message, "Unexpected DAC state. DAC state: %d. Reset times: %d.",
				thingInfo.dacState, resetTimes);
			TEST_FAIL_MESSAGE(message);
		}
	} else if(thingInfo.dacState == ALLOCATED) {
		registerRadioDataSender(sendToGatewayMock2);
	} else if (thingInfo.dacState == CONFIGURED) {
		registerRadioDataSender(sendToGatewayMock4);
	} else {
		char message[128];
		sprintf(message, "Unexpected DAC state. DAC state: %d. Reset times: %d.",
			thingInfo.dacState, resetTimes);
		TEST_FAIL_MESSAGE(message);
	}

	flashProcessed = false;
}

void tearDown() {
	resetImpl();
	resetTimes++;
}

void testLoraDacAllocated() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);

	TEST_ASSERT_EQUAL_INT(NONE, thingInfo.dacState);
	TEST_ASSERT_NULL(thingInfo.address);
	TEST_ASSERT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NULL(thingInfo.gatewayDownlinkAddress);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	uint8_t expectedGatewayUplinkAddress[] = {0x00, 0xef, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayUplinkAddress, thingInfo.gatewayUplinkAddress, 3);
	uint8_t expectedGatewayDownlinkAddress[] = {0x00, 0x00, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayDownlinkAddress, thingInfo.gatewayDownlinkAddress, 3);
	uint8_t expectedAddress[] = {0x00, 0x01, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, thingInfo.address, 3);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);
}

void testLoraDacNotConfigured() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);

	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);
	TEST_ASSERT_NULL(thingInfo.address);
	TEST_ASSERT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NULL(thingInfo.gatewayDownlinkAddress);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);
}

void testLoraDacConfigured() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(configuredNodeAddress, nodeAddress, 3);
}

void testProcessFlashAction() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	TEST_ASSERT_EQUAL_UINT8_ARRAY(configuredNodeAddress, nodeAddress, 3);

	uint32_t passedTimeThisDay =
		10 * (60 * 60 * 1000) +
		37 * (60 * 1000) +
		02 * 1000 +
		435;

	TinyId requestId;
	if(makeTinyId(0, REQUEST, passedTimeThisDay, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");

	Protocol flash = createProtocol(NAME_PROTOCOL_FLASH);
	TEST_ASSERT_EQUAL(0, addIntAttribute(&flash, NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH, 5));

	ProtocolData pData;
	TEST_ASSERT_EQUAL(0, translateLanExecution(requestId, &flash, &pData));

	passedTimeThisDay =
		11 * (60 * 60 * 1000) +
		23 * (60 * 1000) +
		52 * 1000 +
		997;

	if(makeTinyId(0, REQUEST, passedTimeThisDay, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");

	uint8_t expectedTinyIdData[] = {0x00, 0x0b, 0x17, 0xd3, 0xe5};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedTinyIdData, requestId, 5);

	Protocol flashWithErrorAttribute = createProtocol(NAME_PROTOCOL_FLASH);
	TEST_ASSERT_EQUAL(0, addIntAttribute(&flashWithErrorAttribute, NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH, 14));

	ProtocolData pDataProtocolWithErrorAttribute;
	TEST_ASSERT_EQUAL(0, translateLanExecution(requestId, &flashWithErrorAttribute, &pDataProtocolWithErrorAttribute));

	uint8_t expectedLanExecutionData[] = {
		0xff,
			0xf8, 0x04, 0x05, 0x01, 0x01,
				0x06, 0xfb, 0x00, 0x0b, 0x17, 0xd3, 0xe5, 0xfe,
				0xf7, 0x01, 0x00, 0x01, 0x00,
					0x01, 0x31, 0x34,
		0xff
	};
	TEST_ASSERT_EQUAL(sizeof(expectedLanExecutionData), pDataProtocolWithErrorAttribute.dataSize);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedLanExecutionData, pDataProtocolWithErrorAttribute.data, sizeof(expectedLanExecutionData));

	uint8_t data[MAX_SIZE_PROTOCOL_DATA * 2];
	memcpy(data, pData.data, pData.dataSize);
	memcpy(data + pData.dataSize, pDataProtocolWithErrorAttribute.data, pDataProtocolWithErrorAttribute.dataSize);

	TEST_ASSERT_EQUAL_UINT8(TACP_ERROR_WAITING_DATA, processReceivedData(data, pData.dataSize - 5));
	TEST_ASSERT_EQUAL_UINT8(0, processReceivedData(data + (pData.dataSize - 5), 10));
	TEST_ASSERT_TRUE(flashProcessed);

	TEST_ASSERT_EQUAL_UINT8(0, processReceivedData(data + (pData.dataSize + 5), pDataProtocolWithErrorAttribute.dataSize - 5));

	releaseProtocolData(&pData);
	releaseProtocolData(&pDataProtocolWithErrorAttribute);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testLoraDacAllocated);
	RUN_TEST(testLoraDacNotConfigured);
	RUN_TEST(testLoraDacConfigured);
	RUN_TEST(testProcessFlashAction);
	
	return UNITY_END();
}
