#ifndef MUD_PROTOCOLS_H
#define MUD_PROTOCOLS_H

#include <stdint.h>
#include <stdbool.h>

#define DATA_SIZE(data) sizeof(data) / sizeof(uint8_t)
#define CREATE_PROTOCOL_DATA(data) {data, DATA_SIZE(data)}

typedef enum DataType {
	TYPE_BYTE,
	TYPE_BYTES,
	TYPE_CHARS,
	TYPE_RBS
} DataType;

typedef struct ProtocolName {
	uint8_t ns[2];
	uint8_t localName;
} ProtocolName;

typedef union ProtocolAttributeValue {
	uint8_t bValue;
	uint8_t *bsValue;
	char *csValue;
	uint8_t rbsValue;
} ProtocolAttributeValue;

typedef struct ProtocolAttribute {
	struct ProtocolAttribute *previous;
	uint8_t name;
	DataType dataType;
	ProtocolAttributeValue value;
	struct ProtocolAttribute *next;
} ProtocolAttribute;

typedef struct Protocol {
	ProtocolName name;
	ProtocolAttribute *attributes;
	char *text;
} Protocol;

typedef struct ProtocolData {
	uint8_t *data;
	int dataSize;
} ProtocolData;

#endif
