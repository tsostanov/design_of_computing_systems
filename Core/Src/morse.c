#include "morse.h"
#include "button.h"
#include "led.h"

#define MORSE_MAX_SYMBOLS       8

#define MORSE_FEEDBACK_MS       200
#define MORSE_END_PAUSE_MS      2000

#define MORSE_DOT_MS            200
#define MORSE_DASH_MS           600
#define MORSE_SYMBOL_GAP_MS     200

typedef enum
{
    MORSE_DOT,
    MORSE_DASH
} MorseSymbol;

typedef enum
{
    MORSE_MODE_INPUT,
    MORSE_MODE_TRANSMIT_ON,
    MORSE_MODE_TRANSMIT_GAP
} MorseMode;

static MorseSymbol sequence[MORSE_MAX_SYMBOLS];

static uint8_t sequence_length = 0;
static uint8_t transmit_index = 0;

static MorseMode mode = MORSE_MODE_INPUT;

static uint32_t last_input_time = 0;
static uint32_t transmit_time = 0;

static uint8_t has_input = 0;

static uint8_t feedback_active = 0;
static uint32_t feedback_time = 0;


static void Morse_StartFeedback(MorseSymbol symbol, uint32_t now)
{
    LED_BicolorOff();

    if (symbol == MORSE_DOT)
    {
        LED_YellowOn();
    }
    else
    {
        LED_RedOn();
    }

    feedback_time = now;
    feedback_active = 1;
}


static void Morse_UpdateFeedback(uint32_t now)
{
    if (feedback_active &&
        (uint32_t)(now - feedback_time) >= MORSE_FEEDBACK_MS)
    {
        LED_BicolorOff();
        feedback_active = 0;
    }
}


static void Morse_StartTransmission(uint32_t now)
{
    LED_BicolorOff();

    transmit_index = 0;

    mode = MORSE_MODE_TRANSMIT_ON;
    transmit_time = now;

    LED_GreenOn();
}


static uint32_t Morse_CurrentSignalDuration(void)
{
    if (sequence[transmit_index] == MORSE_DOT)
    {
        return MORSE_DOT_MS;
    }

    return MORSE_DASH_MS;
}


static void Morse_FinishTransmission(void)
{
    LED_GreenOff();

    sequence_length = 0;
    transmit_index = 0;

    has_input = 0;
    feedback_active = 0;

    mode = MORSE_MODE_INPUT;

    /*
     * Начинаем отсчёт заново.
     */
    last_input_time = HAL_GetTick();
}


void Morse_Init(void)
{
    LED_Init();
    Button_Init();

    sequence_length = 0;
    transmit_index = 0;

    has_input = 0;
    feedback_active = 0;

    mode = MORSE_MODE_INPUT;

    last_input_time = HAL_GetTick();
    transmit_time = HAL_GetTick();
}


void Morse_Update(void)
{
    uint32_t now = HAL_GetTick();

    ButtonEvent event = Button_Update();

    Morse_UpdateFeedback(now);

    /*
     * Пока пользователь держит кнопку,
     * это НЕ считается паузой между символами.
     *
     * Поэтому постоянно переносим начало отсчёта
     * двухсекундной паузы.
     */
    if (mode == MORSE_MODE_INPUT &&
        Button_IsPressed())
    {
        last_input_time = now;
    }


    if (mode == MORSE_MODE_INPUT)
    {
        /*
         * Короткое нажатие = точка.
         */
        if (event == BUTTON_EVENT_SHORT)
        {
            if (sequence_length < MORSE_MAX_SYMBOLS)
            {
                sequence[sequence_length] = MORSE_DOT;
                sequence_length++;

                has_input = 1;
                last_input_time = now;

                Morse_StartFeedback(
                    MORSE_DOT,
                    now
                );
            }
        }

        /*
         * Длинное нажатие = тире.
         */
        else if (event == BUTTON_EVENT_LONG)
        {
            if (sequence_length < MORSE_MAX_SYMBOLS)
            {
                sequence[sequence_length] = MORSE_DASH;
                sequence_length++;

                has_input = 1;
                last_input_time = now;

                Morse_StartFeedback(
                    MORSE_DASH,
                    now
                );
            }
        }

        /*
         * Передачу начинаем ТОЛЬКО:
         *
         * 1. если уже есть введённые символы;
         * 2. кнопка сейчас НЕ нажата;
         * 3. после последнего ввода прошло 2 секунды.
         */
        if (has_input &&
            !Button_IsPressed() &&
            (uint32_t)(now - last_input_time) >= MORSE_END_PAUSE_MS)
        {
            Morse_StartTransmission(now);
        }
    }


    else if (mode == MORSE_MODE_TRANSMIT_ON)
    {
        uint32_t duration =
            Morse_CurrentSignalDuration();

        if ((uint32_t)(now - transmit_time) >= duration)
        {
            LED_GreenOff();

            transmit_time = now;

            /*
             * Если это был последний символ,
             * заканчиваем передачу.
             */
            if (transmit_index + 1 >= sequence_length)
            {
                Morse_FinishTransmission();
            }
            else
            {
                /*
                 * Иначе ждём промежуток
                 * перед следующим символом.
                 */
                mode = MORSE_MODE_TRANSMIT_GAP;
            }
        }
    }


    else if (mode == MORSE_MODE_TRANSMIT_GAP)
    {
        if ((uint32_t)(now - transmit_time) >= MORSE_SYMBOL_GAP_MS)
        {
            transmit_index++;

            LED_GreenOn();

            transmit_time = now;

            mode = MORSE_MODE_TRANSMIT_ON;
        }
    }
}
