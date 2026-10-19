#include "morse_codec.h"
#include <stddef.h>
#include <string.h>
static const char *const alphabet[26] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",
    ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.",
    "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--.."
};
char Morse_Normalize(char c)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    return (c >= 'A' && c <= 'Z') ? c : 0;
}
const char *Morse_Encode(char c)
{
    c = Morse_Normalize(c);
    return c ? alphabet[c - 'A'] : NULL;
}
char Morse_Decode(const char *symbols)
{
    if (!symbols) return 0;
    for (unsigned i = 0; i < 26; ++i)
        if (strcmp(symbols, alphabet[i]) == 0) return (char)('A' + i);
    return 0;
}
