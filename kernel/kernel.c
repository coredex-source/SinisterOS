#include "io.h"
#include "screen.h"
#include "system_stats.h"
#include "../drivers/keyboard.h"
#include "../fs/filesystem.h"

/* 
 * Main function - entry point for our kernel
 */
void main() {
    // Initialize screen
    clear_screen();
    
    // Initialize system statistics
    init_system_stats();
    
    // Display welcome message
    kprint("  ____  _       _     _              ___  ____  \n");
    kprint(" / ___|(_)_ __ (_)___| |_ ___ _ __  / _ \\/ ___| \n");
    kprint(" \\___ \\| | '_ \\| / __| __/ _ \\ '__|| | | \\___ \\ \n");
    kprint("  ___) | | | | | \\__ \\ ||  __/ |   | |_| |___) |\n");
    kprint(" |____/|_|_| |_|_|___/\\__\\___|_|    \\___/|____/ \n");
    kprint("\n");
    kprint("Welcome to Sinister OS\n");
    kprint("Type 'help' to view available commands\n");
    
    // Initialize filesystem
    fs_init();
    
    // Initialize keyboard
    init_keyboard();
    
    // Simple shell
    kprint("\n> ");
    
    // Main loop with system stats updating
    while(1) {
        // Update system statistics
        update_system_stats();
        
        // Record CPU idle time and wait for interrupts
        record_idle_time();
    }
}
