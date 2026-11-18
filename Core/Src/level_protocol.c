#include "level_protocol.h"
#include <stdio.h>
#include <string.h>
void LevelProtocol_Data(const LevelFrame *f, char *reply, size_t n)
{
    (void)snprintf(reply,n,"DATA %lu %lu %s %ld %ld %ld %d %d %u %u\r\n",
        (unsigned long)f->sequence,(unsigned long)f->ms,Level_ModeName(f->mode),
        (long)f->mg[0],(long)f->mg[1],(long)f->mg[2],f->dx,f->dy,
        (unsigned)f->sensor_ok,(unsigned)f->oled_ok);
}
void LevelProtocol_Command(const char *command, LevelFrame *f,
                           const char *sensor, char *reply, size_t n)
{
    if (!strcmp(command,"PING"))
        (void)snprintf(reply,n,"PONG LEVEL3 1 %s\r\n",sensor);
    else if (!strcmp(command,"GET")) LevelProtocol_Data(f,reply,n);
    else if (!strcmp(command,"MODE HORIZ") || !strcmp(command,"MODE ARROW")) {
        f->mode = !strcmp(command,"MODE ARROW") ? LEVEL_ARROW : LEVEL_HORIZ;
        Level_ComputePointer(f);
        (void)snprintf(reply,n,"OK MODE %s\r\n",Level_ModeName(f->mode));
    } else (void)snprintf(reply,n,"ERR BAD_COMMAND\r\n");
}
