#ifndef INC_MORSE_H_
#define INC_MORSE_H_
#include "lab_config.h"
#include "ring_buffer.h"
typedef enum { MORSE_IDLE, MORSE_ON, MORSE_GAP, MORSE_LETTER_GAP } MorsePhase;
enum { MORSE_INVALID = 0, MORSE_INPUT_OVERFLOW = 1 };
typedef struct {
    RingBuffer letters, results;
    uint8_t letter_storage[MORSE_LETTER_CAPACITY];
    uint8_t result_storage[MORSE_RESULT_CAPACITY];
    char input[MORSE_INPUT_CAPACITY + 1U];
    unsigned input_length, position;
    bool input_overflow, green;
    uint32_t last_input, phase_time, feedback_time, lost_results;
    char current, feedback;
    MorsePhase phase;
} MorseState;
void Morse_Init(MorseState *state, uint32_t now);
bool Morse_CanEnqueue(const MorseState *state);
bool Morse_Enqueue(MorseState *state, char letter, uint32_t now);
void Morse_Input(MorseState *state, char symbol, uint32_t now);
void Morse_Update(MorseState *state, uint32_t now, bool pressed);
#endif
