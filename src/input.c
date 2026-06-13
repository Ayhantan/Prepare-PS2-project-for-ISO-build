#include "input.h"

#include <kernel.h>
#include <libpad.h>
#include <loadfile.h>
#include <sifrpc.h>

static char pad_buffer[256] __attribute__((aligned(64)));
static u32 previous_buttons;

static void load_pad_modules(void)
{
    int result;

    result = SifLoadModule("rom0:SIO2MAN", 0, 0);
    if (result < 0) {
        SleepThread();
    }

    result = SifLoadModule("rom0:PADMAN", 0, 0);
    if (result < 0) {
        SleepThread();
    }
}

static void wait_pad_ready(void)
{
    int state = padGetState(0, 0);

    while (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) {
        if (state == PAD_STATE_DISCONN) {
            break;
        }

        state = padGetState(0, 0);
    }
}

static void enable_dualshock_mode(void)
{
    int modes;
    int i;

    wait_pad_ready();

    modes = padInfoMode(0, 0, PAD_MODETABLE, -1);
    if (modes <= 0) {
        return;
    }

    for (i = 0; i < modes; ++i) {
        if (padInfoMode(0, 0, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK) {
            padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
            wait_pad_ready();
            return;
        }
    }
}

int input_init(void)
{
    sceSifInitRpc(0);
    load_pad_modules();

    padInit(0);
    previous_buttons = 0;

    if (padPortOpen(0, 0, pad_buffer) == 0) {
        return 0;
    }

    enable_dualshock_mode();
    return 1;
}

void input_update(InputState *input)
{
    struct padButtonStatus buttons;
    int state;
    int result;
    u32 current_buttons = 0;

    input->held = 0;
    input->pressed = 0;
    input->connected = 0;

    state = padGetState(0, 0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) {
        previous_buttons = 0;
        return;
    }

    result = padRead(0, 0, &buttons);
    if (result == 0) {
        previous_buttons = 0;
        return;
    }

    current_buttons = 0xffff ^ buttons.btns;
    input->held = current_buttons;
    input->pressed = current_buttons & ~previous_buttons;
    input->connected = 1;
    previous_buttons = current_buttons;
}
