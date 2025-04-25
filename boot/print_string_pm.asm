; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\print_string_pm.asm
[bits 32]
; Constants
VIDEO_MEMORY equ 0xb8000    ; Location of video memory
WHITE_ON_BLACK equ 0x0f     ; Color attribute byte

; Print null-terminated string pointed to by EBX
print_string_pm:
    pusha
    mov edx, VIDEO_MEMORY   ; Set EDX to start of video memory

print_string_pm_loop:
    mov al, [ebx]           ; Store character at EBX in AL
    mov ah, WHITE_ON_BLACK  ; Store attributes in AH
    
    cmp al, 0               ; Check if end of string
    je print_string_pm_done ; If zero, jump to done
    
    mov [edx], ax           ; Store character and attributes at current video memory position
    
    add ebx, 1              ; Increment EBX (next character)
    add edx, 2              ; Move to next video memory position (2 bytes per character)
    
    jmp print_string_pm_loop ; Loop for next character

print_string_pm_done:
    popa
    ret