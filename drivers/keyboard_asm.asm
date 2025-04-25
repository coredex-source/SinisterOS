; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\drivers\keyboard_asm.asm
[bits 32]
[extern keyboard_handler]

; Load IDT
global load_idt
load_idt:
    mov edx, [esp + 4]     ; Get IDT pointer argument
    lidt [edx]             ; Load the IDT
    sti                    ; Enable interrupts
    ret

; ASM keyboard handler that calls the C handler
global keyboard_handler_asm
keyboard_handler_asm:
    pushad                 ; Push all general purpose registers
    cld                    ; Clear direction flag
    call keyboard_handler  ; Call C handler
    popad                  ; Restore all registers
    iret                   ; Return from interrupt