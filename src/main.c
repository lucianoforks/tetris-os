#include "util.h"
#include "screen.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "timer.h"
#include "font.h"
#include "system.h"
#include "keyboard.h"
#include "speaker.h"
#include "fpu.h"
#include "sound.h"
#include "music.h"

#define FPS 30
#define LEVELS 30

#define TILE_SIZE 10

#define LOGO_HEIGHT 5
static const char *LOGO[LOGO_HEIGHT] = {
    "AAA BBB CCC DD  EEE FFF",
    " A  B    C  D D  E  F  ",
    " A  BBB  C  DD   E  FFF",
    " A  B    C  D D  E    F",
    " A  BBB  C  D D EEE FFF",
};

#define NUM_TILES (BORDER + 1)
enum Tile {
    NONE = 0,
    GREEN,
    ORANGE,
    YELLOW,
    PURPLE,
    PINK,
    BLUE,
    CYAN,
    RED,
    BORDER
};

#define TILE_MASK 0x0F
#define TILE_FLAG_FLASH 0x10
#define TILE_FLAG_DESTROY 0x20

static const u8 TILE_COLORS[NUM_TILES] = {
    [NONE] =    COLOR(7, 0, 3),
    [GREEN] =   COLOR(0, 5, 0),
    [ORANGE] =  COLOR(5, 3, 0),
    [YELLOW] =  COLOR(5, 5, 0),
    [PURPLE] =  COLOR(3, 0, 3),
    [PINK] =    COLOR(5, 0, 5),
    [BLUE] =    COLOR(0, 0, 2),
    [CYAN] =    COLOR(0, 3, 3),
    [RED] =     COLOR(5, 0, 0),
    [BORDER] =  COLOR(2, 2, 1),
};

u8 TILE_SPRITES[NUM_TILES][TILE_SIZE * TILE_SIZE] = { 0 };

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BOARD_SIZE (BOARD_WIDTH * BOARD_HEIGHT)

#define BOARD_WIDTH_PX (BOARD_WIDTH * TILE_SIZE)
#define BOARD_HEIGHT_PX (BOARD_HEIGHT * TILE_SIZE)
#define BOARD_X ((SCREEN_WIDTH - BOARD_WIDTH_PX) / 2)
#define BOARD_Y 0

#define IN_BOARD(_x, _y) ((_x) >= 0 && (_y) >= 0 && (_x) < BOARD_WIDTH && (_y) < BOARD_HEIGHT)

// max size of 4x4 to account for all rotations
#define TTM_SIZE 4

#define TTM_BLOCK(_t, _i, _j) (((_t) & (1 << (((_j) * 4) + (_i)))) != 0)

#define TTM_OFFSET_X(_t)\
    MIN(_t & 0x000F ? LOBIT((_t >> 0) & 0xF) : 3,\
        MIN(_t & 0x00F0 ? LOBIT((_t >> 4) & 0xF) : 3,\
            MIN(_t & 0x0F00 ? LOBIT((_t >> 8) & 0xF) : 3,\
                _t & 0xF000 ? LOBIT((_t >> 12) & 0xF) : 3)))

#define TTM_WIDTH(_t)\
    1 + MAX(HIBIT((_t >> 0) & 0xF),\
            MAX(HIBIT((_t >> 4) & 0xF),\
                MAX(HIBIT((_t >> 8) & 0xF), HIBIT((_t >> 12) & 0xF)))) -\
                    TTM_OFFSET_X(_t)

#define TTM_HEIGHT(_t) ((HIBIT(_t) / 4) - (LOBIT(_t) / 4) + 1)
#define TTM_OFFSET_Y(_t) (LOBIT(_t) / 4)

#define TTM_FOREACH(_xname, _yname, _xxname, _yyname, _xbase, _ybase)\
    for (i32 _yname = 0, _yyname = (_ybase); _yname < TTM_SIZE; _yname++,_yyname++)\
        for (i32 _xname = 0, _xxname = (_xbase); _xname < TTM_SIZE; _xname++,_xxname++)\

struct Tetromino {
    enum Tile color;
    u16 rotations[4];
};

#define NUM_TETROMINOS 7
static const struct Tetromino TETROMINOS[NUM_TETROMINOS] = {
    {
        // line
        .color = CYAN,
        .rotations = {
            0x00F0,
            0x2222,
            0x0F00,
            0x4444
        }
    }, {
        // left L
        .color = BLUE,
        .rotations = {
            0x8E00,
            0x6440,
            0x0E20,
            0x44C0
        }
    }, {
        // right L
        .color = ORANGE,
        .rotations = {
            0x2E00,
            0x4460,
            0x0E80,
            0xC440
        }
    }, {
        // cube
        .color = YELLOW,
        .rotations = {
            0xCC00,
            0xCC00,
            0xCC00,
            0xCC00
        }
    }, {
        // right skew
        .color = GREEN,
        .rotations = {
            0x6C00,
            0x4620,
            0x06C0,
            0x8C40
        }
    }, {
        // left skew
        .color = RED,
        .rotations = {
            0xC600,
            0x2640,
            0x0C60,
            0x4C80
        }
    }, {
        // T
        .color = PURPLE,
        .rotations = {
            0x4E00,
            0x4640,
            0x0E40,
            0x4C40
        }
    }
};

#define NUM_LEVELS 30

// from listfist.com/list-of-tetris-levels-by-speed-nes-ntsc-vs-pal
static u8 FRAMES_PER_STEP[NUM_LEVELS] = {
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 1
};

static u32 LINE_MULTIPLIERS[4] = {
    40, 100, 300, 1200
};

#define NUM_CONTROLS 7
struct Control {
    bool down;
    bool last;
    bool pressed;
    u32 pressed_frames;
};

static struct {
    u8 board[BOARD_HEIGHT][BOARD_WIDTH];

    u32 frames, steps, frames_since_step;
    u32 score, lines, level;
    i32 lines_left;
    bool menu, pause, stopped, destroy, game_over, music;

    const struct Tetromino *next;

    struct {
        const struct Tetromino *ttm;
        u8 r;
        i32 x, y;
        bool done;
    } curr;

    union {
        struct {
            struct Control rotate_left;
            struct Control rotate_right;
            struct Control rotate;
            struct Control left;
            struct Control right;
            struct Control down;
            struct Control fast_down;
        };
        struct Control raw[NUM_CONTROLS];
    } controls;
} state;

static void done() {
    // flash tetromino which was just placed
    TTM_FOREACH(x, y, xx, yy, state.curr.x, state.curr.y) {
        if (IN_BOARD(xx, yy) &&
            TTM_BLOCK(state.curr.ttm->rotations[state.curr.r], x, y)) {
            state.board[yy][xx] |= TILE_FLAG_FLASH;
        }
    }

    // check for lines
    u32 lines = 0;

    for (size_t y = 0; y < BOARD_HEIGHT; y++) {
        bool line = true;

        for (size_t x = 0; x < BOARD_WIDTH; x++) {
            if ((state.board[y][x] & TILE_MASK) == NONE) {
                line = false;
                break;
            }
        }

        if (line) {
            lines++;

            for (size_t x = 0; x < BOARD_WIDTH; x++) {
                state.board[y][x] |= TILE_FLAG_FLASH | TILE_FLAG_DESTROY;
            }

            state.destroy = true;
        }
    }

    if (lines > 0) {
        state.lines += lines;
        state.score += LINE_MULTIPLIERS[lines - 1] * (state.level + 1);

        // check for leveling up
        if (state.level != NUM_LEVELS - 1) {
            state.lines_left -= lines;
            if (state.lines_left <= 0) {
                state.level++;
                state.lines_left = 10;
            }
        }
    }

    // new tetromino is spawned in update() after destroy
    state.curr.done = true;
}

static bool try_modify(
    const struct Tetromino *ttm, u16 tc, i32 xc, i32 yc, u16 tn, i32 xn, i32 yn) {
    u8 board[BOARD_HEIGHT][BOARD_WIDTH];
    memcpy(&board, &state.board, sizeof(board));

    // clear current tiles
    if (tc != 0) {
        TTM_FOREACH(x, y, xx, yy, xc, yc) {
            if (IN_BOARD(xx, yy) && TTM_BLOCK(tc, x, y)) {
                state.board[yy][xx] = NONE;
            }
        }
    }

    TTM_FOREACH(x, y, xx, yy, xn, yn) {
        if (yy < 0) {
            if (TTM_BLOCK(tn, x, y) &&
                (xx < 0 || xx >= BOARD_WIDTH)) {
                goto fail;
            }

            continue;
        } else if (!TTM_BLOCK(tn, x, y)) {
            continue;
        } else if (!IN_BOARD(xx, yy) || state.board[yy][xx] != NONE ||
            xx < 0 || xx > (BOARD_WIDTH - 1)) {
            goto fail;
        }

        state.board[yy][xx] = ttm->color;
    }

    return true;
fail:
    memcpy(&state.board, &board, sizeof(board));
    return false;
}

static bool spawn() {
    if (state.next == NULL) {
        state.next = &TETROMINOS[rand() % NUM_TETROMINOS];
    }

    state.curr.ttm = state.next;
    state.curr.r = 0;
    state.curr.x = (BOARD_WIDTH / 2) - 2;
    state.curr.y = -TTM_OFFSET_Y(state.curr.ttm->rotations[state.curr.r]) - 1;
    state.curr.done = false;

    if (!try_modify(
            state.curr.ttm,
            0, 0, 0,
            state.curr.ttm->rotations[state.curr.r],
            state.curr.x, state.curr.y)) {
        return false;
    }

    state.next = &TETROMINOS[rand() % NUM_TETROMINOS];
    return true;
}

static bool move(i32 dx, i32 dy) {
    if (try_modify(
            state.curr.ttm,
            state.curr.ttm->rotations[state.curr.r],
            state.curr.x, state.curr.y,
            state.curr.ttm->rotations[state.curr.r],
            state.curr.x + dx, state.curr.y + dy)) {
        state.curr.x += dx;
        state.curr.y += dy;
        return true;
    }

    return false;
}

static bool rotate(bool right) {
    u8 r = (state.curr.r + (right ? 1 : -1) + 4) % 4;

    if (try_modify(
            state.curr.ttm,
            state.curr.ttm->rotations[state.curr.r],
            state.curr.x, state.curr.y,
            state.curr.ttm->rotations[r],
            state.curr.x, state.curr.y)) {
        state.curr.r = r;
        return true;
    }

    return false;
}

static void generate_sprites() {
    for (enum Tile t = 0; t < NUM_TILES; t++) {
        if (t == NONE) {
            continue;
        }

        u8 color = TILE_COLORS[t];
        u8 *pixels = TILE_SPRITES[t];

        for (size_t y = 0; y < TILE_SIZE; y++) {
            for (size_t x = 0; x < TILE_SIZE; x++) {
                u8 c = color;

                if (y == 0 || x == 0) {
                    c = COLOR_ADD(color, 1);
                } else if (y == TILE_SIZE - 1 || x == TILE_SIZE - 1) {
                    c = COLOR_ADD(color, -1);
                }

                pixels[y * TILE_SIZE + x] = c;
            }
        }
    }
}

static void render_tile(enum Tile tile, size_t x, size_t y) {
    u8 *pixels = TILE_SPRITES[tile];
    for (size_t j = 0; j < TILE_SIZE; j++) {
        memcpy(&screen_offset(x, y + j), pixels + (j * TILE_SIZE), TILE_SIZE);
    }
}

static void render_border() {
    for (size_t y = 0; y < (SCREEN_HEIGHT / TILE_SIZE); y++) {
        size_t yy = BOARD_Y + (y * TILE_SIZE);

        render_tile(
            BORDER,
            BOARD_X - TILE_SIZE,
            yy
        );

        render_tile(
            BORDER,
            BOARD_X + (BOARD_WIDTH * TILE_SIZE),
            yy
        );
    }
}

static void render_board() {
    for (size_t y = 0; y < BOARD_HEIGHT; y++) {
        for (size_t x = 0; x < BOARD_WIDTH; x++) {
            u8 data = state.board[y][x];
            enum Tile tile = data & TILE_MASK;

            size_t xs = BOARD_X + (x * TILE_SIZE),
                   ys = BOARD_Y + (y * TILE_SIZE);

            if (data & TILE_FLAG_FLASH) {
                screen_fill(COLOR(4, 4, 1), xs, ys, TILE_SIZE, TILE_SIZE);
            } else if (tile != NONE) {
                render_tile(tile, xs, ys);
            }
        }
    }
}

static void render_ui() {
#define X_OFFSET_RIGHT (BOARD_X + BOARD_WIDTH_PX + (TILE_SIZE * 2))

#define RENDER_STAT(_title, _value, _color, _x, _y, _w) do {\
        char buf[32];\
        itoa((_value), buf, 32);\
        font_str_doubled((_title), (_x), (_y), COLOR(7, 7, 3));\
        font_str_doubled(buf, (_x) + (_w) - font_width(buf), (_y) + TILE_SIZE, (_color));\
    } while (0);

    size_t w = font_width("SCORE");
    RENDER_STAT("SCORE", state.score, COLOR(5, 5, 0), X_OFFSET_RIGHT, TILE_SIZE * 1, w);
    RENDER_STAT("LINES", state.lines, COLOR(5, 3, 0), X_OFFSET_RIGHT, TILE_SIZE * 4, w);
    RENDER_STAT("LEVEL", state.level, COLOR(5, 0, 0), X_OFFSET_RIGHT, TILE_SIZE * 7, w);

#define X_OFFSET_LEFT (BOARD_X - (TILE_SIZE * 8))
#define Y_OFFSET_LEFT TILE_SIZE

    font_str_doubled("NEXT", X_OFFSET_LEFT, TILE_SIZE, COLOR(7, 7, 3));

    for (size_t j = 0; j < TTM_SIZE; j++) {
        for (size_t i = 0; i < TTM_SIZE; i++) {
            u16 tiles = state.next->rotations[0];

            if (TTM_BLOCK(tiles, i, j)) {
                render_tile(
                    state.next->color,
                    X_OFFSET_LEFT + ((i - TTM_OFFSET_X(tiles)) * TILE_SIZE),
                    Y_OFFSET_LEFT + (TILE_SIZE / 2) + ((j - TTM_OFFSET_Y(tiles) + 1) * TILE_SIZE)
                );
            }
        }
    }
}

static void render_game_over() {
    const size_t w = SCREEN_WIDTH / 3, h = SCREEN_HEIGHT / 3;
    screen_fill(
        COLOR(4, 4, 2),
        (SCREEN_WIDTH - w) / 2,
        (SCREEN_HEIGHT - h) / 2,
        w,
        h
    );

    screen_fill(
        COLOR(2, 2, 1),
        (SCREEN_WIDTH - (w - 8)) / 2,
        (SCREEN_HEIGHT - (h - 8)) / 2,
        w - 8,
        h - 8
    );

    font_str_doubled(
        "GAME OVER",
        (SCREEN_WIDTH - font_width("GAME OVER")) / 2,
        (SCREEN_HEIGHT / 2) - TILE_SIZE,
        (state.frames / 5) % 2 == 0 ?
            COLOR(6, 2, 1) :
            COLOR(7, 4, 2)
    );

    char buf_score[64];
    itoa(state.score, buf_score, 64);

    font_str_doubled(
        "SCORE:",
        (SCREEN_WIDTH - font_width("SCORE:")) / 2,
        (SCREEN_HEIGHT / 2),
        COLOR(6, 6, 0)
    );

    font_str_doubled(
        buf_score,
        (SCREEN_WIDTH - font_width(buf_score)) / 2,
        (SCREEN_HEIGHT / 2) + TILE_SIZE,
        COLOR(7, 7, 3)
    );
}

static void step() {
    bool stopped = !move(0, 1);

    if (stopped && state.stopped) {
        // twice stop = end for this tetromino
        done();
    }

    state.stopped = stopped;
}

void reset(u32 level) {
    // initialize game state
    memset(&state, 0, sizeof(state));
    state.frames_since_step = FRAMES_PER_STEP[0];
    state.level = 0;
    state.lines_left = state.level * 10 + 10;
    spawn();
}

static void update() {
    if (state.game_over) {
        if (keyboard_char('\n')) {
            reset(0);
        }

        return;
    }

    // un-flash flashing tiles, remove destroy tiles
    for (size_t y = 0; y < BOARD_HEIGHT; y++) {
        bool destroy = false;

        for (size_t x = 0; x < BOARD_WIDTH; x++) {
            u8 data = state.board[y][x];

            if (data & TILE_FLAG_DESTROY) {
                state.board[y][x] = NONE;
                destroy = true;
            } else {
                state.board[y][x] &= ~TILE_FLAG_FLASH;
            }
        }

        if (destroy) {
            if (y != 0) {
                memmove(
                    &state.board[1],
                    &state.board[0],
                    sizeof(state.board[0]) * y
                );
            }

            memset(&state.board[0], NONE, sizeof(state.board[0]));
        }
    }

    // spawn a new tetromino if the current one is done
    if (state.curr.done && !spawn()) {
        state.game_over = true;
        return;
    }

    if (state.destroy) {
        state.destroy = false;
        return;
    }

    const bool control_states[NUM_CONTROLS] = {
        keyboard_char('a'),
        keyboard_char('d'),
        keyboard_char('r'),
        keyboard_key(KEY_LEFT),
        keyboard_key(KEY_RIGHT),
        keyboard_key(KEY_DOWN),
        keyboard_char(' ')
    };

    for (size_t i = 0; i < NUM_CONTROLS; i++) {
        struct Control *c = &state.controls.raw[i];
        c->last = c->down;
        c->down = control_states[i];
        c->pressed = !c->last && c->down;

        if (c->pressed) {
            c->pressed_frames = state.frames;
        }
    }

    if (state.controls.rotate_left.pressed) {
        rotate(false);
    } else if (state.controls.rotate_right.pressed ||
        state.controls.rotate.pressed) {
        rotate(true);
    }

    if (state.controls.left.down &&
        (state.frames - state.controls.left.pressed_frames) % 2 == 0) {
        move(-1, 0);
    } else if (state.controls.right.down &&
         (state.frames - state.controls.right.pressed_frames) % 2 == 0) {
        move(1, 0);
    } else if (state.controls.down.down &&
         (state.frames - state.controls.down.pressed_frames) % 2 == 0) {
        if (!move(0, 1)) {
            done();
        }
    } else if (state.controls.fast_down.pressed) {
        while (move(0, 1));
        done();
    }

    if (--state.frames_since_step == 0) {
        step();
        state.steps++;
        state.frames_since_step = FRAMES_PER_STEP[state.level];
    }
}

static void render() {
    screen_clear(COLOR(0, 0, 0));
    render_border();
    render_board();
    render_ui();

    if (state.game_over) {
        render_game_over();
    }
}

void update_menu() {
    if (keyboard_char('\n')) {
        reset(0);
        state.menu = false;
    }
}

void render_menu() {
    screen_clear(COLOR(0, 0, 0));

    // render logo
    size_t logo_width = strlen(LOGO[0]),
           logo_x = (SCREEN_WIDTH - (logo_width * TILE_SIZE)) / 2,
           logo_y = TILE_SIZE * 3;

    for (i32 x = -1; x < (i32) logo_width + 1; x++) {
        render_tile(BORDER, logo_x + (x * TILE_SIZE), logo_y - (TILE_SIZE * 2));
        render_tile(BORDER, logo_x + (x * TILE_SIZE), logo_y + (TILE_SIZE * (1 + LOGO_HEIGHT)));
    }

    for (size_t y = 0; y < LOGO_HEIGHT; y++) {
        for (size_t x = 0; x < logo_width; x++) {
            char c = LOGO[y][x];

            if (c == ' ' || c == '\t' || c == '\n') {
                continue;
            }

            render_tile(
                GREEN + ((((state.frames / 10) + (6 - (c - 'A'))) / 6) % 8),
                logo_x + (x * TILE_SIZE),
                logo_y + (y * TILE_SIZE)
            );
        }
    }

    const char *play = "PRESS ENTER TO PLAY";
    font_str_doubled(
        play,
        (SCREEN_WIDTH - font_width(play)) / 2,
        logo_y + ((LOGO_HEIGHT + 6) * TILE_SIZE),
        (state.frames / 6) % 2 == 0 ?
            COLOR(6, 6, 2) :
            COLOR(7, 7, 3)
    );
}

void _main(u32 magic) {
    idt_init();
    isr_init();
    fpu_init();
    irq_init();
    screen_init();
    timer_init();
    keyboard_init();
    generate_sprites();

    sound_init();

    if (sound_enabled()) {
        music_init();
        state.music = true;
        sound_master(255);
    }

    state.menu = true;


    bool last_music_toggle = false;
    u32 last_frame = 0, last = 0;

    while (true) {
        const u32 now = (u32) timer_get();

        if (sound_enabled() && now != last) {
            music_tick();
            sound_tick();
            last = now;
        }

        if ((now - last_frame) > (TIMER_TPS / FPS)) {
            last_frame = now;

            if (state.menu) {
                update_menu();
                render_menu();
            } else {
                update();
                render();
            }

            if (sound_enabled() && keyboard_char('m')) {
                if (!last_music_toggle) {
                    state.music = !state.music;
                    sound_master(state.music ? 255 : 0);
                }

                last_music_toggle = true;
            } else {
                last_music_toggle = false;
            }

            // controlled in system.c
            const char *notification = get_notification();
            if (notification != NULL) {
                font_str_doubled(notification, 0, 0, COLOR(6, 1, 1));
            }

            screen_swap();
            state.frames++;
        }
    }
}
