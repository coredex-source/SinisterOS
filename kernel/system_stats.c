#include "system_stats.h"
#include "io.h"
#include "../include/sysinfo.h"

// Global system stats structure
static system_stats_t stats;

// Memory detection using BIOS function
// Returns memory size in kilobytes
unsigned int detect_memory_size() {
    // There's no standard way to detect memory in protected mode without BIOS
    // In a real system, we would get this from multiboot information or ACPI tables
    
    // For now, we'll use a hardcoded value based on the typical memory available
    // to our OS in the virtual environment - QEMU typically gives 128MB by default
    unsigned int memory_kb = 131072; // 128 MB in KB
    
    return memory_kb;
}

// Read CPU timestamp counter to measure CPU activity
unsigned long long read_tsc() {
    unsigned long low, high;
    __asm__ volatile("rdtsc" : "=a" (low), "=d" (high));
    return ((unsigned long long)high << 32) | low;
}

// Check for CPUID instruction support
int is_cpuid_supported() {
    int supported = 0;
    __asm__ volatile (
        "pushfl\n\t"
        "pop %%eax\n\t"
        "mov %%eax, %%ecx\n\t"
        "xor $0x200000, %%eax\n\t"
        "push %%eax\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "pop %%eax\n\t"
        "xor %%ecx, %%eax\n\t"
        "shr $21, %%eax\n\t"
        "and $1, %%eax\n\t"
        "mov %%eax, %0\n\t"
        "push %%ecx\n\t"
        "popfl"
        : "=r" (supported)
        : 
        : "eax", "ecx", "cc"
    );
    return supported;
}

// Detect CPU type using CPUID if available
int detect_cpu_type() {
    int result = CPU_GENERIC;
    
    // Check if CPUID instruction is available
    if (!is_cpuid_supported()) {
        // No CPUID, must be 386 or 486 without CPUID
        return CPU_386;
    }
    
    // Get CPU vendor ID
    char vendor[13] = {0};
    unsigned int eax, ebx, ecx, edx;
    
    __asm__ volatile (
        "cpuid" 
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    
    // Store vendor string (12 chars)
    ((unsigned int *)vendor)[0] = ebx;
    ((unsigned int *)vendor)[1] = edx;
    ((unsigned int *)vendor)[2] = ecx;
    vendor[12] = '\0';
    
    // Get CPU features and family information
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    
    // Extract family and model info
    unsigned int family = (eax >> 8) & 0xF;       // Family code
    unsigned int extended_family = (eax >> 20) & 0xFF;  // Extended family
    unsigned int model = (eax >> 4) & 0xF;       // Model number
    unsigned int extended_model = (eax >> 16) & 0xF;    // Extended model
    
    // Determine CPU type based on family
    if (family < 4) {
        result = CPU_386;
    } else if (family == 4) {
        result = CPU_486;
    } else if (family == 5) {
        result = CPU_PENTIUM;
    } else if (family >= 6) {
        result = CPU_PENTIUM_PRO;
    } else {
        result = CPU_UNKNOWN;
    }
    
    return result;
}

// Get CPU string based on detected CPU type
const char* get_cpu_type_string() {
    switch(stats.cpu_type) {
        case CPU_386:
            return "Intel 386";
        case CPU_486:
            return "Intel 486";
        case CPU_PENTIUM:
            return "Intel Pentium";
        case CPU_PENTIUM_PRO:
            return "Intel Pentium Pro or newer";
        case CPU_GENERIC:
            return "Generic x86";
        default:
            return "Unknown CPU";
    }
}

// Detect and store CPU information
void detect_cpu_info(char* buffer, int max_len) {
    // Use the existing CPU type detection
    const char* cpu_type = get_cpu_type_string();
    
    // Copy to the buffer with bounds checking
    int i = 0;
    while (cpu_type[i] != '\0' && i < max_len - 1) {
        buffer[i] = cpu_type[i];
        i++;
    }
    buffer[i] = '\0';
}

// Initialize system statistics
void init_system_stats() {
    // Initialize memory stats with detected values
    stats.memory_total = detect_memory_size();
    
    // Since we don't have a memory manager yet, we'll use a small initial value
    stats.memory_used = 1024;  // Our kernel + boot code using ~1MB
    
    // Disk stats - using known floppy disk size
    stats.disk_total = 1440;   // 1.44 MB floppy disk
    
    // Assuming our OS image uses about 200KB
    stats.disk_used = 200;
    
    // Initialize CPU tracking
    stats.last_tsc = read_tsc();
    stats.idle_tsc = 0;
    stats.cpu_usage = 0;
    
    // Initialize uptime
    stats.uptime_seconds = 0;
    stats.ticks_per_second = 0;
    stats.tick_counter = 0;
    
    // Detect CPU information
    stats.cpu_type = detect_cpu_type();
    detect_cpu_info(stats.cpu_model, sizeof(stats.cpu_model));
}

// Completely avoid 64-bit division by using a simpler approach
unsigned int safe_percentage(unsigned long long a, unsigned long long b) {
    // If b is 0, avoid division by zero
    if (b == 0) return 0;
    
    // Reduce both values to 32-bit by shifting
    while (a > 0xFFFFFFFF || b > 0xFFFFFFFF) {
        a >>= 1;
        b >>= 1;
    }
    
    // Now we're guaranteed to have 32-bit values
    unsigned int a32 = (unsigned int)a;
    unsigned int b32 = (unsigned int)b;
    
    // To avoid overflow when multiplying by 100, we'll check the magnitude
    if (a32 > 0x7FFFFFFF / 100) {
        // If a32 is large, further reduce both values to avoid overflow
        a32 >>= 8;
        b32 >>= 8;
    }
    
    // Now we can safely do the percentage calculation with 32-bit arithmetic
    if (b32 == 0) return 0; // Extra safety check
    
    return (a32 * 100) / b32;
}

// Update system statistics
void update_system_stats() {
    static unsigned int last_update_time = 0;
    static unsigned int update_counter = 0;
    
    // Increment counter every time this function is called
    update_counter++;
    
    // Update every ~10 calls to create a time-based update
    if (update_counter >= 10) {
        update_counter = 0;
        
        // Update uptime counter (roughly once per second)
        stats.uptime_seconds++;
        
        // Capture current timestamp for CPU usage calculation
        unsigned long long current_tsc = read_tsc();
        unsigned long long elapsed_tsc = current_tsc - stats.last_tsc;
        
        // Calculate actual CPU usage
        if (elapsed_tsc > 0) {
            stats.cpu_usage = 100 - safe_percentage(stats.idle_tsc, elapsed_tsc);
            
            // Ensure CPU usage is within reasonable range
            if (stats.cpu_usage > 100) {
                stats.cpu_usage = 100;
            }
        }
        
        // Reset counters for next measurement period
        stats.last_tsc = current_tsc;
        stats.idle_tsc = 0;
        
        // Update memory usage simulation
        if (stats.uptime_seconds % 5 == 0 && stats.memory_used < stats.memory_total / 2) {
            stats.memory_used += 4;  // Growing by 4KB chunks
        }
    }
}

// Record idle time when CPU is halted
void record_idle_time() {
    unsigned long long before = read_tsc();
    __asm__ volatile("hlt");
    unsigned long long after = read_tsc();
    
    // Add to idle time
    stats.idle_tsc += (after - before);
}

// Get current system statistics
system_stats_t get_system_stats() {
    return stats;
}
