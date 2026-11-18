/* Interrupt stubs for matenOS.  Every stub pushes a vector number and an
 * error code (0 when the CPU does not supply one), then falls into the
 * common save/restore path. */
    .code32
    .section .text

.macro ISR_NOERR num
    .global isr\num
isr\num:
    pushl $0
    pushl $\num
    jmp isr_common
.endm

.macro ISR_ERR num
    .global isr\num
isr\num:
    pushl $\num
    jmp isr_common
.endm

    ISR_NOERR 0
    ISR_NOERR 1
    ISR_NOERR 2
    ISR_NOERR 3
    ISR_NOERR 4
    ISR_NOERR 5
    ISR_NOERR 6
    ISR_NOERR 7
    ISR_ERR   8
    ISR_NOERR 9
    ISR_ERR   10
    ISR_ERR   11
    ISR_ERR   12
    ISR_ERR   13
    ISR_ERR   14
    ISR_NOERR 15
    ISR_NOERR 16
    ISR_ERR   17
    ISR_NOERR 18
    ISR_NOERR 19
    ISR_NOERR 20
    ISR_ERR   21
    ISR_NOERR 22
    ISR_NOERR 23
    ISR_NOERR 24
    ISR_NOERR 25
    ISR_NOERR 26
    ISR_NOERR 27
    ISR_NOERR 28
    ISR_ERR   29
    ISR_ERR   30
    ISR_NOERR 31

    ISR_NOERR 32
    ISR_NOERR 33
    ISR_NOERR 34
    ISR_NOERR 35
    ISR_NOERR 36
    ISR_NOERR 37
    ISR_NOERR 38
    ISR_NOERR 39
    ISR_NOERR 40
    ISR_NOERR 41
    ISR_NOERR 42
    ISR_NOERR 43
    ISR_NOERR 44
    ISR_NOERR 45
    ISR_NOERR 46
    ISR_NOERR 47

    .global isr_common
isr_common:
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movl $0x10, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax
    pushl %eax
    call isr_handler
    addl $4, %esp

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    addl $8, %esp
    iret

    .global idt_load
idt_load:
    movl 4(%esp), %eax
    lidt (%eax)
    ret
