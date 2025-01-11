[BITS 16]
[ORG 0x7C00]

start:
    cli
    mov ax, 0x00
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    sti

    mov si, msg
    call PrintString

menu_loop:
    mov si, msg_prompt
    call PrintString

    mov di, input_buffer
    call ReadString

    mov si, input_buffer
    call CompareString
    cmp cx, 0
    je help_command

    mov si, input_buffer
    call CompareStringQuit
    cmp cx, 0
    je shutdown_kernel

    mov si, input_buffer
    call CompareStringSysInfo
    cmp cx, 0
    je sysinfo_command

    jmp menu_loop

help_command:
    mov si, msg_help1
    call PrintString
    mov si, msg_help2
    call PrintString
    mov si, msg_help3
    call PrintString
    mov si, msg_help4
    call PrintString
    jmp menu_loop

sysinfo_command:
    call GetMemorySize
    mov si, msg_sysinfo
    call PrintString
    call PrintMemorySize
    jmp menu_loop

shutdown_kernel:
    mov ax, 0x5307
    mov bx, 0x0001
    mov cx, 0x0003
    int 0x15
    cli
    hlt

PrintString:
    lodsb
    cmp al, 0
    je .done
    mov ah, 0x0E
    int 0x10
    jmp PrintString
.done:
    ret

ReadString:
    xor cx, cx
.read_loop:
    mov ah, 0x00
    int 0x16
    cmp al, 0x0D
    je .done
    stosb
    mov ah, 0x0E
    int 0x10
    inc cx
    jmp .read_loop
.done:
    stosb
    ret

CompareString:
    mov di, help_cmd
    mov cx, 5
    repe cmpsb
    ret

CompareStringQuit:
    mov di, quit_cmd
    mov cx, 5
    repe cmpsb
    ret

CompareStringSysInfo:
    mov di, sysinfo_cmd
    mov cx, 7
    repe cmpsb
    ret

GetMemorySize:
    mov ah, 0x88
    int 0x15
    mov [memory_size], ax
    ret

PrintMemorySize:
    mov ax, [memory_size]
    call PrintDecimal
    mov si, msg_kb
    call PrintString
    ret

PrintDecimal:
    pusha
    xor cx, cx
    mov bx, 10
.convert_loop:
    xor dx, dx
    div bx
    push dx
    inc cx
    test ax, ax
    jnz .convert_loop
.print_loop:
    pop dx
    or dl, '0'
    mov ah, 0x0E
    mov al, dl
    int 0x10
    loop .print_loop
    popa
    ret

msg:        db 'Welcome to SinisterOS!', 0x0A, 0x0D, 0x0A, 0x0D, 0
msg_prompt: db 'sinisteros@root$ ', 0

msg_help1:  db 0x0A, 0x0D, 'Available commands:', 0x0A, 0x0D, 0
msg_help2:  db 'help - Display this help message', 0x0A, 0x0D, 0
msg_help3:  db 'quit - Quit the OS', 0x0A, 0x0D, 0
msg_help4:  db 'sysinfo - Display system information', 0x0A, 0x0D, 0

msg_sysinfo: db 0x0A, 0x0D, 'System Information:', 0x0A, 0x0D, 'Memory Size: ', 0
msg_kb:     db ' KB', 0x0A, 0x0D, 0

help_cmd:   db 'help', 0
quit_cmd:   db 'quit', 0
sysinfo_cmd: db 'sysinfo', 0

memory_size: dw 0

input_buffer: times 20 db 0

times 510 - ($ - $$) db 0

dw 0xAA55