#ifndef FONT_H
#define FONT_H

#include "util.h"
#include "screen.h"

#define font_width(_s) (strlen((_s)) * 8)
#define font_height() (8)
#define font_str_doubled(_s, _x, _y, _c) do {\
        const char *__s = (_s);\
        __typeof__(_x) __x = (_x);\
        __typeof__(_y) __y = (_y);\
        __typeof__(_c) __c = (_c);\
        font_str(__s, __x + 1, __y + 1, COLOR_ADD(__c, -2));\
        font_str(__s, __x, __y, __c);\
    } while (0);

void font_char(char c, size_t x, size_t y, u8 color);
void font_str(const char *s, size_t x, size_t y, u8 color);

#endif
