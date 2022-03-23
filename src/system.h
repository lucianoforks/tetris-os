#ifndef SYSTEM_H
#define SYSTEM_H

#include "util.h"

#define _assert_0() __error_illegal_macro__
#define _assert_1(_e) do { if (!(_e)) panic(NULL); } while (0)
#define _assert_2(_e, _m) do { if (!(_e)) panic((_m)); } while (0)

#define _assert(x, _e, _m, _f, ...) _f

#define assert(...) _assert(,##__VA_ARGS__,\
        _assert_2(__VA_ARGS__),\
        _assert_1(__VA_ARGS__),\
        _assert_0(__VA_ARGS__))

const char *get_notification();
void notify(const char *err);
void panic(const char *err);
u32 rand();
void seed(u32 s);

#endif
