#ifndef LEVEL_GRAPHICS_H
#define LEVEL_GRAPHICS_H
#include "level_model.h"
#define LEVEL_SCREEN_BYTES 1024U
void LevelGraphics_Render(uint8_t pixels[LEVEL_SCREEN_BYTES], const LevelFrame *frame);
#endif
