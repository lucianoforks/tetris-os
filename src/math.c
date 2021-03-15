#include "math.h"

f64 fabs(f64 x) {
    return x < 0.0 ? -x : x;
}

f64 fmod(f64 x, f64 m) {
    f64 result;
    asm("1: fprem\n\t"
        "fnstsw %%ax\n\t"
        "sahf\n\t"
        "jp 1b"
        : "=t"(result) : "0"(x), "u"(m) : "ax", "cc");
    return result;
}

f64 sin(f64 x) {
    f64 result;
    asm("fsin" : "=t"(result) : "0"(x));
    return result;
}

f64 cos(f64 x) {
    return sin(x + PI / 2.0);
}

// black magic
f64 pow(f64 x, f64 y) {
    f64 out;
    asm(
            "fyl2x;"
            "fld %%st;"
            "frndint;"
            "fsub %%st,%%st(1);"
            "fxch;"
            "fchs;"
            "f2xm1;"
            "fld1;"
            "faddp;"
            "fxch;"
            "fld1;"
            "fscale;"
            "fstp %%st(1);"
            "fmulp;" : "=t"(out) : "0"(x),"u"(y) : "st(1)" );
    return out;
}
