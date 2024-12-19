#ifndef _UTIL_INTLIST_H
#define _UTIL_INTLIST_H

#include "result.h"

#include "includes.h"

// for packing efficiency's sake :p
#define INTLIST_NULL (-1)

typedef struct IntList {
    int64_t value;
    struct IntList *next;
} IntList;

IntList *intlist_empty();
IntList *intlist_new(int64_t value);
void intlist_push(IntList *list, uint64_t value);
uint64_t intlist_get_unchecked(IntList *list, size_t nth);
void intlist_free(IntList *list);
void intlist_print(IntList *list);
bool intlist_contains(IntList *list, int64_t value);

// intlist_{union,intersection} perform the respective set operation,
// returning a new list (to be freed separately) containing the appropriate
// elements.
IntList *intlist_union(IntList *left, IntList *right);
IntList *intlist_intersection(IntList *left, IntList *right);

#endif  /* _UTIL_INTLIST_H */
