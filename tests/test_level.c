#include "fake_hal.h"
#include "level_app.h"
#include "level_bus.h"
#include "level_display.h"
#include "level_graphics.h"
#include "level_model.h"
#include "level_protocol.h"
#include "level_sensor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned kind, sensor_writes, pages;
static uint8_t sensor_address, regs[256], latch, config, last_burst;
static uint8_t display_pixels[1024];
static bool sensor_fail, display_fail, data_ready;
static int16_t acceleration[3];
static void ResetBus(unsigned type)
{
    kind=type; sensor_address=type==2 ? 0x6b : 0x1e;
    memset(regs,0,sizeof(regs)); memset(display_pixels,0,sizeof(display_pixels));
    latch=0xaa; config=0xff; sensor_writes=pages=last_burst=0;
    sensor_fail=display_fail=false; data_ready=true;
    acceleration[0]=0; acceleration[1]=0; acceleration[2]=16393;
}
void LevelBus_Init(void) {}
bool LevelBus_Read(uint8_t a,uint8_t r,uint8_t *p,size_t n)
{
    if (a==0x70) {
        if (display_fail) return false;
        assert(n==1); *p=r==1 ? latch : config; return true;
    }
    if (a!=sensor_address || sensor_fail) return false;
    if (r==0x0f) { assert(n==1); *p=kind==1 ? 0x49 : kind==2 ? 0x68 : 0; }
    else if (r==0x27 || r==0x17) { assert(n==1); *p=data_ready ? (kind==1 ? 8 : 1) : 0; }
    else {
        assert(n==6); last_burst=r;
        assert(r==(kind==1 ? 0xa8 : 0x28));
        for (unsigned i=0;i<3;++i) {
            uint16_t value=(uint16_t)acceleration[i];
            p[2*i]=(uint8_t)value; p[2*i+1]=(uint8_t)(value>>8);
        }
    }
    return true;
}
bool LevelBus_Write(uint8_t a,uint8_t r,uint8_t value)
{
    if (a==0x70) {
        if (display_fail) return false;
        if (r==1) latch=value; else { assert(r==3); config=value; }
        return true;
    }
    if (a!=sensor_address || sensor_fail) return false;
    regs[r]=value; ++sensor_writes; return true;
}
bool LevelBus_Send(uint8_t a,const uint8_t *p,size_t n)
{
    assert(a==0x3c);
    if (display_fail) return false;
    if (p[0]==0x40) {
        assert(n==129);
        memcpy(display_pixels+(pages%8)*128,p+1,128); ++pages;
    }
    return true;
}
static void Drain(void)
{
    unsigned limit=10000;
    while (fake_uart.tx && --limit) Fake_CompleteTransmit();
    assert(limit);
    fake_output[fake_output_length]=0;
}
static void Step(uint32_t tick)
{
    fake_tick=tick; LevelApp_Update(); Drain();
}
static void Command(const char *s)
{
    for (;*s;++s) { Fake_Receive((uint8_t)*s); Step(fake_tick); }
    Step(fake_tick); Step(fake_tick);
}
static bool Output(const char *s) { return strstr((char *)fake_output,s)!=NULL; }
static void ClearOutput(void) { fake_output_length=0; fake_output[0]=0; }

static void TestSensor(void)
{
    int32_t mg[3];
    ResetBus(1); assert(LevelSensor_Init());
    assert(!strcmp(LevelSensor_Name(),"LSM9DS0"));
    assert(regs[0x20]==0x6f && regs[0x21]==0xc0);
    acceleration[0]=-16393; acceleration[1]=32767; acceleration[2]=-32768;
    assert(LevelSensor_Read(mg)==LEVEL_SENSOR_DATA);
    assert(mg[0]==-1000 && mg[1]==1999 && mg[2]==-1999 && last_burst==0xa8);
    data_ready=false; assert(LevelSensor_Read(mg)==LEVEL_SENSOR_WAIT);
    sensor_fail=true; assert(LevelSensor_Read(mg)==LEVEL_SENSOR_ERROR);
    ResetBus(2); assert(LevelSensor_Init());
    assert(!strcmp(LevelSensor_Name(),"LSM9DS1"));
    assert(regs[0x22]==0x44 && regs[0x1f]==0x38 && regs[0x20]==0x60);
    assert(LevelSensor_Read(mg)==LEVEL_SENSOR_DATA && mg[2]==1000 && last_burst==0x28);
    ResetBus(0); assert(!LevelSensor_Init() && sensor_writes==0);
    puts("PASS sensor identity, register configuration, signed conversion, data-ready/errors");
}
static void TestGeometry(void)
{
    LevelFrame f={.sensor_ok=true,.mode=LEVEL_HORIZ,.mg={0,0,1000}};
    Level_ComputePointer(&f); assert(f.dx==18 && f.dy==0);
    f.mode=LEVEL_ARROW; Level_ComputePointer(&f); assert(f.dx==0 && f.dy==0);
    f.mg[0]=1000; Level_ComputePointer(&f); assert(f.dx==-18 && f.dy==0);
    for (int x=-1000;x<=1000;x+=100)
        for (int y=-1000;y<=1000;y+=100) {
            if (!x && !y) continue;
            f.mg[0]=x; f.mg[1]=y; f.mode=LEVEL_HORIZ;
            Level_ComputePointer(&f);
            assert(abs(-x*f.dx+y*f.dy)<=abs(x)+abs(y));
            assert(f.dx*f.dx+f.dy*f.dy>=299 && f.dx*f.dx+f.dy*f.dy<=350);
        }
    f.sensor_ok=false; Level_ComputePointer(&f); assert(!f.dx && !f.dy);
    puts("PASS pointer directions, perpendicularity, flat/invalid samples");
}
static void TestDisplay(void)
{
    ResetBus(1); LevelDisplay_Init(100);
    LevelDisplay_Update(100); assert(latch==0xab && config==0xfe);
    LevelDisplay_Update(101); assert(latch==0xab);
    LevelDisplay_Update(102); assert(latch==0xaa);
    LevelDisplay_Update(104); assert(latch==0xab);
    LevelDisplay_Update(106); assert(LevelDisplay_Idle());
    uint8_t pixels[1024]; memset(pixels,0x55,sizeof(pixels));
    LevelDisplay_Submit(pixels); memset(pixels,0xaa,sizeof(pixels));
    for (unsigned i=0;i<8;++i) LevelDisplay_Update(107+i);
    assert(pages==8 && LevelDisplay_Idle());
    for (unsigned i=0;i<1024;++i) assert(display_pixels[i]==0x55);
    LevelDisplay_Submit(pixels); display_fail=true; LevelDisplay_Update(120);
    assert(!LevelDisplay_Ready());
    display_fail=false; LevelDisplay_Update(1119); assert(!LevelDisplay_Ready());
    for (unsigned i=0;i<4;++i) LevelDisplay_Update(1120+2*i);
    assert(LevelDisplay_Ready());
    puts("PASS OLED reset timing, expander preservation, page snapshot, retry");
}
static void TestProtocol(void)
{
    LevelFrame f={.sensor_ok=true,.oled_ok=true,.mg={1000,0,0}};
    char s[160];
    LevelProtocol_Command("PING",&f,"LSM9DS0",s,sizeof(s));
    assert(!strcmp(s,"PONG LEVEL3 1 LSM9DS0\r\n"));
    LevelProtocol_Command("MODE ARROW",&f,"LSM9DS0",s,sizeof(s));
    assert(f.mode==LEVEL_ARROW && f.dx==-18 && !strcmp(s,"OK MODE ARROW\r\n"));
    LevelProtocol_Command("MODE HORIZ extra",&f,"LSM9DS0",s,sizeof(s));
    assert(f.mode==LEVEL_ARROW && !strcmp(s,"ERR BAD_COMMAND\r\n"));
    LevelProtocol_Command("GET",&f,"LSM9DS0",s,sizeof(s));
    assert(!strcmp(s,"DATA 0 0 ARROW 1000 0 0 -18 0 1 1\r\n"));
    uint8_t pixels[1024];
    LevelGraphics_Render(pixels,&f);
    assert(pixels[104+4*128] & 1);
    FILE *preview=fopen("tmp/tests/level-preview.pbm","wb"); assert(preview);
    fprintf(preview,"P4\n128 64\n");
    for (unsigned y=0;y<64;++y) for (unsigned x=0;x<128;x+=8) {
        uint8_t b=0;
        for (unsigned bit=0;bit<8;++bit)
            if (pixels[x+bit+(y/8)*128] & (1U<<(y%8))) b |= 1U<<(7-bit);
        fputc(b,preview);
    }
    fclose(preview);
    puts("PASS exact protocol replies, mode validation and framebuffer rendering");
}
static void TestApplication(void)
{
    Fake_Reset(); ResetBus(1); LevelApp_Init();
    for (unsigned t=0;t<=100;t+=2) Step(t);
    assert(Output(" HORIZ 0 0 1000 18 0 1 1"));
    ClearOutput(); Command("PING\r\n"); assert(Output("PONG LEVEL3 1 LSM9DS0"));
    ClearOutput(); Command("MODE ARROW\nGET\n");
    assert(Output("OK MODE ARROW") && Output(" ARROW 0 0 1000 0 0 1 1"));
    ClearOutput(); Command("MODE HORIZ extra\nGET\n");
    assert(Output("ERR BAD_COMMAND") && Output(" ARROW "));
    ClearOutput(); Command("MODE HORIZ\x01\nGET\n");
    assert(Output("ERR BAD_LINE") && Output(" ARROW "));
    ClearOutput();
    for (unsigned i=0;i<300;++i) Fake_Receive('A');
    Step(110); Command("\nPING\n");
    assert(Output("ERR UART_RX") && Output("PONG LEVEL3"));
    ClearOutput(); data_ready=false;
    for (unsigned t=120;t<=620;t+=20) Step(t);
    Command("GET\n"); assert(Output(" ARROW 0 0 0 0 0 0 1"));
    data_ready=true;
    for (unsigned t=640;t<=1800;t+=20) Step(t);
    ClearOutput(); Command("GET\n"); assert(Output(" ARROW 0 0 1000 0 0 1 1"));
    ClearOutput(); display_fail=true;
    for (unsigned t=1820;t<=2100;t+=20) Step(t);
    Command("GET\n"); assert(Output(" ARROW 0 0 1000 0 0 1 0"));
    puts("PASS application streaming, CRLF, commands, corrupt input, RX overflow, stale sensor, OLED failure");
    Fake_Reset(); ResetBus(1); fake_tick=UINT32_MAX-50; LevelApp_Init();
    for (unsigned i=0;i<=200;i+=2) Step(UINT32_MAX-50+i);
    assert(Output(" HORIZ 0 0 1000 18 0 1 1"));
    puts("PASS 32-bit tick rollover");
}
int main(void)
{
    TestSensor(); TestGeometry(); TestDisplay(); TestProtocol(); TestApplication();
    return 0;
}
