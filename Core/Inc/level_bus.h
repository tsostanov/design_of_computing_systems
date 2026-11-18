#ifndef LEVEL_BUS_H
#define LEVEL_BUS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* All device addresses here are unshifted 7-bit addresses. */
void LevelBus_Init(void);
bool LevelBus_Read(uint8_t address, uint8_t reg, uint8_t *data, size_t size);
bool LevelBus_Write(uint8_t address, uint8_t reg, uint8_t value);
bool LevelBus_Send(uint8_t address, const uint8_t *data, size_t size);
#endif
