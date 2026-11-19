#include "cards.h"
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
#define C_GOLD  0x00F0C040
#define C_TITLE 0x002F6699

typedef struct {
    uint8_t rank;   /* 1..13 */
    uint8_t suit;   /* 0 H, 1 D, 2 C, 3 S */
    uint8_t up;
} card_t;

static uint32_t card_seed = 0x1234567;

static uint32_t card_rand(void)
{
    card_seed ^= card_seed << 13;
    card_seed ^= card_seed >> 17;
    card_seed ^= card_seed << 5;
    return card_seed;
}

static int card_red(const card_t *c)
{
    return c->suit == 0 || c->suit == 1;
}

static void card_text(int rank, char *out)
{
    static const char *names[14] = {
        "?", "A", "2", "3", "4", "5", "6", "7",
        "8", "9", "10", "J", "Q", "K"
    };
    strcpy(out, names[rank]);
}

static void draw_card(int x, int y, int w, int h, const card_t *c, int selected)
{
    char text[4];
    char suit[2];
    static const char suits[4] = { 'H', 'D', 'C', 'S' };

    if (!c->up) {
        gfx_fill(x, y, w, h, C_BLUE);
        gfx_rect(x, y, w, h, C_WHITE);
        gfx_fill(x + 6, y + 6, w - 12, h - 12, 0x002F6699);
        return;
    }

    gfx_fill(x, y, w, h, C_WHITE);
    gfx_rect(x, y, w, h, selected ? C_GOLD : C_TEXT);
    if (selected)
        gfx_rect(x + 1, y + 1, w - 2, h - 2, C_GOLD);

    card_text(c->rank, text);
    suit[0] = suits[c->suit & 3];
    suit[1] = 0;
    gfx_text(x + 4, y + 4, text, card_red(c) ? C_RED : C_TEXT, 1);
    gfx_text(x + 4, y + 18, suit, card_red(c) ? C_RED : C_TEXT, 1);
}

/* ================= Solitaire ================= */

#define SOL_COLS 7
#define SOL_CARD_W 48
#define SOL_CARD_H 68
#define SOL_OVERLAP 18

static card_t sol_tab[SOL_COLS][20];
static int sol_tab_n[SOL_COLS];
static card_t sol_found[4][13];
static int sol_found_n[4];
static card_t sol_stock[24];
static int sol_stock_n;
static card_t sol_waste[24];
static int sol_waste_n;
static int sol_sel_type;   /* 1 tableau, 2 waste */
static int sol_sel_col;
static int sol_sel_index;

static void sol_shuffle(card_t *deck, int count)
{
    int i;
    for (i = count - 1; i > 0; i--) {
        int j = (int)(card_rand() % (uint32_t)(i + 1));
        card_t t = deck[i];
        deck[i] = deck[j];
        deck[j] = t;
    }
}

void solitaire_init(void)
{
    card_t deck[52];
    int i;
    int j;
    int n = 0;

    card_seed = timer_ticks() ^ 0xA5A5A5A5;
    for (i = 0; i < 4; i++)
        for (j = 1; j <= 13; j++) {
            deck[n].rank = (uint8_t)j;
            deck[n].suit = (uint8_t)i;
            deck[n].up = 0;
            n++;
        }
    sol_shuffle(deck, 52);

    memset(sol_tab_n, 0, sizeof(sol_tab_n));
    memset(sol_found_n, 0, sizeof(sol_found_n));
    sol_stock_n = 0;
    sol_waste_n = 0;
    sol_sel_type = 0;

    n = 0;
    for (i = 0; i < SOL_COLS; i++) {
        for (j = 0; j <= i; j++) {
            sol_tab[i][sol_tab_n[i]++] = deck[n++];
        }
        sol_tab[i][sol_tab_n[i] - 1].up = 1;
    }
    while (n < 52)
        sol_stock[sol_stock_n++] = deck[n++];
}

static int sol_can_place(card_t moving, card_t target)
{
    return moving.rank + 1 == target.rank && card_red(&moving) != card_red(&target);
}

static void sol_select_tableau(int col, int index)
{
    int i;
    if (index < 0 || index >= sol_tab_n[col] || !sol_tab[col][index].up)
        return;
    for (i = index; i + 1 < sol_tab_n[col]; i++) {
        if (!sol_tab[col][i + 1].up || !sol_can_place(sol_tab[col][i], sol_tab[col][i + 1]))
            return;
    }
    sol_sel_type = 1;
    sol_sel_col = col;
    sol_sel_index = index;
}

static void sol_move_to_tableau(int col)
{
    int count;
    int i;

    if (sol_sel_type == 1)
        count = sol_tab_n[sol_sel_col] - sol_sel_index;
    else
        count = 1;

    if (sol_tab_n[col] > 0) {
        card_t first;
        if (sol_sel_type == 1)
            first = sol_tab[sol_sel_col][sol_sel_index];
        else
            first = sol_waste[sol_waste_n - 1];
        if (!sol_can_place(first, sol_tab[col][sol_tab_n[col] - 1]))
            return;
    } else {
        card_t first;
        if (sol_sel_type == 1)
            first = sol_tab[sol_sel_col][sol_sel_index];
        else
            first = sol_waste[sol_waste_n - 1];
        if (first.rank != 13)
            return;
    }

    for (i = 0; i < count; i++) {
        if (sol_tab_n[col] >= 20)
            return;
        if (sol_sel_type == 1)
            sol_tab[col][sol_tab_n[col]++] = sol_tab[sol_sel_col][sol_sel_index + i];
        else
            sol_tab[col][sol_tab_n[col]++] = sol_waste[sol_waste_n - 1];
    }

    if (sol_sel_type == 1) {
        sol_tab_n[sol_sel_col] -= count;
        if (sol_tab_n[sol_sel_col] > 0)
            sol_tab[sol_sel_col][sol_tab_n[sol_sel_col] - 1].up = 1;
    } else {
        sol_waste_n--;
    }
    sol_sel_type = 0;
}

static void sol_move_to_foundation(int f)
{
    card_t c;
    int i;

    if (sol_sel_type == 1) {
        if (sol_sel_index != sol_tab_n[sol_sel_col] - 1)
            return;
        c = sol_tab[sol_sel_col][sol_sel_index];
    } else {
        c = sol_waste[sol_waste_n - 1];
    }

    if (sol_found_n[f] == 0) {
        if (c.rank != 1)
            return;
    } else {
        card_t top = sol_found[f][sol_found_n[f] - 1];
        if (c.suit != top.suit || c.rank != top.rank + 1)
            return;
    }

    sol_found[f][sol_found_n[f]++] = c;
    if (sol_sel_type == 1) {
        sol_tab_n[sol_sel_col]--;
        if (sol_tab_n[sol_sel_col] > 0)
            sol_tab[sol_sel_col][sol_tab_n[sol_sel_col] - 1].up = 1;
    } else {
        sol_waste_n--;
    }
    sol_sel_type = 0;

    for (i = 0; i < 4; i++)
        if (sol_found_n[i] != 13)
            return;
    /* win: all foundations complete; show by leaving state */
}

static void sol_new_deal(void)
{
    solitaire_init();
}

static int sol_tableau_index(int col, int my, int y)
{
    int index;
    int i;

    for (i = sol_tab_n[col] - 1; i >= 0; i--) {
        int cy = y + 108 + i * SOL_OVERLAP;
        if (my >= cy)
            return i;
    }
    index = 0;
    (void)index;
    return -1;
}

void solitaire_draw(int x, int y, int w, int h)
{
    int i;
    int j;

    if (zh_language)
        zh_text(x + 4, y + 4, "纸牌 点击选牌，再点目标", C_MUTED, 1);
    else
        gfx_text(x + 4, y + 4, "Klondike  click card, then destination", C_MUTED, 1);
    gfx_fill(x + w - 76, y + 2, 72, 20, C_TITLE);
    gfx_text(x + w - 62, y + 8, "New", C_WHITE, 1);

    draw_card(x + 4, y + 32, SOL_CARD_W, SOL_CARD_H,
              sol_stock_n ? &sol_stock[sol_stock_n - 1] : &(card_t){0, 0, 0},
              0);
    if (sol_stock_n == 0) {
        gfx_fill(x + 4, y + 32, SOL_CARD_W, SOL_CARD_H, C_DARK);
        gfx_rect(x + 4, y + 32, SOL_CARD_W, SOL_CARD_H, C_MUTED);
        if (zh_language)
            zh_text(x + 14, y + 58, "空", C_MUTED, 1);
        else
            gfx_text(x + 14, y + 58, "empty", C_MUTED, 1);
    }
    if (sol_waste_n)
        draw_card(x + 60, y + 32, SOL_CARD_W, SOL_CARD_H,
                  &sol_waste[sol_waste_n - 1],
                  sol_sel_type == 2);

    for (i = 0; i < 4; i++) {
        int fx = x + 248 + i * 56;
        gfx_rect(fx, y + 32, SOL_CARD_W, SOL_CARD_H, C_MUTED);
        if (sol_found_n[i])
            draw_card(fx, y + 32, SOL_CARD_W, SOL_CARD_H,
                      &sol_found[i][sol_found_n[i] - 1], 0);
    }

    for (i = 0; i < SOL_COLS; i++) {
        int cx = x + 4 + i * 72;
        for (j = 0; j < sol_tab_n[i]; j++) {
            int cy = y + 108 + j * SOL_OVERLAP;
            int selected = sol_sel_type == 1 && sol_sel_col == i && j >= sol_sel_index;
            draw_card(cx, cy, SOL_CARD_W, SOL_CARD_H, &sol_tab[i][j], selected);
        }
        if (sol_tab_n[i] == 0) {
            gfx_rect(cx, y + 108, SOL_CARD_W, SOL_CARD_H, C_MUTED);
        }
    }
}

void solitaire_click(int mx, int my, int x, int y, int w, int h)
{
    int i;

    if (mx >= x + w - 76 && mx < x + w - 4 && my >= y + 2 && my < y + 22) {
        sol_new_deal();
        return;
    }

    if (mx >= x + 4 && mx < x + 4 + SOL_CARD_W &&
        my >= y + 32 && my < y + 32 + SOL_CARD_H) {
        if (sol_stock_n > 0) {
            if (sol_waste_n < 24) {
                card_t c = sol_stock[--sol_stock_n];
                c.up = 1;
                sol_waste[sol_waste_n++] = c;
            }
        } else if (sol_waste_n > 0) {
            while (sol_waste_n > 0) {
                card_t c = sol_waste[--sol_waste_n];
                c.up = 0;
                sol_stock[sol_stock_n++] = c;
            }
        }
        sol_sel_type = 0;
        return;
    }

    if (mx >= x + 60 && mx < x + 60 + SOL_CARD_W &&
        my >= y + 32 && my < y + 32 + SOL_CARD_H) {
        if (sol_waste_n > 0) {
            sol_sel_type = 2;
            sol_sel_index = sol_waste_n - 1;
        }
        return;
    }

    for (i = 0; i < 4; i++) {
        int fx = x + 248 + i * 56;
        if (mx >= fx && mx < fx + SOL_CARD_W && my >= y + 32 && my < y + 32 + SOL_CARD_H) {
            if (sol_sel_type)
                sol_move_to_foundation(i);
            return;
        }
    }

    for (i = 0; i < SOL_COLS; i++) {
        int cx = x + 4 + i * 72;
        if (mx >= cx && mx < cx + SOL_CARD_W &&
            my >= y + 108 && my < y + 108 + 260) {
            int index = sol_tableau_index(i, my, y);
            if (sol_sel_type) {
                sol_move_to_tableau(i);
                if (!sol_sel_type)
                    return;
            }
            if (index >= 0)
                sol_select_tableau(i, index);
            return;
        }
    }
}

void solitaire_key(int key)
{
    if (key == 'n' || key == 'N')
        sol_new_deal();
}

/* ================= Spider (1 suit) ================= */

#define SPIDER_COLS 10
#define SPIDER_CARD_W 44
#define SPIDER_CARD_H 64
#define SPIDER_OVERLAP 15

static card_t spider_col[SPIDER_COLS][30];
static int spider_n[SPIDER_COLS];
static card_t spider_stock[50];
static int spider_stock_n;
static int spider_complete;
static int spider_sel_col;
static int spider_sel_index;
static int spider_sel;

static void spider_deal_one(int col, card_t c)
{
    if (spider_n[col] < 30)
        spider_col[col][spider_n[col]++] = c;
}

void spider_init(void)
{
    card_t deck[104];
    int i;
    int j;
    int n = 0;

    card_seed = timer_ticks() ^ 0x5A5A5A5A;
    for (i = 0; i < 8; i++)
        for (j = 1; j <= 13; j++) {
            deck[n].rank = (uint8_t)j;
            deck[n].suit = 0;
            deck[n].up = 0;
            n++;
        }
    sol_shuffle(deck, 104);

    memset(spider_n, 0, sizeof(spider_n));
    spider_stock_n = 0;
    spider_complete = 0;
    spider_sel = 0;

    n = 0;
    for (i = 0; i < SPIDER_COLS; i++) {
        int count = (i < 4) ? 6 : 5;
        for (j = 0; j < count; j++)
            spider_deal_one(i, deck[n++]);
        spider_col[i][spider_n[i] - 1].up = 1;
    }
    while (n < 104)
        spider_stock[spider_stock_n++] = deck[n++];
}

static int spider_top(int col)
{
    return spider_n[col] - 1;
}

static int spider_can_stack(card_t lower, card_t upper)
{
    return lower.rank + 1 == upper.rank;
}

static void spider_select(int col, int index)
{
    int i;
    if (index < 0 || index >= spider_n[col] || !spider_col[col][index].up)
        return;
    for (i = index; i + 1 < spider_n[col]; i++) {
        if (!spider_col[col][i + 1].up ||
            !spider_can_stack(spider_col[col][i], spider_col[col][i + 1]))
            return;
    }
    spider_sel = 1;
    spider_sel_col = col;
    spider_sel_index = index;
}

static void spider_check_complete(int col)
{
    int start;
    int i;

    if (spider_n[col] < 13)
        return;
    start = spider_n[col] - 13;
    if (spider_col[col][start].rank != 13)
        return;
    for (i = 0; i < 13; i++) {
        if (!spider_col[col][start + i].up ||
            spider_col[col][start + i].rank != 13 - i)
            return;
    }
    spider_n[col] = start;
    spider_complete++;
    if (spider_n[col] > 0)
        spider_col[col][spider_n[col] - 1].up = 1;
}

static void spider_move(int col)
{
    card_t first;
    int count;
    int i;

    if (!spider_sel)
        return;
    count = spider_n[spider_sel_col] - spider_sel_index;
    first = spider_col[spider_sel_col][spider_sel_index];

    if (spider_n[col] > 0) {
        if (!spider_can_stack(first, spider_col[col][spider_top(col)]))
            return;
    }

    for (i = 0; i < count; i++)
        spider_deal_one(col, spider_col[spider_sel_col][spider_sel_index + i]);

    spider_n[spider_sel_col] -= count;
    if (spider_n[spider_sel_col] > 0)
        spider_col[spider_sel_col][spider_n[spider_sel_col] - 1].up = 1;

    spider_sel = 0;
    spider_check_complete(col);
}

static void spider_deal_stock(void)
{
    int i;
    if (spider_stock_n < 10)
        return;
    for (i = 0; i < SPIDER_COLS; i++) {
        card_t c = spider_stock[--spider_stock_n];
        c.up = 1;
        spider_deal_one(i, c);
        spider_check_complete(i);
    }
}

void spider_draw(int x, int y, int w, int h)
{
    int i;
    int j;

    if (zh_language)
        zh_text(x + 4, y + 4, "蜘蛛 单花色 点击选牌，再点列", C_MUTED, 1);
    else
        gfx_text(x + 4, y + 4, "Spider 1-suit  click card, then column", C_MUTED, 1);
    gfx_fill(x + w - 76, y + 2, 72, 20, C_TITLE);
    gfx_text(x + w - 62, y + 8, "New", C_WHITE, 1);

    gfx_fill(x + 4, y + 30, 56, 46, C_DARK);
    gfx_rect(x + 4, y + 30, 56, 46, C_MUTED);
    gfx_text(x + 12, y + 44, "Stock", C_WHITE, 1);
    if (spider_stock_n > 0) {
        gfx_fill(x + 44, y + 36, 8, 8, C_GOLD);
    }
    gfx_text(x + 68, y + 44, "deals left:", C_MUTED, 1);
    {
        char num[4];
        int n = spider_stock_n / 10;
        if (n == 0) num[0] = '0', num[1] = 0;
        else {
            int p = 0;
            while (n > 0) { num[p++] = (char)('0' + n % 10); n /= 10; }
            num[p] = 0;
            for (i = 0; i < p / 2; i++) { char t = num[i]; num[i] = num[p-1-i]; num[p-1-i] = t; }
        }
        gfx_text(x + 140, y + 44, num, C_TEXT, 1);
    }

    for (i = 0; i < SPIDER_COLS; i++) {
        int cx = x + 4 + i * 52;
        for (j = 0; j < spider_n[i]; j++) {
            int cy = y + 80 + j * SPIDER_OVERLAP;
            int selected = spider_sel && spider_sel_col == i && j >= spider_sel_index;
            draw_card(cx, cy, SPIDER_CARD_W, SPIDER_CARD_H, &spider_col[i][j], selected);
        }
        if (spider_n[i] == 0)
            gfx_rect(cx, y + 80, SPIDER_CARD_W, SPIDER_CARD_H, C_MUTED);
    }

    gfx_text(x + 4, y + h - 14, "Completed runs:", C_MUTED, 1);
    {
        char num[2];
        num[0] = (char)('0' + spider_complete);
        num[1] = 0;
        gfx_text(x + 140, y + h - 14, num, C_GREEN, 1);
    }
}

void spider_click(int mx, int my, int x, int y, int w, int h)
{
    int i;

    if (mx >= x + w - 76 && mx < x + w - 4 && my >= y + 2 && my < y + 22) {
        spider_init();
        return;
    }

    if (mx >= x + 4 && mx < x + 60 && my >= y + 30 && my < y + 76) {
        spider_deal_stock();
        return;
    }

    for (i = 0; i < SPIDER_COLS; i++) {
        int cx = x + 4 + i * 52;
        if (mx >= cx && mx < cx + SPIDER_CARD_W &&
            my >= y + 80 && my < y + 80 + 280) {
            int index = -1;
            int j;
            for (j = spider_n[i] - 1; j >= 0; j--) {
                int cy = y + 80 + j * SPIDER_OVERLAP;
                if (my >= cy) {
                    index = j;
                    break;
                }
            }
            if (spider_sel) {
                spider_move(i);
                if (!spider_sel)
                    return;
            }
            if (index >= 0)
                spider_select(i, index);
            return;
        }
    }
}

void spider_key(int key)
{
    if (key == 'n' || key == 'N')
        spider_init();
    else if (key == 'd' || key == 'D')
        spider_deal_stock();
}

