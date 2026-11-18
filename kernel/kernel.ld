OUTPUT_FORMAT(pei-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.text.entry)
        *(.text*)
        *(.rdata*)
        *(.rodata*)
        *(.data*)
    }

    __bss_start = .;
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    __bss_end = .;

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
