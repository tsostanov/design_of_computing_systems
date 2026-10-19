#ifndef TEST_HAL_H
#define TEST_HAL_H
#include <stdint.h>
#include <stddef.h>
typedef int IRQn_Type;
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct { uint32_t SR, CR1, CR3; uint8_t DR; } USART_TypeDef;
typedef struct {
    USART_TypeDef *Instance;
    uint32_t gState, RxState, ErrorCode;
    uint16_t TxXferCount;
    uint8_t *rx;
    const uint8_t *tx;
} UART_HandleTypeDef;
#define HAL_UART_STATE_READY 0U
#define HAL_UART_STATE_BUSY_RX 1U
#define HAL_UART_STATE_BUSY_TX 2U
#define HAL_UART_ERROR_NONE 0U
#define UART_FLAG_TXE 1U
#define UART_FLAG_TC 2U
#define UART_FLAG_RXNE 4U
#define UART_FLAG_ORE 8U
#define UART_FLAG_FE 16U
#define UART_FLAG_NE 32U
#define UART_FLAG_PE 64U
#define FAKE_RX_IRQ 1U
#define FAKE_TX_IRQ 2U
#define __HAL_UART_GET_FLAG(h,f) (((h)->Instance->SR & (f)) != 0)
#define __HAL_UART_CLEAR_OREFLAG(h) Fake_ClearErrors(h)
#define __DSB() ((void)0)
#define __ISB() ((void)0)
#define __DMB() ((void)0)
uint32_t HAL_GetTick(void);
uint32_t NVIC_GetEnableIRQ(IRQn_Type irq);
void HAL_NVIC_EnableIRQ(IRQn_Type irq);
void HAL_NVIC_DisableIRQ(IRQn_Type irq);
void HAL_NVIC_ClearPendingIRQ(IRQn_Type irq);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef*, uint8_t*, uint16_t);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef*, const uint8_t*, uint16_t);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef*, uint8_t*, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, const uint8_t*, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef*);
HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef*);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef*);
void HAL_UART_ErrorCallback(UART_HandleTypeDef*);
void Fake_ClearErrors(UART_HandleTypeDef*);
#endif
