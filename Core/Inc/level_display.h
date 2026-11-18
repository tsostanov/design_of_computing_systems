#ifndef LEVEL_DISPLAY_H
#define LEVEL_DISPLAY_H
#include "level_graphics.h"
void LevelDisplay_Init(uint32_t now);
void LevelDisplay_Update(uint32_t now);
bool LevelDisplay_Ready(void);
bool LevelDisplay_Idle(void);
void LevelDisplay_Submit(const uint8_t pixels[LEVEL_SCREEN_BYTES]);
#endif
