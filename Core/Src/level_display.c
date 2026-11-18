#include "level_display.h"
#include "level_bus.h"
#include "level_config.h"
#include <string.h>

static uint8_t pixels[LEVEL_SCREEN_BYTES], latch, page, stage;
static bool ready, flushing;
static uint32_t since;
void LevelDisplay_Init(uint32_t now)
{
    ready=flushing=false; stage=0; since=now-LEVEL_RETRY_MS;
}
bool LevelDisplay_Ready(void) { return ready; }
bool LevelDisplay_Idle(void) { return ready && !flushing; }
void LevelDisplay_Submit(const uint8_t p[LEVEL_SCREEN_BYTES])
{
    if (!LevelDisplay_Idle()) return;
    memcpy(pixels,p,sizeof(pixels)); page=0; flushing=true;
}
void LevelDisplay_Update(uint32_t now)
{
    bool ok=true;
    if (!ready) {
        if ((uint32_t)(now-since) < (stage == 0 ? LEVEL_RETRY_MS : 2U)) return;
        since=now;
        switch (stage) {
        case 0: {
            uint8_t config;
            /* Preserve all other expander pins, including interrupt inputs. */
            ok=LevelBus_Read(0x70,1,&latch,1) && LevelBus_Read(0x70,3,&config,1);
            if (ok) {
                latch |= 1;
                ok=LevelBus_Write(0x70,1,latch) &&
                   LevelBus_Write(0x70,3,(uint8_t)(config & ~1U));
            }
            break;
        }
        case 1: ok=LevelBus_Write(0x70,1,(uint8_t)(latch & ~1U)); break;
        case 2: ok=LevelBus_Write(0x70,1,latch); break;
        default: {
            static const uint8_t commands[]={0x00,0xae,0x20,0x00,0xc8,0x40,
                0x81,0xff,0xa1,0xa6,0xa8,0x3f,0xa4,0xd3,0x00,
                0xd5,0xf0,0xd9,0x22,0xda,0x12,0xdb,0x20,0x8d,0x14,0xaf};
            ok=LevelBus_Send(0x3c,commands,sizeof(commands));
            ready=ok;
            break;
        }
        }
        if (ok) ++stage; else stage=0;
        return;
    }
    if (!flushing) return;
    /* One page per main-loop pass: UART is serviced between I2C transfers.
     * A private snapshot prevents a new render from tearing this transfer. */
    uint8_t window[]={0x00,0x21,0,127,0x22,page,page};
    uint8_t packet[129]; packet[0]=0x40;
    memcpy(packet+1,pixels+128U*page,128);
    ok=LevelBus_Send(0x3c,window,sizeof(window)) &&
       LevelBus_Send(0x3c,packet,sizeof(packet));
    if (!ok) { ready=flushing=false; stage=0; since=now; }
    else if (++page == 8) flushing=false;
}
