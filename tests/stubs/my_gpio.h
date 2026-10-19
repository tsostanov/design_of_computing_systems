#ifndef TEST_GPIO_H
#define TEST_GPIO_H
#include "stm32f4xx_hal.h"
#define GPIOC ((void *)1)
#define MY_GPIO_MODE_INPUT 0
#define MY_GPIO_OUTPUT_PUSH_PULL 0
#define MY_GPIO_SPEED_LOW 0
#define MY_GPIO_PULL_NONE 0
typedef struct { unsigned pins, mode, output_type, speed, pull, alternate, initial_value; } MyGPIO_Config;
void MyGPIO_Init(void *port, const MyGPIO_Config *config);
uint8_t MyGPIO_ReadPin(void *port, uint16_t pin);
#endif
