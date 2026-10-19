#ifndef UART_DRIVER_H
#define UART_DRIVER_H
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
typedef enum { UART_NO_DATA, UART_DATA_READY } UART_ReadStatus;
typedef struct { uint32_t rx_overflow, line_errors, tx_errors; } UART_Diagnostics;
void UART_DriverInit(UART_HandleTypeDef *handle, IRQn_Type irq);
void UART_DriverService(void);
UART_ReadStatus UART_DriverPeek(uint8_t *byte);
UART_ReadStatus UART_DriverRead(uint8_t *byte);
bool UART_DriverWrite(const uint8_t *bytes, size_t size);
bool UART_DriverRequestMode(bool interrupts);
bool UART_DriverInterrupts(void);
bool UART_DriverSwitching(void);
UART_Diagnostics UART_DriverDiagnostics(void);
#endif
