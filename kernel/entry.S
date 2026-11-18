/* matenOS kernel entry point.  The boot sector jumps here with the CPU in
 * 32-bit protected mode.  Clear .bss, set up a stack and call kmain(). */
    .code32
    .section .text.entry, "ax"
    .global _start
_start:
    cli
    cld
    movl $0x700000, %esp
    xorl %ebp, %ebp

    movl $__bss_start, %edi
    movl $__bss_end, %ecx
    subl %edi, %ecx
    xorl %eax, %eax
    rep stosb

    call kmain

1:
    cli
    hlt
    jmp 1b
