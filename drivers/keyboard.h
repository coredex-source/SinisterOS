#ifndef KEYBOARD_H
#define KEYBOARD_H

#define BACKSPACE 0x0E
#define ENTER 0x1C
#define LEFT_SHIFT 0x2A
#define RIGHT_SHIFT 0x36
#define CAPS_LOCK 0x3A

void init_keyboard();
void execute_command(char *command);
int str_equal(char *s1, char *s2);
int str_starts_with(char *s1, char *prefix);
void shutdown();
void reboot();
void display_help();
void display_resource_usage();
void display_version();
void display_uptime();
void echo_command(char *args);
void ls_command();
void cd_command(char *path);
void pwd_command();
void cat_command(char *filename);
void touch_command(char *filename);
void rm_command(char *args);
void mkdir_command(char *dirname);
void cp_command(char *args);
void mv_command(char *args);
void int_to_ascii(int n, char str[]);
int str_length(const char *s);
char* str_copy(char* dest, const char* src);

#endif
