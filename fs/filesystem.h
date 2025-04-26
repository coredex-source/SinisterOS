#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../libc/include/stddef.h"

// Maximum path length
#define MAX_PATH 256

// Maximum filename length in 8.3 format
#define MAX_FILENAME 13

// Maximum number of clusters in our filesystem
#define MAX_CLUSTERS 16

// Maximum size of file content buffer per cluster
#define MAX_FILE_CONTENT 512

// Entry attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

// FAT entry states
#define FAT_FREE       0x000
#define FAT_RESERVED   0xFF0
#define FAT_BAD        0xFF7
#define FAT_LAST       0xFF8

// File system types
typedef enum {
    FS_FAT12,
    FS_FAT16,
    FS_UNKNOWN
} fs_type_t;

// Directory entry structure
typedef struct {
    char filename[MAX_FILENAME];
    unsigned char attributes;
    unsigned long size;
    unsigned long cluster;
    unsigned long time;
    unsigned long date;
} dir_entry_t;

// Virtual file system structure
typedef struct {
    fs_type_t type;
    unsigned int bytes_per_sector;
    unsigned int sectors_per_cluster;
    unsigned int reserved_sectors;
    unsigned int fat_count;
    unsigned int root_dir_entries;
    unsigned int total_sectors;
    unsigned int sectors_per_fat;
    
    // Runtime state
    unsigned int current_dir_cluster;
    char current_path[MAX_PATH];
} filesystem_t;

// Initialize the filesystem
int fs_init();

// List contents of current directory
void fs_list_directory();

// Change current directory
int fs_change_directory(const char* dirname);

// Get current directory path
char* fs_get_current_path();

// Display file content
int fs_cat_file(const char* filename);

// Create or update a file
int fs_touch_file(const char* filename);

// Create a simple in-memory filesystem for testing
void fs_create_test_fs();

// Helper function to format filename
void format_filename(char* dest, const char* src);

// Parse 8.3 filename format
void parse_filename(const char* input, char* output);

#endif /* FILESYSTEM_H */
