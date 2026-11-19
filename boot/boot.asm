; matenOS stage 1 -- 512-byte BIOS boot sector (NASM syntax).
;
;   nasm -f bin -D KERNEL_SECTORS=<n> -o boot.bin boot.asm
;
; Loads the kernel from the floppy image to 0x1000:0000, enables A20,
; installs a flat GDT and enters 32-bit protected mode.

BITS 16
ORG 0x7C00

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 64
%endif

start:
    cli
    cld
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, 0x7C00
    sti

    mov     [boot_drive], dl

    xor     ah, ah                  ; reset disk system
    mov     dl, [boot_drive]
    int     0x13
    jc      disk_error

    mov     ax, 0x1000              ; kernel load segment
    mov     es, ax
    xor     di, di
    mov     si, 1                   ; first kernel sector (LBA 1)
    mov     bp, KERNEL_SECTORS

.read_loop:
    push    si
    push    di
    push    bp

    mov     ax, si                  ; LBA -> CHS (18 sectors, 2 heads)
    xor     dx, dx
    mov     cx, 18
    div     cx
    inc     dl
    mov     bl, dl                  ; keep the sector number in BL
    xor     dx, dx
    mov     cx, 2
    div     cx
    mov     ch, al
    mov     dh, dl
    mov     cl, bl                  ; restore the sector number
    mov     dl, [boot_drive]
    mov     ax, 0x0201              ; read one sector
    mov     bx, di
    int     0x13

    pop     bp
    pop     di
    pop     si
    jc      disk_error

    inc     si
    add     di, 512
    dec     bp
    jnz     .read_loop

    in      al, 0x92                ; fast A20 gate
    or      al, 2
    and     al, 0xFE
    out     0x92, al

    ; Copy the BIOS 8x8 font to 0x8000 so the protected-mode GUI can use it.
    mov     ax, 0x1130
    mov     bh, 0x03
    int     0x10
    push    ds
    push    es
    mov     ax, es
    mov     ds, ax
    mov     si, bp
    mov     ax, 0x0800
    mov     es, ax
    xor     di, di
    mov     cx, 2048
    cld
    rep     movsb
    pop     es
    pop     ds

    ; Bochs/QEMU VBE: 1280x1024x32 linear framebuffer at 0xFD000000.
    mov     dx, 0x1CE
    mov     ax, 0x0004
    out     dx, ax
    inc     dx
    xor     ax, ax
    out     dx, ax              ; disable while changing modes

    mov     dx, 0x1CE
    mov     ax, 0x0001
    out     dx, ax
    inc     dx
    mov     ax, 640
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0002
    out     dx, ax
    inc     dx
    mov     ax, 480
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0003
    out     dx, ax
    inc     dx
    mov     ax, 32
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0006
    out     dx, ax
    inc     dx
    mov     ax, 640
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0008
    out     dx, ax
    inc     dx
    xor     ax, ax
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0009
    out     dx, ax
    inc     dx
    xor     ax, ax
    out     dx, ax

    mov     dx, 0x1CE
    mov     ax, 0x0004
    out     dx, ax
    inc     dx
    mov     ax, 0x0041          ; VBE enabled + linear framebuffer
    out     dx, ax

    mov     word [0x5100], 640
    mov     word [0x5102], 480
    mov     byte [0x5104], 32
    mov     word [0x5106], 2560
    mov     dword [0x5108], 0xFD000000

.video_ok:
    lgdt    [gdt_desc]
    mov     eax, cr0
    or      eax, 1
    mov     cr0, eax
    jmp     0x08:pm_start

disk_error:
    mov     si, disk_message
.print:
    lodsb
    test    al, al
    jz      .hang
    mov     ah, 0x0E
    mov     bx, 0x0007
    int     0x10
    jmp     .print
.hang:
    cli
    hlt
    jmp     .hang

BITS 32
pm_start:
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, 0x700000
    xor     ebp, ebp
    mov     eax, 0x10000
    jmp     eax

; ---------------------------------------------------------------------------
boot_drive:     db 0
disk_message:   db "matenOS: disk read error", 0

align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF           ; 0x08: ring 0 code, base 0, 4 GiB
    dq 0x00CF92000000FFFF           ; 0x10: ring 0 data, base 0, 4 GiB
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

times 510 - ($ - $$) db 0
dw 0xAA55




