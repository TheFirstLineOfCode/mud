#include <ArduinoUniqueID.h>

#include <debug.h>

#include "arduino_unique_id_generator.h"

char *generateThingIdUsingUniqueIdLibrary(const char* modelName) {
  int modelNameLength = strlen(modelName);
  char *thingId = malloc(sizeof(char) * (modelNameLength + 1 + 8 + 1));
  sprintf(thingId, "%s-%x%x%x%x%x%x%x%x", modelName,
      UniqueID8[0] / 16, UniqueID8[1] / 16, UniqueID8[2] / 16, UniqueID8[3] / 16,
      UniqueID8[4] / 16, UniqueID8[5] / 16, UniqueID8[6] / 16, UniqueID8[7] / 16);

#ifdef ENABLE_DEBUG
  char buffer[64];
  sprintf(buffer, "Thing ID has generated. Thing ID: %s.", thingId);
  debugOut(buffer);
#endif

  return thingId;
}
