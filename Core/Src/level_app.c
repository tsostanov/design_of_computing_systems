#include "level_app.h"
#include "level_bus.h"
#include "level_config.h"
#include "level_display.h"
#include "level_protocol.h"
#include "level_sensor.h"
#include "uart_driver.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

static LevelFrame frame;
static uint8_t pixels[LEVEL_SCREEN_BYTES];
static uint32_t sampled, received, rendered, retried, rx_lost, line_errors;
static bool sensor_ready, discard;
static char line[64], reply[160];
static size_t used;
static bool Send(const char *s)
{
    return UART_DriverWrite((const uint8_t *)s,strlen(s));
}
static void Serial(void)
{
    UART_Diagnostics d=UART_DriverDiagnostics();
    if (d.rx_overflow != rx_lost || d.line_errors != line_errors) {
        rx_lost=d.rx_overflow; line_errors=d.line_errors;
        used=0; discard=true;
        /* A missing byte must never turn a corrupted line into a valid command. */
        if (!reply[0]) (void)snprintf(reply,sizeof(reply),"ERR UART_RX %lu %lu\r\n",
            (unsigned long)rx_lost,(unsigned long)line_errors);
    }
    if (reply[0]) {
        if (Send(reply)) reply[0]=0;
        else return;
    }
    uint8_t byte;
    for (unsigned budget=0;budget<64 && UART_DriverRead(&byte)==UART_DATA_READY;++budget) {
        if (byte=='\r' || byte=='\n') {
            if (discard) { discard=false; used=0; }
            else if (used) {
                line[used]=0; used=0;
                LevelProtocol_Command(line,&frame,LevelSensor_Name(),reply,sizeof(reply));
                break;
            }
        } else if (!discard) {
            if (byte<32 || byte>126 || used>=sizeof(line)-1) {
                used=0; discard=true;
                (void)snprintf(reply,sizeof(reply),"ERR BAD_LINE\r\n");
                break;
            }
            line[used++]=(char)byte;
        }
    }
}
void LevelApp_Init(void)
{
    memset(&frame,0,sizeof(frame));
    frame.mode=LEVEL_HORIZ;
    used=0; reply[0]=0; discard=false;
    rx_lost=line_errors=0;
    UART_BoardStart();
    LevelBus_Init();
    uint32_t now=HAL_GetTick();
    LevelDisplay_Init(now);
    sampled=rendered=received=now;
    retried=now-LEVEL_RETRY_MS;
    sensor_ready=false;
}
void LevelApp_Update(void)
{
    UART_DriverService();
    uint32_t now=HAL_GetTick();
    if (!sensor_ready && (uint32_t)(now-retried)>=LEVEL_RETRY_MS) {
        retried=now;
        sensor_ready=LevelSensor_Init();
        if (sensor_ready) received=now;
    }
    if (sensor_ready && (uint32_t)(now-sampled)>=LEVEL_SAMPLE_MS) {
        sampled=now;
        int32_t mg[3];
        LevelSensorResult result=LevelSensor_Read(mg);
        if (result==LEVEL_SENSOR_DATA) {
            frame.mg[0]=mg[LEVEL_X_AXIS]*LEVEL_X_SIGN;
            frame.mg[1]=mg[LEVEL_Y_AXIS]*LEVEL_Y_SIGN;
            frame.mg[2]=mg[LEVEL_Z_AXIS]*LEVEL_Z_SIGN;
            received=now; frame.sensor_ok=true;
        } else if (result==LEVEL_SENSOR_ERROR) {
            sensor_ready=frame.sensor_ok=false; retried=now;
        }
        if ((uint32_t)(now-received)>=LEVEL_STALE_MS) {
            sensor_ready=frame.sensor_ok=false; retried=now;
        }
    }
    if (!frame.sensor_ok) memset(frame.mg,0,sizeof(frame.mg));
    frame.ms=now;
    frame.oled_ok=LevelDisplay_Ready();
    Level_ComputePointer(&frame);
    Serial();
    if ((uint32_t)(now-rendered)>=LEVEL_FRAME_MS) {
        rendered=now; ++frame.sequence;
        if (LevelDisplay_Idle()) {
            LevelGraphics_Render(pixels,&frame);
            LevelDisplay_Submit(pixels);
        }
        /* Responses take priority; telemetry is best effort under TX pressure. */
        if (!reply[0]) {
            char data[160];
            LevelProtocol_Data(&frame,data,sizeof(data));
            (void)Send(data);
        }
    }
    LevelDisplay_Update(now);
    UART_DriverService();
}
