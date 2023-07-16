#include "unity.h"

#include "tuxp.h"

#define CHANGE_MODE_ACTION_ERROR_INVLID_MODE -1

static const ProtocolName NAME_PROTOCOL_INTRODUCTION = {{0xf8, 0x02}, 0x00};
#define NAME_ATTRIBUTE_ADDRESS_PROTOCOL_INTRODUCTION 0x02

static const ProtocolName NAME_PROTOCOL_FLASH = {{0xf7, 0x01}, 0x00};
#define NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH 0x01

struct Flash {
	int repeat;
};

void setUp() {}

void tearDown() {}

void testParseInboundProtocols(void) {
	// Flash
	uint8_t flashData[] = {
		0xff,
			0xf7, 0x01, 0x00, 0x01, 0x00,
				0x01, 0xfc, 0x35,
		0xff
	};

	ProtocolData pDataFlash = CREATE_PROTOCOL_DATA(flashData);
	TEST_ASSERT_TRUE(isProtocol(&pDataFlash, NAME_PROTOCOL_FLASH));

	Protocol flash;
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataFlash, &flash));
	TEST_ASSERT_EQUAL_INT(1, getAttributesSize(&flash));
	TEST_ASSERT_EQUAL_INT(NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH, flash.attributes->name);

	int repeat;
	TEST_ASSERT_TRUE(getIntAttributeValue(&flash, NAME_ATTRIBUTE_REPEAT_PROTOCOL_FLASH, &repeat));
	TEST_ASSERT_EQUAL_INT(5, repeat);

	releaseProtocol(&flash);

	// Introduction
	uint8_t introductionData[] = {
		0xff,
			0xf8, 0x02, 0x00, 0x01, 0x80,
				0x02, 0xfb, 0xef, 0xee, 0x1f, 0xfe,
				0x53, 0x4c, 0x2d, 0x4c, 0x45, 0x30, 0x31, 0x2d, 0x43, 0x39, 0x38, 0x30, 0x41, 0x46, 0x45, 0x39,
		0xff
	};

	ProtocolData pDataIntroduction = CREATE_PROTOCOL_DATA(introductionData);
	TEST_ASSERT_TRUE(isProtocol(&pDataIntroduction, NAME_PROTOCOL_INTRODUCTION));

	Protocol introduction;
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataIntroduction, &introduction));
	TEST_ASSERT_EQUAL_INT(1, getAttributesSize(&introduction));
	uint8_t *address = getBytesAttributeValue(&introduction, NAME_ATTRIBUTE_ADDRESS_PROTOCOL_INTRODUCTION);
	TEST_ASSERT_NOT_NULL(address);
	uint8_t expectedAddress[] = {0x03, 0xef, 0xee, 0x1f};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, address, 4);

	char *thingId = getText(&introduction);
	TEST_ASSERT_EQUAL_STRING("SL-LE01-C980AFE9", thingId);

	releaseProtocol(&introduction);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testParseInboundProtocols);
	
	return UNITY_END();
}
