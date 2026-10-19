/* USER CODE BEGIN Header */
/* USART6: PC6 TX / PC7 RX, 115200 8N1. HAL from STM32Cube F4 V1.27.1. */
/* USER CODE END Header */
#include "usart.h"
/* USER CODE BEGIN 0 */
#include "uart_driver.h"
/* USER CODE END 0 */
UART_HandleTypeDef huart6;

void MX_USART6_UART_Init(void)
{
  /* USER CODE BEGIN USART6_Init 0 */
  /* USER CODE END USART6_Init 0 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK) Error_Handler();
  /* USER CODE BEGIN USART6_Init 2 */
  /* USER CODE END USART6_Init 2 */
}
void HAL_UART_MspInit(UART_HandleTypeDef *handle)
{
  GPIO_InitTypeDef config = {0};
  if (handle->Instance == USART6) {
    /* USER CODE BEGIN USART6_MspInit 0 */
    /* USER CODE END USART6_MspInit 0 */
    __HAL_RCC_USART6_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    config.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    config.Mode = GPIO_MODE_AF_PP;
    config.Pull = GPIO_NOPULL;
    config.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    config.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &config);
    HAL_NVIC_SetPriority(USART6_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    /* USER CODE BEGIN USART6_MspInit 1 */
    /* USER CODE END USART6_MspInit 1 */
  }
}
void HAL_UART_MspDeInit(UART_HandleTypeDef *handle)
{
  if (handle->Instance == USART6) {
    /* USER CODE BEGIN USART6_MspDeInit 0 */
    /* USER CODE END USART6_MspDeInit 0 */
    __HAL_RCC_USART6_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6 | GPIO_PIN_7);
    HAL_NVIC_DisableIRQ(USART6_IRQn);
    /* USER CODE BEGIN USART6_MspDeInit 1 */
    /* USER CODE END USART6_MspDeInit 1 */
  }
}
/* USER CODE BEGIN 1 */
void UART_BoardStart(void)
{
    UART_DriverInit(&huart6, USART6_IRQn);
}
/* USER CODE END 1 */
