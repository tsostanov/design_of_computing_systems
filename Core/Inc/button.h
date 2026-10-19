#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT,
    BUTTON_EVENT_LONG
} ButtonEvent;

void Button_Init(void);
ButtonEvent Button_Update(void);
uint8_t Button_IsPressed(void);

#endif
