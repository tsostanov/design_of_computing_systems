#include "led.h"
#include "my_gpio.h"

#define LED_PORT        GPIOD

#define LED_GREEN_PIN   (1U << 13)
#define LED_YELLOW_PIN  (1U << 14)
#define LED_RED_PIN     (1U << 15)

#define LED_ALL_PINS \
    (LED_GREEN_PIN | LED_YELLOW_PIN | LED_RED_PIN)

void LED_Init(void)
{
    MyGPIO_Config config;

    config.pins = LED_ALL_PINS;
    config.mode = MY_GPIO_MODE_OUTPUT;
    config.output_type = MY_GPIO_OUTPUT_PUSH_PULL;
    config.speed = MY_GPIO_SPEED_LOW;
    config.pull = MY_GPIO_PULL_NONE;
    config.alternate = 0;
    config.initial_value = 0;

    MyGPIO_Init(LED_PORT, &config);

    LED_AllOff();
}

void LED_AllOff(void)
{
    MyGPIO_WritePins(
        LED_PORT,
        LED_ALL_PINS,
        0
    );
}

void LED_GreenOn(void)
{
    MyGPIO_WritePin(
        LED_PORT,
        LED_GREEN_PIN,
        1
    );
}

void LED_GreenOff(void)
{
    MyGPIO_WritePin(
        LED_PORT,
        LED_GREEN_PIN,
        0
    );
}

void LED_RedOn(void)
{
    MyGPIO_WritePins(
        LED_PORT,
        LED_RED_PIN | LED_YELLOW_PIN,
        LED_RED_PIN
    );
}

void LED_YellowOn(void)
{
    MyGPIO_WritePins(
        LED_PORT,
        LED_RED_PIN | LED_YELLOW_PIN,
        LED_YELLOW_PIN
    );
}

void LED_BicolorOff(void)
{
    MyGPIO_WritePins(
        LED_PORT,
        LED_RED_PIN | LED_YELLOW_PIN,
        0
    );
}
