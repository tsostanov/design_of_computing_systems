#ifndef FAKE_HAL_H
#define FAKE_HAL_H
#include "stm32f4xx_hal.h"
#include <stdbool.h>
extern UART_HandleTypeDef fake_uart;
extern uint32_t fake_tick, fake_irq_enabled;
extern uint8_t fake_button_level;
extern bool fake_green;
extern char fake_feedback;
extern uint8_t fake_output[32768];
extern size_t fake_output_length;
extern HAL_StatusTypeDef fake_tx_status;
void Fake_Reset(void);
void Fake_Receive(uint8_t byte);
void Fake_CompleteTransmit(void);
#endif
