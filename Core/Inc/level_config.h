#ifndef LEVEL_CONFIG_H
#define LEVEL_CONFIG_H
#define LEVEL_SAMPLE_MS 20U
#define LEVEL_FRAME_MS 100U
#define LEVEL_STALE_MS 500U
#define LEVEL_RETRY_MS 1000U
#define LEVEL_I2C_TIMEOUT_MS 10U
/* Map sensor axes to the display: X points right, Y points up.
 * Confirm these signs on the actual board; change only this mapping if needed. */
#define LEVEL_X_AXIS 0
#define LEVEL_Y_AXIS 1
#define LEVEL_Z_AXIS 2
#define LEVEL_X_SIGN 1
#define LEVEL_Y_SIGN 1
#define LEVEL_Z_SIGN 1
#endif
