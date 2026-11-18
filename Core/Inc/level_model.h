#ifndef LEVEL_MODEL_H
#define LEVEL_MODEL_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { LEVEL_HORIZ, LEVEL_ARROW } LevelMode;
typedef struct {
    uint32_t sequence, ms;
    int32_t mg[3];
    int dx, dy;
    LevelMode mode;
    bool sensor_ok, oled_ok;
} LevelFrame;
void Level_ComputePointer(LevelFrame *frame);
const char *Level_ModeName(LevelMode mode);
#endif
