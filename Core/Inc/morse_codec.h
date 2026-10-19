#ifndef MORSE_CODEC_H
#define MORSE_CODEC_H
char Morse_Normalize(char letter);
const char *Morse_Encode(char letter);
char Morse_Decode(const char *symbols);
#endif
