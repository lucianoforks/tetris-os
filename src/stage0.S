.code16
.org 0

.text

.global _start
_start:
    cli

    /* segment setup */
    mov %cs, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    /* place stack pointer in middle of free memory area */
    movw $0x3000, %sp

    /* save drive number to read kernel later */
    mov %dl, drive_num

    sti

    /* should print TETRIS TIME */
    movw $welcome_str, %si
    call print

    /* read drive parameters, INT 13h AH=8h */
    movb $8, %ah
    movb drive_num, %dl
    int $0x13

    movb %dh, num_heads
    andb $0x3f, %cl
    movb %cl, sectors_per_track

    /* read kernel into memory at 0x10000 (segment 0x1000).
     * kernel binary has been placed on the disk directly after the first sector
     * number of sectors to be read is %cx.
     */

    movw $0x40, %cx
    movw $0x1000, segment
    movw $0x0000, offset
    movw $1, sector
    movw $2, chs_sector
sector_loop:
    push %cx

    /* attempt read with "extended read sectors from drive" (AH=42h) */
    movb $0x42, %ah
    movb drive_num, %dl
    movw $disk_packet, %si
    int $0x13
    jnc disk_success

    /* fall back to CHS read/"read sectors from drive" (AH=02h) */
    movb $0x02, %ah
    movb $1, %al
    movb chs_cylinder, %ch
    movb chs_sector, %cl
    movb drive_num, %dl
    movb chs_head, %dh
    movw segment, %es
    movw offset, %bx
    int $0x13
    jc disk_error
disk_success:
    incb sector

    /* increase sector count and check for track limit */
    incb chs_sector

    movb sectors_per_track, %al
    cmp %al, chs_sector
    jna sector_same_head_cylinder

    /* sectors per track exceeded, reset sector to 1, increase heads and check
     * for head limit
     */
    movb $1, chs_sector
    incb chs_head
    mov num_heads, %al
    cmp %al, chs_head
    jna sector_same_head_cylinder

    /* head count exceeded, reset head to 0, increase cylinder */
    movb $0, chs_head
    incb chs_cylinder

sector_same_head_cylinder:
    /* single 512 sector read, increase offset by 0x200 (512) */
    addw $0x200, offset
    jnc sector_same_segment

    /* increment segment, reset offset if on different segment */
    addw $0x1000, segment
    movw $0x0000, offset
sector_same_segment:
    pop %cx
    loop sector_loop

    /* video mode: 320x200 @ 16 colors */
    movb $0x00, %ah
    movb $0x13, %al
    int $0x10

    /* enable A20 line */
    cli

    /* read and save state */
    call enable_a20_wait0
    movb $0xD0, %al
    outb $0x64
    call enable_a20_wait1
    xorw %ax, %ax
    inb $0x60

    /* write new state with A20 bit set (0x2) */
    pushw %ax
    call enable_a20_wait0
    movb $0xD1, %al
    outb $0x64
    call enable_a20_wait0
    popw %ax
    orw $0x2, %ax
    outb $0x60

    /* enable PE flag */
    movl %cr0, %eax
    orl $0x1, %eax
    movl %eax, %cr0

    /* jmp to flush prefetch queue */
    jmp flush
flush:
    lidt idt
    lgdt gdtp

    movw $(gdt_data_segment - gdt_start), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    movl $0x3000, %esp
    ljmp $0x8, $entry32

.code32
entry32:
    /* jump to kernel loaded at 0x10000 */
    movl $0x10000, %eax
    jmpl *%eax

_loop:
    jmp _loop

.code16
enable_a20_wait0:
    xorw %ax, %ax
    inb $0x64
    btw $1, %ax
    jc enable_a20_wait0
    ret

enable_a20_wait1:
    xorw %ax, %ax
    inb $0x64
    btw $0, %ax
    jnc enable_a20_wait1
    ret

disk_error:
    movw $disk_error_str, %si
    call print

/* prints string in %ds:si */
print:
    xorb %bh, %bh
    movb $0x0E, %ah

    lodsb

    /* NULL check */
    cmpb $0, %al
    je 1f

    /* print %al to screen */
    int $0x10
    jmp print

1:  ret

welcome_str:
    .asciz "TETRIS TIME\r\n"
disk_error_str:
    .asciz "DISK ERROR\r\n"

chs_sector:
    .byte 0x01
chs_cylinder:
    .byte 0x00
chs_head:
    .byte 0x00

/* SAVED DRIVE NUMBER TO READ FROM */
drive_num:
    .byte 0x00

marker:
    .byte 0xde,0xad,0xbe,0xef
num_heads:
    .byte 0x00
sectors_per_track:
    .byte 0x00

/* INT 13H PACKET */
disk_packet:
    .byte 0x10
    .byte 0x00
num_sectors:
    .word 0x0001
offset:
    .word 0x0000
segment:
    .word 0x0000
sector:
    .quad 0x00000000

/* GDT */
.align 16
gdtp:
    .word gdt_end - gdt_start - 1
    /* .long (0x07C0 << 4) + gdt */
    .long gdt_start

.align 16
gdt_start:
gdt_null:
    .quad 0
gdt_code_segment:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10011010
    .byte 0b11001111
    .byte 0x00
gdt_data_segment:
    .word 0xffff
    .word 0x0000
    .byte 0x00
    .byte 0b10010010
    .byte 0b11001111
    .byte 0x00
gdt_end:

/* IDT */
idt:
    .word 0
    .long 0

/* MBR BOOT SIGNATURE */
.fill 510-(.-_start), 1, 0
.word 0xAA55
