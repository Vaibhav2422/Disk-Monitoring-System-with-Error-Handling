#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Simple string builder
typedef struct {
    char* buf;
    size_t len;
    size_t cap;
} StrBuf;

int sb_init(StrBuf* sb, size_t initial_cap);
int sb_append(StrBuf* sb, const char* s);
int sb_append_n(StrBuf* sb, const char* s, size_t n);
int sb_appendf(StrBuf* sb, const char* fmt, ...);
char* sb_take(StrBuf* sb); // returns buffer and resets sb

// File IO
int file_exists(const char* path);
int read_text_file(const char* path, StrBuf* out);
int write_text_file_atomic(const char* path, const char* content);

// Parsing helpers (very naive JSON-like parsing)
int parse_json_int(const char* json, const char* key, int* out_value);
int parse_json_string(const char* json, const char* key, char* out, size_t out_len);

// Trim
void trim_whitespace(char* s);

// Random helpers
void utils_srand();
int utils_rand_range(int min_inclusive, int max_inclusive);

#endif // UTILS_H
