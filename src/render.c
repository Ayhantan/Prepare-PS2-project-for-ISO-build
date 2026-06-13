#include "render.h"

#include <stdio.h>

typedef struct
{
    char symbol;
    unsigned char rows[7];
} Glyph;

enum
{
    COLOR_BACKGROUND = 0,
    COLOR_ARENA = 1,
    COLOR_PLAYER = 2,
    COLOR_ENEMY = 3,
    COLOR_BULLET = 4,
    COLOR_TEXT = 5,
    COLOR_TEXT_ACCENT = 6,
    COLOR_PANEL = 7,
    COLOR_BUTTON = 8
};

static const Glyph glyphs[] = {
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {':', {0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x07, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C}},
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x12, 0x11, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'[', {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}}
};

static u64 color_table[] = {
    GS_SETREG_RGBAQ(0x06, 0x0A, 0x16, 0x00, 0x00),
    GS_SETREG_RGBAQ(0x18, 0x4E, 0xA6, 0x00, 0x00),
    GS_SETREG_RGBAQ(0xF0, 0xD0, 0x20, 0x00, 0x00),
    GS_SETREG_RGBAQ(0xE0, 0x42, 0x42, 0x00, 0x00),
    GS_SETREG_RGBAQ(0xFF, 0xFF, 0xFF, 0x00, 0x00),
    GS_SETREG_RGBAQ(0xF4, 0xF1, 0xD8, 0x00, 0x00),
    GS_SETREG_RGBAQ(0x82, 0xD7, 0xFF, 0x00, 0x00),
    GS_SETREG_RGBAQ(0x0C, 0x18, 0x2A, 0x00, 0x00),
    GS_SETREG_RGBAQ(0xD7, 0x67, 0x1B, 0x00, 0x00)
};

static const unsigned char cedilla_rows[7] = {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x06};

static u64 color_value(int index)
{
    return color_table[index];
}

static void draw_rect(GSGLOBAL *gsGlobal, int x, int y, int width, int height, u64 color)
{
    gsKit_prim_sprite(gsGlobal,
                      (float)x, (float)y,
                      (float)(x + width), (float)(y + height),
                      1,
                      color);
}

static const unsigned char *find_glyph_rows(char symbol)
{
    int i;
    int count = (int)(sizeof(glyphs) / sizeof(glyphs[0]));

    for (i = 0; i < count; ++i) {
        if (glyphs[i].symbol == symbol) {
            return glyphs[i].rows;
        }
    }

    return glyphs[0].rows;
}

static void draw_glyph_rows(GSGLOBAL *gsGlobal, int x, int y, int scale, const unsigned char *rows, u64 color)
{
    int row;
    int column;

    for (row = 0; row < 7; ++row) {
        for (column = 0; column < 5; ++column) {
            if ((rows[row] & (1 << (4 - column))) == 0) {
                continue;
            }

            draw_rect(gsGlobal,
                      x + (column * scale),
                      y + (row * scale),
                      scale,
                      scale,
                      color);
        }
    }
}

static int draw_text(GSGLOBAL *gsGlobal, int x, int y, const char *text, int scale, u64 color)
{
    int cursor_x = x;
    const unsigned char *bytes = (const unsigned char *)text;

    while (*bytes != '\0') {
        if (bytes[0] == 0xC3 && (bytes[1] == 0x87 || bytes[1] == 0xA7)) {
            draw_glyph_rows(gsGlobal, cursor_x, y, scale, cedilla_rows, color);
            cursor_x += (6 * scale);
            bytes += 2;
            continue;
        }

        draw_glyph_rows(gsGlobal, cursor_x, y, scale, find_glyph_rows((char)*bytes), color);
        cursor_x += (6 * scale);
        bytes++;
    }

    return cursor_x;
}

static void draw_menu(GSGLOBAL *gsGlobal)
{
    int button_x = 220;
    int button_y = 244;
    int button_w = 200;
    int button_h = 44;

    draw_rect(gsGlobal, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color_value(COLOR_BACKGROUND));
    draw_rect(gsGlobal, ARENA_X, ARENA_Y, ARENA_WIDTH, ARENA_HEIGHT, color_value(COLOR_PANEL));
    draw_text(gsGlobal, 168, 112, "PS2 SURVIVOR", 4, color_value(COLOR_TEXT_ACCENT));
    draw_text(gsGlobal, 170, 168, "AYHAN TAN A\xC3\x87" "AR", 3, color_value(COLOR_TEXT));
    draw_rect(gsGlobal, 108, 124, 40, 40, color_value(COLOR_PLAYER));
    draw_rect(gsGlobal, button_x, button_y, button_w, button_h, color_value(COLOR_BUTTON));
    draw_text(gsGlobal, 280, 258, "PLAY", 3, color_value(COLOR_TEXT));
    draw_text(gsGlobal, 132, 328, "PRESS START OR CROSS", 2, color_value(COLOR_TEXT));
}

static void draw_playing(GSGLOBAL *gsGlobal, const GameState *game)
{
    int i;
    char level_text[16];
    int player_x = game->player_x >> FIXED_SHIFT;
    int player_y = game->player_y >> FIXED_SHIFT;

    draw_rect(gsGlobal, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color_value(COLOR_BACKGROUND));
    draw_rect(gsGlobal, ARENA_X, ARENA_Y, ARENA_WIDTH, ARENA_HEIGHT, color_value(COLOR_ARENA));

    for (i = 0; i < MAX_BULLETS; ++i) {
        const Bullet *bullet = &game->bullets[i];

        if (!bullet->active) {
            continue;
        }

        draw_rect(gsGlobal,
                  bullet->x >> FIXED_SHIFT,
                  bullet->y >> FIXED_SHIFT,
                  bullet->size,
                  bullet->size,
                  color_value(COLOR_BULLET));
    }

    for (i = 0; i < MAX_ENEMIES; ++i) {
        const Enemy *enemy = &game->enemies[i];

        if (!enemy->active) {
            continue;
        }

        draw_rect(gsGlobal,
                  enemy->x >> FIXED_SHIFT,
                  enemy->y >> FIXED_SHIFT,
                  enemy->size,
                  enemy->size,
                  color_value(COLOR_ENEMY));
    }

    draw_rect(gsGlobal, player_x, player_y, game->player_size, game->player_size, color_value(COLOR_PLAYER));

    snprintf(level_text, sizeof(level_text), "LEVEL %d", game->level);
    draw_text(gsGlobal, 18, 18, level_text, 2, color_value(COLOR_TEXT));
}

static void draw_game_over(GSGLOBAL *gsGlobal, const GameState *game)
{
    draw_playing(gsGlobal, game);
    draw_rect(gsGlobal, 164, 148, 312, 124, color_value(COLOR_PANEL));
    draw_text(gsGlobal, 214, 176, "GAME OVER", 3, color_value(COLOR_TEXT_ACCENT));
    draw_text(gsGlobal, 178, 226, "PRESS START OR CROSS", 2, color_value(COLOR_TEXT));
}

void render_init(GSGLOBAL *gsGlobal)
{
    gsGlobal->Mode = GS_MODE_NTSC;
    gsGlobal->Interlace = GS_INTERLACED;
    gsGlobal->Field = GS_FIELD;

    gsKit_init_screen(gsGlobal);
    gsKit_mode_switch(gsGlobal, GS_ONESHOT);
}

void render_frame(GSGLOBAL *gsGlobal, const GameState *game)
{
    gsKit_clear(gsGlobal, color_value(COLOR_BACKGROUND));

    if (game->state == STATE_MENU) {
        draw_menu(gsGlobal);
    } else if (game->state == STATE_GAME_OVER) {
        draw_game_over(gsGlobal, game);
    } else {
        draw_playing(gsGlobal, game);
    }
}
