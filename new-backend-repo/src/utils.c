#define _POSIX_C_SOURCE 200809L
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>

int sb_init(StrBuf* sb, size_t initial_cap) {
    if (!sb) return -1;
    if (initial_cap < 16) initial_cap = 16;
    sb->buf = (char*)malloc(initial_cap);
    if (!sb->buf) return -1;
    sb->len = 0;
    sb->cap = initial_cap;
    sb->buf[0] = '\0';
    return 0;
}

static int sb_ensure(StrBuf* sb, size_t extra) {
    if (sb->len + extra + 1 <= sb->cap) return 0;
    size_t newcap = sb->cap * 2;
    while (newcap < sb->len + extra + 1) newcap *= 2;
    char* nb = (char*)realloc(sb->buf, newcap);
    if (!nb) return -1;
    sb->buf = nb;
    sb->cap = newcap;
    return 0;
}

int sb_append(StrBuf* sb, const char* s) {
    if (!sb || !s) return -1;
    size_t n = strlen(s);
    if (sb_ensure(sb, n) != 0) return -1;
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
    return 0;
}

int sb_append_n(StrBuf* sb, const char* s, size_t n) {
    if (!sb || !s) return -1;
    if (sb_ensure(sb, n) != 0) return -1;
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
    return 0;
}

int sb_appendf(StrBuf* sb, const char* fmt, ...) {
    if (!sb || !fmt) return -1;
    va_list ap;
    va_start(ap, fmt);
    char tmp[1024];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) return -1;
    if ((size_t)n >= sizeof(tmp)) {
        // Fallback: allocate exact
        va_start(ap, fmt);
        char* big = (char*)malloc((size_t)n + 1);
        if (!big) { va_end(ap); return -1; }
        vsnprintf(big, (size_t)n + 1, fmt, ap);
        va_end(ap);
        int r = sb_append_n(sb, big, (size_t)n);
        free(big);
        return r;
    }
    return sb_append_n(sb, tmp, (size_t)n);
}

char* sb_take(StrBuf* sb) {
    if (!sb) return NULL;
    char* out = sb->buf;
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
    return out;
}

int file_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int read_text_file(const char* path, StrBuf* out) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    if (sb_init(out, 1024) != 0) { fclose(f); return -1; }
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (sb_append_n(out, buf, n) != 0) { fclose(f); return -1; }
    }
    fclose(f);
    return 0;
}

int write_text_file_atomic(const char* path, const char* content) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE* f = fopen(tmp, "wb");
    if (!f) return -1;
    size_t n = strlen(content);
    if (fwrite(content, 1, n, f) != n) {
        fclose(f);
        remove(tmp);
        return -1;
    }
    fclose(f);
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return -1;
    }
    return 0;
}

void trim_whitespace(char* s) {
    if (!s) return;
    size_t len = strlen(s);
    size_t start = 0;
    while (start < len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) start++;
    size_t end = len;
    while (end > start && (s[end-1] == ' ' || s[end-1] == '\t' || s[end-1] == '\n' || s[end-1] == '\r')) end--;
    if (start > 0) memmove(s, s + start, end - start);
    s[end - start] = '\0';
}

// Very naive JSON parsing: looks for "key": <number>
int parse_json_int(const char* json, const char* key, int* out_value) {
    if (!json || !key || !out_value) return -1;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return -1;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    char* endptr = NULL;
    long v = strtol(p, &endptr, 10);
    if (p == endptr) return -1;
    *out_value = (int)v;
    return 0;
}

// Very naive JSON parsing: "key": "value"
int parse_json_string(const char* json, const char* key, char* out, size_t out_len) {
    if (!json || !key || !out || out_len == 0) return -1;
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(json, pattern);
    if (!p) return -1;
    p = strchr(p + strlen(pattern), ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '\"') return -1;
    p++;
    const char* q = strchr(p, '\"');
    if (!q) return -1;
    size_t n = (size_t)(q - p);
    if (n >= out_len) n = out_len - 1;
    memcpy(out, p, n);
    out[n] = '\0';
    return 0;
}

void utils_srand() {
    static int seeded = 0;
    if (!seeded) {
        seeded = 1;
        srand((unsigned int)time(NULL));
    }
}

int utils_rand_range(int min_inclusive, int max_inclusive) {
    if (max_inclusive <= min_inclusive) return min_inclusive;
    int span = max_inclusive - min_inclusive + 1;
    return min_inclusive + (rand() % span);
}
