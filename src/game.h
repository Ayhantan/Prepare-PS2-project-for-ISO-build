#ifndef GAME_H
#define GAME_H

#include <tamtypes.h>

#include "input.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 448

#define ARENA_X 72
#define ARENA_Y 72
#define ARENA_WIDTH 496
#define ARENA_HEIGHT 304

#define FIXED_SHIFT 8
#define FIXED_ONE (1 << FIXED_SHIFT)

#define MAX_BULLETS 24
#define MAX_ENEMIES 24

typedef enum
{
    STATE_MENU = 0,
    STATE_PLAYING = 1,
    STATE_GAME_OVER = 2
} GameStateId;

typedef struct
{
    int active;
    int x;
    int y;
    int vx;
    int vy;
    int size;
} Bullet;

typedef struct
{
    int active;
    int x;
    int y;
    int speed;
    int size;
} Enemy;

typedef struct
{
    GameStateId state;
    int frame_counter;
    int level;
    int spawn_timer;
    int fire_cooldown;
    int player_x;
    int player_y;
    int player_size;
    int last_move_x;
    int last_move_y;
    Bullet bullets[MAX_BULLETS];
    Enemy enemies[MAX_ENEMIES];
} GameState;

void game_init(GameState *game);
void game_update(GameState *game, const InputState *input);

#endif
