#ifndef SYSINFO_H
#define SYSINFO_H

// System version information
#define OS_NAME          "SinisterOS"
#define OS_VERSION       "0.3.1"
#define KERNEL_NAME      "SinisterKernel"
#define KERNEL_VERSION   "0.0.2"
#define BUILD_DATE       "260425"
#define BUILD_ARCH       "32bit"
#define SHELL_NAME       "SinisterShell"
#define SHELL_VERSION    "0.0.1"
#define DISPLAY_MODE     "80x25 Text Mode"

// System capability flags
#define HAS_NETWORK      0
#define HAS_GRAPHICS     0
#define HAS_SOUND        0
#define HAS_USB          0
#define HAS_FILESYSTEM   1  // Updated now that we've added filesystem support

// CPU types - determined at runtime
#define CPU_GENERIC      0
#define CPU_386          1
#define CPU_486          2
#define CPU_PENTIUM      3
#define CPU_PENTIUM_PRO  4
#define CPU_UNKNOWN      255

// Function to detect CPU type - implemented in system_stats.c
int detect_cpu_type();
const char* get_cpu_type_string();

#endif /* SYSINFO_H */
