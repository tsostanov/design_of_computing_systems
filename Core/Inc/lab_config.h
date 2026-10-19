#ifndef LAB_CONFIG_H
#define LAB_CONFIG_H
#define BUTTON_DEBOUNCE_MS       30U
#define BUTTON_LONG_PRESS_MS     600U
#define MORSE_FEEDBACK_MS        200U
#define MORSE_END_PAUSE_MS       2000U
#define MORSE_DOT_MS             200U
#define MORSE_DASH_MS            600U
#define MORSE_SYMBOL_GAP_MS      200U
#define MORSE_LETTER_GAP_MS      600U
#define MORSE_INPUT_CAPACITY     8U
#define MORSE_LETTER_CAPACITY    8U
#define MORSE_RESULT_CAPACITY    8U
#ifndef UART_RX_CAPACITY
#define UART_RX_CAPACITY         256U
#endif
#ifndef UART_TX_CAPACITY
#define UART_TX_CAPACITY         2048U
#endif
#endif
