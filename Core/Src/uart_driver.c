#include "uart_driver.h"
#include "ring_buffer.h"
#include "lab_config.h"
#include <string.h>

_Static_assert(UART_RX_CAPACITY > 0, "UART RX queue must not be empty");
_Static_assert(UART_TX_CAPACITY >= 96, "TX must hold a complete diagnostic message");

static UART_HandleTypeDef *uart;
static IRQn_Type uart_irq;
static RingBuffer rx_queue, tx_queue;
static uint8_t rx_storage[UART_RX_CAPACITY], tx_storage[UART_TX_CAPACITY];
/* HAL keeps these pointers until the corresponding completion callback. */
static uint8_t rx_byte, tx_byte;
static volatile bool interrupts_on, switch_pending, target_interrupts, tx_active;
static UART_Diagnostics diagnostics;

/* Only USART is masked. SysTick and unrelated peripherals stay enabled.
 * This protects queue count/index updates and HAL state against the UART ISR.
 * The ring itself is deliberately independent of ARM/HAL. */
static uint32_t Lock(void)
{
    uint32_t enabled = NVIC_GetEnableIRQ(uart_irq);
    HAL_NVIC_DisableIRQ(uart_irq);
    __DSB();
    __ISB();
    return enabled;
}
static void Unlock(uint32_t enabled)
{
    __DMB();
    if (enabled) HAL_NVIC_EnableIRQ(uart_irq);
}
static void StoreReceived(uint8_t byte)
{
    if (!Ring_Write(&rx_queue, &byte, 1)) ++diagnostics.rx_overflow;
}
static void ArmReceive(void)
{
    if (uart->RxState == HAL_UART_STATE_READY)
        (void)HAL_UART_Receive_IT(uart, &rx_byte, 1);
}
static void StartInterruptTransmit(void)
{
    if (tx_active || switch_pending || !Ring_Peek(&tx_queue, &tx_byte)) return;
    tx_active = true;
    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(uart, &tx_byte, 1);
    if (status != HAL_OK) {
        tx_active = false;
        if (status != HAL_BUSY) ++diagnostics.tx_errors;
    }
}
static void PollReceive(void)
{
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_ORE) ||
        __HAL_UART_GET_FLAG(uart, UART_FLAG_FE) ||
        __HAL_UART_GET_FLAG(uart, UART_FLAG_NE) ||
        __HAL_UART_GET_FLAG(uart, UART_FLAG_PE)) {
        ++diagnostics.line_errors;
        /* F4 clears these errors by SR/DR reads; discard the damaged byte. */
        __HAL_UART_CLEAR_OREFLAG(uart);
        return;
    }
    if (__HAL_UART_GET_FLAG(uart, UART_FLAG_RXNE) &&
        HAL_UART_Receive(uart, &rx_byte, 1, 0) == HAL_OK)
        StoreReceived(rx_byte);
}
static void PollTransmit(void)
{
    if (!__HAL_UART_GET_FLAG(uart, UART_FLAG_TXE) ||
        !Ring_Peek(&tx_queue, &tx_byte)) return;
    HAL_StatusTypeDef status = HAL_UART_Transmit(uart, &tx_byte, 1, 0);
    /* F4 HAL 1.27.1 can write DR and then time out waiting for TC.
     * TxXferCount == 0 proves that byte was handed to the peripheral.
     * Retrying it would duplicate output. TC is checked later, without waiting.
     * Never use this zero-timeout path concurrently with HAL's IT operations:
     * its timeout cleanup resets both HAL states and disables UART IRQ sources. */
    if (status == HAL_OK || (status == HAL_TIMEOUT && uart->TxXferCount == 0)) {
        uint8_t consumed;
        (void)Ring_Read(&tx_queue, &consumed);
    } else if (status != HAL_BUSY && status != HAL_TIMEOUT) {
        ++diagnostics.tx_errors;
    }
}
void UART_DriverInit(UART_HandleTypeDef *handle, IRQn_Type irq)
{
    uart = handle;
    uart_irq = irq;
    HAL_NVIC_DisableIRQ(uart_irq);
    Ring_Init(&rx_queue, rx_storage, sizeof(rx_storage));
    Ring_Init(&tx_queue, tx_storage, sizeof(tx_storage));
    memset(&diagnostics, 0, sizeof(diagnostics));
    interrupts_on = true;
    target_interrupts = true;
    switch_pending = tx_active = false;
    (void)HAL_UART_AbortTransmit(uart);
    (void)HAL_UART_AbortReceive(uart);
    HAL_NVIC_ClearPendingIRQ(uart_irq);
    ArmReceive();
    HAL_NVIC_EnableIRQ(uart_irq);
}
void UART_DriverService(void)
{
    uint32_t enabled = Lock();
    if (interrupts_on) ArmReceive();
    else PollReceive();

    /* Stop launching new bytes, but allow the active byte to finish.
     * Pending RX bytes and both software queues survive the change. */
    if (switch_pending && !tx_active && __HAL_UART_GET_FLAG(uart, UART_FLAG_TC)) {
        (void)HAL_UART_AbortReceive(uart);
        (void)HAL_UART_AbortTransmit(uart);
        PollReceive();
        HAL_NVIC_ClearPendingIRQ(uart_irq);
        interrupts_on = target_interrupts;
        switch_pending = false;
        enabled = interrupts_on ? 1U : 0U;
        if (interrupts_on) ArmReceive();
    }
    if (!switch_pending) {
        if (interrupts_on) StartInterruptTransmit();
        else PollTransmit();
    }
    Unlock(enabled);
}
UART_ReadStatus UART_DriverPeek(uint8_t *byte)
{
    uint32_t enabled = Lock();
    bool found = Ring_Peek(&rx_queue, byte);
    Unlock(enabled);
    return found ? UART_DATA_READY : UART_NO_DATA;
}
UART_ReadStatus UART_DriverRead(uint8_t *byte)
{
    uint32_t enabled = Lock();
    bool found = Ring_Read(&rx_queue, byte);
    Unlock(enabled);
    return found ? UART_DATA_READY : UART_NO_DATA;
}
bool UART_DriverWrite(const uint8_t *bytes, size_t size)
{
    uint32_t enabled = Lock();
    bool accepted = Ring_Write(&tx_queue, bytes, size);
    Unlock(enabled);
    return accepted;
}
bool UART_DriverRequestMode(bool interrupts)
{
    uint32_t enabled = Lock();
    bool accepted = !switch_pending;
    if (accepted && interrupts != interrupts_on) {
        target_interrupts = interrupts;
        switch_pending = true;
    }
    Unlock(enabled);
    return accepted;
}
bool UART_DriverInterrupts(void) { return interrupts_on; }
bool UART_DriverSwitching(void) { return switch_pending; }
UART_Diagnostics UART_DriverDiagnostics(void)
{
    uint32_t enabled = Lock();
    UART_Diagnostics result = diagnostics;
    Unlock(enabled);
    return result;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
    if (handle != uart) return;
    if (handle->ErrorCode == HAL_UART_ERROR_NONE) {
        StoreReceived(rx_byte);
        if (interrupts_on) ArmReceive();
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
    if (handle != uart || !tx_active) return;
    uint8_t consumed;
    (void)Ring_Read(&tx_queue, &consumed);
    tx_active = false;
    if (interrupts_on) StartInterruptTransmit();
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
    if (handle != uart) return;
    ++diagnostics.line_errors;
    (void)HAL_UART_AbortReceive(uart);
    __HAL_UART_CLEAR_OREFLAG(uart);
    /* Service will rearm reception. No formatting or application logic in ISR. */
}
