#ifndef IO_H
#define IO_H

/* I/O ports manipulation functions */

/**
 * Read a byte from a port
 */
unsigned char port_byte_in(unsigned short port);

/**
 * Write a byte to a port
 */
void port_byte_out(unsigned short port, unsigned char data);

/**
 * Read a word (2 bytes) from a port
 */
unsigned short port_word_in(unsigned short port);

/**
 * Write a word (2 bytes) to a port
 */
void port_word_out(unsigned short port, unsigned short data);

/**
 * Write a double word (4 bytes) to port
 */
void port_dword_out(unsigned short port, unsigned int data);

/**
 * Read a double word (4 bytes) from a port
 */
unsigned int port_dword_in(unsigned short port);

#endif /* IO_H */
