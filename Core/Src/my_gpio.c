#include "my_gpio.h"


static uint8_t MyGPIO_IsValidPinNumber(uint8_t pin_number)
{
    return pin_number < 16;
}


MyGPIO_Status MyGPIO_EnableClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    }
    else if (port == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    }
    else if (port == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    }
    else if (port == GPIOD)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    }
    else if (port == GPIOE)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    }
    else if (port == GPIOF)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    }
    else if (port == GPIOG)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    }
    else if (port == GPIOH)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    }

#ifdef GPIOI
    else if (port == GPIOI)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOIEN;
    }
#endif

#ifdef GPIOJ
    else if (port == GPIOJ)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOJEN;
    }
#endif

#ifdef GPIOK
    else if (port == GPIOK)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOKEN;
    }
#endif

    else
    {
        return MY_GPIO_ERROR;
    }

    /*
     * Читаем регистр обратно, чтобы операция включения
     * тактирования точно завершилась до обращения к GPIO.
     */
    volatile uint32_t dummy = RCC->AHB1ENR;
    (void)dummy;

    return MY_GPIO_OK;
}


MyGPIO_Status MyGPIO_Init(
    GPIO_TypeDef *port,
    const MyGPIO_Config *config)
{
    if (port == NULL || config == NULL)
    {
        return MY_GPIO_ERROR;
    }

    if (config->pins == 0)
    {
        return MY_GPIO_ERROR;
    }

    if (config->alternate > 15)
    {
        return MY_GPIO_ERROR;
    }

    if (MyGPIO_EnableClock(port) != MY_GPIO_OK)
    {
        return MY_GPIO_ERROR;
    }

    /*
     * Если вывод становится выходом, сначала задаём
     * нужное значение в выходном регистре.
     *
     * И только после этого переключаем MODER в OUTPUT.
     *
     * Это нужно, чтобы при переинициализации не возникал
     * короткий нежелательный импульс на ножке.
     */
    if (config->mode == MY_GPIO_MODE_OUTPUT)
    {
        uint32_t set_mask =
            ((uint32_t)config->initial_value) &
            ((uint32_t)config->pins);

        uint32_t reset_mask =
            ((uint32_t)config->pins) &
            ~((uint32_t)config->initial_value);

        port->BSRR =
            set_mask |
            (reset_mask << 16);
    }

    for (uint8_t pin = 0; pin < 16; pin++)
    {
        uint16_t pin_mask = (uint16_t)(1U << pin);

        if ((config->pins & pin_mask) == 0)
        {
            continue;
        }

        /*
         * OTYPER:
         * 0 - push-pull
         * 1 - open-drain
         */
        if (config->output_type == MY_GPIO_OUTPUT_OPEN_DRAIN)
        {
            port->OTYPER |= (1U << pin);
        }
        else
        {
            port->OTYPER &= ~(1U << pin);
        }

        /*
         * OSPEEDR:
         * по два бита на каждый вывод.
         */
        port->OSPEEDR =
            (port->OSPEEDR & ~(3U << (pin * 2))) |
            ((uint32_t)config->speed << (pin * 2));

        /*
         * PUPDR:
         * 00 - без подтяжки
         * 01 - pull-up
         * 10 - pull-down
         */
        port->PUPDR =
            (port->PUPDR & ~(3U << (pin * 2))) |
            ((uint32_t)config->pull << (pin * 2));

        /*
         * AFR:
         * задаётся только для Alternate Function.
         *
         * AFR[0] - пины 0..7
         * AFR[1] - пины 8..15
         */
        if (config->mode == MY_GPIO_MODE_ALTERNATE)
        {
            uint8_t afr_index = pin / 8;
            uint8_t afr_shift = (pin % 8) * 4;

            port->AFR[afr_index] =
                (port->AFR[afr_index] & ~(0xFU << afr_shift)) |
                ((uint32_t)config->alternate << afr_shift);
        }

        /*
         * MODER меняем ПОСЛЕДНИМ, одной записью.
         * Не сбрасываем поле отдельной операцией, чтобы
         * выход при повторной настройке не становился входом.
         *
         * 00 - Input
         * 01 - Output
         * 10 - Alternate Function
         * 11 - Analog
         */
        port->MODER =
            (port->MODER & ~(3U << (pin * 2))) |
            ((uint32_t)config->mode << (pin * 2));
    }

    return MY_GPIO_OK;
}


MyGPIO_Status MyGPIO_GetConfig(
    GPIO_TypeDef *port,
    uint8_t pin_number,
    MyGPIO_PinConfig *config)
{
    if (port == NULL || config == NULL)
    {
        return MY_GPIO_ERROR;
    }

    if (!MyGPIO_IsValidPinNumber(pin_number))
    {
        return MY_GPIO_ERROR;
    }

    uint8_t shift = pin_number * 2;

    /*
     * MODER
     */
    config->mode =
        (MyGPIO_Mode)
        ((port->MODER >> shift) & 0x3U);

    /*
     * OTYPER
     */
    config->output_type =
        (MyGPIO_OutputType)
        ((port->OTYPER >> pin_number) & 0x1U);

    /*
     * OSPEEDR
     */
    config->speed =
        (MyGPIO_Speed)
        ((port->OSPEEDR >> shift) & 0x3U);

    /*
     * PUPDR
     */
    config->pull =
        (MyGPIO_Pull)
        ((port->PUPDR >> shift) & 0x3U);

    /*
     * AFR
     */
    uint8_t afr_index = pin_number / 8;
    uint8_t afr_shift = (pin_number % 8) * 4;

    config->alternate =
        (uint8_t)
        ((port->AFR[afr_index] >> afr_shift) & 0xFU);

    /*
     * Текущее состояние выходного регистра.
     */
    config->output_state =
        (port->ODR & (1U << pin_number))
        ? 1U
        : 0U;

    return MY_GPIO_OK;
}


uint8_t MyGPIO_ReadPin(
    GPIO_TypeDef *port,
    uint16_t pin)
{
    if (port == NULL)
    {
        return 0;
    }

    return (port->IDR & pin)
        ? 1U
        : 0U;
}


uint16_t MyGPIO_ReadPins(
    GPIO_TypeDef *port,
    uint16_t pins)
{
    if (port == NULL)
    {
        return 0;
    }

    return (uint16_t)
        (port->IDR & pins);
}


void MyGPIO_WritePin(
    GPIO_TypeDef *port,
    uint16_t pin,
    uint8_t state)
{
    if (port == NULL)
    {
        return;
    }

    if (state)
    {
        /*
         * Нижние 16 бит BSRR устанавливают выводы.
         */
        port->BSRR = pin;
    }
    else
    {
        /*
         * Верхние 16 бит BSRR сбрасывают выводы.
         */
        port->BSRR =
            ((uint32_t)pin << 16);
    }
}


void MyGPIO_WritePins(
    GPIO_TypeDef *port,
    uint16_t pins,
    uint16_t values)
{
    if (port == NULL)
    {
        return;
    }

    /*
     * Какие выбранные выводы установить в 1.
     */
    uint32_t set_mask =
        ((uint32_t)values) &
        ((uint32_t)pins);

    /*
     * Какие выбранные выводы установить в 0.
     */
    uint32_t reset_mask =
        ((uint32_t)pins) &
        ~((uint32_t)values);

    /*
     * Одной записью:
     *
     * BSRR[0..15]  -> SET
     * BSRR[16..31] -> RESET
     *
     * Поэтому сразу несколько ножек меняются
     * одной операцией.
     */
    port->BSRR =
        set_mask |
        (reset_mask << 16);
}
