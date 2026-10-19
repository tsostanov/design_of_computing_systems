#include "fake_hal.h"
#include "uart_driver.h"
#include "my_gpio.h"
#include "led.h"
#include <assert.h>
#include <string.h>
static USART_TypeDef peripheral;
UART_HandleTypeDef fake_uart;
uint32_t fake_tick, fake_irq_enabled;
uint8_t fake_button_level;
bool fake_green;
char fake_feedback;
uint8_t fake_output[32768];
size_t fake_output_length;
HAL_StatusTypeDef fake_tx_status;

void Fake_Reset(void)
{
    memset(&peripheral, 0, sizeof(peripheral));
    memset(&fake_uart, 0, sizeof(fake_uart));
    peripheral.SR = UART_FLAG_TXE | UART_FLAG_TC;
    fake_uart.Instance = &peripheral;
    fake_tick = fake_irq_enabled = 0;
    fake_button_level = 1;
    fake_output_length = 0;
    fake_green = false;
    fake_feedback = 0;
    fake_tx_status = HAL_TIMEOUT;
}
uint32_t HAL_GetTick(void) { return fake_tick; }
uint32_t NVIC_GetEnableIRQ(IRQn_Type irq) { (void)irq; return fake_irq_enabled; }
void HAL_NVIC_EnableIRQ(IRQn_Type irq) { (void)irq; fake_irq_enabled = 1; }
void HAL_NVIC_DisableIRQ(IRQn_Type irq) { (void)irq; fake_irq_enabled = 0; }
void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq) { (void)irq; }
void MyGPIO_Init(void *p, const MyGPIO_Config *c) { (void)p; (void)c; }
uint8_t MyGPIO_ReadPin(void *p, uint16_t pin) { (void)p; (void)pin; return fake_button_level; }
void LED_Init(void) { fake_green = false; fake_feedback = 0; }
void LED_GreenOn(void) { fake_green = true; }
void LED_GreenOff(void) { fake_green = false; }
void LED_YellowOn(void) { fake_feedback = '.'; }
void LED_RedOn(void) { fake_feedback = '-'; }
void LED_BicolorOff(void) { fake_feedback = 0; }
void LED_AllOff(void) { LED_Init(); }
void UART_BoardStart(void) { UART_DriverInit(&fake_uart, 71); }
void Fake_ClearErrors(UART_HandleTypeDef *h)
{
    h->Instance->SR &= ~(UART_FLAG_ORE | UART_FLAG_FE | UART_FLAG_NE | UART_FLAG_PE | UART_FLAG_RXNE);
    h->ErrorCode = 0;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *h)
{
    h->Instance->CR1 &= ~FAKE_RX_IRQ;
    h->Instance->CR3 = 0;
    h->RxState = HAL_UART_STATE_READY;
    h->rx = NULL;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *h)
{
    h->Instance->CR1 &= ~FAKE_TX_IRQ;
    h->gState = HAL_UART_STATE_READY;
    h->tx = NULL;
    h->TxXferCount = 0;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *h, uint8_t *p, uint16_t n)
{
    assert(n == 1);
    if (h->RxState != HAL_UART_STATE_READY) return HAL_BUSY;
    h->rx = p;
    h->RxState = HAL_UART_STATE_BUSY_RX;
    h->Instance->CR1 |= FAKE_RX_IRQ;
    h->Instance->CR3 = 1;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *h, const uint8_t *p, uint16_t n)
{
    assert(n == 1);
    if (h->gState != HAL_UART_STATE_READY) return HAL_BUSY;
    h->tx = p;
    h->gState = HAL_UART_STATE_BUSY_TX;
    h->TxXferCount = 1;
    h->Instance->CR1 |= FAKE_TX_IRQ;
    h->Instance->SR &= ~UART_FLAG_TC;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *h, uint8_t *p, uint16_t n, uint32_t timeout)
{
    assert(n == 1 && timeout == 0);
    if (!(h->Instance->SR & UART_FLAG_RXNE)) return HAL_TIMEOUT;
    *p = h->Instance->DR;
    h->Instance->SR &= ~UART_FLAG_RXNE;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, const uint8_t *p, uint16_t n, uint32_t timeout)
{
    assert(n == 1 && timeout == 0);
    assert(!fake_irq_enabled && !(h->Instance->CR1 & (FAKE_RX_IRQ | FAKE_TX_IRQ)));
    h->TxXferCount = 1;
    if (fake_tx_status == HAL_ERROR || fake_tx_status == HAL_BUSY) return fake_tx_status;
    assert(fake_output_length < sizeof(fake_output));
    fake_output[fake_output_length++] = *p;
    h->TxXferCount = 0; /* accepted byte, but TC is not set yet */
    h->Instance->SR &= ~(UART_FLAG_TC | UART_FLAG_TXE);
    return fake_tx_status;
}
void Fake_Receive(uint8_t byte)
{
    fake_uart.Instance->DR = byte;
    fake_uart.Instance->SR |= UART_FLAG_RXNE;
    if (fake_irq_enabled && (fake_uart.Instance->CR1 & FAKE_RX_IRQ)) {
        assert(fake_uart.rx);
        *fake_uart.rx = byte;
        fake_uart.Instance->SR &= ~UART_FLAG_RXNE;
        fake_uart.RxState = HAL_UART_STATE_READY;
        fake_uart.Instance->CR1 &= ~FAKE_RX_IRQ;
        HAL_UART_RxCpltCallback(&fake_uart);
    }
}
void Fake_CompleteTransmit(void)
{
    const uint8_t *p = fake_uart.tx;
    fake_uart.tx = NULL;
    fake_uart.Instance->SR |= UART_FLAG_TC | UART_FLAG_TXE;
    if (p) {
        assert(fake_irq_enabled);
        assert(fake_output_length < sizeof(fake_output));
        fake_output[fake_output_length++] = *p;
        fake_uart.gState = HAL_UART_STATE_READY;
        fake_uart.Instance->CR1 &= ~FAKE_TX_IRQ;
        fake_uart.TxXferCount = 0;
        HAL_UART_TxCpltCallback(&fake_uart);
    }
}
