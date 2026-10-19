#include "app.h"
#include "button.h"
#include "led.h"
#include "morse.h"
#include "morse_codec.h"
#include "uart_driver.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static MorseState morse;
static bool mode_reply_pending;
static UART_Diagnostics reported;
static uint32_t reported_results;

static bool Message(const char *message)
{
    return UART_DriverWrite((const uint8_t *)message, strlen(message));
}
static void ReportDiagnostics(void)
{
    UART_Diagnostics current = UART_DriverDiagnostics();
    char message[96];
    if (current.rx_overflow != reported.rx_overflow ||
        current.line_errors != reported.line_errors || current.tx_errors != reported.tx_errors) {
        (void)snprintf(message, sizeof(message),
            "\r\nerror: uart rx_overflow=%lu line=%lu tx=%lu\r\n",
            (unsigned long)current.rx_overflow, (unsigned long)current.line_errors,
            (unsigned long)current.tx_errors);
        if (Message(message)) reported = current;
    }
    if (morse.lost_results != reported_results) {
        (void)snprintf(message, sizeof(message),
            "\r\nerror: button result queue overflow (%lu)\r\n",
            (unsigned long)morse.lost_results);
        if (Message(message)) reported_results = morse.lost_results;
    }
}
static void ProcessButtonResult(void)
{
    uint8_t result;
    if (!Ring_Peek(&morse.results, &result)) return;
    char message[64];
    if (result == MORSE_INVALID)
        strcpy(message, "\r\nerror: invalid morse sequence\r\n");
    else if (result == MORSE_INPUT_OVERFLOW)
        strcpy(message, "\r\nerror: morse input overflow\r\n");
    else
        (void)snprintf(message, sizeof(message), "\r\nreceived symbol: %c\r\n", result);
    if (Message(message)) (void)Ring_Read(&morse.results, &result);
}
static void ProcessSerial(uint32_t now)
{
    if (mode_reply_pending) {
        if (!UART_DriverSwitching() &&
            Message(UART_DriverInterrupts() ? "\r\ninterrupts on\r\n" : "\r\ninterrupts off\r\n"))
            mode_reply_pending = false;
        return;
    }
    uint8_t byte;
    if (UART_DriverPeek(&byte) != UART_DATA_READY) return;
    /* One transaction contains the original echo byte plus its response.
     * Do not consume/execute the command unless its entire response fits. */
    char packet[64];
    packet[0] = (char)byte;
    size_t length = 1;
    char letter = Morse_Normalize((char)byte);
    bool accepted_letter = letter && Morse_CanEnqueue(&morse);
    const char *response = NULL;
    if (letter) {
        if (accepted_letter)
            length += (size_t)snprintf(packet + 1, sizeof(packet) - 1,
                         "\r\ntransmitting symbol: %c\r\n", letter);
        else response = "\r\nerror: letter queue full\r\n";
    } else if (byte != '+' && byte != '\r' && byte != '\n') {
        response = "\r\nerror: unsupported input\r\n";
    }
    if (response) {
        size_t n = strlen(response);
        memcpy(packet + length, response, n);
        length += n;
    }
    if (!UART_DriverWrite((const uint8_t *)packet, length)) return;
    (void)UART_DriverRead(&byte);
    if (accepted_letter) (void)Morse_Enqueue(&morse, letter, now);
    if (byte == '+') {
        (void)UART_DriverRequestMode(!UART_DriverInterrupts());
        mode_reply_pending = true;
    }
}
void App_Init(void)
{
    LED_Init();
    Button_Init();
    Morse_Init(&morse, HAL_GetTick());
    mode_reply_pending = true; /* Startup notification uses the same retry path. */
    memset(&reported, 0, sizeof(reported));
    reported_results = 0;
    UART_BoardStart();
}
void App_Update(void)
{
    UART_DriverService();
    ButtonEvent event = Button_Update();
    uint32_t now = HAL_GetTick();
    if (event == BUTTON_EVENT_SHORT) Morse_Input(&morse, '.', now);
    if (event == BUTTON_EVENT_LONG) Morse_Input(&morse, '-', now);
    Morse_Update(&morse, now, Button_IsPressed() != 0);
    ReportDiagnostics();
    ProcessButtonResult();
    ProcessSerial(now);
    if (morse.green) LED_GreenOn(); else LED_GreenOff();
    if (morse.feedback == '.') LED_YellowOn();
    else if (morse.feedback == '-') LED_RedOn();
    else LED_BicolorOff();
    UART_DriverService();
}
