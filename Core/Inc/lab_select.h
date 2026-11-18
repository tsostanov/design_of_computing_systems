#ifndef LAB_SELECT_H
#define LAB_SELECT_H
/* Set to 2 to build the preserved UART/Morse laboratory. */
#ifndef LAB_NUMBER
#define LAB_NUMBER 3
#endif
#if LAB_NUMBER != 2 && LAB_NUMBER != 3
#error Unsupported laboratory number
#endif
#endif
