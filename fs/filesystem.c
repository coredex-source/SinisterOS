#include "filesystem.h"
#include "../kernel/screen.h"
#include "../drivers/keyboard.h"

// Global file system instance
static filesystem_t fs;

// Test directory entries for our in-memory filesystem
static dir_entry_t test_root_dir[] = {
    {".          ", ATTR_DIRECTORY, 0, 0, 0, 0},
    {"..         ", ATTR_DIRECTORY, 0, 0, 0, 0},
    {"BIN        ", ATTR_DIRECTORY, 0, 1, 0, 0},
    {"HOME       ", ATTR_DIRECTORY, 0, 2, 0, 0},
    {"ETC        ", ATTR_DIRECTORY, 0, 3, 0, 0},
    {"README  TXT", 0, 256, 4, 0, 0},
    {"KERNEL  BIN", 0, 1024, 5, 0, 0},
    {"LICENSE TXT", 0, 512, 6, 0, 0}
};

static dir_entry_t test_bin_dir[] = {
    {".          ", ATTR_DIRECTORY, 0, 1, 0, 0},
    {"..         ", ATTR_DIRECTORY, 0, 0, 0, 0},
    {"SHELL   EXE", 0, 512, 7, 0, 0},
    {"INIT    EXE", 0, 256, 8, 0, 0}
};

static dir_entry_t test_home_dir[] = {
    {".          ", ATTR_DIRECTORY, 0, 2, 0, 0},
    {"..         ", ATTR_DIRECTORY, 0, 0, 0, 0},
    {"USER       ", ATTR_DIRECTORY, 0, 9, 0, 0},
    {"GUEST      ", ATTR_DIRECTORY, 0, 10, 0, 0}
};

static dir_entry_t test_etc_dir[] = {
    {".          ", ATTR_DIRECTORY, 0, 3, 0, 0},
    {"..         ", ATTR_DIRECTORY, 0, 0, 0, 0},
    {"PASSWD  TXT", 0, 128, 11, 0, 0},
    {"HOSTS   TXT", 0, 64, 12, 0, 0}
};

// Simulated file content for test files
static char test_file_content[MAX_CLUSTERS][MAX_FILE_CONTENT] = {
    "", "", "", "",  // First 4 clusters are directories
    "Welcome to SinisterOS!\n\nThis is a simple read-me file for the system.\nType 'help' to see available commands.\n",
    "KERNEL BINARY FILE - EXECUTABLE DATA",
    "This is the license file for SinisterOS.\nCopyright (c) 2023, Samarth\n\nAll rights reserved.\n",
    "#!/bin/sh\necho \"Shell starting...\"\n",
    "INIT PROCESS - SYSTEM INITIALIZATION",
    "", "", "", // Empty or unused clusters
    "root:x:0:0:root:/root:/bin/sh\nguest:x:1000:1000:Guest User:/home/guest:/bin/sh\n",
    "127.0.0.1 localhost\n::1 localhost\n"
};

// Array to simulate clusters (each points to directory entries)
#define MAX_CLUSTERS 16
static dir_entry_t* cluster_map[MAX_CLUSTERS];
static int cluster_sizes[MAX_CLUSTERS];

// Create a simple in-memory filesystem for testing
void fs_create_test_fs() {
    // Initialize clusters with directories
    cluster_map[0] = test_root_dir;
    cluster_sizes[0] = sizeof(test_root_dir) / sizeof(dir_entry_t);
    
    cluster_map[1] = test_bin_dir;
    cluster_sizes[1] = sizeof(test_bin_dir) / sizeof(dir_entry_t);
    
    cluster_map[2] = test_home_dir;
    cluster_sizes[2] = sizeof(test_home_dir) / sizeof(dir_entry_t);
    
    cluster_map[3] = test_etc_dir;
    cluster_sizes[3] = sizeof(test_etc_dir) / sizeof(dir_entry_t);
    
    // Other clusters would be file content in a real system
    for (int i = 4; i < MAX_CLUSTERS; i++) {
        cluster_map[i] = 0;
        cluster_sizes[i] = 0;
    }
}

// Initialize filesystem
int fs_init() {
    // Initialize filesystem structure with FAT16 defaults
    fs.type = FS_FAT16;
    fs.bytes_per_sector = 512;
    fs.sectors_per_cluster = 1;
    fs.reserved_sectors = 1;
    fs.fat_count = 2;
    fs.root_dir_entries = 512;
    fs.total_sectors = 2880;  // 1.44MB floppy
    fs.sectors_per_fat = 9;
    
    // Start at root directory
    fs.current_dir_cluster = 0;
    fs.current_path[0] = '/';
    fs.current_path[1] = '\0';
    
    // Create test filesystem
    fs_create_test_fs();
    
    kprint("Filesystem initialized: FAT16, 1.44MB\n");
    return 0;
}

// Convert 8.3 filename format to standard format
void format_filename(char* dest, const char* src) {
    int i = 0, j = 0;
    
    // Copy filename part (up to 8 chars, skipping spaces)
    while (i < 8 && src[i] != ' ') {
        dest[j++] = src[i++];
    }
    
    // Skip any remaining spaces in the filename part
    while (i < 8 && src[i] == ' ') {
        i++;
    }
    
    // If there's an extension, add a dot and the extension
    if (i >= 8 && src[8] != ' ') {
        dest[j++] = '.';
        
        // Copy extension (up to 3 chars)
        int ext_pos = 8;
        while (ext_pos < 11 && src[ext_pos] != ' ') {
            dest[j++] = src[ext_pos++];
        }
    }
    
    // Null terminate
    dest[j] = '\0';
}

// List contents of the current directory
void fs_list_directory() {
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    int files = 0, dirs = 0;
    unsigned long total_size = 0;
    char formatted_name[MAX_FILENAME];
    
    kprint("\nListing directory: ");
    kprint(fs.current_path);
    kprint("\n\n");
    
    // Column headers
    kprint("Name            Type    Size\n");
    kprint("-----------------------------\n");
    
    // List each entry
    for (int i = 0; i < dir_size; i++) {
        format_filename(formatted_name, dir[i].filename);
        
        kprint(formatted_name);
        
        // Pad to align columns
        int pad = 16 - str_length(formatted_name);
        for (int j = 0; j < pad; j++) {
            kprint(" ");
        }
        
        // Show type
        if (dir[i].attributes & ATTR_DIRECTORY) {
            kprint("DIR     ");
            dirs++;
        } else {
            kprint("FILE    ");
            files++;
            total_size += dir[i].size;
        }
        
        // Show size for files
        if (!(dir[i].attributes & ATTR_DIRECTORY)) {
            char size_str[16];
            int_to_ascii(dir[i].size, size_str);
            kprint(size_str);
        }
        
        kprint("\n");
    }
    
    // Summary
    kprint("\n");
    char count_str[16];
    int_to_ascii(files, count_str);
    kprint(count_str);
    kprint(" file(s), ");
    
    int_to_ascii(dirs, count_str);
    kprint(count_str);
    kprint(" dir(s), ");
    
    int_to_ascii(total_size, count_str);
    kprint(count_str);
    kprint(" bytes\n");
}

// Compare two strings, case-insensitive
int str_equal_nocase(const char* s1, const char* s2) {
    int i = 0;
    
    while (s1[i] != '\0' && s2[i] != '\0') {
        char c1 = s1[i];
        char c2 = s2[i];
        
        // Convert to uppercase for comparison
        if (c1 >= 'a' && c1 <= 'z') c1 = c1 - 32;
        if (c2 >= 'a' && c2 <= 'z') c2 = c2 - 32;
        
        if (c1 != c2) return 0;
        i++;
    }
    
    return (s1[i] == '\0' && s2[i] == '\0');
}

// Change current directory
int fs_change_directory(const char* dirname) {
    // Special case for root directory
    if (str_equal_nocase(dirname, "/")) {
        fs.current_dir_cluster = 0;
        fs.current_path[0] = '/';
        fs.current_path[1] = '\0';
        return 0;
    }
    
    // Special case for parent directory
    if (str_equal_nocase(dirname, "..")) {
        dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
        
        // Get the parent directory's cluster from ".." entry
        if (cluster_sizes[fs.current_dir_cluster] >= 2) {
            unsigned int parent_cluster = dir[1].cluster;
            fs.current_dir_cluster = parent_cluster;
            
            // Update path by removing last directory
            int i = str_length(fs.current_path) - 1;
            if (i > 0) {
                // Remove trailing slash if exists
                if (fs.current_path[i] == '/') {
                    i--;
                }
                
                // Find previous slash
                while (i > 0 && fs.current_path[i] != '/') {
                    i--;
                }
                
                // Keep the slash for root path
                if (i == 0) {
                    fs.current_path[1] = '\0';
                } else {
                    fs.current_path[i+1] = '\0';
                }
            }
            
            return 0;
        }
        return -1; // No parent directory found
    }
    
    // Special case for current directory
    if (str_equal_nocase(dirname, ".")) {
        return 0; // No change needed
    }
    
    // Look for the directory in current directory
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    for (int i = 0; i < dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, dirname)) {
            // Found matching entry - check if it's a directory
            if (dir[i].attributes & ATTR_DIRECTORY) {
                fs.current_dir_cluster = dir[i].cluster;
                
                // Update current path
                int path_len = str_length(fs.current_path);
                
                // Add trailing slash if not root directory
                if (path_len > 1 && fs.current_path[path_len-1] != '/') {
                    fs.current_path[path_len] = '/';
                    path_len++;
                }
                
                // Add new directory name
                int j = 0;
                while (formatted_name[j] != '\0' && path_len < MAX_PATH - 1) {
                    fs.current_path[path_len++] = formatted_name[j++];
                }
                fs.current_path[path_len] = '\0';
                
                return 0;
            } else {
                kprint("Error: Not a directory\n");
                return -1;
            }
        }
    }
    
    kprint("Error: Directory not found\n");
    return -1;
}

// Get current directory path
char* fs_get_current_path() {
    return fs.current_path;
}

// Display file content
int fs_cat_file(const char* filename) {
    // Look for the file in current directory
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    for (int i = 0; i < dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, filename)) {
            // Found matching entry - check if it's a file
            if (!(dir[i].attributes & ATTR_DIRECTORY)) {
                unsigned long cluster = dir[i].cluster;
                
                // If cluster is valid and within range
                if (cluster > 0 && cluster < MAX_CLUSTERS) {
                    // Output the simulated file content
                    kprint(test_file_content[cluster]);
                    kprint("\n");
                    return 0;
                } else {
                    kprint("Error: File has invalid data cluster\n");
                    return -1;
                }
            } else {
                kprint("Error: Not a file\n");
                return -1;
            }
        }
    }
    
    kprint("Error: File not found\n");
    return -1;
}

// Convert input filename to 8.3 format
void parse_filename(const char* input, char* output) {
    int i = 0, j = 0;
    int dot_pos = -1;
    
    // Find the dot position if exists
    while (input[i] != '\0') {
        if (input[i] == '.') {
            dot_pos = i;
        }
        i++;
    }
    
    // If no extension, just copy the name part (max 8 chars)
    if (dot_pos == -1) {
        i = 0;
        while (input[i] != '\0' && i < 8) {
            output[i] = input[i];
            i++;
        }
        
        // Pad with spaces
        while (i < 11) {
            output[i++] = ' ';
        }
    } else {
        // Copy filename part (up to 8 chars)
        i = 0;
        while (i < dot_pos && i < 8) {
            output[i] = input[i];
            i++;
        }
        
        // Pad name part with spaces
        while (i < 8) {
            output[i++] = ' ';
        }
        
        // Copy extension part (up to 3 chars)
        j = dot_pos + 1;
        while (input[j] != '\0' && i < 11) {
            output[i++] = input[j++];
        }
        
        // Pad extension part with spaces
        while (i < 11) {
            output[i++] = ' ';
        }
    }
    
    output[11] = '\0';
}

// Find a free cluster to store content
int find_free_cluster() {
    for (int i = 4; i < MAX_CLUSTERS; i++) {
        if (cluster_map[i] == 0 && cluster_sizes[i] == 0) {
            return i;
        }
    }
    return -1; // No free clusters
}

// Create or update a file
int fs_touch_file(const char* filename) {
    char formatted_name[MAX_FILENAME];
    char fat_name[12]; // 8.3 format + null terminator
    
    // Parse filename to 8.3 format
    parse_filename(filename, fat_name);
    
    // Check if file already exists
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    for (int i = 0; i < dir_size; i++) {
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, filename)) {
            // Found matching entry - update timestamp
            dir[i].time = 0x5000; // Fake timestamp for now
            dir[i].date = 0x4D58; // Fake date (2021-11-24)
            kprint("File updated: ");
            kprint(filename);
            kprint("\n");
            return 0;
        }
    }
    
    // File doesn't exist, create it
    // First check if directory has space
    if (dir_size >= 20) { // Arbitrary limit for test filesystem
        kprint("Error: Directory full\n");
        return -1;
    }
    
    // Allocate a new cluster
    int cluster = find_free_cluster();
    if (cluster == -1) {
        kprint("Error: No free space\n");
        return -1;
    }
    
    // Add new entry to directory
    dir_entry_t new_entry;
    for (int i = 0; i < 11; i++) {
        new_entry.filename[i] = fat_name[i];
    }
    new_entry.attributes = 0;
    new_entry.size = 0;
    new_entry.cluster = cluster;
    new_entry.time = 0x5000; // Fake timestamp
    new_entry.date = 0x4D58; // Fake date
    
    // Initialize empty file content
    test_file_content[cluster][0] = '\0';
    
    // Add to current directory
    dir[dir_size] = new_entry;
    cluster_sizes[fs.current_dir_cluster]++;
    
    kprint("File created: ");
    kprint(filename);
    kprint("\n");
    
    return 0;
}
