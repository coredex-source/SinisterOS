/* filepath: c:\Users\Samarth\Documents\GitHub\Sinister-OS\linker.ld */
ENTRY(main)
SECTIONS
{
    /* Kernel starts at 1MB (0x100000) but we offset by 0x1000 for the boot sector */
    . = 0x1000;
    
    /* Text section containing code */
    .text : {
        *(.text)
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata)
    }
    
    /* Initialized data */
    .data : {
        *(.data)
    }
    
    /* Uninitialized data */
    .bss : {
        *(COMMON)
        *(.bss)
    }
}