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

/**
 * Compare two strings, case-insensitive
 */
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

// Helper function to check if a string ends with a specific extension
int has_extension(const char* filename, const char* extension) {
    int filename_len = str_length(filename);
    int extension_len = str_length(extension);
    
    // Check if the filename is long enough to contain the extension
    if (filename_len < extension_len) {
        return 0;
    }
    
    // Compare the end of filename with the extension
    const char* filename_ext = filename + (filename_len - extension_len);
    int i = 0;
    while (extension[i] != '\0') {
        char c1 = filename_ext[i];
        char c2 = extension[i];
        
        // Case insensitive comparison
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32; // Convert to lowercase
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32; // Convert to lowercase
        
        if (c1 != c2) {
            return 0;
        }
        i++;
    }
    
    return 1;
}

// Delete a file from the current directory
int fs_delete_file(const char* filename) {
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    // Look for the file in current directory
    for (int i = 0; i < dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, filename)) {
            // Found matching entry - check if it's a file
            if (!(dir[i].attributes & ATTR_DIRECTORY)) {
                unsigned long cluster = dir[i].cluster;
                
                // Clear the file content if cluster is valid
                if (cluster > 0 && cluster < MAX_CLUSTERS) {
                    // Clear the cluster
                    cluster_map[cluster] = 0;
                    cluster_sizes[cluster] = 0;
                    test_file_content[cluster][0] = '\0'; // Clear content
                }
                
                // Remove entry by shifting remaining entries
                for (int j = i; j < dir_size - 1; j++) {
                    dir[j] = dir[j + 1];
                }
                
                // Update directory size
                cluster_sizes[fs.current_dir_cluster]--;
                
                kprint("File deleted: ");
                kprint(filename);
                kprint("\n");
                return 0;
            } else {
                kprint("Error: Cannot delete a directory with -f option. Use -dir or -f-dir instead.\n");
                return -1;
            }
        }
    }
    
    kprint("Error: File not found: ");
    kprint(filename);
    kprint("\n");
    return -1;
}

// Check if directory is empty
int is_directory_empty(int cluster) {
    // Directories always have at least "." and ".." entries
    return (cluster_sizes[cluster] <= 2);
}

// Delete a directory
int fs_delete_directory(const char* dirname, int force) {
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    // Prevent deletion of "." and ".." directories
    if (str_equal_nocase(dirname, ".") || str_equal_nocase(dirname, "..")) {
        kprint("Error: Cannot delete '.' or '..' directories\n");
        return -1;
    }
    
    // Look for the directory in current directory
    for (int i = 0; i < dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, dirname)) {
            // Found matching entry - check if it's a directory
            if (dir[i].attributes & ATTR_DIRECTORY) {
                unsigned int target_cluster = dir[i].cluster;
                
                // Check if directory is empty or force flag is set
                if (!force && !is_directory_empty(target_cluster)) {
                    kprint("Error: Directory not empty. Use -f-dir to force delete.\n");
                    return -1;
                }
                
                // If force is enabled, delete all files in the directory
                if (force && !is_directory_empty(target_cluster)) {
                    dir_entry_t* target_dir = cluster_map[target_cluster];
                    int target_size = cluster_sizes[target_cluster];
                    
                    // Delete all entries except "." and ".."
                    for (int j = 2; j < target_size; j++) {
                        unsigned long file_cluster = target_dir[j].cluster;
                        
                        // Clear file data if it's a file
                        if (!(target_dir[j].attributes & ATTR_DIRECTORY) && 
                            file_cluster > 0 && file_cluster < MAX_CLUSTERS) {
                            // Clear the cluster
                            cluster_map[file_cluster] = 0;
                            cluster_sizes[file_cluster] = 0;
                            test_file_content[file_cluster][0] = '\0';
                        }
                        // We're not handling recursive directory deletion for simplicity
                    }
                }
                
                // Clear the directory's cluster
                cluster_map[target_cluster] = 0;
                cluster_sizes[target_cluster] = 0;
                
                // Remove entry by shifting remaining entries
                for (int j = i; j < dir_size - 1; j++) {
                    dir[j] = dir[j + 1];
                }
                
                // Update directory size
                cluster_sizes[fs.current_dir_cluster]--;
                
                kprint("Directory deleted: ");
                kprint(dirname);
                kprint("\n");
                return 0;
            } else {
                kprint("Error: Not a directory: ");
                kprint(dirname);
                kprint("\n");
                return -1;
            }
        }
    }
    
    kprint("Error: Directory not found: ");
    kprint(dirname);
    kprint("\n");
    return -1;
}

// Delete all files in current directory
int fs_delete_all_files() {
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    int deleted_count = 0;
    
    // Scan through entries and delete files
    // We iterate backwards to handle the shifting of entries correctly
    for (int i = dir_size - 1; i >= 0; i--) {
        // Skip directories (including "." and "..")
        if (dir[i].attributes & ATTR_DIRECTORY) {
            continue;
        }
        
        // Found a file, delete it
        unsigned long cluster = dir[i].cluster;
        
        // Clear the file content if cluster is valid
        if (cluster > 0 && cluster < MAX_CLUSTERS) {
            // Clear the cluster
            cluster_map[cluster] = 0;
            cluster_sizes[cluster] = 0;
            test_file_content[cluster][0] = '\0'; // Clear content
        }
        
        // Remove entry by shifting remaining entries
        for (int j = i; j < dir_size - 1; j++) {
            dir[j] = dir[j + 1];
        }
        
        // Update directory size
        cluster_sizes[fs.current_dir_cluster]--;
        dir_size--;
        deleted_count++;
    }
    
    char count_str[16];
    int_to_ascii(deleted_count, count_str);
    kprint(count_str);
    kprint(" file(s) deleted\n");
    
    return 0;
}

// Delete all files with a specific extension
int fs_delete_files_by_extension(const char* extension) {
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    int deleted_count = 0;
    
    // Scan through entries and delete files with matching extension
    // We iterate backwards to handle the shifting of entries correctly
    for (int i = dir_size - 1; i >= 0; i--) {
        // Skip directories
        if (dir[i].attributes & ATTR_DIRECTORY) {
            continue;
        }
        
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        // Check if file has the specified extension
        if (has_extension(formatted_name, extension)) {
            // Found matching file, delete it
            unsigned long cluster = dir[i].cluster;
            
            // Clear the file content if cluster is valid
            if (cluster > 0 && cluster < MAX_CLUSTERS) {
                // Clear the cluster
                cluster_map[cluster] = 0;
                cluster_sizes[cluster] = 0;
                test_file_content[cluster][0] = '\0'; // Clear content
            }
            
            // Remove entry by shifting remaining entries
            for (int j = i; j < dir_size - 1; j++) {
                dir[j] = dir[j + 1];
            }
            
            // Update directory size
            cluster_sizes[fs.current_dir_cluster]--;
            dir_size--;
            deleted_count++;
        }
    }
    
    char count_str[16];
    int_to_ascii(deleted_count, count_str);
    kprint(count_str);
    kprint(" file(s) with extension ");
    kprint(extension);
    kprint(" deleted\n");
    
    return 0;
}

// Extract parent path from a full path
char* get_parent_path(const char* path, char* buffer) {
    int len = str_length(path);
    int i;
    
    // Find the last slash
    for (i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            break;
        }
    }
    
    // If no slash found, use root directory
    if (i < 0) {
        buffer[0] = '/';
        buffer[1] = '\0';
        return buffer;
    }
    
    // Copy up to the last slash
    int j;
    for (j = 0; j <= i; j++) {
        buffer[j] = path[j];
    }
    buffer[j] = '\0';
    
    // Handle root directory case
    if (buffer[0] == '\0') {
        buffer[0] = '/';
        buffer[1] = '\0';
    }
    
    return buffer;
}

// Extract filename from a full path
char* get_filename(const char* path, char* buffer) {
    int len = str_length(path);
    int i;
    
    // Find the last slash
    for (i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            break;
        }
    }
    
    // Copy after the last slash
    int j;
    for (j = 0; j < len - i - 1; j++) {
        buffer[j] = path[i + j + 1];
    }
    buffer[j] = '\0';
    
    return buffer;
}

// Resolve a path to find its cluster
int resolve_path(const char* path, dir_entry_t** entry) {
    // Save current directory state
    unsigned int saved_cluster = fs.current_dir_cluster;
    char saved_path[MAX_PATH];
    str_copy(saved_path, fs.current_path);
    
    // Special case for root directory
    if (str_equal_nocase(path, "/")) {
        *entry = NULL; // No specific entry for root directory
        return 0;      // Root directory cluster
    }
    
    // Start from root if path begins with /
    if (path[0] == '/') {
        fs.current_dir_cluster = 0;
        fs.current_path[0] = '/';
        fs.current_path[1] = '\0';
    }
    
    // Create a copy of the path we can modify
    char path_copy[MAX_PATH];
    str_copy(path_copy, path);
    
    // Skip leading slash for absolute paths
    char* current = path_copy;
    if (current[0] == '/') {
        current++;
    }
    
    // Parse and navigate through path components
    char component[MAX_FILENAME];
    int i = 0, j = 0;
    
    while (current[i] != '\0') {
        // Extract next path component
        j = 0;
        while (current[i] != '/' && current[i] != '\0') {
            component[j++] = current[i++];
        }
        component[j] = '\0';
        
        // Skip multiple slashes
        while (current[i] == '/') {
            i++;
        }
        
        // Skip empty components
        if (component[0] == '\0') {
            continue;
        }
        
        // Navigate to this component
        dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
        int dir_size = cluster_sizes[fs.current_dir_cluster];
        int found = 0;
        
        for (j = 0; j < dir_size; j++) {
            char formatted_name[MAX_FILENAME];
            format_filename(formatted_name, dir[j].filename);
            
            if (str_equal_nocase(formatted_name, component)) {
                if (current[i] == '\0') {
                    // This is the final component - return it
                    *entry = &dir[j];
                    int result = fs.current_dir_cluster;
                    
                    // Restore original directory state
                    fs.current_dir_cluster = saved_cluster;
                    str_copy(fs.current_path, saved_path);
                    
                    return result;
                } else if (dir[j].attributes & ATTR_DIRECTORY) {
                    // Continue navigating into this directory
                    fs.current_dir_cluster = dir[j].cluster;
                    found = 1;
                    break;
                } else {
                    // Can't navigate into a file
                    fs.current_dir_cluster = saved_cluster;
                    str_copy(fs.current_path, saved_path);
                    return -1;
                }
            }
        }
        
        if (!found) {
            // Component not found
            fs.current_dir_cluster = saved_cluster;
            str_copy(fs.current_path, saved_path);
            return -1;
        }
    }
    
    // If we get here, the path ended with a slash, indicating a directory
    *entry = NULL;
    int result = fs.current_dir_cluster;
    
    // Restore original directory state
    fs.current_dir_cluster = saved_cluster;
    str_copy(fs.current_path, saved_path);
    
    return result;
}

// Check if a path exists
int path_exists(const char* path) {
    dir_entry_t* entry;
    return (resolve_path(path, &entry) >= 0);
}

// Check if path points to a directory
int is_directory(const char* path) {
    dir_entry_t* entry;
    int cluster = resolve_path(path, &entry);
    
    if (cluster < 0) {
        return 0; // Path doesn't exist
    }
    
    if (entry == NULL) {
        return 1; // Root directory or ended with slash
    }
    
    return (entry->attributes & ATTR_DIRECTORY) != 0;
}

// Create a directory
int fs_create_directory(const char* dirname) {
    // Check if the name is valid
    if (dirname == NULL || dirname[0] == '\0') {
        kprint("Error: Invalid directory name\n");
        return -1;
    }
    
    // Extract parent path and directory name
    char parent_path[MAX_PATH];
    char dir_name[MAX_FILENAME];
    
    if (dirname[0] == '/') {
        // Absolute path
        get_parent_path(dirname, parent_path);
        get_filename(dirname, dir_name);
    } else {
        // Relative path - check for subdirectories
        int has_slash = 0;
        for (int i = 0; dirname[i] != '\0'; i++) {
            if (dirname[i] == '/') {
                has_slash = 1;
                break;
            }
        }
        
        if (has_slash) {
            get_parent_path(dirname, parent_path);
            get_filename(dirname, dir_name);
        } else {
            // Simple name in current directory
            str_copy(parent_path, fs.current_path);
            str_copy(dir_name, dirname);
        }
    }
    
    // Save current state
    unsigned int saved_cluster = fs.current_dir_cluster;
    char saved_path[MAX_PATH];
    str_copy(saved_path, fs.current_path);
    
    // Navigate to parent path
    if (fs_change_directory(parent_path) != 0) {
        kprint("Error: Parent directory not found\n");
        return -1;
    }
    
    // Check if directory already exists
    dir_entry_t* dir = cluster_map[fs.current_dir_cluster];
    int dir_size = cluster_sizes[fs.current_dir_cluster];
    
    for (int i = 0; i < dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dir[i].filename);
        
        if (str_equal_nocase(formatted_name, dir_name)) {
            kprint("Error: Directory already exists\n");
            
            // Restore original state
            fs.current_dir_cluster = saved_cluster;
            str_copy(fs.current_path, saved_path);
            
            return -1;
        }
    }
    
    // Allocate a new cluster for the directory
    int new_cluster = find_free_cluster();
    if (new_cluster == -1) {
        kprint("Error: No free space\n");
        
        // Restore original state
        fs.current_dir_cluster = saved_cluster;
        str_copy(fs.current_path, saved_path);
        
        return -1;
    }
    
    // Create the directory entry
    dir_entry_t new_dir;
    char fat_name[12]; // 8.3 format + null terminator
    
    // Parse and pad the name
    parse_filename(dir_name, fat_name);
    for (int i = 0; i < 11; i++) {
        new_dir.filename[i] = fat_name[i];
    }
    
    new_dir.attributes = ATTR_DIRECTORY;
    new_dir.size = 0;
    new_dir.cluster = new_cluster;
    new_dir.time = 0x5000; // Fake timestamp
    new_dir.date = 0x4D58; // Fake date
    
    // Add to current directory
    dir[dir_size] = new_dir;
    cluster_sizes[fs.current_dir_cluster]++;
    
    // Create "." and ".." entries in the new directory
    dir_entry_t dot_entries[2];
    
    // "." entry - points to this directory
    dot_entries[0].filename[0] = '.';
    for (int i = 1; i < 11; i++) {
        dot_entries[0].filename[i] = ' ';
    }
    dot_entries[0].attributes = ATTR_DIRECTORY;
    dot_entries[0].size = 0;
    dot_entries[0].cluster = new_cluster;
    dot_entries[0].time = 0x5000;
    dot_entries[0].date = 0x4D58;
    
    // ".." entry - points to parent directory
    dot_entries[1].filename[0] = '.';
    dot_entries[1].filename[1] = '.';
    for (int i = 2; i < 11; i++) {
        dot_entries[1].filename[i] = ' ';
    }
    dot_entries[1].attributes = ATTR_DIRECTORY;
    dot_entries[1].size = 0;
    dot_entries[1].cluster = fs.current_dir_cluster;
    dot_entries[1].time = 0x5000;
    dot_entries[1].date = 0x4D58;
    
    // Set up the new directory's cluster
    cluster_map[new_cluster] = dot_entries;
    cluster_sizes[new_cluster] = 2;
    
    kprint("Directory created: ");
    kprint(dir_name);
    kprint("\n");
    
    // Restore original state
    fs.current_dir_cluster = saved_cluster;
    str_copy(fs.current_path, saved_path);
    
    return 0;
}

// Copy a file from source to destination
int fs_copy_file(const char* src_path, const char* dest_path) {
    // Find the source file
    dir_entry_t* src_entry;
    int src_dir_cluster = resolve_path(src_path, &src_entry);
    
    if (src_dir_cluster < 0 || src_entry == NULL) {
        kprint("Error: Source file not found\n");
        return -1;
    }
    
    // Check that source is a file, not a directory
    if (src_entry->attributes & ATTR_DIRECTORY) {
        kprint("Error: Source is a directory, not a file\n");
        return -1;
    }
    
    // Extract destination parent path and filename
    char dest_parent[MAX_PATH];
    char dest_name[MAX_FILENAME];
    
    get_parent_path(dest_path, dest_parent);
    get_filename(dest_path, dest_name);
    
    // Check if destination parent directory exists
    dir_entry_t* dest_parent_entry;
    int dest_parent_cluster = resolve_path(dest_parent, &dest_parent_entry);
    
    if (dest_parent_cluster < 0) {
        kprint("Error: Destination directory not found\n");
        return -1;
    }
    
    // Save current state
    unsigned int saved_cluster = fs.current_dir_cluster;
    char saved_path[MAX_PATH];
    str_copy(saved_path, fs.current_path);
    
    // Navigate to destination directory
    fs.current_dir_cluster = dest_parent_cluster;
    
    // Check if destination already exists
    dir_entry_t* dest_dir = cluster_map[dest_parent_cluster];
    int dest_dir_size = cluster_sizes[dest_parent_cluster];
    
    for (int i = 0; i < dest_dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dest_dir[i].filename);
        
        if (str_equal_nocase(formatted_name, dest_name)) {
            kprint("Error: Destination file already exists\n");
            
            // Restore original state
            fs.current_dir_cluster = saved_cluster;
            str_copy(fs.current_path, saved_path);
            
            return -1;
        }
    }
    
    // Allocate a new cluster for the destination file
    int dest_cluster = find_free_cluster();
    if (dest_cluster == -1) {
        kprint("Error: No free space\n");
        
        // Restore original state
        fs.current_dir_cluster = saved_cluster;
        str_copy(fs.current_path, saved_path);
        
        return -1;
    }
    
    // Copy file content
    str_copy(test_file_content[dest_cluster], test_file_content[src_entry->cluster]);
    
    // Create the destination entry
    dir_entry_t new_entry;
    char fat_name[12]; // 8.3 format + null terminator
    
    parse_filename(dest_name, fat_name);
    for (int i = 0; i < 11; i++) {
        new_entry.filename[i] = fat_name[i];
    }
    
    new_entry.attributes = src_entry->attributes;
    new_entry.size = src_entry->size;
    new_entry.cluster = dest_cluster;
    new_entry.time = 0x5000; // Current time (fake)
    new_entry.date = 0x4D58; // Current date (fake)
    
    // Add to destination directory
    dest_dir[dest_dir_size] = new_entry;
    cluster_sizes[dest_parent_cluster]++;
    
    kprint("File copied: ");
    kprint(src_path);
    kprint(" to ");
    kprint(dest_path);
    kprint("\n");
    
    // Restore original state
    fs.current_dir_cluster = saved_cluster;
    str_copy(fs.current_path, saved_path);
    
    return 0;
}

// Move a file or directory (rename or relocate)
int fs_move(const char* src_path, const char* dest_path) {
    // Find the source file or directory
    dir_entry_t* src_entry;
    int src_dir_cluster = resolve_path(src_path, &src_entry);
    
    if (src_dir_cluster < 0 || src_entry == NULL) {
        kprint("Error: Source not found\n");
        return -1;
    }
    
    // Extract source directory and name
    char src_parent[MAX_PATH];
    char src_name[MAX_FILENAME];
    
    get_parent_path(src_path, src_parent);
    get_filename(src_path, src_name);
    
    // Extract destination parent path and filename
    char dest_parent[MAX_PATH];
    char dest_name[MAX_FILENAME];
    
    get_parent_path(dest_path, dest_parent);
    get_filename(dest_path, dest_name);
    
    // Check if destination parent directory exists
    dir_entry_t* dest_parent_entry;
    int dest_parent_cluster = resolve_path(dest_parent, &dest_parent_entry);
    
    if (dest_parent_cluster < 0) {
        kprint("Error: Destination directory not found\n");
        return -1;
    }
    
    // Check if destination already exists
    dir_entry_t* dest_dir = cluster_map[dest_parent_cluster];
    int dest_dir_size = cluster_sizes[dest_parent_cluster];
    
    for (int i = 0; i < dest_dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, dest_dir[i].filename);
        
        if (str_equal_nocase(formatted_name, dest_name)) {
            kprint("Error: Destination already exists\n");
            return -1;
        }
    }
    
    // Create the destination entry as a copy of source
    dir_entry_t new_entry = *src_entry;
    char fat_name[12]; // 8.3 format + null terminator
    
    parse_filename(dest_name, fat_name);
    for (int i = 0; i < 11; i++) {
        new_entry.filename[i] = fat_name[i];
    }
    
    // Update ".." entry if moving a directory
    if (src_entry->attributes & ATTR_DIRECTORY && src_entry->cluster > 0) {
        // Update the parent reference in the ".." entry of the directory
        dir_entry_t* dir_entries = cluster_map[src_entry->cluster];
        if (cluster_sizes[src_entry->cluster] >= 2) {
            dir_entries[1].cluster = dest_parent_cluster;
        }
    }
    
    // Add to destination directory
    dest_dir[dest_dir_size] = new_entry;
    cluster_sizes[dest_parent_cluster]++;
    
    // Remove from source directory
    dir_entry_t* src_dir = cluster_map[src_dir_cluster];
    int src_dir_size = cluster_sizes[src_dir_cluster];
    
    // Find the source entry in its parent directory
    for (int i = 0; i < src_dir_size; i++) {
        char formatted_name[MAX_FILENAME];
        format_filename(formatted_name, src_dir[i].filename);
        
        if (str_equal_nocase(formatted_name, src_name)) {
            // Remove by shifting subsequent entries
            for (int j = i; j < src_dir_size - 1; j++) {
                src_dir[j] = src_dir[j + 1];
            }
            
            // Update directory size
            cluster_sizes[src_dir_cluster]--;
            break;
        }
    }
    
    kprint("Moved: ");
    kprint(src_path);
    kprint(" to ");
    kprint(dest_path);
    kprint("\n");
    
    return 0;
}
