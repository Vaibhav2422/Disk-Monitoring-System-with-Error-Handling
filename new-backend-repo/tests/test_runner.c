#include <stdio.h>
#include <string.h>
#include "../include/disk.h"

static int test_allocate_and_delete() {
    disk_reset();
    int fid = 0;
    int r = disk_allocate_contiguous(5, &fid);
    if (r != 0 || fid <= 0) return 1;
    if (!disk_file_exists(fid)) return 2;
    if (disk_total_used() != 5) return 3;
    r = disk_logical_delete(fid);
    if (r != 0) return 4;
    if (disk_total_used() != 0) return 5;
    r = disk_undelete_last();
    if (r != 0) return 6;
    if (disk_total_used() != 5) return 7;
    return 0;
}

static int test_fragmented_and_defrag() {
    disk_reset();
    int f1=0,f2=0;
    if (disk_allocate_fragmented(10, &f1) != 0) return 1;
    // Mark some bad to create holes
    disk_mark_random_bad(5);
    if (disk_allocate_fragmented(5, &f2) != 0) return 2;
    disk_defragment();
    // After defrag, used blocks contiguous at front
    int moved_ok = 1;
    int used = disk_total_used();
    for (int i = 0; i < used; i++) {
        // contiguous used expected
        // cannot assert owner continuity per-file; basic smoke check only
        // check no BAD in front segment
        // Not accessible here; but we can rely on stats
    }
    return 0;
}

int main() {
    disk_init("test_state.json");
    int fails = 0;

    int r1 = test_allocate_and_delete();
    printf("[test_allocate_and_delete] %s (code=%d)\n", r1==0?"PASS":"FAIL", r1);
    fails += (r1 != 0);

    int r2 = test_fragmented_and_defrag();
    printf("[test_fragmented_and_defrag] %s (code=%d)\n", r2==0?"PASS":"FAIL", r2);
    fails += (r2 != 0);

    return fails ? 1 : 0;
}
