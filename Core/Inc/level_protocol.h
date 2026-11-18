#ifndef LEVEL_PROTOCOL_H
#define LEVEL_PROTOCOL_H
#include "level_model.h"
#include <stddef.h>
void LevelProtocol_Command(const char *command, LevelFrame *frame,
                           const char *sensor, char *reply, size_t capacity);
void LevelProtocol_Data(const LevelFrame *frame, char *reply, size_t capacity);
#endif
