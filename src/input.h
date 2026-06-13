#ifndef INPUT_H
#define INPUT_H

#include <tamtypes.h>

typedef struct
{
    u32 held;
    u32 pressed;
    int connected;
} InputState;

int input_init(void);
void input_update(InputState *input);

#endif
