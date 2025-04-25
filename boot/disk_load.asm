; filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\boot\disk_load.asm
; Load DH sectors to ES:BX from drive DL
disk_load:
    push dx                 ; Store DX on stack for later comparison
    push cx                 ; Save registers we'll modify
    
    mov ah, 0x02            ; BIOS read sector function
    mov al, dh              ; Read DH sectors
    mov ch, 0x00            ; Select cylinder 0
    mov dh, 0x00            ; Select head 0
    mov cl, 0x02            ; Start reading from second sector (after boot sector)
    
    int 0x13                ; BIOS interrupt for disk functions
    
    jc disk_error           ; If carry flag set, there was an error
    
    pop cx                  ; Restore registers
    pop dx                  ; Restore DX from the stack
    cmp dh, al              ; Compare sectors read (AL) with sectors expected (DH)
    jne sectors_error       ; If they're not equal, there was an error
    ret

disk_error:
    mov si, DISK_ERROR_MSG
    call print_string
    jmp disk_loop           ; Hang

sectors_error:
    mov si, SECTORS_ERROR_MSG
    call print_string
    
disk_loop:
    jmp $                   ; Infinite loop

; Error messages
DISK_ERROR_MSG db "Disk read error! Press any key to reboot...", 0
SECTORS_ERROR_MSG db "Incorrect number of sectors read! Press any key to reboot...", 0