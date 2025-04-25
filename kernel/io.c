#include "io.h"

// Read a byte from the specified port
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    __asm__("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

// Write a byte to the specified port
void port_byte_out(unsigned short port, unsigned char data) {
    __asm__("out %%al, %%dx" : : "a" (data), "d" (port));
}

// Read a word from the specified port
unsigned short port_word_in(unsigned short port) {
    unsigned short result;
    __asm__("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

// Write a word to the specified port
void port_word_out(unsigned short port, unsigned short data) {
    __asm__("out %%ax, %%dx" : : "a" (data), "d" (port));
}

// Read a double word from the specified port
unsigned int port_dword_in(unsigned short port) {
    unsigned int result;
    __asm__("inl %%dx, %%eax" : "=a" (result) : "d" (port));
    return result;
}

// Write a double word to the specified port
void port_dword_out(unsigned short port, unsigned int data) {
    __asm__("outl %%eax, %%dx" : : "a" (data), "d" (port));
}
