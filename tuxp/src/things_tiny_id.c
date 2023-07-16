#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "things_tiny_id.h"

static const uint32_t MAX_VALUE_PASSED_TIME_THIS_DAY = 86399999;

int makeTinyId2(MessageType messageType, uint8_t lanId, uint8_t hours, uint8_t minutes,
			uint8_t seconds, uint16_t milliseconds, TinyId tinyId) {
	if (lanId < 0 || lanId > 255)
		return TINY_ID_ERROR_LAN_ID_OVERFLOW;

	if(hours < 0 || hours > 23)
		return TINY_ID_ERROR_INVALID_HOURS;

	if(minutes < 0 || minutes > 59)
		return TINY_ID_ERROR_INVALID_MINUTES;

	if(seconds < 0 || seconds > 59)
		return TINY_ID_ERROR_INVALID_SECONDS;

	if(milliseconds < 0 || milliseconds > 999)
		return TINY_ID_ERROR_INVALID_MILLISECONDS;

	tinyId[0] = lanId;
	
	tinyId[1] = ((messageType << 6) | hours);

	tinyId[2] = minutes;

	uint8_t leftest6BitsOfByte3 = seconds;
	uint8_t rightest2BitsOfBytes3 = milliseconds >> 8;
	tinyId[3] = (uint8_t)((leftest6BitsOfByte3 << 2) | rightest2BitsOfBytes3);

	tinyId[4] = (uint8_t)(milliseconds & 0xff);

	return 0;
}

int makeTinyId(uint8_t lanId,MessageType messageType,
	uint32_t passedTimeThisDay,TinyId tinyId) {
	if(lanId > 255)
		return TINY_ID_ERROR_LAN_ID_OVERFLOW;

	if(passedTimeThisDay >= MAX_VALUE_PASSED_TIME_THIS_DAY)
		return TINY_ID_ERROR_INVALID_PASSED_TIME;

	uint32_t remainedTime = passedTimeThisDay;

	uint32_t aHour = 3600000;
	uint8_t hours = remainedTime / aHour;
	remainedTime = remainedTime % aHour;

	uint32_t aMinute = 60000;
	uint8_t minutes = remainedTime / aMinute;
	remainedTime = remainedTime % aMinute;

	uint32_t aSecond = 1000;
	uint8_t seconds = remainedTime / aSecond;
	remainedTime = remainedTime % aSecond;

	uint16_t milliseconds = remainedTime;

	return makeTinyId2(messageType,lanId,hours,minutes,seconds,milliseconds,tinyId);
}

bool isAnswerTinyIdOf(const TinyId answerId, const TinyId requestId) {
	if(answerId[0] != requestId[0] ||
			answerId[2] != requestId[2] ||
			answerId[3] != requestId[3] ||
			answerId[4] != requestId[4]) {
		return false;
	}

	int iType = (answerId[1] & 0xff) >> 6;
	if (iType != RESPONSE && iType != ERROR) {
		return false;
	}

	uint8_t requestIdHours = requestId[1] & 0X3f;
	uint8_t answerIdHours = requestId[1] & 0X3f;

	return requestIdHours == answerIdHours;
}

enum MessageType getMessageTypeFromTinyId(const TinyId tinyId) {
	int iType = (tinyId[1] & 0xff) >> 6;

	if (iType == REQUEST)
		return REQUEST;
	else if (iType == RESPONSE)
		return RESPONSE;
	else
		return ERROR;
}

bool isRequestTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == REQUEST;
}

bool isResponseTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == RESPONSE;
}

bool isErrorTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == ERROR;
}

uint8_t getLanIdFromTinyId(const TinyId tinyId) {
	return tinyId[0];
}

uint32_t getPassedTimeThisDayFromTinyId(const TinyId tinyId) {
	uint8_t lanId = tinyId[0];
	uint8_t hours = tinyId[1] & 0X3f;
	uint8_t minutes = tinyId[2];
	uint8_t seconds = tinyId[3] >> 2;
	int rightest2BitsOfByte3 = tinyId[3] & 0x3;
	uint16_t milliseconds = (rightest2BitsOfByte3 << 8) | tinyId[4];

	uint32_t passedTimeThisDay = 0;
	passedTimeThisDay += hours * 3600000;
	passedTimeThisDay +=  minutes * 60000;
	passedTimeThisDay +=  seconds * 1000;
	passedTimeThisDay += milliseconds;

	return passedTimeThisDay;
}

int makeAnswerTinyId(const TinyId requestId, MessageType messageType, TinyId answerId) {
	if (messageType == REQUEST)
		return TINY_ID_ERROR_NOT_ANSWER_MESSAGE_TYPE;

	answerId[0] = requestId[0];

	uint8_t hours = requestId[1] & 0x3f;
	answerId[1] = (messageType << 6) | hours;

	answerId[2] = requestId[2];
	answerId[3] = requestId[3];
	answerId[4] = requestId[4];

	return 0;
}

int makeResponseTinyId(const TinyId requestId, TinyId responseId) {
	return makeAnswerTinyId(requestId, RESPONSE, responseId);
}

int makeErrorTinyId(const TinyId requestId, TinyId errorId) {
	return makeAnswerTinyId(requestId, ERROR, errorId);
}
