#include "button.h"
#include "my_gpio.h"

#define BUTTON_PORT            GPIOC
#define BUTTON_PIN             (1U << 15)

#define BUTTON_DEBOUNCE_MS     30
#define BUTTON_LONG_PRESS_MS   600

static uint8_t raw_state;
static uint8_t stable_state;

static uint32_t raw_change_time;
static uint32_t press_start_time;

static uint8_t Button_Read(void)
{
    /*
     * На стенде нажатая кнопка даёт логический 0.
     */
    return MyGPIO_ReadPin(
        BUTTON_PORT,
        BUTTON_PIN
    ) == 0;
}

void Button_Init(void)
{
    MyGPIO_Config config;

    config.pins = BUTTON_PIN;
    config.mode = MY_GPIO_MODE_INPUT;
    config.output_type = MY_GPIO_OUTPUT_PUSH_PULL;
    config.speed = MY_GPIO_SPEED_LOW;
    config.pull = MY_GPIO_PULL_NONE;
    config.alternate = 0;
    config.initial_value = 0;

    MyGPIO_Init(BUTTON_PORT, &config);

    raw_state = Button_Read();
    stable_state = raw_state;

    raw_change_time = HAL_GetTick();
    press_start_time = HAL_GetTick();
}

ButtonEvent Button_Update(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t current = Button_Read();

    if (current != raw_state)
    {
        raw_state = current;
        raw_change_time = now;
    }

    /*
     * Антидребезг.
     */
    if ((current != stable_state) &&
        ((uint32_t)(now - raw_change_time) >= BUTTON_DEBOUNCE_MS))
    {
        stable_state = current;

        if (stable_state)
        {
            press_start_time = now;
        }
        else
        {
            uint32_t duration =
                (uint32_t)(now - press_start_time);

            if (duration >= BUTTON_LONG_PRESS_MS)
            {
                return BUTTON_EVENT_LONG;
            }

            return BUTTON_EVENT_SHORT;
        }
    }

    return BUTTON_EVENT_NONE;
}

uint8_t Button_IsPressed(void)
{
    return stable_state;
}
