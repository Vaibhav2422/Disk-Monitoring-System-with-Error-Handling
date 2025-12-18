#ifndef SYSTEM_DISK_H
#define SYSTEM_DISK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned long long total_space;  // Total space in bytes
    unsigned long long free_space;   // Free space in bytes
    unsigned long long used_space;   // Used space in bytes
    double total_gb;                 // Total space in GB
    double free_gb;                  // Free space in GB
    double used_gb;                  // Used space in GB
    double used_percentage;          // Used space percentage
    int bad_sectors;                 // Number of bad sectors (if available)
    char path[256];                  // Path to the disk
} SystemDiskInfo;

// Get system disk information
int get_system_disk_info(SystemDiskInfo* info);

// Create a file on the actual disk
int create_file_on_disk(const char* filename, size_t size);

// Delete a file from the actual disk
int delete_file_from_disk(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_DISK_H