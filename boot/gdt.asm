; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\gdt.asm
; GDT (Global Descriptor Table)
gdt_start:

gdt_null:       ; Null descriptor - mandatory
    dd 0x0      ; Double word (4 bytes) of zeros
    dd 0x0

gdt_code:       ; Code segment descriptor
    ; Base=0x0, Limit=0xfffff
    ; 1st flags: (present)1 (privilege)00 (descriptor type)1 -> 1001b
    ; Type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    ; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
    dw 0xffff   ; Limit (bits 0-15)
    dw 0x0      ; Base (bits 0-15)
    db 0x0      ; Base (bits 16-23)
    db 10011010b ; 1st flags, type flags
    db 11001111b ; 2nd flags, Limit (bits 16-19)
    db 0x0      ; Base (bits 24-31)

gdt_data:       ; Data segment descriptor
    ; Same as code segment except for type flags:
    ; Type flags: (code)0 (expand down)0 (writable)1 (accessed)0 -> 0010b
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:        ; Label to calculate size

; GDT descriptor
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size of GDT, one byte less than true size
    dd gdt_start                ; Start address of GDT

; Define constants for segment descriptors
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start