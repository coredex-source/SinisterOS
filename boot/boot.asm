; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\boot.asm
[org 0x7c00]                ; Set the origin to 0x7c00 (where BIOS loads bootloader)
[bits 16]                   ; We're in 16-bit real mode

KERNEL_OFFSET equ 0x1000    ; Memory offset for kernel

; Save boot drive number passed by BIOS in DL
mov [BOOT_DRIVE], dl        ; Store boot drive immediately after BIOS loads us

; Set up the stack
mov bp, 0x9000              ; Set the base of the stack
mov sp, bp                  ; Stack pointer to base

; Print welcome message
mov si, MSG_REAL_MODE
call print_string

; Load the kernel
call load_kernel

; Switch to protected mode
call switch_to_protected_mode
jmp $                       ; Infinite loop (should never reach here)

; Include routines
%include "boot/print_string.asm"
%include "boot/disk_load.asm"
%include "boot/gdt.asm"
%include "boot/switch_pm.asm"
%include "boot/print_string_pm.asm"

[bits 16]
load_kernel:
    mov si, MSG_LOAD_KERNEL
    call print_string

    mov bx, KERNEL_OFFSET   ; Destination in memory
    mov dh, 50              ; Load more sectors (increased from 20 to 50) to accommodate larger kernel
    mov dl, [BOOT_DRIVE]    ; Boot drive number
    call disk_load          ; Load from disk
    ret

[bits 32]
BEGIN_PM:
    mov ebx, MSG_PROT_MODE
    call print_string_pm
    
    ; Jump to kernel code
    call KERNEL_OFFSET
    jmp $                   ; Hang if kernel returns

; Global variables
BOOT_DRIVE      db 0
MSG_REAL_MODE   db "Started in 16-bit Real Mode...", 0x0D, 0x0A, 0
MSG_LOAD_KERNEL db "Loading kernel into memory...", 0x0D, 0x0A, 0
MSG_PROT_MODE   db "Successfully switched to 32-bit Protected Mode!", 0

; Bootloader padding and signature
times 510-($-$$) db 0       ; Pad with zeros
dw 0xaa55                   ; Boot signature