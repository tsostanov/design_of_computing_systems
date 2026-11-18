#ifndef LEVEL_SENSOR_H
#define LEVEL_SENSOR_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { LEVEL_SENSOR_ERROR = -1, LEVEL_SENSOR_WAIT, LEVEL_SENSOR_DATA } LevelSensorResult;
bool LevelSensor_Init(void);
LevelSensorResult LevelSensor_Read(int32_t mg[3]);
const char *LevelSensor_Name(void);
#endif
