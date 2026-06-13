#ifndef RENDER_H
#define RENDER_H

#include <gsKit.h>

#include "game.h"

void render_init(GSGLOBAL *gsGlobal);
void render_frame(GSGLOBAL *gsGlobal, const GameState *game);

#endif
