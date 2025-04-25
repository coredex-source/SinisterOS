#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

// System statistics tracking
typedef struct {
    unsigned int uptime_seconds;
    unsigned int memory_total;  // in KB
    unsigned int memory_used;   // in KB
    unsigned int disk_total;    // in KB
    unsigned int disk_used;     // in KB
    unsigned int cpu_usage;     // percentage (0-100)
    
    // Fields for CPU usage calculation
    unsigned long long last_tsc;
    unsigned long long idle_tsc;
    
    // Fields for timing calculations
    unsigned int ticks_per_second;
    unsigned int tick_counter;
} system_stats_t;

// Initialize system statistics
void init_system_stats();

// Update system statistics (call periodically)
void update_system_stats();

// Record idle time when CPU is halted
void record_idle_time();

// Get current system statistics
system_stats_t get_system_stats();

#endif /* SYSTEM_STATS_H */
