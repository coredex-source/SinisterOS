#ifndef KEYBOARD_H
#define KEYBOARD_H

#define BACKSPACE 0x0E
#define ENTER 0x1C

void init_keyboard();
void execute_command(char *command);
int str_equal(char *s1, char *s2);
void shutdown();
void display_help();
void display_resource_usage();
void int_to_ascii(int n, char str[]);

#endif
