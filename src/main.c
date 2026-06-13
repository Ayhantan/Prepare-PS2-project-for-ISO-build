#include <tamtypes.h>
#include <dmaKit.h>
#include <gsKit.h>
#include <kernel.h>

#include "audio.h"
#include "game.h"
#include "input.h"
#include "render.h"

int main(int argc, char *argv[])
{
    GSGLOBAL *gsGlobal;
    GameState game;
    InputState input;

    (void)argc;
    (void)argv;

    dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
                D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);

    gsGlobal = gsKit_init_global();
    render_init(gsGlobal);

    if (!input_init()) {
        SleepThread();
    }

    audio_init();
    game_init(&game);

    for (;;) {
        /* Each frame:
           1. Read input.
           2. Stream audio only while playing.
           3. Update game logic.
           4. Build and execute the draw queue. */
        input_update(&input);
        audio_update(game.state == STATE_PLAYING);
        game_update(&game, &input);
        render_frame(gsGlobal, &game);

        gsKit_queue_exec(gsGlobal);
        gsKit_finish();
        gsKit_sync_flip(gsGlobal);
        gsKit_queue_reset(gsGlobal->Os_Queue);
    }

    return 0;
}
