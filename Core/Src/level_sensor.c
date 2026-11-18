#include "level_sensor.h"
#include "level_bus.h"
#include <stddef.h>

static uint8_t address, kind;
/* LSM9DS0 XM: WHO_AM_I=0x49; LSM9DS1 AG: WHO_AM_I=0x68.
 * Never configure a device until its identity matches. */
bool LevelSensor_Init(void)
{
    const uint8_t candidates[] = {0x1d, 0x1e, 0x6a, 0x6b};
    kind = 0;
    for (size_t i = 0; i < sizeof(candidates); ++i) {
        uint8_t id;
        address = candidates[i];
        if (!LevelBus_Read(address, 0x0f, &id, 1)) continue;
        if (i < 2 && id == 0x49) {
            /* 100 Hz, BDU, XYZ on; +/-2g, normal self-test, 50 Hz filter. */
            if (LevelBus_Write(address, 0x21, 0xc0) &&
                LevelBus_Write(address, 0x20, 0x6f)) kind = 1;
        } else if (i >= 2 && id == 0x68) {
            /* BDU + IF_ADD_INC; XYZ on; 119 Hz, +/-2g. */
            if (LevelBus_Write(address, 0x22, 0x44) &&
                LevelBus_Write(address, 0x1f, 0x38) &&
                LevelBus_Write(address, 0x20, 0x60)) kind = 2;
        }
        if (kind) return true;
    }
    return false;
}
LevelSensorResult LevelSensor_Read(int32_t mg[3])
{
    uint8_t status, raw[6];
    if (!kind || !LevelBus_Read(address, kind == 1 ? 0x27 : 0x17, &status, 1))
        return LEVEL_SENSOR_ERROR;
    if (!(status & (kind == 1 ? 0x08 : 0x01))) return LEVEL_SENSOR_WAIT;
    if (!LevelBus_Read(address, kind == 1 ? 0xa8 : 0x28, raw, sizeof(raw)))
        return LEVEL_SENSOR_ERROR;
    for (unsigned i = 0; i < 3; ++i) {
        uint16_t bits = (uint16_t)raw[2*i] | ((uint16_t)raw[2*i+1] << 8);
        int32_t value = bits < 32768U ? (int32_t)bits : (int32_t)bits - 65536;
        /* Both sensors at +/-2g: 0.061 mg/LSB, symmetric rounding. */
        mg[i] = (value * 61 + (value >= 0 ? 500 : -500)) / 1000;
    }
    return LEVEL_SENSOR_DATA;
}
const char *LevelSensor_Name(void)
{
    return kind == 1 ? "LSM9DS0" : kind == 2 ? "LSM9DS1" : "NONE";
}
