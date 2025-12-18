// Disk Management Simulator - Core API (C99)

#ifndef DISK_H
#define DISK_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define DISK_MAX_BLOCKS 512
#define DISK_MAX_LOGS 1024
#define DISK_LOG_MSG_LEN 128
#define DISK_PERSIST_PATH_LEN 256

// Block states
typedef enum {
    BLOCK_FREE = 0,
    BLOCK_USED = 1,
    BLOCK_BAD  = 2
} BlockState;

// File status
typedef enum {
    FILE_UNUSED = 0,
    FILE_ACTIVE = 1,
    FILE_DELETED = 2
} FileStatus;

typedef struct {
    int id;
    FileStatus status;
} FileMeta;

// Deleted file snapshot (for undelete_last)
typedef struct {
    int valid;
    int file_id;
    int count;
    int indices[DISK_MAX_BLOCKS];
} DeletedSnapshot;

// Global disk state (singleton)
typedef struct {
    int initialized;
    int blocks; // total blocks
    BlockState state[DISK_MAX_BLOCKS]; // block state
    int owner[DISK_MAX_BLOCKS];        // file id for used blocks, -1 otherwise
    FileMeta files[DISK_MAX_BLOCKS];   // simplistic file id registry
    int next_file_id;
    char logs[DISK_MAX_LOGS][DISK_LOG_MSG_LEN];
    int log_head; // ring buffer
    char persist_path[DISK_PERSIST_PATH_LEN];
    DeletedSnapshot last_deleted;
} Disk;

// Lifecycle
int disk_init(const char* persist_path);
int disk_load();
int disk_save();
int disk_reset();

// Allocation APIs
int disk_allocate_contiguous(int size, int *out_file_id);
int disk_allocate_fragmented(int size, int *out_file_id);
int disk_allocate_custom(int size, const char *strategy, int *out_file_id);

// File operations
int disk_logical_delete(int file_id);
int disk_undelete_last();
int disk_defragment();
int disk_mark_random_bad(int count);
int disk_repair();

// Stats and info
double disk_fragmentation_percent();
char* disk_get_state();   // JSON string, caller frees
char* disk_get_files();   // JSON string, caller frees
char* disk_get_stats();   // JSON string, caller frees
char* disk_get_logs();    // JSON string, caller frees

// Utility
int disk_total_free();
int disk_total_used();
int disk_total_bad();
int disk_file_exists(int file_id);
void disk_shutdown();

#ifdef __cplusplus
}
#endif

#endif // DISK_H
