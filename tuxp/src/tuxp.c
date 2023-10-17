#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "tuxp.h"

#define MIN_SIZE_PROTOCOL_DATA 2 + 3
#define MIN_SIZE_LAN_EXECUTION_DATA MIN_SIZE_PROTOCOL_DATA + (SIZE_THINGS_TINY_ID + 2) + 5
#define MIN_SIZE_LAN_RESPONSE_DATA 2 + 5 + 1 + 1 + SIZE_THINGS_TINY_ID
#define MIN_SIZE_LAN_ERROR_DATA 2 + 5 + 1 + 1 + SIZE_THINGS_TINY_ID + 1 + 2 + 1
#define MIN_SIZE_LAN_NOTIFICATION_DATA MIN_SIZE_PROTOCOL_DATA + (SIZE_THINGS_TINY_ID + 2) + 5

static const uint8_t LAN_EXECUTION_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x04, 0x05, 0x01, 0x01
};
static const int SIZE_LAN_EXECUTION_PREFIX_BYTES = sizeof(LAN_EXECUTION_PREFIX_BYTES);

static const uint8_t LAN_ERROR_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x02, 0x07, 0x02, 0x00
};
static const uint8_t LAN_RESPONSE_PREFIX_BYTES[] = {
	0xff, 0xf8, 0x02, 0x07, 0x01, 0x00
};
static const int SIZE_LAN_ANSWER_PREFIX_BYTES = sizeof(LAN_ERROR_PREFIX_BYTES);

static const uint8_t LAN_NOTIFICATION_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x02, 0x05, 0x01, 0x01
};
static const int SIZE_LAN_NOTIFICATION_PREFIX_BYTES = sizeof(LAN_NOTIFICATION_PREFIX_BYTES);

static const uint8_t LAN_REPORT_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x0a, 0x05, 0x01, 0x01
};
static const int SIZE_LAN_REPORT_PREFIX_BYTES = sizeof(LAN_REPORT_PREFIX_BYTES);

Protocol createEmptyProtocol() {
	ProtocolName name = {{0xff, 0xff}, 0xff};
	return createProtocol(name);
}

Protocol createProtocol(ProtocolName name) {
	Protocol pEmpty = {name, NULL, NULL};
	return pEmpty;
}

void addAttributeToProtocol(Protocol *protocol, ProtocolAttribute *attribute) {
	attribute->previous = NULL;
	attribute->next = NULL;

	if (!protocol->attributes) {
		protocol->attributes = attribute;
	} else {
		ProtocolAttribute *lastAttribute = protocol->attributes;
		while (lastAttribute) {
			if (lastAttribute->next)
				lastAttribute = lastAttribute->next;
			else
				break;
		}

		lastAttribute->next = attribute;
		attribute->previous = lastAttribute;
	}
}

int addBytesAttribute(Protocol *protocol, uint8_t name, uint8_t bytes[], int size) {
	if (protocol->text)
		return debugErrorAndReturn("addBytesAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);

	if (size > MAX_SIZE_ATTRIBUTE_DATA)
		return debugErrorAndReturn("addBytesAttribute", TUXP_ERROR_ATTRIBUTE_DATA_TOO_LARGE);

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
	attribute->name = name;
	attribute->dataType = TYPE_BYTES;

	attribute->value.bsValue = malloc((size + 1) * sizeof(uint8_t));
	if (!attribute->value.bsValue)
		return debugErrorAndReturn("addBytesAttribute", TUXP_ERROR_OUT_OF_MEMEORY);
	*(attribute->value.bsValue) = (uint8_t)size;
	memcpy(attribute->value.bsValue + 1, bytes, size);

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addStringAttribute(Protocol *protocol, uint8_t name, char string[]) {
	if(protocol->text)
		return debugErrorAndReturn("addStringAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);
	
	long strLength = strlen(string);
	if (strLength > MAX_SIZE_ATTRIBUTE_DATA)
		return debugErrorAndReturn("addStringAttribute", TUXP_ERROR_ATTRIBUTE_DATA_TOO_LARGE);
	
	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
	if (!attribute)
		return debugErrorAndReturn("addStringAttribute", TUXP_ERROR_OUT_OF_MEMEORY);
	
	attribute->name = name;
	attribute->dataType = TYPE_CHARS;
	
	attribute->value.csValue = malloc((strLength + 1) * sizeof(char));
	if (!attribute->value.csValue)
		return debugErrorAndReturn("addStringAttribute", TUXP_ERROR_OUT_OF_MEMEORY);
	
	strcpy(attribute->value.csValue, string);
	
	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addByteAttribute(Protocol *protocol, uint8_t name, uint8_t bValue) {
	if(protocol->text)
		return debugErrorAndReturn("addByteAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->name = name;
	attribute->dataType = TYPE_BYTE;

	attribute->value.bValue = bValue;

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addIntAttribute(Protocol *protocol, uint8_t name, int iValue) {
	if(protocol->text)
		return debugErrorAndReturn("addIntAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);
	
	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->name = name;
	attribute->dataType = TYPE_CHARS;

	char charsData[16];
	sprintf(charsData, "%d", iValue);
	attribute->value.csValue = malloc(sizeof(char) * (strlen(charsData) + 1));
	if(!attribute->value.csValue)
		return debugErrorAndReturn("addIntAttribute", TUXP_ERROR_OUT_OF_MEMEORY);
	strcpy(attribute->value.csValue, charsData);

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addFloatAttribute(Protocol *protocol, uint8_t name, float fValue) {
	if(protocol->text)
		return debugErrorAndReturn("addFloatAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);
	
	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->name = name;
	attribute->dataType = TYPE_CHARS;

	char charsData[32];
#ifdef ARDUINO
	dtostrf(fValue, -8, 2, charsData);
	for (int i = 0; i < 32; i++) {
		if(charsData[i] == ' ') {
			charsData[i] = 0;
			break;
		} else if (charsData[i] == 0) {
			break;
		} else {
			// NOOP
		}
	}
#else
	sprintf(charsData, "%f", fValue);
#endif

	attribute->value.csValue = malloc(sizeof(char) * (strlen(charsData) + 1));
	if(!attribute->value.csValue)
		return debugErrorAndReturn("addFloatAttribute", TUXP_ERROR_OUT_OF_MEMEORY);
	strcpy(attribute->value.csValue, charsData);
	
	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addRbsAttribute(Protocol *protocol, uint8_t name, uint8_t rbsValue) {
	if(protocol->text)
		return debugErrorAndReturn("addRbsAttribute", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->name = name;
	attribute->dataType = TYPE_RBS;

	attribute->value.rbsValue = rbsValue;

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int setText(Protocol *protocol, char *text) {
	if(protocol->text)
		return debugErrorAndReturn("setText", TUXP_ERROR_PROTOCOL_CHANGE_CLOSED);

	if (strlen(text) > MAX_SIZE_TEXT_DATA)
		return debugErrorAndReturn("setText", TUXP_ERROR_TEXT_DATA_TOO_LARGE);

	protocol->text = malloc(sizeof(char) * (strlen(text) + 1));
	if (!protocol->text)
		return debugErrorAndReturn("setText", TUXP_ERROR_OUT_OF_MEMEORY);

	strcpy(protocol->text, text);
	return 0;
}

int escape(uint8_t data[], int size, ProtocolData *pData) {
	if (size > MAX_SIZE_ATTRIBUTE_DATA)
		return TUXP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	int position = 0;
	uint8_t buff[MAX_SIZE_ATTRIBUTE_DATA];
	for (int i = 0; i < size; i++) {
		if (position > MAX_SIZE_ATTRIBUTE_DATA)
			return TUXP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

		if (data[i] == FLAG_DOC_BEGINNING_END ||
				data[i] == FLAG_UNIT_SPLITTER ||
				data[i] == FLAG_ESCAPE ||
				data[i] == FLAG_BYTES_TYPE ||
				data[i] == FLAG_BYTE_TYPE) {
			buff[position] = FLAG_ESCAPE;
			position++;
		}

		buff[position] = data[i];
		position++;
	}

	if (position == 1) {
		pData->data = malloc(sizeof(uint8_t) * 2);
		if (!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		pData->data[1] = buff[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 2;
	} else {
		pData->data = malloc(sizeof(uint8_t) * position);
		if(!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff, position);
		pData->dataSize = position;
	}

	return 0;
}

bool isEscapedByte(uint8_t b) {
	return b >= 0xfa && b <= 0xff;
}

int unescape(uint8_t data[], int size, ProtocolData *pData) {
	if (size > MAX_SIZE_ATTRIBUTE_DATA)
		return TUXP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	int position = 0;
	uint8_t buff[MAX_SIZE_ATTRIBUTE_DATA];
	for (int i = 0; i < size; i++) {
		if (data[i] == FLAG_ESCAPE && i < (size - 1) && isEscapedByte(data[i + 1])) {
			continue;
		}

		buff[position] = data[i];
		position++;
	}

	if (position == 1) {
		pData->data = malloc(sizeof(uint8_t) * 1);
		if (!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = buff[0];
		pData->dataSize = 1;
	} else if (buff[0] == FLAG_BYTES_TYPE) {
		pData->data = malloc(sizeof(uint8_t) * (position - 1));
		if(!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff + 1, position - 1);
		pData->dataSize = position - 1;
	} else if ((position == 2 && buff[0] == FLAG_BYTE_TYPE) ||
				(position == 2 && buff[0] == FLAG_NOREPLACE)) {
		pData->data = malloc(sizeof(uint8_t) * 1);
		if(!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = buff[1];
		pData->dataSize = 1;
	} else {
		pData->data = malloc(sizeof(uint8_t) * position);
		if(!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff, position);
		pData->dataSize = position;
	}

	return 0;
}

bool isValidProtocolData(ProtocolData *pData) {
	if (pData->dataSize < MIN_SIZE_PROTOCOL_DATA)
		return false;

	if (pData->data[0] != 0xff || pData->data[pData->dataSize - 1] != 0xff)
		return false;

	return true;
}

bool isProtocol(ProtocolData *pData, ProtocolName name) {
	if (!isValidProtocolData(pData))
		return false;

	return pData->data[1] == name.ns[0] &&
		pData->data[2] == name.ns[1] &&
		pData->data[3] == name.localName;
}

bool isBareProtocol(ProtocolData *pData, ProtocolName name) {
	if(pData->dataSize != 5)
		return false;

	return pData->data[1] == name.ns[0] &&
		pData->data[2] == name.ns[1] &&
		pData->data[3] == name.localName;
}

int findAttributeValueEnd(ProtocolData *pData, int position, int *escapeNumber) {
	while (position <= (pData->dataSize - 1)) {
		uint8_t current = pData->data[position];
		if (current == FLAG_ESCAPE) {
			if ((position + 1) >= (pData->dataSize - 1))
				return -1;

			if (pData->data[position + 1] < 0xfa)
				return -1;

			(*escapeNumber)++;
			position += 2;
			continue;
		}

		if (pData->data[position] == FLAG_UNIT_SPLITTER ||
				pData->data[position] == FLAG_DOC_BEGINNING_END)
			return position;

		position++;
	}

	return -1;
}

int assembleProtocolAttributeValue(ProtocolData *pData, ProtocolAttribute *attribute,
			int attributeValueStartPosition, int attributeValueEndPosition, int escapeNumber) {
	int attributeValueSize = (attributeValueEndPosition - attributeValueStartPosition - escapeNumber);
	uint8_t firstByteOfAttribute = pData->data[attributeValueStartPosition];
	if (firstByteOfAttribute == FLAG_BYTE_TYPE) {
		if (escapeNumber > 1)
			return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;

		attribute->dataType = TYPE_BYTE;
		if (escapeNumber == 0) {
			attribute->value.bValue = pData->data[attributeValueStartPosition + 1];
		} else {
			attribute->value.bValue = pData->data[attributeValueStartPosition + 2];
		}
	} else {
		ProtocolData unescapedValue;
		int attributeDataSize = attributeValueEndPosition - attributeValueStartPosition;
		int unescapeResult = unescape(pData->data + attributeValueStartPosition,
			attributeDataSize, &unescapedValue);
		if (unescapeResult != 0) {
			releaseProtocolData(&unescapedValue);

			return debugErrorDetailAndReturn("assembleProtocolAttributeValue",
				TUXP_ERROR_FAILED_TO_UNESCAPE, unescapeResult);
		}

		if(unescapedValue.dataSize == 1 &&
				firstByteOfAttribute != FLAG_NOREPLACE) {
			attribute->dataType = TYPE_RBS;
			attribute->value.rbsValue = unescapedValue.data[0];
		} else if (firstByteOfAttribute == FLAG_BYTES_TYPE) {
			attribute->value.bsValue = malloc(sizeof(uint8_t) * (unescapedValue.dataSize + 1));
			if(!attribute->value.bsValue) {
				releaseProtocolData(&unescapedValue);
				return TUXP_ERROR_OUT_OF_MEMEORY;
			}

			attribute->dataType = TYPE_BYTES;
			memcpy(attribute->value.bsValue + 1, unescapedValue.data, unescapedValue.dataSize);
			attribute->value.bsValue[0] = unescapedValue.dataSize;
		} else {
			attribute->value.csValue = malloc(sizeof(char) * (unescapedValue.dataSize + 1));
			if(!attribute->value.csValue) {
				releaseProtocolData(&unescapedValue);
				return TUXP_ERROR_OUT_OF_MEMEORY;
			}

			attribute->dataType = TYPE_CHARS;
			memcpy(attribute->value.csValue, unescapedValue.data, unescapedValue.dataSize);
			attribute->value.csValue[unescapedValue.dataSize] = '\0';
		}
		releaseProtocolData(&unescapedValue);
	}

	return 0;
}

int getAttributesSize(Protocol *protocol) {
	if (!protocol->attributes)
		return 0;

	int size = 0;
	ProtocolAttribute *current = protocol->attributes;
	while (current) {
		size++;
		current = current->next;
	}

	return size;
}

int doParseProtocol(ProtocolData *pData, Protocol *protocol) {
	protocol->attributes = NULL;
	protocol->text = NULL;

	if (!isValidProtocolData(pData))
		return TUXP_ERROR_NOT_VALID_PROTOCOL;

	// Is a bare protocol data.
	if (pData->dataSize == 5) {
		protocol->name.ns[0] = pData->data[1];
		protocol->name.ns[1] = pData->data[2];
		protocol->name.localName = pData->data[3];

		return 0;
	}

	if (pData->dataSize < (MIN_SIZE_PROTOCOL_DATA + 2))
		return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;

	uint8_t childrenSize = pData->data[5] & 0x7f;
	if (childrenSize > 0)
		return TUXP_ERROR_FEATURE_CHILD_ELEMENT_NOT_IMPLEMENTED;

	ProtocolName name = {
		{pData->data[1], pData->data[2]},
		pData->data[3]
	};
	protocol->name = name;
	protocol->attributes = NULL;
	protocol->text = NULL;

	uint8_t attributeSize = pData->data[4];
	bool hasText = ((pData->data[5] & 0x80) == 0x80);

	if (attributeSize == 0 && !hasText) {
		if (pData->dataSize != MIN_SIZE_PROTOCOL_DATA + 2)
			return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;

		return 0;
	}

	int position = 5;
	for (int i = 0; i < attributeSize; i++) {
		if (position++ >= (pData->dataSize - 1))
			return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;

		ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
		if (!attribute)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		attribute->name = pData->data[position];

		position++;
		int escapeNumber = 0;
		int attributeValueEndPosition = findAttributeValueEnd(pData, position, &escapeNumber);
		if (attributeValueEndPosition <= 0) {
			free(attribute);
			return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;
		}

		if (assembleProtocolAttributeValue(pData, attribute, position,
				attributeValueEndPosition, escapeNumber) != 0) {
			free(attribute);
			return TUXP_ERROR_FAILED_TO_ASSEMBLE_PROTOCOL_ATTRIBUTE;
		}

		position = attributeValueEndPosition;
		addAttributeToProtocol(protocol, attribute);
	}

	if (!hasText && (position != (pData->dataSize - 1))) {
		return TUXP_ERROR_MALFORMED_PROTOCOL_DATA;
	}

	if (!hasText)
		return 0;

	position++;
	int textDataSize = pData->dataSize - position - 1;
	ProtocolData textData;
	int unescapeResult = unescape(pData->data + position, textDataSize, &textData);
	if (unescapeResult != 0) {
		releaseProtocolData(&textData);
		return debugErrorDetailAndReturn("doParseProtocol", TUXP_ERROR_FAILED_TO_UNESCAPE, unescapeResult);
	}

	char *text = malloc(sizeof(char) * (textDataSize + 1));
	if(!text)
		return TUXP_ERROR_OUT_OF_MEMEORY;

	memcpy(text, textData.data, textDataSize);
	*(text + textDataSize) = 0;
	releaseProtocolData(&textData);

	protocol->text = text;

	return 0;
}

int parseProtocol(ProtocolData *pData, Protocol *protocol) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter parseProtocol."));
#else
	DEBUG_OUT("enter parseProtocol.");
#endif

	int result = doParseProtocol(pData, protocol);
	if (result != 0) {
		releaseProtocol(protocol);
	}

	return result;
}

void releaseProtocol(Protocol *protocol) {
	if (protocol->text) {
		free(protocol->text);
		protocol->text = NULL;
	}

	ProtocolAttribute *last = protocol->attributes;
	while (last) {
		if (last->next) {
			last = last->next;
		} else {
			break;
		}
	}

	if (!last)
		return;

	ProtocolAttribute *previous = NULL;
	while (last) {
		previous = last->previous;

		if (last->dataType == TYPE_BYTES && last->value.bsValue != NULL) {
			free(last->value.bsValue);
		} else if (last->dataType == TYPE_CHARS && last->value.csValue != NULL) {
			free(last->value.csValue);
		} else {
			// NOOP
		}
		free(last);

		if (previous)
			previous->next = NULL;

		last = previous;
	}
}

void releaseProtocolData(ProtocolData *pData) {
	if (pData->dataSize != 0 && pData->data != NULL) {
		free(pData->data);
		pData->data = NULL;
		pData->dataSize = 0;
	}
}

int translateProtocol(Protocol *protocol, ProtocolData *pData) {
	if (getAttributesSize(protocol) > MAX_SIZE_ATTRIBUTES)
		return debugErrorAndReturn("translateProtocol", TUXP_ERROR_TOO_MANY_ATTRIBUTES);

	// It's a bare protocol.
	if (getAttributesSize(protocol) == 0 && !protocol->text) {
		pData->dataSize = MIN_SIZE_PROTOCOL_DATA;
		pData->data = malloc(sizeof(uint8_t) * MIN_SIZE_PROTOCOL_DATA);
		if (!pData->data)
			return TUXP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = 0xff;
		pData->data[1] = protocol->name.ns[0];
		pData->data[2] = protocol->name.ns[1];
		pData->data[3] = protocol->name.localName;
		pData->data[4] = 0xff;

		return 0;
	}

	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];
	buff[0] = 0xff;
	buff[1] = protocol->name.ns[0];
	buff[2] = protocol->name.ns[1];
	buff[3] = protocol->name.localName;

	int attributesSize = getAttributesSize(protocol);
	buff[4] = attributesSize;
	buff[5] = protocol->text ? 0x80 : 0x00;

	int position = 6;

	ProtocolAttribute *attribute = protocol->attributes;
	while (attribute) {
		if (position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

		buff[position] = attribute->name;
		position++;

		if(position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

		ProtocolData attributeData;
		int escapeResult = 0;
		if (attribute->dataType == TYPE_BYTE) {
			if (isEscapedByte(attribute->value.bValue)) {
				attributeData.data = malloc(sizeof(uint8_t) * 2);
				attributeData.data[0] = FLAG_ESCAPE;
				attributeData.data[1] = attribute->value.bValue;
				attributeData.dataSize = 2;
			} else {
				attributeData.data = malloc(sizeof(uint8_t) * 1);
				attributeData.data[0] = attribute->value.bValue;
				attributeData.dataSize = 1;
			}
		} else if (attribute->dataType == TYPE_BYTES) {
			int bsSize = attribute->value.bsValue[0];
			escapeResult = escape(attribute->value.bsValue + 1, bsSize, &attributeData);
		} else if (attribute->dataType == TYPE_RBS) {
			if(isEscapedByte(attribute->value.rbsValue)) {
				attributeData.data = malloc(sizeof(uint8_t) * 2);
				attributeData.data[0] = FLAG_ESCAPE;
				attributeData.data[1] = attribute->value.rbsValue;
				attributeData.dataSize = 2;
			} else {
				attributeData.data = malloc(sizeof(uint8_t) * 1);
				attributeData.data[0] = attribute->value.rbsValue;
				attributeData.dataSize = 1;
			}
		} else { // attributeDescription->dataType == TYPE_CHARS
			escapeResult = escape(attribute->value.csValue, strlen(attribute->value.csValue), &attributeData);
		}

		if (escapeResult != 0) {
			releaseProtocolData(&attributeData);
			return debugErrorDetailAndReturn("translateProtocol", TUXP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL, escapeResult);
		} else if (attribute->dataType == TYPE_BYTE) {
			buff[position] = FLAG_BYTE_TYPE;
			position++;
		} else if (attribute->dataType == TYPE_BYTES) {
			buff[position] = FLAG_BYTES_TYPE;
			position++;
		} else {
			// NOOP
		}

		if (position >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);
		}

		if (position + attributeData.dataSize >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);
		}
		
		memcpy(buff + position, attributeData.data, attributeData.dataSize);
		position += attributeData.dataSize;

		buff[position] = FLAG_UNIT_SPLITTER;
		position++;
		
		if (position >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);
		}

		releaseProtocolData(&attributeData);

		attribute = attribute->next;
	}

	if (protocol->text) {
		int textSize = strlen(protocol->text);
		if (position + textSize >= MAX_SIZE_PROTOCOL_DATA - 1)
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

		memcpy(buff + position, protocol->text, textSize);
		position += textSize;
	}

	int dataSize;
	if (buff[position - 1] == FLAG_UNIT_SPLITTER) {
		buff[position - 1] = FLAG_DOC_BEGINNING_END;
		dataSize = position;
	} else {
		if (position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return debugErrorAndReturn("translateProtocol", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

		buff[position] = FLAG_DOC_BEGINNING_END;
		dataSize = position + 1;
	}

	pData->data = NULL;
	pData->dataSize = 0;

	pData->data = malloc(dataSize * sizeof(uint8_t));
	if (!pData->data)
		return debugErrorAndReturn("translateProtocol", TUXP_ERROR_OUT_OF_MEMEORY);

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;

	return 0;
}

int translateAndRelease(Protocol *protocol, ProtocolData *pData) {
	int result = translateProtocol(protocol, pData);
	releaseProtocol(protocol);

	return result;
}

int translateLanExecution(TinyId requestId, Protocol *action, ProtocolData *pData) {
	ProtocolData pDataAction;
	if (translateProtocol(action, &pDataAction) != 0)
		return debugErrorAndReturn("translateLanExecution", TUXP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL);

	ProtocolData pDataEscapedTinyId;
	int escapeResult = escape(requestId, SIZE_THINGS_TINY_ID, &pDataEscapedTinyId);
	if (escapeResult != 0) {
		return debugErrorDetailAndReturn("translateLanExecution", TUXP_ERROR_FAILED_TO_ESCAPE, escapeResult);
	}

	int pDataActionSize = pDataAction.dataSize - 2;
	int lanExecutionSize = MIN_SIZE_PROTOCOL_DATA + 3 + pDataEscapedTinyId.dataSize + pDataActionSize;
	if (lanExecutionSize > MAX_SIZE_PROTOCOL_DATA)
		return debugErrorAndReturn("translateLanExecution", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

	uint8_t leBuff[MAX_SIZE_PROTOCOL_DATA];
	int position = 0;
	memcpy(leBuff, LAN_EXECUTION_PREFIX_BYTES, SIZE_LAN_EXECUTION_PREFIX_BYTES);
	position += SIZE_LAN_EXECUTION_PREFIX_BYTES;

	leBuff[position] = 0x06;
	position++;
	leBuff[position] = FLAG_BYTES_TYPE;
	position++;

	memcpy(leBuff + position, pDataEscapedTinyId.data, pDataEscapedTinyId.dataSize);
	position += pDataEscapedTinyId.dataSize;

	leBuff[position] = FLAG_UNIT_SPLITTER;
	position++;

	memcpy(leBuff + position, pDataAction.data + 1, pDataAction.dataSize - 2);
	position += pDataAction.dataSize - 2;

	leBuff[position] = FLAG_DOC_BEGINNING_END;
	
	pData->data = malloc(sizeof(uint8_t) * (position + 1));
	if (!pData->data)
		return TUXP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, leBuff, (position + 1));
	pData->dataSize = position + 1;

	return 0;
}

ProtocolAttribute *getAttributeByName(Protocol *protocol, uint8_t name) {
	ProtocolAttribute *attribute = protocol->attributes;
	while(attribute) {
		if (attribute->name == name)
			return attribute;

		attribute = attribute->next;
	}

	return NULL;
}

bool getIntAttributeValue(Protocol *protocol, uint8_t name, int *value) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if (!attribute)
		return false;

	if(attribute->dataType != TYPE_CHARS)
		return false;

	*value = atoi(attribute->value.csValue);
	return true;
}

bool getByteAttributeValue(Protocol *protocol, uint8_t name, uint8_t *value) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if(!attribute)
		return false;

	if (attribute->dataType != TYPE_BYTE)
		return false;

	*value = attribute->value.bValue;
	return true;
}

char *getStringAttributeValue(Protocol *protocol, uint8_t name) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if(!attribute)
		return NULL;

	if(attribute->dataType != TYPE_CHARS)
		return NULL;

	return attribute->value.csValue;
}

uint8_t *getBytesAttributeValue(Protocol *protocol, uint8_t name) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if(!attribute)
		return NULL;

	if(attribute->dataType != TYPE_BYTES)
		return NULL;

	return attribute->value.bsValue;
}

bool getFloatAttributeValue(Protocol *protocol, uint8_t name, float *value) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if(!attribute)
		return false;

	if(attribute->dataType != TYPE_CHARS)
		return false;

	*value = atof(attribute->value.csValue);
	return true;
}

bool getRbsAttributeValue(Protocol *protocol, uint8_t name, uint8_t *value) {
	ProtocolAttribute *attribute = getAttributeByName(protocol, name);
	if(!attribute)
		return false;

	if(attribute->dataType != TYPE_RBS)
		return false;

	*value = attribute->value.rbsValue;
	return true;
}

char *getText(Protocol *protocol) {
	return protocol->text;
}

bool isLanAnswer(ProtocolData *pData) {
	if(!isValidProtocolData(pData))
		return false;

	if (pData->dataSize < MIN_SIZE_LAN_RESPONSE_DATA)
		return false;

	return pData->data[1] == 0xf8 &&
		pData->data[2] == 0x02 &&
		pData->data[3] == 0x07;
}

LanAnswer createLanResonse(TinyId requestId) {
	LanAnswer answer;
	makeResponseTinyId(requestId, answer.traceId);
	answer.errorNumber = 0;

	return answer;
}

LanAnswer createLanError(TinyId requestId, int8_t errorNumber) {
	LanAnswer answer;
	makeErrorTinyId(requestId, answer.traceId);
	answer.errorNumber = errorNumber;

	return answer;
}

bool isLanExecution(ProtocolData *pData) {
	if(!isValidProtocolData(pData))
		return false;

	return pData->data[1] == 0xf8 &&
		pData->data[2] == 0x04 &&
		pData->data[3] == 0x05;
}

int parseLanAnswer(ProtocolData *pData, LanAnswer *answer) {
	if(!isLanAnswer(pData))
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	int position = SIZE_LAN_ANSWER_PREFIX_BYTES;
	if (pData->data[position] != 0x06)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	position++;
	if(pData->data[position] != FLAG_BYTES_TYPE)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	position++;
	int traceIdEndPosition = -1;
	for (int i = position; i < pData->dataSize; i++) {
		if (pData->data[i] == FLAG_UNIT_SPLITTER || pData->data[i] == FLAG_DOC_BEGINNING_END) {
			traceIdEndPosition = i;
			break;
		}
	}

	if (traceIdEndPosition == -1)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	ProtocolData traceId;
	int unescapeResult = unescape(pData->data + position, traceIdEndPosition - position, &traceId);
	if (unescapeResult != 0)
		return debugErrorDetailAndReturn("parseLanAnswer", TUXP_ERROR_FAILED_TO_UNESCAPE, unescapeResult);

	if (traceId.dataSize != SIZE_THINGS_TINY_ID)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_FAILED_TO_UNESCAPE);

	memcpy(answer->traceId, traceId.data, SIZE_THINGS_TINY_ID);

	if (isResponseTinyId(answer->traceId)) {
		if (pData->data[traceIdEndPosition] != FLAG_DOC_BEGINNING_END)
			return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

		answer->errorNumber = 0;

		return 0;
	}

	if(!isErrorTinyId(answer->traceId))
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE);

	position = traceIdEndPosition;
	if(pData->data[position] != FLAG_UNIT_SPLITTER)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	if (position + 1 + 1 + 1 > (pData->dataSize - 1))
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	position++;
	if (pData->data[position] != 0x08)
		return debugErrorAndReturn("parseLanAnswer", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	position++;
	int LengthOfErrorNumber = pData->dataSize - position - 1;
	char csErrorNumber[8] = {0};
	memcpy(csErrorNumber, pData->data + position, LengthOfErrorNumber);
	answer->errorNumber = atoi(csErrorNumber);

	position += LengthOfErrorNumber;

	if (pData->data[position] != FLAG_DOC_BEGINNING_END)
		return debugErrorAndReturn("parseLanAnswer",TUXP_ERROR_MALFORMED_PROTOCOL_DATA);
	
	return 0;
}

int translateLanResonse(LanAnswer *answer, ProtocolData *pData, ProtocolData *escapedTraceId) {
	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];

	int position = 0;
	memcpy(buff, LAN_RESPONSE_PREFIX_BYTES, SIZE_LAN_ANSWER_PREFIX_BYTES);
	position += SIZE_LAN_ANSWER_PREFIX_BYTES;

	buff[position] = 0x06;
	position++;

	buff[position] = FLAG_BYTES_TYPE;
	position++;
	memcpy(buff + position, escapedTraceId->data, escapedTraceId->dataSize);
	position += escapedTraceId->dataSize;

	buff[position] = 0xff;

	int dataSize = position + 1;
	pData->data = malloc(sizeof(uint8_t) * dataSize);
	if(!pData->data)
		return debugErrorAndReturn("translateLanResonse", TUXP_ERROR_OUT_OF_MEMEORY);

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;
	
	return 0;
}

int translateLanError(LanAnswer *answer, ProtocolData *pData, ProtocolData *escapedTraceId) {
	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];

	int position = 0;
	memcpy(buff, LAN_ERROR_PREFIX_BYTES, SIZE_LAN_ANSWER_PREFIX_BYTES);
	position += SIZE_LAN_ANSWER_PREFIX_BYTES;

	buff[position] = 0x06;
	position++;

	buff[position] = FLAG_BYTES_TYPE;
	position++;
	memcpy(buff + position, escapedTraceId->data, escapedTraceId->dataSize);
	position += escapedTraceId->dataSize;

	buff[position] = FLAG_UNIT_SPLITTER;
	position++;

	buff[position] = 0x08;
	position++;

	char csErrorNumber[8] = {0};
	itoa(answer->errorNumber, csErrorNumber, 10);

	int lengthOfErrorNumber = strlen(csErrorNumber);
	memcpy(buff + position, csErrorNumber, lengthOfErrorNumber);
	position += lengthOfErrorNumber;

	buff[position] = 0xff;

	int dataSize = position + 1;
	pData->data = malloc(sizeof(uint8_t) * dataSize);
	if(!pData->data)
		return debugErrorAndReturn("translateLanError", TUXP_ERROR_OUT_OF_MEMEORY);

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;

	return 0;
}

int translateLanAnswer(LanAnswer *answer, ProtocolData *pData) {
	ProtocolData escapedTraceId = {NULL, 0};
	int escapeResult = escape(answer->traceId, SIZE_THINGS_TINY_ID, &escapedTraceId);
	if (escapeResult != 0) {
		releaseProtocolData(&escapedTraceId);
		return debugErrorDetailAndReturn("translateLanAnswer", TUXP_ERROR_FAILED_TO_ESCAPE, escapeResult);
	}

	int result;
	if (isResponseTinyId(answer->traceId)) {
		result = translateLanResonse(answer, pData, &escapedTraceId);
	} else if (isErrorTinyId(answer->traceId)) {
		result = translateLanError(answer, pData, &escapedTraceId);
	} else {
		releaseProtocolData(&escapedTraceId);
		return debugErrorAndReturn("translateLanAnswer", TUXP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE);
	}
	releaseProtocolData(&escapedTraceId);

	if (result != 0) {
		releaseProtocolData(pData);
		return debugErrorAndReturn("translateLanAnswer", TUXP_ERROR_FAILED_TO_TRANSLATE_ANSWER);
	}

	return 0;
}

int parseLanExecution(ProtocolData *pData, TinyId requestId, Protocol *action) {
#if defined(ARDUINO) && defined(ENABLE_DEBUG)
	Serial.println(F("enter parseLanExecution."));
#else
	DEBUG_OUT("enter parseLanExecution.");
#endif

	if(!isLanExecution(pData) || pData->dataSize < MIN_SIZE_LAN_EXECUTION_DATA)
		return debugErrorAndReturn("parseLanExecution", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	uint8_t attributeSize = pData->data[4];
	uint8_t childrenSize = pData->data[5] & 0x7f;
	bool hasText = (pData->data[5] & 0x80) == 0x80;

	if (attributeSize != 1 || childrenSize != 1 || hasText)
		return debugErrorAndReturn("parseLanExecution", TUXP_ERROR_MALFORMED_PROTOCOL_DATA);

	int requestIdStartPosition = 1 + 5 + 1 + 1;
	memcpy(requestId, pData->data + requestIdStartPosition, SIZE_THINGS_TINY_ID);

	int actionStartPosition = requestIdStartPosition + SIZE_THINGS_TINY_ID + 1;
	uint8_t actionBuff[MAX_SIZE_PROTOCOL_DATA];
	int actionDataSize = pData->dataSize - actionStartPosition - 1;
	memcpy(actionBuff + 1, pData->data + actionStartPosition, actionDataSize);
	actionBuff[0] = 0xff;
	actionBuff[actionDataSize + 1] = 0xff;

	ProtocolData pDataAction = {actionBuff, actionDataSize + 2};
	int parseActionResult = parseProtocol(&pDataAction, action);
	if(parseActionResult != 0) {
		releaseProtocol(action);
		return parseActionResult;
	}

	return 0;
}

int translateLanNotification(TinyId requestId, Protocol *event, bool ackRequired, ProtocolData *pData) {
	ProtocolData pDataEvent;
	int result = translateProtocol(event, &pDataEvent);
	if(result != 0) {
		releaseProtocolData(&pDataEvent);
		return debugErrorDetailAndReturn("translateLanNotification",
			TUXP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL, result);
	}

	ProtocolData pDataEscapedTinyId;
	int escapeResult = escape(requestId, SIZE_THINGS_TINY_ID, &pDataEscapedTinyId);
	if(escapeResult != 0) {
		releaseProtocolData(&pDataEscapedTinyId);
		return debugErrorDetailAndReturn("translateLanNotification", TUXP_ERROR_FAILED_TO_ESCAPE, escapeResult);
	}

	int pDataEventSize = pDataEvent.dataSize - 2;
	int lanNotificationSize = MIN_SIZE_PROTOCOL_DATA + 3 + pDataEscapedTinyId.dataSize + pDataEventSize;
	if(lanNotificationSize > MAX_SIZE_PROTOCOL_DATA)
		return debugErrorAndReturn("translateLanNotification", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

	uint8_t lnBuff[MAX_SIZE_PROTOCOL_DATA];
	int position = 0;
	memcpy(lnBuff, LAN_NOTIFICATION_PREFIX_BYTES, SIZE_LAN_NOTIFICATION_PREFIX_BYTES);
	position += SIZE_LAN_NOTIFICATION_PREFIX_BYTES;

	if (ackRequired)
		lnBuff[4] = 0x02;

	lnBuff[position] = 0x06;
	position++;
	lnBuff[position] = FLAG_BYTES_TYPE;
	position++;

	memcpy(lnBuff + position, pDataEscapedTinyId.data, pDataEscapedTinyId.dataSize);
	position += pDataEscapedTinyId.dataSize;
	releaseProtocolData(&pDataEscapedTinyId);

	lnBuff[position] = FLAG_UNIT_SPLITTER;
	position++;

	if (ackRequired) {
		lnBuff[position] = 0x01;
		lnBuff[position] = 0x02;
		lnBuff[position] = FLAG_UNIT_SPLITTER;

		position += 3;
	}

	memcpy(lnBuff + position, pDataEvent.data + 1, pDataEvent.dataSize - 2);
	position += pDataEvent.dataSize - 2;
	releaseProtocolData(&pDataEvent);

	lnBuff[position] = FLAG_DOC_BEGINNING_END;

	pData->data = malloc(sizeof(uint8_t) * (position + 1));
	if(!pData->data)
		return TUXP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, lnBuff, (position + 1));
	pData->dataSize = position + 1;

	return 0;
}

int translateLanReport(TinyId requestId, Protocol *data, bool ackRequired, ProtocolData *pData) {
	ProtocolData pDataData;
	int result = translateProtocol(data, &pDataData);
	if(result != 0) {
		releaseProtocolData(&pDataData);
		return debugErrorDetailAndReturn("translateLanReport",
			TUXP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL, result);
	}

	ProtocolData pDataEscapedTinyId;
	int escapeResult = escape(requestId, SIZE_THINGS_TINY_ID, &pDataEscapedTinyId);
	if(escapeResult != 0) {
		releaseProtocolData(&pDataEscapedTinyId);
		return debugErrorDetailAndReturn("translateLanReport", TUXP_ERROR_FAILED_TO_ESCAPE, escapeResult);
	}

	int pDataDataSize = pDataData.dataSize - 2;
	int lanReportSize = MIN_SIZE_PROTOCOL_DATA + 3 + pDataEscapedTinyId.dataSize + pDataDataSize;
	if(lanReportSize > MAX_SIZE_PROTOCOL_DATA)
		return debugErrorAndReturn("translateLanReport", TUXP_ERROR_PROTOCOL_DATA_TOO_LARGE);

	uint8_t lrBuff[MAX_SIZE_PROTOCOL_DATA];
	int position = 0;
	memcpy(lrBuff, LAN_REPORT_PREFIX_BYTES, SIZE_LAN_REPORT_PREFIX_BYTES);
	position += SIZE_LAN_REPORT_PREFIX_BYTES;

	if(ackRequired)
		lrBuff[4] = 0x02;

	lrBuff[position] = 0x06;
	position++;
	lrBuff[position] = FLAG_BYTES_TYPE;
	position++;

	memcpy(lrBuff + position, pDataEscapedTinyId.data, pDataEscapedTinyId.dataSize);
	position += pDataEscapedTinyId.dataSize;
	releaseProtocolData(&pDataEscapedTinyId);

	lrBuff[position] = FLAG_UNIT_SPLITTER;
	position++;

	if(ackRequired) {
		lrBuff[position] = 0x01;
		lrBuff[position] = 0x02;
		lrBuff[position] = FLAG_UNIT_SPLITTER;

		position += 3;
	}

	memcpy(lrBuff + position, pDataData.data + 1, pDataData.dataSize - 2);
	position += pDataData.dataSize - 2;
	releaseProtocolData(&pDataData);

	lrBuff[position] = FLAG_DOC_BEGINNING_END;

	pData->data = malloc(sizeof(uint8_t) * (position + 1));
	if(!pData->data)
		return TUXP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, lrBuff, (position + 1));
	pData->dataSize = position + 1;

	return 0;
}
