#include "level_model.h"
#include <math.h>
const char *Level_ModeName(LevelMode mode)
{
    return mode == LEVEL_ARROW ? "ARROW" : "HORIZ";
}
void Level_ComputePointer(LevelFrame *f)
{
    f->dx = f->dy = 0;
    if (!f->sensor_ok) return;
    float x = (float)f->mg[0], y = (float)f->mg[1];
    float norm = sqrtf(x*x + y*y);
    if (norm < 50.0f) {
        if (f->mode == LEVEL_HORIZ) f->dx = 18;
        return; /* Board flat: in-plane gravity has no defined direction. */
    }
    /* A stationary accelerometer measures support force, opposite gravity.
     * Screen Y points down; display axes X right, Y up. */
    if (f->mode == LEVEL_ARROW) {
        f->dx = (int)lroundf(-18.0f * x / norm);
        f->dy = (int)lroundf(18.0f * y / norm);
    } else {
        f->dx = (int)lroundf(18.0f * y / norm);
        f->dy = (int)lroundf(18.0f * x / norm);
    }
}
