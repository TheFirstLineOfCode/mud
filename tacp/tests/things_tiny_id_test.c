#include "unity.h"

#include "things_tiny_id.h"

void setUp() {}

void tearDown() {}

void testThingsTinyId() {
	uint8_t lanId = 24;
	uint8_t hours = 11;
	uint8_t minutes = 23;
	uint8_t seconds = 52;
	uint16_t milliseconds = 997;

	uint32_t passedTimeThisDay =
		11 * (60 * 60 * 1000) +
		23 * (60 * 1000) +
		52 * 1000 +
		997;

	TinyId requestId = {0};
	if (makeTinyId(lanId, REQUEST, passedTimeThisDay, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");

	TinyId responseId = {0};
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, makeResponseTinyId(requestId, responseId),
		"Failed to make response things tiny ID.");
	TEST_ASSERT_EQUAL_UINT(RESPONSE, getMessageTypeFromTinyId(responseId));
	TEST_ASSERT_EQUAL_UINT8(lanId, getLanIdFromTinyId(responseId));
	TEST_ASSERT_EQUAL_INT32(passedTimeThisDay, getPassedTimeThisDayFromTinyId(responseId));

	TinyId errorId = {0};
	if(makeErrorTinyId(requestId, errorId) != 0)
		TEST_FAIL_MESSAGE("Failed to make error things tiny ID.");

	TEST_ASSERT_EQUAL_UINT(ERROR, getMessageTypeFromTinyId(errorId));
	TEST_ASSERT_EQUAL_UINT8(lanId, getLanIdFromTinyId(errorId));
	TEST_ASSERT_EQUAL_INT32(passedTimeThisDay,getPassedTimeThisDayFromTinyId(errorId));
}

void testInvalidPassedTimeTinyId() {
	TinyId requestId = {0};
	if(makeTinyId(0x01, REQUEST, 215001, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testThingsTinyId);
	RUN_TEST(testInvalidPassedTimeTinyId);
	
	return UNITY_END();
}
