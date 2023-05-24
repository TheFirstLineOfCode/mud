#ifndef MUD_TACP_H
#define MUD_TACP_H

#include "things_tiny_id.h"
#include "protocols.h"

#define TACP_ERROR_NOT_VALID_PROTOCOL -1
#define TACP_ERROR_UNKNOWN_PROTOCOL_NAME -2
#define TACP_ERROR_FEATURE_EMBEDDED_PROTOCOL_NOT_IMPLEMENTED -3
#define TACP_ERROR_MALFORMED_PROTOCOL_DATA -4
#define TACP_ERROR_OUT_OF_MEMEORY -5
#define TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE -6
#define TACP_ERROR_TEXT_DATA_TOO_LARGE -7
#define TACP_ERROR_TOO_MANY_ATTRIBUTES -8
#define TACP_ERROR_PROTOCOL_DATA_TOO_LARGE -9
#define TACP_ERROR_MESSAGES_BUFF_OVERFLOW -10
#define TACP_ERROR_PROTOCOL_CHANGE_CLOSED -11
#define TACP_ERROR_NOT_SUITABLE_DAC_PROTOCOL -12
#define TACP_ERROR_INVALID_DAC_STATE -13
#define TACP_ERROR_LACK_OF_ALLOCATION_PARAMETERS -14
#define TACP_ERROR_ILLEGAL_ALLOCATED_ADDRESS -15
#define TACP_ERROR_NO_REGISTRATED_PROCESSOR -16
#define TACP_ERROR_FAILED_TO_ESCAPE -17
#define TACP_ERROR_FAILED_TO_UNESCAPE -18
#define TACP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL -19
#define TACP_ERROR_FAILED_TO_PARSE_PROTOCOL -20
#define TACP_ERROR_FAILED_TO_ASSEMBLE_PROTOCOL_ATTRIBUTE -21
#define TACP_ERROR_ABANDON_MALFORMED_DATA -22
#define TACP_ERROR_WAITING_DATA -23
#define TACP_ERROR_FAILED_TO_TRANSLATE_ANSWER -24
#define TACP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE -25

#define FLAG_DOC_BEGINNING_END 0xff
#define FLAG_UNIT_SPLITTER 0xfe
#define FLAG_ESCAPE 0xfd
#define FLAG_NOREPLACE 0xfc
#define FLAG_BYTES_TYPE 0xfb
#define FLAG_BYTE_TYPE 0xfa

#define MAX_SIZE_PROTOCOL_DATA 64
#define MAX_SIZE_ATTRIBUTE_DATA 16
#define MAX_SIZE_TEXT_DATA 32
#define MAX_SIZE_ATTRIBUTES 8

typedef struct LanAnwser {
	TinyId traceId;
	int8_t errorNumber;
} LanAnswer;

static const ProtocolName NAME_TACP_PROTOCOL_INTRODUCTION = {{0xf8, 0x03}, 0x00};
#define NAME_ATTRIBUTE_ADDRESS_TACP_PROTOCOL_INTRODUCTION 0x01

static const ProtocolName NAME_TACP_PROTOCOL_ALLOCATION = {{0xf8, 0x03}, 0x02};
#define NAME_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS_TACP_PROTOCOL_ALLOCATION 0x03
#define NAME_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS_TACP_PROTOCOL_ALLOCATION 0x04
#define NAME_ATTRIBUTE_ALLOCATED_ADDRESS_TACP_PROTOCOL_ALLOCATION 0x05

static const ProtocolName NAME_TACP_PROTOCOL_ALLOCATED = {{0xf8, 0x03}, 0x06};
static const ProtocolName NAME_TACP_PROTOCOL_CONFIGURED = {{0xf8, 0x03}, 0x07};

static const ProtocolName NAME_TACP_PROTOCOL_IS_CONFIGURED = {{0xf8, 0x03}, 0x08};
#define NAME_ATTRIBUTE_ADDRESS_TACP_PROTOCOL_IS_CONFIGURED 0x01

static const ProtocolName NAME_TACP_PROTOCOL_NOT_CONFIGURED = {{0xf8, 0x03}, 0x09};

Protocol createEmptyProtocol();
Protocol createProtocol(ProtocolName name);
int addIntAttribute(Protocol *protocol, uint8_t name, int iValue);
int addFloatAttribute(Protocol *protocol, uint8_t name, float fValue);
int addByteAttribute(Protocol *protocol, uint8_t name, uint8_t bValue);
int addBytesAttribute(Protocol *protocol, uint8_t name, uint8_t bytes[], int size);
int addStringAttribute(Protocol *protocol, uint8_t name, char string[]);
int addRbsAttribute(Protocol *protocol, uint8_t name, uint8_t rbsValue);
int setText(Protocol *protocol, char *text);

bool isProtocol(ProtocolData *pData, ProtocolName name);
bool isBareProtocol(ProtocolData *pData, ProtocolName name);
int parseProtocol(ProtocolData *pData, Protocol *protocol);
void releaseProtocol(Protocol *protocol);
void releaseProtocolData(ProtocolData *pData);
int translateProtocol(Protocol *protocol, ProtocolData *pData);
int translateAndRelease(Protocol *protocol, ProtocolData *pData);
int translateLanExecution(TinyId requestId, Protocol *action, ProtocolData *pData);

int getAttributesSize(Protocol *protocol);
bool getByteAttributeValue(Protocol *protocol, uint8_t name, uint8_t *value);
uint8_t *getBytesAttributeValue(Protocol *protocol, uint8_t name);
bool getIntAttributeValue(Protocol *protocol, uint8_t name, int *value);
char *getStringAttributeValue(Protocol *protocol, uint8_t name);
bool getFloatAttributeValue(Protocol *protocol, uint8_t name, float *value);
bool getRbsAttributeValue(Protocol *protocol, uint8_t name, uint8_t *value);
char *getText(Protocol *protocol);

bool isLanAnswer(ProtocolData *pData);
LanAnswer createLanResonse(TinyId requestId);
LanAnswer createLanError(TinyId requestId, int8_t errorNumber);
int parseLanAnswer(ProtocolData *pData, LanAnswer *lanAnswer);
int translateLanAnswer(LanAnswer *answer, ProtocolData *pData);

bool isLanExecution(ProtocolData *pData);
int parseLanExecution(ProtocolData *pData, TinyId requestId, Protocol *action);
int parseProtocol(ProtocolData *pData, Protocol *protocol);
int translateLanNotification(TinyId requestId, Protocol *event, bool ackRequired, ProtocolData *pData);
int translateLanReport(TinyId requestId, Protocol *data, bool ackRequired, ProtocolData *pData);

#endif
