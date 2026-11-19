#include "games.h"
#include "gfx.h"
#include "string.h"
#include "timer.h"
#include "zh.h"

#define C_TEXT  0x00141C24
#define C_MUTED 0x00607080
#define C_GREEN 0x006EE7A8
#define C_WHITE 0x00FFFFFF
#define C_BLUE  0x004A90D9
#define C_RED   0x00D9534F
#define C_DARK  0x00202830
#define C_GREY  0x00A0A8B0

static uint32_t random_state = 0x1234567;

static uint32_t rnd(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

/* ================= Minesweeper ================= */

#define MINE_W 10
#define MINE_H 10
#define MINE_COUNT 12
#define MINE_CELL 24

static uint8_t mine[MINE_W * MINE_H];
static uint8_t open_cell[MINE_W * MINE_H];
static uint8_t flag[MINE_W * MINE_H];
static int mines_dead;
static int mines_won;
static int mines_flags;

static void mines_reset(void)
{
    int i;
    int placed = 0;
    int first = 1;

    memset(mine, 0, sizeof(mine));
    memset(open_cell, 0, sizeof(open_cell));
    memset(flag, 0, sizeof(flag));
    mines_dead = 0;
    mines_won = 0;
    mines_flags = 0;
    random_state = timer_ticks() ^ 0x9E3779B9;

    while (placed < MINE_COUNT) {
        i = (int)(rnd() % (MINE_W * MINE_H));
        if (first) {
            first = 0;
            continue;
        }
        if (!mine[i]) {
            mine[i] = 1;
            placed++;
        }
    }
}

static int mine_neighbors(int cx, int cy)
{
    int dx;
    int dy;
    int count = 0;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int nx = cx + dx;
            int ny = cy + dy;
            if (nx >= 0 && ny >= 0 && nx < MINE_W && ny < MINE_H &&
                mine[ny * MINE_W + nx])
                count++;
        }
    }
    return count;
}

static void mines_open(int cx, int cy)
{
    int stack[128];
    int top = 0;

    stack[top++] = cy * MINE_W + cx;

    while (top > 0) {
        int index = stack[--top];
        int x;
        int y;
        int dx;
        int dy;

        if (open_cell[index] || flag[index])
            continue;
        open_cell[index] = 1;
        if (mine[index]) {
            mines_dead = 1;
            return;
        }
        if (mine_neighbors(index % MINE_W, index / MINE_W) != 0)
            continue;

        x = index % MINE_W;
        y = index / MINE_W;
        for (dy = -1; dy <= 1; dy++) {
            for (dx = -1; dx <= 1; dx++) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx >= 0 && ny >= 0 && nx < MINE_W && ny < MINE_H &&
                    !open_cell[ny * MINE_W + nx] && !flag[ny * MINE_W + nx] &&
                    top < 128)
                    stack[top++] = ny * MINE_W + nx;
            }
        }
    }
}

static void mines_check_win(void)
{
    int i;
    for (i = 0; i < MINE_W * MINE_H; i++) {
        if (!mine[i] && !open_cell[i])
            return;
    }
    mines_won = 1;
}

static void mines_grid_origin(int x, int y, int w, int h, int *gx, int *gy)
{
    *gx = x + (w - MINE_W * MINE_CELL) / 2;
    *gy = y + (h - MINE_H * MINE_CELL) / 2 + 14;
}

void mines_init(void)
{
    mines_reset();
}

void mines_draw(int x, int y, int w, int h)
{
    int gx;
    int gy;
    int cx;
    int cy;
    char text[40];

    mines_grid_origin(x, y, w, h, &gx, &gy);

    if (zh_language) {
        if (mines_dead)
            strcpy(text, "失败 点击重置");
        else if (mines_won)
            strcpy(text, "胜利 点击重置");
        else {
            strcpy(text, "雷 12  旗 ");
            char number[4];
            int n = mines_flags;
            int i = 0;
            if (n == 0) number[i++] = '0';
            while (n > 0) { number[i++] = (char)('0' + n % 10); n /= 10; }
            while (i > 0) text[strlen(text)] = number[--i];
            text[strlen(text)] = 0;
        }
        zh_text(x + 6, y + 6, text, mines_dead ? C_RED : C_TEXT, 1);
    } else {
        strcpy(text, mines_dead ? "BOOM!  click Reset" : (mines_won ? "You win!  click Reset" : "Mines 12   flags "));
        if (!mines_dead && !mines_won) {
            char number[4];
            int n = mines_flags;
            int i = 0;
            if (n == 0) number[i++] = '0';
            while (n > 0) { number[i++] = (char)('0' + n % 10); n /= 10; }
            while (i > 0) text[strlen(text)] = number[--i];
            text[strlen(text)] = 0;
        }
        gfx_text(x + 6, y + 6, text, mines_dead ? C_RED : C_TEXT, 1);
    }

    gfx_fill(x + w - 92, y + 4, 86, 24, C_BLUE);
    gfx_text(x + w - 78, y + 10, "Reset", C_WHITE, 1);

    for (cy = 0; cy < MINE_H; cy++) {
        for (cx = 0; cx < MINE_W; cx++) {
            int index = cy * MINE_W + cx;
            int px = gx + cx * MINE_CELL;
            int py = gy + cy * MINE_CELL;
            uint32_t color = open_cell[index] ? 0x00D8E0E8 : 0x00708090;

            gfx_fill(px, py, MINE_CELL - 1, MINE_CELL - 1, color);
            if (open_cell[index]) {
                if (mine[index]) {
                    gfx_fill(px + 6, py + 6, 12, 12, C_RED);
                } else {
                    int count = mine_neighbors(cx, cy);
                    if (count > 0) {
                        char num[2];
                        num[0] = (char)('0' + count);
                        num[1] = 0;
                        gfx_text(px + 8, py + 8, num,
                                 count == 1 ? C_BLUE : (count == 2 ? C_GREEN : C_RED), 1);
                    }
                }
            } else if (flag[index]) {
                gfx_fill(px + 6, py + 4, 4, 16, C_TEXT);
                gfx_fill(px + 10, py + 4, 10, 8, C_RED);
            }
        }
    }
}

void mines_click(int mx, int my, int x, int y, int w, int h, int right)
{
    int gx;
    int gy;
    int cx;
    int cy;
    int index;

    mines_grid_origin(x, y, w, h, &gx, &gy);

    if (mx >= x + w - 92 && mx < x + w - 6 && my >= y + 4 && my < y + 28) {
        mines_reset();
        return;
    }
    if (mines_dead || mines_won)
        return;

    cx = (mx - gx) / MINE_CELL;
    cy = (my - gy) / MINE_CELL;
    if (cx < 0 || cy < 0 || cx >= MINE_W || cy >= MINE_H)
        return;

    index = cy * MINE_W + cx;
    if (right) {
        if (!open_cell[index]) {
            flag[index] = !flag[index];
            mines_flags += flag[index] ? 1 : -1;
        }
        return;
    }
    if (flag[index] || open_cell[index])
        return;

    mines_open(cx, cy);
    if (!mines_dead)
        mines_check_win();
}

/* ================= Snake ================= */

#define SNAKE_W 20
#define SNAKE_H 12
#define SNAKE_CELL 22
#define SNAKE_MAX (SNAKE_W * SNAKE_H)

static int snake_x[SNAKE_MAX];
static int snake_y[SNAKE_MAX];
static int snake_len;
static int snake_dx;
static int snake_dy;
static int snake_food_x;
static int snake_food_y;
static int snake_dead;
static int snake_score;
static uint32_t snake_last_tick;

static void snake_place_food(void)
{
    int i;
    for (;;) {
        snake_food_x = (int)(rnd() % SNAKE_W);
        snake_food_y = (int)(rnd() % SNAKE_H);
        for (i = 0; i < snake_len; i++) {
            if (snake_x[i] == snake_food_x && snake_y[i] == snake_food_y)
                break;
        }
        if (i == snake_len)
            return;
    }
}

static void snake_reset(void)
{
    int i;
    snake_len = 4;
    snake_dx = 1;
    snake_dy = 0;
    snake_dead = 0;
    snake_score = 0;
    snake_last_tick = timer_ticks();
    random_state = timer_ticks() ^ 0x85EBCA6B;

    for (i = 0; i < snake_len; i++) {
        snake_x[i] = 6 - i;
        snake_y[i] = 6;
    }
    snake_place_food();
}

void snake_init(void)
{
    snake_reset();
}

void snake_tick(void)
{
    int i;
    int nx;
    int ny;

    if (snake_dead)
        return;
    if ((timer_ticks() - snake_last_tick) < 12)
        return;
    snake_last_tick = timer_ticks();

    nx = snake_x[0] + snake_dx;
    ny = snake_y[0] + snake_dy;

    if (nx < 0 || ny < 0 || nx >= SNAKE_W || ny >= SNAKE_H) {
        snake_dead = 1;
        return;
    }
    for (i = 0; i < snake_len - 1; i++) {
        if (snake_x[i] == nx && snake_y[i] == ny) {
            snake_dead = 1;
            return;
        }
    }

    for (i = snake_len - 1; i > 0; i--) {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_x[0] = nx;
    snake_y[0] = ny;

    if (nx == snake_food_x && ny == snake_food_y) {
        if (snake_len < SNAKE_MAX) {
            snake_x[snake_len] = snake_x[snake_len - 1];
            snake_y[snake_len] = snake_y[snake_len - 1];
            snake_len++;
        }
        snake_score++;
        snake_place_food();
    }
}

static void snake_origin(int x, int y, int w, int h, int *gx, int *gy)
{
    *gx = x + (w - SNAKE_W * SNAKE_CELL) / 2;
    *gy = y + (h - SNAKE_H * SNAKE_CELL) / 2 + 10;
}

void snake_draw(int x, int y, int w, int h)
{
    int i;
    int gx;
    int gy;
    char text[32];

    snake_origin(x, y, w, h, &gx, &gy);

    strcpy(text, "score ");
    {
        char num[8];
        int n = snake_score;
        int p = 0;
        if (n == 0) num[p++] = '0';
        while (n > 0) { num[p++] = (char)('0' + n % 10); n /= 10; }
        while (p > 0) text[strlen(text)] = num[--p];
        text[strlen(text)] = 0;
    }
    gfx_text(x + 6, y + 6, text, C_TEXT, 1);
    if (zh_language)
        zh_text(x + 140, y + 6, "WASD 移动, R 重置", C_MUTED, 1);
    else
        gfx_text(x + 140, y + 6, "WASD to move, R reset", C_MUTED, 1);

    gfx_fill(gx, gy, SNAKE_W * SNAKE_CELL, SNAKE_H * SNAKE_CELL, C_DARK);
    for (i = 0; i < snake_len; i++) {
        gfx_fill(gx + snake_x[i] * SNAKE_CELL + 1,
                 gy + snake_y[i] * SNAKE_CELL + 1,
                 SNAKE_CELL - 2, SNAKE_CELL - 2,
                 i == 0 ? C_GREEN : 0x0040A060);
    }
    gfx_fill(gx + snake_food_x * SNAKE_CELL + 4,
             gy + snake_food_y * SNAKE_CELL + 4,
             SNAKE_CELL - 8, SNAKE_CELL - 8, C_RED);

    if (snake_dead) {
        gfx_text(gx + 90, gy + SNAKE_H * SNAKE_CELL / 2, "GAME OVER - R", C_RED, 2);
    }
}

void snake_click(int mx, int my, int x, int y, int w, int h)
{
    (void)mx;
    (void)my;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    if (snake_dead)
        snake_reset();
}

void snake_key(int key)
{
    if (key == 'w' || key == 'W') {
        if (snake_dy != 1) { snake_dx = 0; snake_dy = -1; }
    } else if (key == 's' || key == 'S') {
        if (snake_dy != -1) { snake_dx = 0; snake_dy = 1; }
    } else if (key == 'a' || key == 'A') {
        if (snake_dx != 1) { snake_dx = -1; snake_dy = 0; }
    } else if (key == 'd' || key == 'D') {
        if (snake_dx != -1) { snake_dx = 1; snake_dy = 0; }
    } else if (key == 'r' || key == 'R') {
        snake_reset();
    }
}
