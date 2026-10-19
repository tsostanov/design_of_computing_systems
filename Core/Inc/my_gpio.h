#ifndef INC_MY_GPIO_H_
#define INC_MY_GPIO_H_

#include "main.h"

typedef enum
{
    MY_GPIO_OK = 0,
    MY_GPIO_ERROR
} MyGPIO_Status;

typedef enum
{
    MY_GPIO_MODE_INPUT = 0,
    MY_GPIO_MODE_OUTPUT = 1,
    MY_GPIO_MODE_ALTERNATE = 2,
    MY_GPIO_MODE_ANALOG = 3
} MyGPIO_Mode;

typedef enum
{
    MY_GPIO_OUTPUT_PUSH_PULL = 0,
    MY_GPIO_OUTPUT_OPEN_DRAIN = 1
} MyGPIO_OutputType;

typedef enum
{
    MY_GPIO_SPEED_LOW = 0,
    MY_GPIO_SPEED_MEDIUM = 1,
    MY_GPIO_SPEED_HIGH = 2,
    MY_GPIO_SPEED_VERY_HIGH = 3
} MyGPIO_Speed;

typedef enum
{
    MY_GPIO_PULL_NONE = 0,
    MY_GPIO_PULL_UP = 1,
    MY_GPIO_PULL_DOWN = 2
} MyGPIO_Pull;

typedef struct
{
    uint16_t pins;

    MyGPIO_Mode mode;
    MyGPIO_OutputType output_type;
    MyGPIO_Speed speed;
    MyGPIO_Pull pull;

    uint8_t alternate;

    /*
     * Начальное состояние выходов.
     *
     * Для каждого бита из pins:
     * 1 -> высокий уровень
     * 0 -> низкий уровень
     */
    uint16_t initial_value;

} MyGPIO_Config;

typedef struct
{
    MyGPIO_Mode mode;
    MyGPIO_OutputType output_type;
    MyGPIO_Speed speed;
    MyGPIO_Pull pull;

    uint8_t alternate;

    uint8_t output_state;

} MyGPIO_PinConfig;

MyGPIO_Status MyGPIO_EnableClock(GPIO_TypeDef *port);

MyGPIO_Status MyGPIO_Init(
    GPIO_TypeDef *port,
    const MyGPIO_Config *config
);

MyGPIO_Status MyGPIO_GetConfig(
    GPIO_TypeDef *port,
    uint8_t pin_number,
    MyGPIO_PinConfig *config
);

uint8_t MyGPIO_ReadPin(
    GPIO_TypeDef *port,
    uint16_t pin
);

uint16_t MyGPIO_ReadPins(
    GPIO_TypeDef *port,
    uint16_t pins
);

void MyGPIO_WritePin(
    GPIO_TypeDef *port,
    uint16_t pin,
    uint8_t state
);

void MyGPIO_WritePins(
    GPIO_TypeDef *port,
    uint16_t pins,
    uint16_t values
);

#endif
