#include "idt.h"

struct IDTEntry {
    u16 offset_low;
    u16 selector;
    u8 __ignored;
    u8 type;
    u16 offset_high;
} PACKED;

struct IDTPointer {
    u16 limit;
    uintptr_t base;
} PACKED;

static struct {
    struct IDTEntry entries[256];
    struct IDTPointer pointer;
} idt;

// in start.S
extern void idt_load();

void idt_set(u8 index, void (*base)(struct Registers*), u16 selector, u8 flags) {
    idt.entries[index] = (struct IDTEntry) {
        .offset_low = ((uintptr_t) base) & 0xFFFF,
        .offset_high = (((uintptr_t) base) >> 16) & 0xFFFF,
        .selector = selector,
        .type = flags | 0x60,
        .__ignored = 0
    };
}

void idt_init() {
    idt.pointer.limit = sizeof(idt.entries) - 1;
    idt.pointer.base = (uintptr_t) &idt.entries[0];
    memset(&idt.entries[0], 0, sizeof(idt.entries));
    idt_load((uintptr_t) &idt.pointer);
}
