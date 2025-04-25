; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\print_string.asm
; Function to print a null-terminated string
; Input: SI register points to string
print_string:
    pusha
    mov ah, 0x0e            ; BIOS teletype function

print_char:
    lodsb                   ; Load byte at SI into AL and increment SI
    or al, al               ; Check if AL is zero (end of string)
    jz done_print           ; If zero, we're done
    int 0x10                ; Print the character in AL
    jmp print_char          ; Loop to next character

done_print:
    popa
    ret