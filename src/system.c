#include "system.h"
#include "screen.h"
#include "font.h"

static u32 rseed = 1;

void seed(u32 s) {
    rseed = s;
}

u32 rand() {
    static u32 x = 123456789;
    static u32 y = 362436069;
    static u32 z = 521288629;
    static u32 w = 88675123;

    x *= 23786259 - rseed;

    u32 t;

    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void panic(const char *err) {
    screen_clear(COLOR(7, 0, 0));

    if (err != NULL) {
        font_str(err, (SCREEN_WIDTH - font_width(err)) / 2, SCREEN_HEIGHT / 2 - 4, COLOR(7, 7, 3));
    }

    screen_swap();
    for (;;) {}
}
