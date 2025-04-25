#include "screen.h"
#include "io.h"
#include "../libc/include/stddef.h"

// Declaration of private functions
int get_cursor_offset();
void set_cursor_offset(int offset);
int print_char(char c, int col, int row, char attr);
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);

/**
 * Print a message at specified column and row
 */
void kprint_at(const char *message, int col, int row) {
    // Set cursor if col/row are negative
    int offset;
    if (col >= 0 && row >= 0)
        offset = get_offset(col, row);
    else {
        offset = get_cursor_offset();
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }

    // Loop through message and print it
    int i = 0;
    while (message[i] != 0) {
        offset = print_char(message[i++], col, row, WHITE_ON_BLACK);
        // Compute row/col for next iteration
        row = get_offset_row(offset);
        col = get_offset_col(offset);
    }
}

/**
 * Print a message at current cursor location
 */
void kprint(const char *message) {
    kprint_at(message, -1, -1);
}

/**
 * Handle a backspace - removing the last character
 */
void kprint_backspace() {
    int offset = get_cursor_offset() - 2;
    int row = get_offset_row(offset);
    int col = get_offset_col(offset);
    print_char(' ', col, row, WHITE_ON_BLACK);
    set_cursor_offset(offset);
}

/**
 * Clear the screen by filling it with spaces
 */
void clear_screen() {
    int screen_size = MAX_ROWS * MAX_COLS;
    int i;
    char *screen = (char*) VIDEO_ADDRESS;

    for (i = 0; i < screen_size; i++) {
        screen[i*2] = ' ';
        screen[i*2+1] = WHITE_ON_BLACK;
    }
    set_cursor_offset(get_offset(0, 0));
}

/**
 * Print a character at the specified location with attributes
 */
int print_char(char c, int col, int row, char attr) {
    unsigned char *vidmem = (unsigned char*) VIDEO_ADDRESS;
    if (!attr) attr = WHITE_ON_BLACK;

    /* Error handling: print a red 'E' if position is invalid */
    if (col >= MAX_COLS || row >= MAX_ROWS) {
        vidmem[2*(MAX_ROWS-1)*MAX_COLS + 2*(MAX_COLS-1)] = 'E';
        vidmem[2*(MAX_ROWS-1)*MAX_COLS + 2*(MAX_COLS-1) + 1] = RED_ON_WHITE;
        return get_offset(col, row);
    }

    int offset;
    if (col >= 0 && row >= 0) offset = get_offset(col, row);
    else offset = get_cursor_offset();

    // Handle special characters
    if (c == '\n') {
        row = get_offset_row(offset);
        offset = get_offset(0, row + 1);
    } else if (c == 0x08) { // Backspace
        vidmem[offset] = ' ';
        vidmem[offset + 1] = attr;
    } else {
        vidmem[offset] = c;
        vidmem[offset + 1] = attr;
        offset += 2;
    }

    // Scroll if needed
    if (offset >= MAX_ROWS * MAX_COLS * 2) {
        int i;
        for (i = 1; i < MAX_ROWS; i++) {
            // Copy current line to previous line
            int j;
            for (j = 0; j < MAX_COLS * 2; j++) {
                vidmem[(i-1) * MAX_COLS * 2 + j] = vidmem[i * MAX_COLS * 2 + j];
            }
        }
        // Clear last line
        char *last_line = (char*)(vidmem + (MAX_ROWS-1) * MAX_COLS * 2);
        for (i = 0; i < MAX_COLS * 2; i += 2) {
            last_line[i] = ' ';
            last_line[i+1] = WHITE_ON_BLACK;
        }
        offset -= MAX_COLS * 2;
    }

    set_cursor_offset(offset);
    return offset;
}

/**
 * Get current cursor position
 */
int get_cursor_offset() {
    // Use VGA ports to get cursor position
    port_byte_out(REG_SCREEN_CTRL, 14); // Request high byte
    int offset = port_byte_in(REG_SCREEN_DATA) << 8;
    port_byte_out(REG_SCREEN_CTRL, 15); // Request low byte
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2; // Position * 2 to convert it to character cell offset
}

/**
 * Set the cursor position
 */
void set_cursor_offset(int offset) {
    offset /= 2; // Convert from cell offset to cursor offset
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset & 0xff));
}

/**
 * Convert col,row to offset
 */
int get_offset(int col, int row) {
    return 2 * (row * MAX_COLS + col);
}

/**
 * Get row from offset
 */
int get_offset_row(int offset) {
    return offset / (2 * MAX_COLS);
}

/**
 * Get col from offset
 */
int get_offset_col(int offset) {
    return (offset - (get_offset_row(offset) * 2 * MAX_COLS)) / 2;
}
