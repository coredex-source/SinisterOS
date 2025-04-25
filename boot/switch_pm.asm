; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\switch_pm.asm
[bits 16]
; Switch to protected mode
switch_to_protected_mode:
    cli                     ; Disable interrupts
    lgdt [gdt_descriptor]   ; Load the GDT descriptor
    
    ; Set PE (Protection Enable) bit in CR0
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; Perform far jump to select code segment and force CPU to flush pipeline
    jmp CODE_SEG:init_pm

[bits 32]
; Initialize protected mode
init_pm:
    ; Update segment registers
    mov ax, DATA_SEG        ; Data segment selector
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Update stack position
    mov ebp, 0x90000        ; Set up stack at the top of free space
    mov esp, ebp

    call BEGIN_PM           ; Call a well-known label