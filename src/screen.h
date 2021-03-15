#ifndef SCREEN_H
#define SCREEN_H

#include "util.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

#define COLOR(_r, _g, _b)((u8)( \
    (((_r) & 0x7) << 5) |       \
    (((_g) & 0x7) << 2) |       \
    (((_b) & 0x3) << 0)))

#define COLOR_R(_index) (((_index) >> 5) & 0x7)
#define COLOR_G(_index) (((_index) >> 2) & 0x7)
#define COLOR_B(_index) (((_index) >> 0) & 0x3)

#define COLOR_ADD(_index, _d) __extension__({   \
        __typeof__(_index) _c = (_index);       \
        __typeof__(_d) __d = (_d);              \
        COLOR(                                  \
            CLAMP(COLOR_R(_c) + __d, 0, 7),     \
            CLAMP(COLOR_G(_c) + __d, 0, 7),     \
            CLAMP(COLOR_B(_c) + __d, 0, 3)      \
        );})

extern u8 _sbuffers[2][SCREEN_SIZE];
extern u8 _sback;

#define screen_buffer() (_sbuffers[_sback])

#define screen_set(_p, _x, _y)\
    (_sbuffers[_sback][((_y) * SCREEN_WIDTH + (_x))]=(_p))

#define screen_offset(_x, _y) (screen_buffer()[(_y) * SCREEN_WIDTH + (_x)])

#define screen_fill(_c, _x, _y, _w, _h) do {\
        __typeof__(_x) __x = (_x);\
        __typeof__(_y) __y = (_y);\
        __typeof__(_w) __w = (_w);\
        __typeof__(_y) __ymax = __y + (_h);\
        __typeof__(_c) __c = (_c);\
        for (; __y < __ymax; __y++) {\
            memset(&screen_buffer()[__y * SCREEN_WIDTH + __x], __c, __w);\
        }\
    } while (0)

void screen_swap();
void screen_clear(u8 color);
void screen_init();

#endif
