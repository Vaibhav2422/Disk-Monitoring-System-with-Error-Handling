#define _POSIX_C_SOURCE 200809L
#include "../include/disk.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
static Disk G;

// Internal helpers
static void logf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char line[DISK_LOG_MSG_LEN];
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    int idx = G.log_head % DISK_MAX_LOGS;
    strncpy(G.logs[idx], line, DISK_LOG_MSG_LEN - 1);
    G.logs[idx][DISK_LOG_MSG_LEN - 1] = '\0';
    G.log_head++;
}

static void clear_disk() {
    G.blocks = DISK_MAX_BLOCKS;
    for (int i = 0; i < G.blocks; i++) {
        G.state[i] = BLOCK_FREE;
        G.owner[i] = -1;
    }
    for (int i = 0; i < DISK_MAX_BLOCKS; i++) {
        G.files[i].id = 0;
        G.files[i].status = FILE_UNUSED;
    }
    G.next_file_id = 1;
    G.log_head = 0;
    G.last_deleted.valid = 0;
}

static void ensure_initialized() {
    if (!G.initialized) {
        memset(&G, 0, sizeof(G));
        clear_disk();
        strncpy(G.persist_path, "disk_state.json", sizeof(G.persist_path)-1);
        G.initialized = 1;
    }
}

int disk_init(const char* persist_path) {
    ensure_initialized();
    if (persist_path && strlen(persist_path) < sizeof(G.persist_path)) {
        strncpy(G.persist_path, persist_path, sizeof(G.persist_path)-1);
    }
    utils_srand();
    // Try load existing
    if (disk_load() != 0) {
        // fresh disk
        clear_disk();
    }
    logf("disk_init: blocks=%d persist='%s'", G.blocks, G.persist_path);
    return 0;
}

int disk_save() {
    ensure_initialized();
    StrBuf sb;
    if (sb_init(&sb, 4096) != 0) return -1;
    sb_append(&sb, "{\n");
    sb_appendf(&sb, "  \"blocks\": %d,\n", G.blocks);
    sb_append(&sb, "  \"state\": [");
    for (int i = 0; i < G.blocks; i++) {
        sb_appendf(&sb, "%d", (int)G.state[i]);
        if (i + 1 < G.blocks) sb_append(&sb, ",");
    }
    sb_append(&sb, "],\n  \"owner\": [");
    for (int i = 0; i < G.blocks; i++) {
        sb_appendf(&sb, "%d", G.owner[i]);
        if (i + 1 < G.blocks) sb_append(&sb, ",");
    }
    sb_append(&sb, "],\n  \"files\": [");
    int first = 1;
    for (int i = 0; i < DISK_MAX_BLOCKS; i++) {
        if (G.files[i].status != FILE_UNUSED) {
            if (!first) sb_append(&sb, ",");
            first = 0;
            sb_appendf(&sb, "{\"id\":%d,\"status\":%d}", G.files[i].id, (int)G.files[i].status);
        }
    }
    sb_append(&sb, "],\n  \"next_file_id\": ");
    sb_appendf(&sb, "%d,\n", G.next_file_id);
    sb_append(&sb, "  \"logs\": [");
    int count = G.log_head < DISK_MAX_LOGS ? G.log_head : DISK_MAX_LOGS;
    for (int i = 0; i < count; i++) {
        int idx = (G.log_head - count + i);
        if (idx < 0) idx = 0;
        idx = idx % DISK_MAX_LOGS;
        sb_appendf(&sb, "\"%s\"", G.logs[idx]);
        if (i + 1 < count) sb_append(&sb, ",");
    }
    sb_append(&sb, "]\n}\n");
    char* content = sb_take(&sb);
    int r = write_text_file_atomic(G.persist_path, content);
    free(content);
    return r;
}

int disk_load() {
    ensure_initialized();
    if (!file_exists(G.persist_path)) return -1;
    StrBuf in;
    if (read_text_file(G.persist_path, &in) != 0) return -1;
    // Very naive: assumes same format as save(); no full JSON parsing
    // Reset then load arrays by scanning tokens
    clear_disk();
    int blocks = 0;
    parse_json_int(in.buf, "blocks", &blocks);
    if (blocks > 0 && blocks <= DISK_MAX_BLOCKS) G.blocks = blocks;

    // Parse state array
    const char* ps = strstr(in.buf, "\"state\"");
    if (ps) {
        ps = strchr(ps, '[');
        const char* pe = strchr(ps, ']');
        if (ps && pe && pe > ps) {
            int idx = 0;
            const char* p = ps + 1;
            while (p < pe && idx < G.blocks) {
                while (*p == ' ' || *p == ',') p++;
                if (p >= pe) break;
                int val = (int)strtol(p, (char**)&p, 10);
                if (val < 0 || val > 2) val = 0;
                G.state[idx++] = (BlockState)val;
                while (*p != ',' && p < pe && *p != ']') p++;
            }
        }
    }
    // Parse owner array
    const char* po = strstr(in.buf, "\"owner\"");
    if (po) {
        po = strchr(po, '[');
        const char* pe = strchr(po, ']');
        if (po && pe && pe > po) {
            int idx = 0;
            const char* p = po + 1;
            while (p < pe && idx < G.blocks) {
                while (*p == ' ' || *p == ',') p++;
                if (p >= pe) break;
                int val = (int)strtol(p, (char**)&p, 10);
                G.owner[idx++] = val;
                while (*p != ',' && p < pe && *p != ']') p++;
            }
        }
    }
    // Parse next_file_id
    int nfid = 0;
    if (parse_json_int(in.buf, "next_file_id", &nfid) == 0 && nfid > 0) {
        G.next_file_id = nfid;
    } else {
        // fallback: derive from max owner
        int maxid = 0;
        for (int i = 0; i < G.blocks; i++) if (G.owner[i] > maxid) maxid = G.owner[i];
        G.next_file_id = maxid + 1;
    }
    // Rebuild files table
    for (int i = 0; i < G.blocks; i++) {
        if (G.owner[i] > 0 && G.state[i] == BLOCK_USED) {
            int id = G.owner[i];
            if (id >= 0 && id < DISK_MAX_BLOCKS) {
                if (G.files[id].status == FILE_UNUSED) {
                    G.files[id].id = id;
                    G.files[id].status = FILE_ACTIVE;
                }
            }
        }
    }
    free(in.buf);
    logf("disk_load: loaded from '%s'", G.persist_path);
    return 0;
}

int disk_reset() {
    ensure_initialized();
    clear_disk();
    logf("disk_reset: disk reinitialized");
    return disk_save();
}

int disk_total_free() {
    ensure_initialized();
    int c = 0;
    for (int i = 0; i < G.blocks; i++) if (G.state[i] == BLOCK_FREE) c++;
    return c;
}

int disk_total_used() {
    ensure_initialized();
    int c = 0;
    for (int i = 0; i < G.blocks; i++) if (G.state[i] == BLOCK_USED) c++;
    return c;
}

int disk_total_bad() {
    ensure_initialized();
    int c = 0;
    for (int i = 0; i < G.blocks; i++) if (G.state[i] == BLOCK_BAD) c++;
    return c;
}

int disk_file_exists(int file_id) {
    if (file_id <= 0 || file_id >= DISK_MAX_BLOCKS) return 0;
    return G.files[file_id].status == FILE_ACTIVE;
}

static int register_file(int id) {
    if (id <= 0 || id >= DISK_MAX_BLOCKS) return -1;
    G.files[id].id = id;
    G.files[id].status = FILE_ACTIVE;
    return 0;
}

int disk_allocate_contiguous(int size, int *out_file_id) {
    ensure_initialized();
    if (size <= 0 || size > G.blocks) return -1;
    // find run
    int streak = 0;
    int start = -1;
    for (int i = 0; i < G.blocks; i++) {
        if (G.state[i] == BLOCK_FREE) {
            if (streak == 0) start = i;
            streak++;
            if (streak >= size) {
                int fid = G.next_file_id++;
                register_file(fid);
                for (int j = start; j < start + size; j++) {
                    G.state[j] = BLOCK_USED;
                    G.owner[j] = fid;
                }
                if (out_file_id) *out_file_id = fid;
                logf("allocate_contiguous: id=%d size=%d start=%d", fid, size, start);
                disk_save();
                return 0;
            }
        } else {
            streak = 0;
        }
    }
    return -2; // no space
}

int disk_allocate_fragmented(int size, int *out_file_id) {
    ensure_initialized();
    if (size <= 0) return -1;
    if (disk_total_free() < size) return -2;
    int fid = G.next_file_id++;
    register_file(fid);
    int allocated = 0;
    for (int i = 0; i < G.blocks && allocated < size; i++) {
        if (G.state[i] == BLOCK_FREE) {
            G.state[i] = BLOCK_USED;
            G.owner[i] = fid;
            allocated++;
        }
    }
    if (out_file_id) *out_file_id = fid;
    logf("allocate_fragmented: id=%d size=%d", fid, size);
    disk_save();
    return 0;
}

static int find_holes(int* holes_start, int* holes_len, int max_holes) {
    int count = 0;
    int i = 0;
    while (i < G.blocks && count < max_holes) {
        while (i < G.blocks && G.state[i] != BLOCK_FREE) i++;
        if (i >= G.blocks) break;
        int s = i;
        int l = 0;
        while (i < G.blocks && G.state[i] == BLOCK_FREE) { i++; l++; }
        holes_start[count] = s;
        holes_len[count] = l;
        count++;
    }
    return count;
}

int disk_allocate_custom(int size, const char *strategy, int *out_file_id) {
    ensure_initialized();
    if (size <= 0) return -1;
    int starts[DISK_MAX_BLOCKS];
    int lens[DISK_MAX_BLOCKS];
    int hcount = find_holes(starts, lens, DISK_MAX_BLOCKS);
    if (hcount == 0) return -2;
    int choice = -1;
    if (strategy && strcmp(strategy, "best-fit") == 0) {
        int best_len = 999999;
        for (int i = 0; i < hcount; i++) {
            if (lens[i] >= size && lens[i] < best_len) {
                best_len = lens[i]; choice = i;
            }
        }
    } else if (strategy && strcmp(strategy, "worst-fit") == 0) {
        int worst_len = -1;
        for (int i = 0; i < hcount; i++) {
            if (lens[i] >= size && lens[i] > worst_len) {
                worst_len = lens[i]; choice = i;
            }
        }
    } else { // first-fit default
        for (int i = 0; i < hcount; i++) {
            if (lens[i] >= size) { choice = i; break; }
        }
    }
    if (choice == -1) return -2;
    int start = starts[choice];
    int fid = G.next_file_id++;
    register_file(fid);
    for (int j = start; j < start + size; j++) {
        G.state[j] = BLOCK_USED;
        G.owner[j] = fid;
    }
    if (out_file_id) *out_file_id = fid;
    logf("allocate_custom: id=%d size=%d strategy=%s start=%d", fid, size, strategy?strategy:"first-fit", start);
    disk_save();
    return 0;
}

int disk_logical_delete(int file_id) {
    ensure_initialized();
    if (!disk_file_exists(file_id)) return -1;
    // capture blocks
    int indices[DISK_MAX_BLOCKS];
    int cnt = 0;
    for (int i = 0; i < G.blocks; i++) {
        if (G.owner[i] == file_id && G.state[i] == BLOCK_USED) {
            indices[cnt++] = i;
        }
    }
    if (cnt == 0) {
        G.files[file_id].status = FILE_DELETED;
        logf("delete: id=%d (no blocks)", file_id);
        disk_save();
        return 0;
    }
    // free them
    for (int k = 0; k < cnt; k++) {
        G.state[indices[k]] = BLOCK_FREE;
        G.owner[indices[k]] = -1;
    }
    // mark file deleted
    G.files[file_id].status = FILE_DELETED;
    // record last_deleted
    G.last_deleted.valid = 1;
    G.last_deleted.file_id = file_id;
    G.last_deleted.count = cnt;
    for (int k = 0; k < cnt; k++) G.last_deleted.indices[k] = indices[k];
    logf("delete: id=%d freed=%d blocks", file_id, cnt);
    disk_save();
    return 0;
}

int disk_undelete_last() {
    ensure_initialized();
    if (!G.last_deleted.valid) return -1;
    int fid = G.last_deleted.file_id;
    int cnt = G.last_deleted.count;
    // check availability of original indices
    int can_restore_same = 1;
    for (int k = 0; k < cnt; k++) {
        int idx = G.last_deleted.indices[k];
        if (idx < 0 || idx >= G.blocks || G.state[idx] != BLOCK_FREE) { can_restore_same = 0; break; }
    }
    if (can_restore_same) {
        for (int k = 0; k < cnt; k++) {
            int idx = G.last_deleted.indices[k];
            G.state[idx] = BLOCK_USED;
            G.owner[idx] = fid;
        }
    } else {
        // fall back to fragmented allocation
        if (disk_total_free() < cnt) return -2;
        int allocated = 0;
        for (int i = 0; i < G.blocks && allocated < cnt; i++) {
            if (G.state[i] == BLOCK_FREE) {
                G.state[i] = BLOCK_USED;
                G.owner[i] = fid;
                allocated++;
            }
        }
    }
    G.files[fid].status = FILE_ACTIVE;
    G.last_deleted.valid = 0;
    logf("undelete_last: id=%d restored=%d blocks", fid, cnt);
    disk_save();
    return 0;
}

int disk_defragment() {
    ensure_initialized();
    int write_idx = 0;
    for (int read_idx = 0; read_idx < G.blocks; read_idx++) {
        if (G.state[read_idx] == BLOCK_USED) {
            if (write_idx != read_idx) {
                // move block owner to write_idx
                G.state[write_idx] = BLOCK_USED;
                G.owner[write_idx] = G.owner[read_idx];
                G.state[read_idx] = BLOCK_FREE;
                G.owner[read_idx] = -1;
            }
            write_idx++;
        }
    }
    logf("defragment: compacted used blocks to front (used=%d)", write_idx);
    disk_save();
    return 0;
}

int disk_mark_random_bad(int count) {
    ensure_initialized();
    if (count <= 0) return -1;
    utils_srand();
    int marked = 0;
    for (int tries = 0; tries < G.blocks * 4 && marked < count; tries++) {
        int idx = utils_rand_range(0, G.blocks - 1);
        if (G.state[idx] == BLOCK_FREE) {
            G.state[idx] = BLOCK_BAD;
            G.owner[idx] = -1;
            marked++;
        }
    }
    logf("mark_bad: requested=%d marked=%d", count, marked);
    disk_save();
    return marked > 0 ? 0 : -2;
}

int disk_repair() {
    ensure_initialized();
    // Simple repair: convert some BAD to FREE
    int repaired = 0;
    for (int i = 0; i < G.blocks; i++) {
        if (G.state[i] == BLOCK_BAD) {
            // 50% chance to repair
            if (utils_rand_range(0, 1) == 1) {
                G.state[i] = BLOCK_FREE;
                repaired++;
            }
        }
    }
    logf("repair: repaired=%d bad->free", repaired);
    disk_save();
    return 0;
}

static int file_count_and_fragmented(int* out_total_files, int* out_fragmented_files) {
    // Count active unique file ids and whether each is fragmented
    int seen[DISK_MAX_BLOCKS] = {0};
    int total = 0, frag = 0;
    for (int fid = 1; fid < DISK_MAX_BLOCKS; fid++) {
        if (G.files[fid].status == FILE_ACTIVE) {
            total++;
            // count segments for this fid
            int segments = 0;
            int in_run = 0;
            for (int i = 0; i < G.blocks; i++) {
                if (G.owner[i] == fid && G.state[i] == BLOCK_USED) {
                    if (!in_run) { in_run = 1; segments++; }
                } else {
                    in_run = 0;
                }
            }
            if (segments > 1) frag++;
        }
    }
    if (out_total_files) *out_total_files = total;
    if (out_fragmented_files) *out_fragmented_files = frag;
    return 0;
}

double disk_fragmentation_percent() {
    ensure_initialized();
    int total = 0, frag = 0;
    file_count_and_fragmented(&total, &frag);
    if (total == 0) return 0.0;
    return (100.0 * (double)frag) / (double)total;
}

char* disk_get_state() {
    ensure_initialized();
    StrBuf sb;
    if (sb_init(&sb, 4096) != 0) return NULL;
    sb_append(&sb, "{ \"blocks\": [");
    for (int i = 0; i < G.blocks; i++) {
        sb_appendf(&sb, "{\"index\":%d,\"state\":\"%s\",\"fileId\":%s}",
                   i,
                   (G.state[i] == BLOCK_FREE ? "free" : (G.state[i] == BLOCK_USED ? "used" : "bad")),
                   (G.owner[i] > 0 && G.state[i] == BLOCK_USED) ? "" : "null");
        if (G.owner[i] > 0 && G.state[i] == BLOCK_USED) {
            // overwrite the last "fileId": part properly
            sb.len -= 5; // remove "null}"
            sb.buf[sb.len] = '\0';
            sb_appendf(&sb, "%d}", G.owner[i]);
        }
        if (i + 1 < G.blocks) sb_append(&sb, ",");
    }
    sb_append(&sb, "] }");
    return sb_take(&sb);
}

char* disk_get_files() {
    ensure_initialized();
    StrBuf sb;
    if (sb_init(&sb, 2048) != 0) return NULL;
    sb_append(&sb, "{ \"files\": [");
    int first = 1;
    for (int fid = 1; fid < DISK_MAX_BLOCKS; fid++) {
        if (G.files[fid].status != FILE_UNUSED) {
            // compute size
            int size = 0;
            for (int i = 0; i < G.blocks; i++) {
                if (G.owner[i] == fid && G.state[i] == BLOCK_USED) size++;
            }
            if (!first) sb_append(&sb, ",");
            first = 0;
            sb_appendf(&sb, "{\"id\":%d,\"status\":\"%s\",\"size\":%d}",
                       fid,
                       (G.files[fid].status == FILE_ACTIVE ? "active" : "deleted"),
                       size);
        }
    }
    sb_append(&sb, "] }");
    return sb_take(&sb);
}

char* disk_get_stats() {
    ensure_initialized();
    StrBuf sb;
    if (sb_init(&sb, 512) != 0) return NULL;
    int total = G.blocks;
    int used = disk_total_used();
    int freeb = disk_total_free();
    int bad = disk_total_bad();
    double fragp = disk_fragmentation_percent();
    sb_appendf(&sb, "{ \"total\": %d, \"used\": %d, \"free\": %d, \"bad\": %d, \"fragmentationPercent\": %.2f }",
               total, used, freeb, bad, fragp);
    return sb_take(&sb);
}

char* disk_get_logs() {
    ensure_initialized();
    StrBuf sb;
    if (sb_init(&sb, 2048) != 0) return NULL;
    sb_append(&sb, "{ \"logs\": [");
    int count = G.log_head < DISK_MAX_LOGS ? G.log_head : DISK_MAX_LOGS;
    for (int i = 0; i < count; i++) {
        int idx = (G.log_head - count + i);
        if (idx < 0) idx = 0;
        idx = idx % DISK_MAX_LOGS;
        sb_appendf(&sb, "\"%s\"", G.logs[idx]);
        if (i + 1 < count) sb_append(&sb, ",");
    }
    sb_append(&sb, "] }");
    return sb_take(&sb);
}

void disk_shutdown() {
    disk_save();
}
