#include "str.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

bool str_ieq(const char *s1, const char *s2)
{
    int i;
    for (i = 0; s1[i] && s2[i]; i++) {
        if (tolower(s1[i]) != tolower(s2[i]))
            return false;
    }

    if (!s1[i] && !s2[i])
        return true;

    return false;
}

bool str_contains(const char *str, char c)
{
    while (*str) {
        if (*str == c)
            return true;
        str++;
    }
    return false; 
}


size_t str_count_utf8_glyphs(const char *str)
{
    size_t count = 0;

    unsigned char *s = (unsigned char *)str;

    while (*s) {
        if (*s <= 0x7F) {
            // Single-byte character (ASCII)
            s += 1;
        } else if ((*s & 0xE0) == 0xC0) {
            // Two-byte character
            s += 2;
        } else if ((*s & 0xF0) == 0xE0) {
            // Three-byte character
            s += 3;
        } else if ((*s & 0xF8) == 0xF0) {
            // Four-byte character
            s += 4;
        }
        count++;
    }

    return count;
}

IntConvResult str_to_int(const char *str)
{
    errno = 0;
    uint64_t conv = strtoll(str, NULL, 10);
    if (errno == EINVAL)
        return (IntConvResult) { .result = RESULT_ERR };

    return (IntConvResult) { 
        .result = RESULT_OK,
        .value = conv 
    };
}


StrList *strlist_empty()
{
    StrList *empty = malloc(sizeof(StrList));
    if (!empty)
        return NULL;

    *empty = (StrList) {
        .str = NULL,
        .strlen = 0,
        .next = NULL
    };

    return empty;
}

StrList *strlist_new(const char *str)
{
    StrList *new = strlist_empty();
    if (!new)
        return NULL;

    new->str = strdup(str);
    new->strlen = strlen(str);

    return new;
}

void strlist_free(StrList *list)
{
    StrList *curr, *nxt;
    curr = list;
    do {
        nxt = curr->next;
        if (curr->str)
            free(curr->str);
        free(curr);
    } while ((curr = nxt));
}

void strlist_push(StrList *list, const char *value)
{
    // special case of the root node being empty
    if (!list->str) {
        list->str = strdup(value);
        list->strlen = strlen(value);
        return;
    }

    while(list->next)
        list = list->next;

    list->next = strlist_new(value);
}

StrList *strlist_seek_forward(StrList *list, size_t n)
{
    while (list && n > 0) {
        list = list->next;
        n--;
    }
    return list;
}

// Copy a slice from a string list containing [n] elements
StrList *strlist_copy(StrList *from, size_t n)
{
    if (!n)
        return NULL;

    StrList *new = strlist_new(from->str);

    for (size_t i = 0; i < n - 1; i++) {
        from = from->next;
        strlist_push(new, from->str);
    }

    return new;
}


/// Split text into a strlist based on delim
StrList *strlist_from_split(const char *string, const char *delim)
{
    StrList *list = strlist_empty();
    char *clone = strdup(string);

    char *tok = strtok(clone, delim);
    // empty string case
    if (!tok)
        goto bail;

    do {
        // stringlist_push strdup's, so this is fine
        strlist_push(list, tok);
    } while((tok = strtok(NULL, delim)));

bail:
    free(clone);
    return list;
}

StrList *strlist_from_split_quoted(const char *string, const char *delim)
{
    StrList *list = strlist_empty();

    char *clone = strdup(string);
    char *end = clone, *start = clone;

    // TODO: this doesn't handle quotes that are not at the start/end
    // of a token

    while (*end) {
        if (*end == '"') {
            end++;

            // skip to the ending quote
            while (*end && *end != '"')
                end++;

            // EOF before quote, just break
            if (!*end)
                break;
        }

        if (str_contains(delim, *end)) {
            *end = 0;

            if (start != end) {
                if (*start == '"' && *(end - 1) == '"') {
                    start++;
                    *(end - 1) = 0;
                }
                strlist_push(list, start);
            }

            end++;

            while (*end && str_contains(delim, *end))
                end++;

            start = end;

            if (!*end)
                break;

            continue;
        }
        
        end++;
    }

    if (start != end) {
        if (*start == '"' && *(end - 1) == '"') {
            start++;
            *(end - 1) = 0;
        }
        strlist_push(list, start);
    }
    free(clone);
    return list;
}

// Merge two lists, i.e. append the second list to the end of the first one
// without copying. This function allocates no new memory, but it might free
// one of the lists. The result is that the caller should only call free on the
// returned list.
StrList *strlist_merge(StrList *l1, StrList *l2)
{
    // The result of merging and empty list with a non-empty list is the non-empty list
    if (!l1->str) {
        strlist_free(l1);
        return l2;
    } else if (!l2->str) {
        strlist_free(l2);
        return l1;
    }

    // seek to the end of the first list
    StrList *curr = l1;
    while (curr->next)
        curr = curr->next;

    curr->next = l2;

    return l1;
}

void strlist_print(StrList *list)
{
    if (!list) {
        fputs("<nullptr>", stdout);
        return;
    }

    if(!list->str) {
        fputs("[ <empty> ]", stdout);
        return;
    }

    fputs("[ ", stdout);
    while (list) {
        if (!list->str) {
            // shouldn't really happen but oh well
            fputs("<null> ", stdout);
        } else {
            printf("'%s' ", list->str);
        }
        list = list->next;
    }
    fputs("]", stdout);
}

bool strlist_contains(StrList *list, const char *str)
{
    while (list) {
        if (!strcmp(list->str, str))
            return true;
        list = list->next;
    }
    return false;
}
