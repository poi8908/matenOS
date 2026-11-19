#include "arcade.h"
#include "gfx.h"
#include "string.h"
#include "timer.h"
#include "mouse.h"

#define C_TEXT  0x00141C24
#define C_MUTED 0x00607080
#define C_GREEN 0x006EE7A8
#define C_WHITE 0x00FFFFFF
#define C_BLUE  0x004A90D9
#define C_RED   0x00D9534F
#define C_DARK  0x00202830
#define C_GOLD  0x00F0C040
#define C_PANEL 0x00E8EEF4

/* app ids from gui.c */
#define A_TTT      2
#define A_MEMORY   13
#define A_2048     14
#define A_PONG     15
#define A_BREAK    16
#define A_LIFE     20
#define A_MAZE     22

static uint32_t seed = 0x2468ACE;

static uint32_t rnd(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static void num(char *out, int value)
{
    char tmp[12];
    int i = 0;
    int n = 0;
    if (value < 0) { out[n++] = '-'; value = -value; }
    if (value == 0) { out[n++] = '0'; out[n] = 0; return; }
    while (value > 0) { tmp[i++] = (char)('0' + value % 10); value /= 10; }
    while (i > 0) out[n++] = tmp[--i];
    out[n] = 0;
}

/* ---------- tic-tac-toe ---------- */
static int ttt[9];
static int ttt_turn;
static int ttt_winner;

static int ttt_check(void)
{
    static const int lines[8][3] = {
        {0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}
    };
    int i;
    for (i = 0; i < 8; i++) {
        int a = lines[i][0], b = lines[i][1], c = lines[i][2];
        if (ttt[a] && ttt[a] == ttt[b] && ttt[a] == ttt[c])
            return ttt[a];
    }
    return 0;
}

static void ttt_ai(void)
{
    int empty[9], n = 0, i;
    for (i = 0; i < 9; i++) if (!ttt[i]) empty[n++] = i;
    if (n) ttt[empty[rnd() % n]] = 2;
}

static void ttt_reset(void)
{
    int i;
    for (i = 0; i < 9; i++) ttt[i] = 0;
    ttt_turn = 1;
    ttt_winner = 0;
}

/* ---------- memory ---------- */
static int mem_val[16];
static int mem_open[16];
static int mem_done[16];
static int mem_first = -1;
static int mem_moves;

static void mem_reset(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        mem_val[i * 2] = i + 1;
        mem_val[i * 2 + 1] = i + 1;
    }
    for (i = 15; i > 0; i--) {
        int j = (int)(rnd() % (uint32_t)(i + 1));
        int t = mem_val[i]; mem_val[i] = mem_val[j]; mem_val[j] = t;
    }
    memset(mem_open, 0, sizeof(mem_open));
    memset(mem_done, 0, sizeof(mem_done));
    mem_first = -1;
    mem_moves = 0;
}

/* ---------- 2048 ---------- */
static int g2048[16];
static int score2048;

static void add_tile(void)
{
    int empty[16], n = 0, i;
    for (i = 0; i < 16; i++) if (!g2048[i]) empty[n++] = i;
    if (n) g2048[empty[rnd() % n]] = (rnd() & 3) ? 2 : 4;
}

static void g2048_reset(void)
{
    memset(g2048, 0, sizeof(g2048));
    score2048 = 0;
    add_tile();
    add_tile();
}

static int slide_line(int *line)
{
    int out[4] = {0,0,0,0};
    int i, p = 0, moved = 0;
    for (i = 0; i < 4; i++) {
        if (line[i]) {
            if (p && out[p-1] == line[i]) {
                out[p-1] *= 2;
                score2048 += out[p-1];
                moved = 1;
            } else {
                out[p++] = line[i];
            }
        }
    }
    for (i = 0; i < 4; i++)
        if (line[i] != out[i]) moved = 1;
    for (i = 0; i < 4; i++) line[i] = out[i];
    return moved;
}

static void g2048_move(int dir)
{
    int i, j, moved = 0;
    int line[4];
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (dir == 0) line[j] = g2048[j * 4 + i];
            else if (dir == 1) line[j] = g2048[(3 - j) * 4 + i];
            else if (dir == 2) line[j] = g2048[i * 4 + j];
            else line[j] = g2048[i * 4 + (3 - j)];
        }
        moved |= slide_line(line);
        for (j = 0; j < 4; j++) {
            if (dir == 0) g2048[j * 4 + i] = line[j];
            else if (dir == 1) g2048[(3 - j) * 4 + i] = line[j];
            else if (dir == 2) g2048[i * 4 + j] = line[j];
            else g2048[i * 4 + (3 - j)] = line[j];
        }
    }
    if (moved) add_tile();
}

/* ---------- pong / breakout ---------- */
static int pong_x, pong_y, pong_vx, pong_vy;
static int pong_py, pong_ay;
static int pong_score, pong_ai;
static int brk_x, brk_y, brk_vx, brk_vy, brk_paddle, brk_score;
static uint8_t brk_bricks[40];

static void pong_reset(void)
{
    pong_x = 268; pong_y = 160;
    pong_vx = 4; pong_vy = 3;
    pong_py = 120; pong_ay = 120;
    pong_score = 0; pong_ai = 0;
}

static void brk_reset(void)
{
    int i;
    brk_x = 268; brk_y = 200;
    brk_vx = 3; brk_vy = -3;
    brk_paddle = 220;
    brk_score = 0;
    for (i = 0; i < 40; i++) brk_bricks[i] = 1;
}

/* ---------- life ---------- */
#define LIFE_W 40
#define LIFE_H 24
static uint8_t life[LIFE_W * LIFE_H];
static int life_gen;
static int life_timer;

static void life_reset(void)
{
    int i;
    for (i = 0; i < LIFE_W * LIFE_H; i++)
        life[i] = (uint8_t)(rnd() & 1);
    life_gen = 0;
    life_timer = 0;
}

static void life_step(void)
{
    uint8_t next[LIFE_W * LIFE_H];
    int x, y;
    for (y = 0; y < LIFE_H; y++) {
        for (x = 0; x < LIFE_W; x++) {
            int n = 0, dx, dy;
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    int nx, ny;
                    if (!dx && !dy) continue;
                    nx = (x + dx + LIFE_W) % LIFE_W;
                    ny = (y + dy + LIFE_H) % LIFE_H;
                    n += life[ny * LIFE_W + nx] ? 1 : 0;
                }
            }
            next[y * LIFE_W + x] = life[y * LIFE_W + x]
                ? (n == 2 || n == 3) : (n == 3);
        }
    }
    memcpy(life, next, sizeof(life));
    life_gen++;
}

/* ---------- maze ---------- */
static char maze_map[13][21];
static int maze_x, maze_y, maze_won;
static int maze_level;

static void maze_generate(int level)
{
    int r, c, i;
    int sx[100], sy[100], sp = 0;
    uint32_t old_seed = seed;

    seed = 0x9E3779B9u ^ ((uint32_t)(level + 1) * 2654435761u);
    for (r = 0; r < 13; r++)
        for (c = 0; c < 20; c++)
            maze_map[r][c] = '#';
    for (r = 0; r < 13; r++)
        maze_map[r][20] = 0;

    maze_map[1][1] = ' ';
    sx[sp] = 1; sy[sp] = 1; sp++;

    while (sp > 0) {
        int x = sx[sp - 1];
        int y = sy[sp - 1];
        int dirs[4] = { 0, 1, 2, 3 };
        int moved = 0;
        for (i = 3; i > 0; i--) {
            int j = (int)(rnd() % (uint32_t)(i + 1));
            int tmp = dirs[i]; dirs[i] = dirs[j]; dirs[j] = tmp;
        }
        for (i = 0; i < 4; i++) {
            int dx = 0, dy = 0;
            if (dirs[i] == 0) dx = 2;
            else if (dirs[i] == 1) dx = -2;
            else if (dirs[i] == 2) dy = 2;
            else dy = -2;
            {
                int nx = x + dx, ny = y + dy;
                if (nx > 0 && nx < 19 && ny > 0 && ny < 12 &&
                    maze_map[ny][nx] == '#') {
                    maze_map[y + dy / 2][x + dx / 2] = ' ';
                    maze_map[ny][nx] = ' ';
                    sx[sp] = nx; sy[sp] = ny; sp++;
                    moved = 1;
                    break;
                }
            }
        }
        if (!moved)
            sp--;
    }

    maze_map[1][1] = 'S';
    maze_map[11][17] = ' ';
    maze_map[11][18] = 'G';
    maze_x = 1; maze_y = 1; maze_won = 0;
    seed = old_seed;
}

static void maze_reset(void)
{
    maze_generate(maze_level);
}

/* ---------- common ---------- */
void arcade_init(int app)
{
    seed ^= timer_ticks();
    if (app == A_TTT) ttt_reset();
    else if (app == A_MEMORY) mem_reset();
    else if (app == A_2048) g2048_reset();
    else if (app == A_PONG) pong_reset();
    else if (app == A_BREAK) brk_reset();
    else if (app == A_LIFE) life_reset();
    else if (app == A_MAZE) maze_reset();
}

static void draw_ttt(int x, int y)
{
    int i;
    char text[2];
    gfx_text(x + 20, y + 8, "tic-tac-toe - click a cell", C_MUTED, 1);
    for (i = 0; i < 9; i++) {
        int cx = x + 150 + (i % 3) * 80;
        int cy = y + 50 + (i / 3) * 80;
        gfx_fill(cx, cy, 76, 76, C_PANEL);
        gfx_rect(cx, cy, 76, 76, C_TEXT);
        if (ttt[i]) {
            text[0] = ttt[i] == 1 ? 'X' : 'O';
            text[1] = 0;
            gfx_text(cx + 26, cy + 22, text, ttt[i] == 1 ? C_BLUE : C_RED, 4);
        }
    }
    if (ttt_winner)
        gfx_text(x + 20, y + 300, ttt_winner == 1 ? "you win - click to reset" : "AI wins - click to reset", C_GREEN, 1);
}

static void draw_memory(int x, int y)
{
    int i;
    char text[2];
    gfx_text(x + 20, y + 8, "memory - click two cards", C_MUTED, 1);
    for (i = 0; i < 16; i++) {
        int cx = x + 100 + (i % 4) * 80;
        int cy = y + 36 + (i / 4) * 68;
        gfx_fill(cx, cy, 64, 56, mem_done[i] ? C_GREEN : (mem_open[i] ? C_WHITE : C_BLUE));
        gfx_rect(cx, cy, 64, 56, C_TEXT);
        if (mem_open[i] || mem_done[i]) {
            text[0] = (char)('0' + mem_val[i]);
            text[1] = 0;
            gfx_text(cx + 24, cy + 16, text, C_TEXT, 3);
        }
    }
}

static void draw_2048(int x, int y)
{
    int i;
    char text[8];
    gfx_text(x + 20, y + 8, "2048 - WASD", C_MUTED, 1);
    for (i = 0; i < 16; i++) {
        int cx = x + 100 + (i % 4) * 70;
        int cy = y + 40 + (i / 4) * 64;
        gfx_fill(cx, cy, 64, 58, g2048[i] ? C_BLUE : C_PANEL);
        gfx_rect(cx, cy, 64, 58, C_TEXT);
        if (g2048[i]) {
            num(text, g2048[i]);
            gfx_text(cx + 10, cy + 18, text, C_WHITE, 2);
        }
    }
    num(text, score2048);
    gfx_text(x + 20, y + 300, "score:", C_MUTED, 1);
    gfx_text(x + 80, y + 300, text, C_TEXT, 1);
}

static void draw_pong(int x, int y)
{
    char text[8];
    gfx_text(x + 20, y + 8, "pong - move mouse", C_MUTED, 1);
    gfx_rect(x + 20, y + 24, 500, 280, C_MUTED);
    gfx_fill(x + 30, y + pong_py, 8, 60, C_GREEN);
    gfx_fill(x + 502, y + pong_ay, 8, 60, C_RED);
    gfx_fill(x + pong_x, y + pong_y, 8, 8, C_WHITE);
    num(text, pong_score); gfx_text(x + 230, y + 300, text, C_GREEN, 1);
    num(text, pong_ai); gfx_text(x + 300, y + 300, text, C_RED, 1);
}

static void draw_break(int x, int y)
{
    int i;
    char text[8];
    gfx_text(x + 20, y + 8, "breakout - move mouse", C_MUTED, 1);
    gfx_rect(x + 20, y + 24, 500, 280, C_MUTED);
    for (i = 0; i < 40; i++) {
        if (brk_bricks[i]) {
            int cx = x + 30 + (i % 10) * 48;
            int cy = y + 34 + (i / 10) * 20;
            gfx_fill(cx, cy, 44, 16, i < 20 ? C_BLUE : C_GREEN);
        }
    }
    gfx_fill(x + brk_paddle, y + 290, 80, 8, C_GOLD);
    gfx_fill(x + brk_x, y + brk_y, 8, 8, C_WHITE);
    num(text, brk_score); gfx_text(x + 20, y + 306, text, C_TEXT, 1);
}

static void draw_life(int x, int y)
{
    int i;
    char text[8];
    gfx_text(x + 20, y + 8, "game of life - click cells, S step", C_MUTED, 1);
    for (i = 0; i < LIFE_W * LIFE_H; i++) {
        if (life[i])
            gfx_fill(x + 20 + (i % LIFE_W) * 12, y + 30 + (i / LIFE_W) * 11, 10, 9, C_GREEN);
    }
    num(text, life_gen); gfx_text(x + 20, y + 306, text, C_TEXT, 1);
}

static void draw_maze(int x, int y)
{
    int r, c;
    {
        char lv[16];
        num(lv, maze_level + 1);
        gfx_text(x + 20, y + 8, "maze", C_MUTED, 1);
        gfx_text(x + 76, y + 8, lv, C_TEXT, 1);
        gfx_text(x + 106, y + 8, "/120 - WASD, N/P", C_MUTED, 1);
    }
    for (r = 0; r < 13; r++) {
        for (c = 0; c < 20; c++) {
            char ch = maze_map[r][c];
            uint32_t color = ch == '#' ? C_DARK : C_PANEL;
            if (ch == 'G') color = C_GOLD;
            gfx_fill(x + 40 + c * 22, y + 24 + r * 22, 22, 22, color);
        }
    }
    gfx_fill(x + 40 + maze_x * 22 + 4, y + 24 + maze_y * 22 + 4, 14, 14, C_RED);
    if (maze_won)
        gfx_text(x + 20, y + 310, "you found the exit", C_GREEN, 1);
}

void arcade_draw(int app, int x, int y, int w, int h)
{
    (void)w; (void)h;
    if (app == A_TTT) draw_ttt(x, y);
    else if (app == A_MEMORY) draw_memory(x, y);
    else if (app == A_2048) draw_2048(x, y);
    else if (app == A_PONG) draw_pong(x, y);
    else if (app == A_BREAK) draw_break(x, y);
    else if (app == A_LIFE) draw_life(x, y);
    else if (app == A_MAZE) draw_maze(x, y);
}

void arcade_click(int app, int mx, int my, int x, int y, int w, int h)
{
    (void)w; (void)h;
    if (app == A_TTT) {
        if (ttt_winner) { ttt_reset(); return; }
        if (mx >= x + 150 && mx < x + 150 + 240 &&
            my >= y + 50 && my < y + 50 + 240) {
            int col = (mx - (x + 150)) / 80;
            int row = (my - (y + 50)) / 80;
            int idx = row * 3 + col;
            if (idx >= 0 && idx < 9 && !ttt[idx] && !ttt_winner) {
                ttt[idx] = 1;
                if (!ttt_check()) {
                    ttt_ai();
                    ttt_winner = ttt_check();
                } else ttt_winner = 1;
                if (!ttt_winner && ttt_turn) {
                    int i, full = 1;
                    for (i = 0; i < 9; i++) if (!ttt[i]) full = 0;
                    if (full) ttt_winner = 3;
                }
            }
        }
    } else if (app == A_MEMORY) {
        if (mx >= x + 100 && mx < x + 100 + 320 &&
            my >= y + 36 && my < y + 36 + 272) {
            int idx = ((my - (y + 36)) / 68) * 4 + ((mx - (x + 100)) / 80);
            if (idx >= 0 && idx < 16 && !mem_open[idx] && !mem_done[idx]) {
                mem_open[idx] = 1;
                if (mem_first < 0) mem_first = idx;
                else if (mem_first != idx) {
                    if (mem_val[mem_first] == mem_val[idx]) {
                        mem_done[mem_first] = mem_done[idx] = 1;
                    } else {
                        mem_open[mem_first] = 0;
                        mem_open[idx] = 0;
                    }
                    mem_first = -1;
                    mem_moves++;
                }
            }
        }
    } else if (app == A_LIFE) {
        if (mx >= x + 20 && mx < x + 20 + LIFE_W * 12 &&
            my >= y + 30 && my < y + 30 + LIFE_H * 11) {
            int idx = ((my - (y + 30)) / 11) * LIFE_W + ((mx - (x + 20)) / 12);
            if (idx >= 0 && idx < LIFE_W * LIFE_H)
                life[idx] = !life[idx];
        }
    } else if (app == A_MAZE) {
        (void)mx; (void)my;
    }
}

void arcade_key(int app, int key)
{
    if (app == A_2048) {
        if (key == 'w' || key == 'W') g2048_move(0);
        else if (key == 's' || key == 'S') g2048_move(1);
        else if (key == 'a' || key == 'A') g2048_move(2);
        else if (key == 'd' || key == 'D') g2048_move(3);
    } else if (app == A_MAZE) {
        int nx = maze_x, ny = maze_y;
        if (key == 'n' || key == 'N') {
            maze_level = (maze_level + 1) % 120;
            maze_generate(maze_level);
            return;
        }
        if (key == 'p' || key == 'P') {
            maze_level = (maze_level + 119) % 120;
            maze_generate(maze_level);
            return;
        }
        if (key == 'w' || key == 'W') ny--;
        else if (key == 's' || key == 'S') ny++;
        else if (key == 'a' || key == 'A') nx--;
        else if (key == 'd' || key == 'D') nx++;
        if (ny >= 0 && ny < 13 && nx >= 0 && nx < 20 && maze_map[ny][nx] != '#') {
            maze_x = nx; maze_y = ny;
            if (maze_map[ny][nx] == 'G') maze_won = 1;
        }
    } else if (app == A_LIFE && (key == 's' || key == 'S')) {
        life_step();
    }
}

void arcade_tick(int app)
{
    if (app == A_PONG) {
        pong_py = mouse_y - 90 - 30;
        if (pong_py < 0) pong_py = 0;
        if (pong_py > 236) pong_py = 236;
        pong_x += pong_vx;
        pong_y += pong_vy;
        if (pong_y <= 24 || pong_y >= 296) pong_vy = -pong_vy;
        if (pong_x <= 30) {
            if (pong_y + 8 >= pong_py && pong_y <= pong_py + 60) pong_vx = -pong_vx;
            else pong_reset();
        }
        if (pong_x >= 500) {
            if (pong_y + 8 >= pong_ay && pong_y <= pong_ay + 60) { pong_vx = -pong_vx; pong_score++; }
            else { pong_ai++; pong_reset(); }
        }
        pong_ay += (pong_y - pong_ay) / 8;
    } else if (app == A_BREAK) {
        brk_paddle = mouse_x - 52 - 40;
        if (brk_paddle < 0) brk_paddle = 0;
        if (brk_paddle > 420) brk_paddle = 420;
        brk_x += brk_vx;
        brk_y += brk_vy;
        if (brk_y <= 24 || brk_y >= 300) brk_vy = -brk_vy;
        if (brk_x <= 20 || brk_x >= 512) brk_vx = -brk_vx;
        if (brk_y >= 280 && brk_x >= brk_paddle && brk_x <= brk_paddle + 80) {
            brk_vy = -brk_vy; brk_score++;
        }
        if (brk_x >= 20 && brk_x < 500 && brk_y >= 24 && brk_y < 120) {
            int idx = ((brk_y - 24) / 20) * 10 + ((brk_x - 20) / 48);
            if (idx >= 0 && idx < 40 && brk_bricks[idx]) {
                brk_bricks[idx] = 0;
                brk_vy = -brk_vy;
                brk_score += 5;
            }
        }
        if (brk_y > 300) brk_reset();
    } else if (app == A_LIFE) {
        if (++life_timer >= 20) {
            life_timer = 0;
            life_step();
        }
    }
}
