#include "morse.h"
#include "morse_codec.h"
#include <string.h>
static void StartLetter(MorseState *s, char c, uint32_t now)
{
    s->current = c;
    s->position = 0;
    s->phase = MORSE_ON;
    s->phase_time = now;
    s->green = true;
}
void Morse_Init(MorseState *s, uint32_t now)
{
    memset(s, 0, sizeof(*s));
    Ring_Init(&s->letters, s->letter_storage, sizeof(s->letter_storage));
    Ring_Init(&s->results, s->result_storage, sizeof(s->result_storage));
    s->last_input = s->phase_time = now;
}
bool Morse_CanEnqueue(const MorseState *s)
{
    return s->phase == MORSE_IDLE || Ring_Free(&s->letters) != 0;
}
bool Morse_Enqueue(MorseState *s, char c, uint32_t now)
{
    c = Morse_Normalize(c);
    if (!c) return false;
    if (s->phase == MORSE_IDLE) {
        StartLetter(s, c, now);
        return true;
    }
    uint8_t byte = (uint8_t)c;
    return Ring_Write(&s->letters, &byte, 1);
}
void Morse_Input(MorseState *s, char symbol, uint32_t now)
{
    if (symbol != '.' && symbol != '-') return;
    if (s->input_length < MORSE_INPUT_CAPACITY) {
        s->input[s->input_length++] = symbol;
        s->input[s->input_length] = '\0';
    } else s->input_overflow = true;
    s->last_input = s->feedback_time = now;
    s->feedback = symbol;
}
void Morse_Update(MorseState *s, uint32_t now, bool pressed)
{
    if (s->feedback && (uint32_t)(now - s->feedback_time) >= MORSE_FEEDBACK_MS)
        s->feedback = 0;
    if (pressed) s->last_input = now;
    if (!pressed && s->input_length &&
        (uint32_t)(now - s->last_input) >= MORSE_END_PAUSE_MS) {
        uint8_t result = s->input_overflow ? MORSE_INPUT_OVERFLOW :
                         (uint8_t)Morse_Decode(s->input);
        if (!Ring_Write(&s->results, &result, 1)) ++s->lost_results;
        s->input_length = 0;
        s->input[0] = '\0';
        s->input_overflow = false;
    }
    uint32_t elapsed = (uint32_t)(now - s->phase_time);
    const char *code = Morse_Encode(s->current);
    switch (s->phase) {
    case MORSE_ON:
        if (elapsed >= (code[s->position] == '.' ? MORSE_DOT_MS : MORSE_DASH_MS)) {
            s->green = false;
            s->phase_time = now;
            if (code[++s->position]) s->phase = MORSE_GAP;
            else { s->phase = MORSE_LETTER_GAP; s->current = 0; }
        }
        break;
    case MORSE_GAP:
        if (elapsed >= MORSE_SYMBOL_GAP_MS) {
            s->green = true;
            s->phase = MORSE_ON;
            s->phase_time = now;
        }
        break;
    case MORSE_LETTER_GAP:
        if (elapsed >= MORSE_LETTER_GAP_MS) {
            uint8_t c;
            if (Ring_Read(&s->letters, &c)) StartLetter(s, (char)c, now);
            else s->phase = MORSE_IDLE;
        }
        break;
    case MORSE_IDLE:
        break;
    }
}
