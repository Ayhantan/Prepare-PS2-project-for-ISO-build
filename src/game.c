#include "game.h"

#include <libpad.h>
#include <stdlib.h>

static int clamp_value(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }

    if (value > max_value) {
        return max_value;
    }

    return value;
}

static int sign_value(int value)
{
    if (value < 0) {
        return -1;
    }

    if (value > 0) {
        return 1;
    }

    return 0;
}

static int rects_overlap(int ax, int ay, int asize, int bx, int by, int bsize)
{
    if ((ax + asize) <= bx || (bx + bsize) <= ax) {
        return 0;
    }

    if ((ay + asize) <= by || (by + bsize) <= ay) {
        return 0;
    }

    return 1;
}

static void clear_entities(GameState *game)
{
    int i;

    for (i = 0; i < MAX_BULLETS; ++i) {
        game->bullets[i].active = 0;
    }

    for (i = 0; i < MAX_ENEMIES; ++i) {
        game->enemies[i].active = 0;
    }
}

static void start_new_game(GameState *game)
{
    game->state = STATE_PLAYING;
    game->frame_counter = 0;
    game->level = 1;
    game->spawn_timer = 45;
    game->fire_cooldown = 0;
    game->player_size = 24;
    game->player_x = ((ARENA_X + (ARENA_WIDTH / 2)) - (game->player_size / 2)) << FIXED_SHIFT;
    game->player_y = ((ARENA_Y + (ARENA_HEIGHT / 2)) - (game->player_size / 2)) << FIXED_SHIFT;
    game->last_move_x = 0;
    game->last_move_y = -1;
    clear_entities(game);
}

static void spawn_bullet(GameState *game)
{
    int i;
    int bullet_size = 6;
    int bullet_speed = 5 * FIXED_ONE;
    int dir_x = game->last_move_x;
    int dir_y = game->last_move_y;

    if (dir_x == 0 && dir_y == 0) {
        dir_y = -1;
    }

    for (i = 0; i < MAX_BULLETS; ++i) {
        Bullet *bullet = &game->bullets[i];

        if (bullet->active) {
            continue;
        }

        bullet->active = 1;
        bullet->size = bullet_size;
        bullet->x = game->player_x + (((game->player_size - bullet_size) / 2) << FIXED_SHIFT);
        bullet->y = game->player_y + (((game->player_size - bullet_size) / 2) << FIXED_SHIFT);
        bullet->vx = dir_x * bullet_speed;
        bullet->vy = dir_y * bullet_speed;
        game->fire_cooldown = 10;
        break;
    }
}

static int count_active_enemies(const GameState *game)
{
    int i;
    int total = 0;

    for (i = 0; i < MAX_ENEMIES; ++i) {
        if (game->enemies[i].active) {
            total++;
        }
    }

    return total;
}

static void spawn_enemy(GameState *game)
{
    int i;
    int edge;
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int enemy_size = 22;
    int spawn_x = ARENA_X;
    int spawn_y = ARENA_Y;

    for (i = 0; i < MAX_ENEMIES; ++i) {
        Enemy *enemy = &game->enemies[i];

        if (enemy->active) {
            continue;
        }

        min_x = ARENA_X;
        max_x = ARENA_X + ARENA_WIDTH - enemy_size;
        min_y = ARENA_Y;
        max_y = ARENA_Y + ARENA_HEIGHT - enemy_size;
        edge = rand() % 4;

        if (edge == 0) {
            spawn_x = min_x + (rand() % (max_x - min_x + 1));
            spawn_y = min_y;
        } else if (edge == 1) {
            spawn_x = max_x;
            spawn_y = min_y + (rand() % (max_y - min_y + 1));
        } else if (edge == 2) {
            spawn_x = min_x + (rand() % (max_x - min_x + 1));
            spawn_y = max_y;
        } else {
            spawn_x = min_x;
            spawn_y = min_y + (rand() % (max_y - min_y + 1));
        }

        enemy->active = 1;
        enemy->size = enemy_size;
        enemy->x = spawn_x << FIXED_SHIFT;
        enemy->y = spawn_y << FIXED_SHIFT;
        enemy->speed = (FIXED_ONE / 3) + (game->level * 20);
        return;
    }
}

static void update_player(GameState *game, const InputState *input)
{
    int move_x = 0;
    int move_y = 0;
    int player_speed = (FIXED_ONE * 3) / 2;
    int min_x = ARENA_X;
    int max_x = ARENA_X + ARENA_WIDTH - game->player_size;
    int min_y = ARENA_Y;
    int max_y = ARENA_Y + ARENA_HEIGHT - game->player_size;

    if (input->held & PAD_LEFT) {
        move_x -= 1;
    }
    if (input->held & PAD_RIGHT) {
        move_x += 1;
    }
    if (input->held & PAD_UP) {
        move_y -= 1;
    }
    if (input->held & PAD_DOWN) {
        move_y += 1;
    }

    game->player_x += move_x * player_speed;
    game->player_y += move_y * player_speed;

    game->player_x = clamp_value(game->player_x, min_x << FIXED_SHIFT, max_x << FIXED_SHIFT);
    game->player_y = clamp_value(game->player_y, min_y << FIXED_SHIFT, max_y << FIXED_SHIFT);

    if (move_x != 0 || move_y != 0) {
        game->last_move_x = sign_value(move_x);
        game->last_move_y = sign_value(move_y);
    }

    if ((input->pressed & PAD_CROSS) && game->fire_cooldown == 0) {
        spawn_bullet(game);
    }

    if (game->fire_cooldown > 0) {
        game->fire_cooldown--;
    }
}

static void update_bullets(GameState *game)
{
    int i;
    int min_x = ARENA_X - 8;
    int max_x = ARENA_X + ARENA_WIDTH + 8;
    int min_y = ARENA_Y - 8;
    int max_y = ARENA_Y + ARENA_HEIGHT + 8;

    for (i = 0; i < MAX_BULLETS; ++i) {
        Bullet *bullet = &game->bullets[i];
        int draw_x;
        int draw_y;

        if (!bullet->active) {
            continue;
        }

        bullet->x += bullet->vx;
        bullet->y += bullet->vy;

        draw_x = bullet->x >> FIXED_SHIFT;
        draw_y = bullet->y >> FIXED_SHIFT;

        if (draw_x < min_x || draw_x > max_x || draw_y < min_y || draw_y > max_y) {
            bullet->active = 0;
        }
    }
}

static void update_enemies(GameState *game)
{
    int i;
    int player_x = game->player_x >> FIXED_SHIFT;
    int player_y = game->player_y >> FIXED_SHIFT;
    int player_size = game->player_size;

    for (i = 0; i < MAX_ENEMIES; ++i) {
        Enemy *enemy = &game->enemies[i];
        int enemy_x;
        int enemy_y;

        if (!enemy->active) {
            continue;
        }

        if (enemy->x < game->player_x) {
            enemy->x += enemy->speed;
        } else if (enemy->x > game->player_x) {
            enemy->x -= enemy->speed;
        }

        if (enemy->y < game->player_y) {
            enemy->y += enemy->speed;
        } else if (enemy->y > game->player_y) {
            enemy->y -= enemy->speed;
        }

        enemy_x = enemy->x >> FIXED_SHIFT;
        enemy_y = enemy->y >> FIXED_SHIFT;

        if (rects_overlap(player_x, player_y, player_size, enemy_x, enemy_y, enemy->size)) {
            game->state = STATE_GAME_OVER;
        }
    }
}

static void resolve_hits(GameState *game)
{
    int i;
    int j;

    for (i = 0; i < MAX_BULLETS; ++i) {
        Bullet *bullet = &game->bullets[i];
        int bullet_x;
        int bullet_y;

        if (!bullet->active) {
            continue;
        }

        bullet_x = bullet->x >> FIXED_SHIFT;
        bullet_y = bullet->y >> FIXED_SHIFT;

        for (j = 0; j < MAX_ENEMIES; ++j) {
            Enemy *enemy = &game->enemies[j];
            int enemy_x;
            int enemy_y;

            if (!enemy->active) {
                continue;
            }

            enemy_x = enemy->x >> FIXED_SHIFT;
            enemy_y = enemy->y >> FIXED_SHIFT;

            if (!rects_overlap(bullet_x, bullet_y, bullet->size, enemy_x, enemy_y, enemy->size)) {
                continue;
            }

            bullet->active = 0;
            enemy->active = 0;
            break;
        }
    }
}

static void update_spawning(GameState *game)
{
    int spawn_interval;
    int target_enemies;

    spawn_interval = 90 - ((game->level - 1) * 6);
    if (spawn_interval < 18) {
        spawn_interval = 18;
    }

    target_enemies = 2 + game->level;
    if (target_enemies > MAX_ENEMIES) {
        target_enemies = MAX_ENEMIES;
    }

    game->spawn_timer--;
    if (game->spawn_timer > 0) {
        return;
    }

    game->spawn_timer = spawn_interval;

    if (count_active_enemies(game) < target_enemies) {
        spawn_enemy(game);
    }
}

void game_init(GameState *game)
{
    game->state = STATE_MENU;
    game->frame_counter = 0;
    game->level = 1;
    game->spawn_timer = 0;
    game->fire_cooldown = 0;
    game->player_size = 24;
    game->player_x = 0;
    game->player_y = 0;
    game->last_move_x = 0;
    game->last_move_y = -1;
    clear_entities(game);
    srand(1);
}

void game_update(GameState *game, const InputState *input)
{
    if (game->state == STATE_MENU) {
        if (input->pressed & (PAD_START | PAD_CROSS)) {
            start_new_game(game);
        }
        return;
    }

    if (game->state == STATE_GAME_OVER) {
        if (input->pressed & (PAD_START | PAD_CROSS)) {
            start_new_game(game);
        }
        return;
    }

    game->frame_counter++;
    game->level = 1 + (game->frame_counter / 900);

    update_player(game, input);
    update_bullets(game);
    update_spawning(game);
    update_enemies(game);
    resolve_hits(game);
}
