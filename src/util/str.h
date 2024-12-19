// various string utilities
#ifndef _UTIL_STRING_H
#define _UTIL_STRING_H

#include "includes.h"
#include "result.h"

// this is technically breaking the (or atleast some kind of) 
// C standard, because you are not supposed to use the str 
// prefix for non libc functions (whoopss)

/// A case-insensitive compare two strings. They have to be
/// the exact same length for this to return true
bool str_ieq(const char *s1, const char *s2);

typedef struct IntConvResult {
    BazaResult result;
    uint64_t value;
} IntConvResult;

IntConvResult str_to_int(const char *str);
bool str_contains(const char *str, char c);
size_t str_count_utf8_glyphs(const char *str);

typedef struct StrList {
    char *str;
    size_t strlen;
    struct StrList *next;
} StrList;

StrList *strlist_empty();
StrList *strlist_new(const char *str);
void strlist_free(StrList *list);

void strlist_push(StrList *list, const char *value);

StrList *strlist_seek_forward(StrList *list, size_t n);
StrList *strlist_copy(StrList *from, size_t n);

StrList *strlist_from_split(const char *string, const char *delim);
StrList *strlist_from_split_quoted(const char *string, const char *delim);
StrList *strlist_merge(StrList *l1, StrList *l2);

bool strlist_contains(StrList *list, const char *str);

void strlist_print(StrList *list);

#endif /* STRING_H */
