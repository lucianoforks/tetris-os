#ifndef UTIL_H
#define UTIL_H

// fixed width integer types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef u32 size_t;
typedef u32 uintptr_t;
typedef float f32;
typedef double f64;

typedef u8 bool;
#define true (1)
#define false (0)

#define NULL (0)

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define __MIN_IMPL(_x, _y, _xn, _yn) __extension__({\
        __typeof__(_x) _xn = (_x);\
        __typeof__(_y) _yn = (_y);\
        (_xn < _yn ? _xn : _yn);\
        })
#define MIN(_x, _y) __MIN_IMPL(_x, _y, CONCAT(__x, __COUNTER__), CONCAT(__y, __COUNTER__))

#define __MAX_IMPL(_x, _y, _xn, _yn) __extension__({\
        __typeof__(_x) _xn = (_x);\
        __typeof__(_y) _yn = (_y);\
        (_xn > _yn ? _xn : _yn);\
        })
#define MAX(_x, _y) __MAX_IMPL(_x, _y, CONCAT(__x, __COUNTER__), CONCAT(__y, __COUNTER__))

#define CLAMP(_x, _mi, _ma) (MAX(_mi, MIN(_x, _ma)))

// returns the highest set bit of x
// i.e. if x == 0xF, HIBIT(x) == 3 (4th index)
// WARNING: currently only works for up to 32-bit types
#define HIBIT(_x) (31 - __builtin_clz((_x)))

// returns the lowest set bit of x
#define LOBIT(_x)\
    __extension__({ __typeof__(_x) __x = (_x); HIBIT(__x & -__x); })

// returns _v with _n-th bit = _x
#define BIT_SET(_v, _n, _x) __extension__({\
        __typeof__(_v) __v = (_v);\
        (__v ^ ((-(_x) ^ __v) & (1 << (_n))));\
        })

#define PACKED __attribute__((packed))

#ifndef asm
#define asm __asm__ volatile
#endif

#define CLI() asm ("cli")
#define STI() asm ("sti")

static inline u16 inports(u16 port) {
    u16 r;
    asm("inw %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

static inline void outports(u16 port, u16 data) {
    asm("outw %1, %0" : : "dN" (port), "a" (data));
}

static inline u8 inportb(u16 port) {
    u8 r;
    asm("inb %1, %0" : "=a" (r) : "dN" (port));
    return r;
}

static inline void outportb(u16 port, u8 data) {
    asm("outb %1, %0" : : "dN" (port), "a" (data));
}

static inline size_t strlen(const char *str) {
    size_t l = 0;
    while (*str++ != 0) {
        l++;
    }
    return l;
}

static inline char *itoa(i32 x, char *s, size_t sz) {
    // TODO: holy god this is bad code we need some error handling here
    if (sz < 20) {
        extern void panic(const char *);
        panic("ITOA BUFFER TOO SMALL");
    }

    u32 tmp;
    i32 i, j;

    tmp = x;
    i = 0;

    do {
        tmp = x % 10;
        s[i++] = (tmp < 10) ? (tmp + '0') : (tmp + 'a' - 10);
    } while (x /= 10);
    s[i--] = 0;

    for (j = 0; j < i; j++, i--) {
        tmp = s[j];
        s[j] = s[i];
        s[i] = tmp;
    }

    return s;
}

static inline void memset(void *dst, u8 value, size_t n) {
    u8 *d = dst;

    while (n-- > 0) {
        *d++ = value;
    }
}

static inline void *memcpy(void *dst, const void *src, size_t n) {
    u8 *d = dst;
    const u8 *s = src;

    while (n-- > 0) {
        *d++ = *s++;
    }

    return d;
}

static inline void *memmove(void *dst, const void *src, size_t n) {
    // OK since we know that memcpy copies forwards
    if (dst < src) {
        return memcpy(dst, src, n);
    }

    u8 *d = dst;
    const u8 *s = src;

    for (size_t i = n; i > 0; i--) {
        d[i - 1] = s[i - 1];
    }

    return dst;
}

// SEE: https://opensource.apple.com/source/Libc/Libc-1158.30.7/string/strlcat.c.auto.html
static inline size_t strlcat(char *dst, const char *src, size_t size) {
    const size_t sl = strlen(src),
          dl = strlen(dst);

    if (dl == size) {
        return size + sl;
    }

    if (sl < (size - dl)) {
        memcpy(dst + dl, src, sl + 1);
    } else {
        memcpy(dst + dl, src, size - dl - 1);
        dst[size - 1] = '\0';
    }

    return sl + dl;
}

static inline size_t strlcpy(char *dst, const char *src, size_t n) {
    // copy as many bytes as can fit
    char *d = dst;
    const char *s = src;
    size_t size = n;

    while (--n > 0) {
        if ((*d++ = *s++) == 0) {
            break;
        }
    }

    // if we ran out of space, null terminate
    if (n == 0) {
        if (size != 0) {
            *d = 0;
        }

        // traverse the rest of s
        while (*s++);
    }

    return s - src - 1;
}

#endif
