#include "keyboard.h"
#include "../kernel/io.h"
#include "../kernel/screen.h"
#include "../kernel/system_stats.h"
#include "../include/sysinfo.h"
#include "../fs/filesystem.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_COMMAND_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

// Function prototypes
void keyboard_handler();
char get_ascii_char(unsigned char key_code);
void int_to_ascii(int n, char str[]);

// External assembly function
extern void load_idt(unsigned long *idt_ptr);
extern void keyboard_handler_asm();

// IDT entries
struct IDT_entry {
    unsigned short int offset_lowerbits;
    unsigned short int selector;
    unsigned char zero;
    unsigned char type_attr;
    unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

// Buffer to store command
char command_buffer[256];
int buffer_pos = 0;

// Add state variables for shift and caps lock
static int shift_pressed = 0;
static int caps_lock_on = 0;

void init_keyboard() {
    // Setup IDT
    unsigned long keyboard_address = (unsigned long)keyboard_handler_asm;
    unsigned long idt_address = (unsigned long)IDT;
    unsigned long idt_ptr[2];

    // Populate IDT entries with zeros
    for (int i = 0; i < IDT_SIZE; i++) {
        IDT[i].offset_lowerbits = 0;
        IDT[i].selector = 0;
        IDT[i].zero = 0;
        IDT[i].type_attr = 0;
        IDT[i].offset_higherbits = 0;
    }

    // Populate IDT entry for keyboard
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address >> 16) & 0xffff;

    // ICW1 - begin initialization
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);
    
    // ICW2 - remap offset address of IDT
    port_byte_out(0x21, 0x20);
    port_byte_out(0xA1, 0x28);
    
    // ICW3 - setup cascading
    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);
    
    // ICW4 - environment info
    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);
    
    // Mask interrupts - enable only keyboard (IRQ1)
    port_byte_out(0x21, 0xFD);  // 1111 1101 - enable IRQ1 (keyboard)
    port_byte_out(0xA1, 0xFF);  // Disable all slave interrupts

    // Fill IDT descriptor
    idt_ptr[0] = (sizeof(struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
    idt_ptr[1] = idt_address >> 16;

    // Load IDT
    load_idt(idt_ptr);
    
    // Reset the keyboard controller (helps with some hardware issues)
    port_byte_out(KEYBOARD_COMMAND_PORT, 0xAE);  // Enable keyboard
}

void keyboard_handler() {
    // Read the keyboard status
    unsigned char status = port_byte_in(KEYBOARD_STATUS_PORT);
    
    // Only process keyboard data when the output buffer is full (bit 0 of status)
    if (status & 0x01) {
        // Read the scancode
        unsigned char scancode = port_byte_in(KEYBOARD_DATA_PORT);
        
        // Handle key release events (top bit is set when key is released)
        if (scancode & 0x80) {
            // Convert to the pressed key code by clearing the top bit
            unsigned char released_key = scancode & 0x7F;
            
            // Handle special key releases
            if (released_key == LEFT_SHIFT || released_key == RIGHT_SHIFT) {
                shift_pressed = 0;
            }
        } 
        // Handle key press events
        else {
            // Handle special keys
            if (scancode == LEFT_SHIFT || scancode == RIGHT_SHIFT) {
                shift_pressed = 1;
            } 
            else if (scancode == CAPS_LOCK) {
                caps_lock_on = !caps_lock_on;
            }
            else if (scancode == ENTER) {
                kprint("\n");
                command_buffer[buffer_pos] = '\0';
                
                // Process command if not empty
                if (buffer_pos > 0) {
                    execute_command(command_buffer);
                }
                
                kprint("> ");
                buffer_pos = 0;
            } 
            else if (scancode == BACKSPACE) {
                if (buffer_pos > 0) {
                    buffer_pos--;
                    command_buffer[buffer_pos] = '\0';
                    kprint_backspace();
                }
            } 
            else {
                // Convert scancode to ASCII character
                char letter = get_ascii_char(scancode);
                
                // Only process if it's a valid character
                if (letter != 0) {
                    // Apply shift and caps lock modifications
                    if (letter >= 'a' && letter <= 'z') {
                        // For letters, apply caps lock XOR shift
                        if (caps_lock_on ^ shift_pressed) {
                            letter = letter - 32; // Convert to uppercase
                        }
                    } 
                    else if (shift_pressed) {
                        // For other characters, apply shift mapping
                        switch (letter) {
                            case '1': letter = '!'; break;
                            case '2': letter = '@'; break;
                            case '3': letter = '#'; break;
                            case '4': letter = '$'; break;
                            case '5': letter = '%'; break;
                            case '6': letter = '^'; break;
                            case '7': letter = '&'; break;
                            case '8': letter = '*'; break;
                            case '9': letter = '('; break;
                            case '0': letter = ')'; break;
                            case '-': letter = '_'; break;
                            case '=': letter = '+'; break;
                            case '[': letter = '{'; break;
                            case ']': letter = '}'; break;
                            case '\\': letter = '|'; break;
                            case ';': letter = ':'; break;
                            case '\'': letter = '\"'; break;
                            case ',': letter = '<'; break;
                            case '.': letter = '>'; break;
                            case '/': letter = '?'; break;
                            case '`': letter = '~'; break;
                        }
                    }
                    
                    // Print the character and add to command buffer
                    char str[2] = {letter, '\0'};
                    kprint(str);
                    
                    // Only add to buffer if there's space
                    if (buffer_pos < 255) {
                        command_buffer[buffer_pos] = letter;
                        buffer_pos++;
                    }
                }
            }
        }
    }
    
    // Always send EOI to acknowledge the interrupt
    port_byte_out(0x20, 0x20);
}

/**
 * Get string length
 */
int str_length(const char *s) {
    int len = 0;
    while(s[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * Copy string and return pointer to destination
 */
char* str_copy(char* dest, const char* src) {
    int i = 0;
    while(src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

/**
 * Compare two strings
 * Returns 1 if equal, 0 if not
 */
int str_equal(char *s1, char *s2) {
    int i = 0;
    while(s1[i] != '\0' && s2[i] != '\0') {
        if(s1[i] != s2[i]) return 0;
        i++;
    }
    return (s1[i] == '\0' && s2[i] == '\0');
}

/**
 * Check if string starts with a prefix
 * Returns 1 if it starts with, 0 if not
 */
int str_starts_with(char *s1, char *prefix) {
    int i = 0;
    while(prefix[i] != '\0') {
        if(s1[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

/**
 * Execute command based on user input
 */
void execute_command(char *command) {
    if(str_equal(command, "cls") || str_equal(command, "clear")) {
        clear_screen();
    } else if(str_equal(command, "shutdown")) {
        shutdown();
    } else if(str_equal(command, "help")) {
        display_help();
    } else if(str_equal(command, "monitor")) {
        display_resource_usage();
    } else if(str_equal(command, "version")) {
        display_version();
    } else if(str_equal(command, "uptime")) {
        display_uptime();
    } else if(str_equal(command, "reboot")) {
        reboot();
    } else if(str_starts_with(command, "echo ")) {
        echo_command(command + 5); // Skip "echo " prefix
    } else if(str_equal(command, "ls")) {
        ls_command();
    } else if(str_starts_with(command, "cd ")) {
        cd_command(command + 3); // Skip "cd " prefix
    } else if(str_equal(command, "pwd")) {
        pwd_command();
    } else if(str_starts_with(command, "cat ")) {
        cat_command(command + 4); // Skip "cat " prefix
    } else if(str_starts_with(command, "touch ")) {
        touch_command(command + 6); // Skip "touch " prefix
    } else if(str_starts_with(command, "rm ")) {
        rm_command(command + 3); // Skip "rm " prefix
    } else if(str_starts_with(command, "mkdir ")) {
        mkdir_command(command + 6); // Skip "mkdir " prefix
    } else if(str_starts_with(command, "cp ")) {
        cp_command(command + 3); // Skip "cp " prefix
    } else if(str_starts_with(command, "mv ")) {
        mv_command(command + 3); // Skip "mv " prefix
    } else {
        kprint("Unknown command: ");
        kprint(command);
        kprint("\n");
    }
}

/**
 * List directory contents
 */
void ls_command() {
    fs_list_directory();
}

/**
 * Change directory
 */
void cd_command(char *path) {
    if (fs_change_directory(path) == 0) {
        // Success, show current path
        pwd_command();
    }
}

/**
 * Print working directory
 */
void pwd_command() {
    char* current_path = fs_get_current_path();
    kprint("Current directory: ");
    kprint(current_path);
    kprint("\n");
}

/**
 * Display file content
 */
void cat_command(char *filename) {
    fs_cat_file(filename);
}

/**
 * Create or update a file
 */
void touch_command(char *filename) {
    fs_touch_file(filename);
}

/**
 * Delete files or directories
 */
void rm_command(char *args) {
    // Check for empty args
    if (args[0] == '\0') {
        kprint("Usage: rm [option] [filename]\n");
        kprint("Options:\n");
        kprint("  -dir     - Delete an empty directory\n");
        kprint("  -f-dir   - Force delete a directory and its contents\n");
        kprint("  -f       - Delete a file (default)\n");
        kprint("  -*f      - Delete all files in current directory\n");
        kprint("  -*f-ext [ext] - Delete all files with specific extension\n");
        return;
    }
    
    // Parse options and filename
    if (str_starts_with(args, "-dir ")) {
        // Delete an empty directory
        fs_delete_directory(args + 5, 0);
    } else if (str_starts_with(args, "-f-dir ")) {
        // Force delete a directory and its contents
        fs_delete_directory(args + 7, 1);
    } else if (str_starts_with(args, "-*f-ext ")) {
        // Delete all files with specific extension
        fs_delete_files_by_extension(args + 8);
    } else if (str_equal(args, "-*f")) {
        // Delete all files in current directory
        fs_delete_all_files();
    } else if (str_starts_with(args, "-f ")) {
        // Delete a file (explicit option)
        fs_delete_file(args + 3);
    } else {
        // Default: delete a file
        fs_delete_file(args);
    }
}

/**
 * Create a new directory
 */
void mkdir_command(char *dirname) {
    if (dirname[0] == '\0') {
        kprint("Usage: mkdir <directory_name>\n");
        return;
    }
    
    fs_create_directory(dirname);
}

/**
 * Copy a file from source to destination
 */
void cp_command(char *args) {
    // Parse arguments - find space between paths
    char *space_pos = args;
    while(*space_pos != ' ' && *space_pos != '\0') {
        space_pos++;
    }
    
    if (*space_pos == '\0') {
        kprint("Usage: cp <source_file> <destination_file>\n");
        return;
    }
    
    // Split into two paths
    *space_pos = '\0';
    char *src_path = args;
    char *dest_path = space_pos + 1;
    
    // Skip leading spaces in destination path
    while (*dest_path == ' ') {
        dest_path++;
    }
    
    if (*dest_path == '\0') {
        kprint("Usage: cp <source_file> <destination_file>\n");
        return;
    }
    
    fs_copy_file(src_path, dest_path);
}

/**
 * Move a file or directory
 */
void mv_command(char *args) {
    // Parse arguments - find space between paths
    char *space_pos = args;
    while(*space_pos != ' ' && *space_pos != '\0') {
        space_pos++;
    }
    
    if (*space_pos == '\0') {
        kprint("Usage: mv <source> <destination>\n");
        return;
    }
    
    // Split into two paths
    *space_pos = '\0';
    char *src_path = args;
    char *dest_path = space_pos + 1;
    
    // Skip leading spaces in destination path
    while (*dest_path == ' ') {
        dest_path++;
    }
    
    if (*dest_path == '\0') {
        kprint("Usage: mv <source> <destination>\n");
        return;
    }
    
    fs_move(src_path, dest_path);
}

/**
 * Display help text
 */
void display_help() {
    kprint("Sinister OS - Basic Commands\n");
    kprint("---------------------------\n");
    kprint("help     - Display this help text\n");
    kprint("cls      - Clear the screen\n");
    kprint("shutdown - Shut down the system\n");
    kprint("reboot   - Restart the system\n");
    kprint("monitor  - Display system resource usage\n");
    kprint("version  - Display system version information\n");
    kprint("uptime   - Show system uptime\n");
    kprint("echo     - Display text after command\n");
    kprint("ls       - List directory contents\n");
    kprint("cd       - Change directory\n");
    kprint("pwd      - Print working directory\n");
    kprint("cat      - Display file contents\n");
    kprint("touch    - Create a new file or update timestamp\n");
    kprint("rm       - Delete files or directories\n");
    kprint("mkdir    - Create a new directory\n");
    kprint("cp       - Copy a file\n");
    kprint("mv       - Move or rename a file or directory\n");
}

/**
 * Shutdown the system
 */
void shutdown() {
    kprint("Shutting down...\n");
    
    // Try ACPI first
    port_word_out(0x604, 0x2000);
    
    // If that didn't work, try the keyboard controller
    port_byte_out(0x64, 0xFE);
    
    // If we get here, the shutdown didn't work
    kprint("Failed to shut down. It is now safe to turn off your computer.\n");
}

/**
 * Reboot the system
 */
void reboot() {
    kprint("Rebooting...\n");
    
    // Wait for the keyboard buffer to be cleared
    unsigned char temp;
    do {
        temp = port_byte_in(0x64);
        if((temp & 1) != 0) {
            port_byte_in(0x60); // read and discard
        }
    } while((temp & 2) != 0);
    
    // Send reset command to the keyboard controller
    port_byte_out(0x64, 0xFE); // Pulse reset line
    
    // If we get here, the above method didn't work - try alternative reset
    kprint("First reset method failed, trying alternative...\n");
    
    // Use a different approach for triple fault - avoid direct memory reference
    __asm__ volatile (
        "movl $0, %%eax\n\t"
        "movl %%eax, %%cr3\n\t"  // Invalid CR3 will cause a fault
        "int $3"
        : : : "eax"
    );
    
    // If we get here, nothing worked
    kprint("Failed to reboot. Please press the reset button.\n");
}

/**
 * Echo back the arguments provided
 */
void echo_command(char *args) {
    kprint(args);
    kprint("\n");
}

/**
 * Display version information in a neofetch-like style
 */
void display_version() {
    system_stats_t stats = get_system_stats();
    char buffer[32];
    
    kprint("\n");
    kprint("  \\\\\\\\\\\\\\\\       \\\\\\\\\\\\\\\\    User: Guest\n");
    kprint("  \\\\\\\\\\\\\\\\\\\\\\\\   \\\\\\\\\\\\\\\\    OS: ");
    kprint(OS_NAME);
    kprint(" ");
    kprint(OS_VERSION);
    kprint("\n");
    
    kprint("  \\\\\\\\ \\\\\\\\\\\\\\\\\\ \\\\\\\\       Kernel: ");
    kprint(KERNEL_NAME);
    kprint(" ");
    kprint(KERNEL_VERSION);
    kprint("\n");
    
    kprint("   \\\\\\\\ \\\\\\ \\\\\\ \\\\\\\\       Build: ");
    kprint(BUILD_DATE);
    kprint("-");
    kprint(BUILD_ARCH);
    kprint("\n");
    
    kprint("    \\\\\\\\ \\\\  \\\\\\ \\\\\\\\      Shell: ");
    kprint(SHELL_NAME);
    kprint(" ");
    kprint(SHELL_VERSION);
    kprint("\n");
    
    kprint("     \\\\\\\\\\\\ \\\\\\\\\\\\      Resolution: ");
    kprint(DISPLAY_MODE);
    kprint("\n");
    
    kprint("      \\\\\\\\\\\\\\\\\\\\\\\\       CPU: ");
    kprint(stats.cpu_model);
    kprint("\n");
    
    kprint("       \\\\\\\\\\\\\\\\\\\\        Memory: ");
    int_to_ascii(stats.memory_total / 1024, buffer);
    kprint(buffer);
    kprint(" MB Total\n");
    
    kprint("\n");
    kprint("RRRRR   EEEEE   AAA   DDDD   Y   Y\n"); 
    kprint("R   R   E      A   A  D   D   Y Y \n");
    kprint("RRRRR   EEEE   AAAAA  D   D    Y  \n");
    kprint("R  R    E      A   A  D   D    Y  \n");
    kprint("R   R   EEEEE  A   A  DDDD     Y  \n");
    kprint("\n");
}

/**
 * Display uptime information
 */
void display_uptime() {
    system_stats_t stats = get_system_stats();
    char buffer[32];
    
    kprint("\nSystem uptime: ");
    
    // Days
    int days = stats.uptime_seconds / (3600 * 24);
    if (days > 0) {
        int_to_ascii(days, buffer);
        kprint(buffer);
        kprint(" day");
        if (days != 1) kprint("s");
        kprint(", ");
    }
    
    // Hours
    unsigned int hours = (stats.uptime_seconds % (3600 * 24)) / 3600;
    if (hours > 0 || days > 0) {
        int_to_ascii(hours, buffer);
        kprint(buffer);
        kprint(" hour");
        if (hours != 1) kprint("s");
        kprint(", ");
    }
    
    // Minutes
    unsigned int mins = (stats.uptime_seconds % 3600) / 60;
    int_to_ascii(mins, buffer);
    kprint(buffer);
    kprint(" minute");
    if (mins != 1) kprint("s");
    kprint(", ");
    
    // Seconds
    unsigned int secs = stats.uptime_seconds % 60;
    int_to_ascii(secs, buffer);
    kprint(buffer);
    kprint(" second");
    if (secs != 1) kprint("s");
    kprint("\n");
}

/**
 * Display system resource usage information
 */
void display_resource_usage() {
    system_stats_t stats = get_system_stats();
    char buffer[32];
    
    kprint("\nSINISTER-OS RESOURCE MONITOR\n");
    kprint("===========================\n");
    
    // Add timestamp
    kprint("Time: ");
    int_to_ascii(stats.uptime_seconds / 3600, buffer);
    if (stats.uptime_seconds / 3600 < 10) kprint("0");
    kprint(buffer);
    kprint(":");
    
    unsigned int min = (stats.uptime_seconds % 3600) / 60;
    if (min < 10) kprint("0");
    int_to_ascii(min, buffer);
    kprint(buffer);
    kprint(":");
    
    unsigned int sec = stats.uptime_seconds % 60;
    if (sec < 10) kprint("0");
    int_to_ascii(sec, buffer);
    kprint(buffer);
    kprint(" uptime\n\n");
    
    // CPU Usage
    kprint("CPU Usage: ");
    kprint("[");

    for (int i = 0; i < 10; i++) {
        if (i < stats.cpu_usage / 10) {
            kprint("#");
        } else {
            kprint("-");
        }
    }
    
    // Format: xx%
    int_to_ascii(stats.cpu_usage, buffer);
    kprint("] ");
    kprint(buffer);
    kprint("%\n");
    
    // Memory Usage
    kprint("Memory Usage: ");
    kprint("[");
    int memory_percentage = (stats.memory_used * 100) / (stats.memory_total > 0 ? stats.memory_total : 1);
    for (int i = 0; i < 10; i++) {
        if (i < memory_percentage / 10) {
            kprint("#");
        } else {
            kprint("-");
        }
    }
    
    // Format: (xxx KB / xxxx KB)
    kprint("] ");
    int_to_ascii(memory_percentage, buffer);
    kprint(buffer);
    kprint("% (");
    int_to_ascii(stats.memory_used, buffer);
    kprint(buffer);
    kprint(" KB / ");
    int_to_ascii(stats.memory_total, buffer);
    kprint(buffer);
    kprint(" KB)\n");
    
    // Disk Usage
    kprint("Disk Usage: ");
    kprint("[");

    int disk_percentage = (stats.disk_used * 100) / (stats.disk_total > 0 ? stats.disk_total : 1);
    for (int i = 0; i < 10; i++) {
        if (i < disk_percentage / 10) {
            kprint("#");
        } else {
            kprint("-");
        }
    }
    
    // Format: (xx.x KB / x.xx MB)
    kprint("] ");
    int_to_ascii(disk_percentage, buffer);
    kprint(buffer);
    kprint("% (");
    int_to_ascii(stats.disk_used, buffer);
    kprint(buffer);
    kprint(" KB / ");
    
    // Convert KB to MB with 1 decimal place
    int mb_value = stats.disk_total / 1000;
    int decimal = (stats.disk_total % 1000) / 100;
    int_to_ascii(mb_value, buffer);
    kprint(buffer);
    kprint(".");
    int_to_ascii(decimal, buffer);
    kprint(buffer);
    kprint(" MB)\n");
    
    // System uptime - use different variable names to avoid conflict
    unsigned int hours = stats.uptime_seconds / 3600;
    unsigned int mins = (stats.uptime_seconds % 3600) / 60;
    unsigned int secs = stats.uptime_seconds % 60;
    
    kprint("System Uptime: ");
    if (hours < 10) kprint("0");
    int_to_ascii(hours, buffer);
    kprint(buffer);
    kprint(":");
    if (mins < 10) kprint("0");
    int_to_ascii(mins, buffer);
    kprint(buffer);
    kprint(":");
    if (secs < 10) kprint("0");
    int_to_ascii(secs, buffer);
    kprint(buffer);
    kprint("\n\n");
    
    // Process display (actual kernel process)
    kprint("Active Processes: 1\n");
    kprint("PID  NAME       STATUS   CPU\n");
    kprint("---  ---------  -------  ---\n");
    kprint("001  kernel     Running  ");
    int_to_ascii(stats.cpu_usage, buffer);
    kprint(buffer);
    kprint("%\n");
}

/**
 * Helper function to convert integer to ASCII
 */
void int_to_ascii(int n, char str[]) {
    int i = 0;
    int sign = n;
    
    // Handle 0 explicitly
    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    // Process individual digits
    while (n > 0) {
        str[i++] = n % 10 + '0';
        n = n / 10;
    }
    
    if (sign < 0) {
        str[i++] = '-';
    }
    
    str[i] = '\0'; // Null terminator
    
    // Reverse the string
    int j, k;
    char c;
    for (j = 0, k = i - 1; j < k; j++, k--) {
        c = str[j];
        str[j] = str[k];
        str[k] = c;
    }
}

// Enhanced keycode to ASCII mapping for a full QWERTY keyboard
char get_ascii_char(unsigned char key_code) {
    switch(key_code) {
        // Numbers row
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        
        // QWERTY row
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x2B: return '\\';
        
        // ASDFG row
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\''; 
        case 0x29: return '`';
        
        // ZXCVB row
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        
        // Space bar and other common keys
        case 0x39: return ' ';
        
        default: return 0;
    }
}
