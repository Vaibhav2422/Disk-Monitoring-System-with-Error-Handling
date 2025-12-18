#define _POSIX_C_SOURCE 200809L
#include "system_disk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

int get_system_disk_info(SystemDiskInfo* info) {
    if (!info) return -1;
    
#ifdef _WIN32
    // On Windows, get disk space information
    ULARGE_INTEGER free_bytes_available;
    ULARGE_INTEGER total_number_of_bytes;
    ULARGE_INTEGER total_number_of_free_bytes;
    
    if (GetDiskFreeSpaceEx(NULL, 
                          &free_bytes_available, 
                          &total_number_of_bytes, 
                          &total_number_of_free_bytes)) {
        info->total_space = (unsigned long long)total_number_of_bytes.QuadPart;
        info->free_space = (unsigned long long)free_bytes_available.QuadPart;
        info->used_space = info->total_space - info->free_space;
        
        // Convert to GB for display
        info->total_gb = (double)info->total_space / (1024 * 1024 * 1024);
        info->free_gb = (double)info->free_space / (1024 * 1024 * 1024);
        info->used_gb = (double)info->used_space / (1024 * 1024 * 1024);
        info->used_percentage = (info->total_space > 0) ? 
                               ((double)info->used_space / (double)info->total_space) * 100.0 : 0.0;
        
        strncpy(info->path, "C:\\", sizeof(info->path) - 1);
        info->path[sizeof(info->path) - 1] = '\0';
        
        // Note: Bad sector information is not easily accessible without admin privileges
        // This would require more complex Windows API calls
        info->bad_sectors = 0;
        
        return 0;
    }
#else
    // On Unix-like systems
    struct statvfs buf;
    if (statvfs("/", &buf) == 0) {
        info->total_space = (unsigned long long)buf.f_blocks * (unsigned long long)buf.f_frsize;
        info->free_space = (unsigned long long)buf.f_bavail * (unsigned long long)buf.f_frsize;
        info->used_space = info->total_space - info->free_space;
        
        // Convert to GB for display
        info->total_gb = (double)info->total_space / (1024 * 1024 * 1024);
        info->free_gb = (double)info->free_space / (1024 * 1024 * 1024);
        info->used_gb = (double)info->used_space / (1024 * 1024 * 1024);
        info->used_percentage = (info->total_space > 0) ? 
                               ((double)info->used_space / (double)info->total_space) * 100.0 : 0.0;
        
        strncpy(info->path, "/", sizeof(info->path) - 1);
        info->path[sizeof(info->path) - 1] = '\0';
        info->bad_sectors = 0; // Not easily accessible
        
        return 0;
    }
#endif
    
    return -1;
}

int create_file_on_disk(const char* filename, size_t size) {
    if (!filename) return -1;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    // Create a file of the specified size
    if (size > 0) {
        // Seek to size-1 and write a byte to allocate the space
        if (fseek(file, size - 1, SEEK_SET) == 0) {
            fputc(0, file);
        }
    }
    
    fclose(file);
    return 0;
}

int delete_file_from_disk(const char* filename) {
    if (!filename) return -1;
    
#ifdef _WIN32
    return DeleteFile(filename) ? 0 : -1;
#else
    return remove(filename);
#endif
}