/* Minimaler Host-Shim fuer Layout-Tests (nur was die View braucht). */
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLAMP(x, upper, lower) (MIN(upper, MAX(x, lower)))
#define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))
#define UNUSED(x) (void)(x)
#define furi_assert(x) ((void)0)
#define furi_check(x) ((void)0)
#define FURI_LOG_E(tag, fmt, ...) fprintf(stderr, "[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define FURI_LOG_W(tag, fmt, ...) fprintf(stderr, "[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
#define FURI_LOG_I(tag, fmt, ...) fprintf(stderr, "[I][%s] " fmt "\n", tag, ##__VA_ARGS__)

typedef struct {
    char* s;
} FuriString;

FuriString* furi_string_alloc(void);
FuriString* furi_string_alloc_set_str(const char* s);
FuriString* furi_string_alloc_set_fs(const FuriString* s);
FuriString* furi_string_alloc_printf(const char* fmt, ...);
void furi_string_free(FuriString* s);
void furi_string_set_str(FuriString* s, const char* v);
void furi_string_set_fs(FuriString* s, const FuriString* v);
void furi_string_cat_str(FuriString* s, const char* v);
void furi_string_cat_fs(FuriString* s, const FuriString* v);
void furi_string_printf(FuriString* s, const char* fmt, ...);
const char* furi_string_get_cstr(const FuriString* s);
bool furi_string_empty(const FuriString* s);
bool furi_string_equal_str(const FuriString* s, const char* v);
bool furi_string_equal_fs(const FuriString* s, const FuriString* v);
void furi_string_left(FuriString* s, size_t n);
size_t furi_string_size(const FuriString* s);

#define FS_GEN(x, f_str, f_fs) \
    _Generic((x), char*: f_str, const char*: f_fs##_str_dummy, FuriString*: f_fs, const FuriString*: f_fs)
#define furi_string_alloc_set(x) \
    _Generic((x), char*: furi_string_alloc_set_str, const char*: furi_string_alloc_set_str, default: furi_string_alloc_set_fs)(x)
#define furi_string_set(s, x) \
    _Generic((x), char*: furi_string_set_str, const char*: furi_string_set_str, default: furi_string_set_fs)(s, x)
#define furi_string_cat(s, x) \
    _Generic((x), char*: furi_string_cat_str, const char*: furi_string_cat_str, default: furi_string_cat_fs)(s, x)
#define furi_string_equal(s, x) \
    _Generic((x), char*: furi_string_equal_str, const char*: furi_string_equal_str, default: furi_string_equal_fs)(s, x)

size_t strlcpy(char* dst, const char* src, size_t size);
