#include "irq.h"
#include "idt.h"
#include "isr.h"

// PIC constants
#define PIC1 0x20
#define PIC1_OFFSET 0x20
#define PIC1_DATA (PIC1 + 1)

#define PIC2 0xA0
#define PIC2_OFFSET 0x28
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20
#define PIC_MODE_8086 0x01
#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10

#define PIC_WAIT() do {         \
        asm ("jmp 1f\n\t"       \
                "1:\n\t"        \
                "    jmp 2f\n\t"\
                "2:");          \
    } while (0)

static void (*handlers[32])(struct Registers *regs) = { 0 };

static void stub(struct Registers *regs) {
    if (regs->int_no <= 47 && regs->int_no >= 32) {
        if (handlers[regs->int_no - 32]) {
            handlers[regs->int_no - 32](regs);
        }
    }

    // send EOI
    if (regs->int_no >= 0x40) {
        outportb(PIC2, PIC_EOI);
    }

    outportb(PIC1, PIC_EOI);
}

static void irq_remap() {
    u8 mask1 = inportb(PIC1_DATA), mask2 = inportb(PIC2_DATA);
    outportb(PIC1, ICW1_INIT | ICW1_ICW4);
    outportb(PIC2, ICW1_INIT | ICW1_ICW4);
    outportb(PIC1_DATA, PIC1_OFFSET);
    outportb(PIC2_DATA, PIC2_OFFSET);
    outportb(PIC1_DATA, 0x04); // PIC2 at IRQ2
    outportb(PIC2_DATA, 0x02); // Cascade indentity
    outportb(PIC1_DATA, PIC_MODE_8086);
    outportb(PIC1_DATA, PIC_MODE_8086);
    outportb(PIC1_DATA, mask1);
    outportb(PIC2_DATA, mask2);
}

static void irq_set_mask(size_t i) {
    u16 port = i < 8 ? PIC1_DATA : PIC2_DATA;
    u8 value = inportb(port) | (1 << i);
    outportb(port, value);
}

static void irq_clear_mask(size_t i) {
    u16 port = i < 8 ? PIC1_DATA : PIC2_DATA;
    u8 value = inportb(port) & ~(1 << i);
    outportb(port, value);
}

void irq_install(size_t i, void (*handler)(struct Registers *)) {
    CLI();
    handlers[i] = handler;
    irq_clear_mask(i);
    STI();
}

void irq_init() {
    irq_remap();

    for (size_t i = 0; i < 16; i++) {
        isr_install(32 + i, stub);
    }
}
