[bits 32]
[global _start]
[extern main]   ; Define external symbol - our C kernel main() function

_start:
    ; Set up a basic stack
    mov esp, stack_space
    
    ; Call the C kernel
    call main
    
    ; Hang if kernel returns
    jmp $
    
; Allocate some stack space
section .bss
    resb 8192   ; Reserve 8KB for stack
stack_space: