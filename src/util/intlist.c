#include "intlist.h"

#include <stdlib.h>
#include <stdio.h>

IntList *intlist_empty()
{
    IntList *empty = malloc(sizeof(IntList));
    if (!empty)
        return NULL;

    empty->value = INTLIST_NULL;
    empty->next = NULL;

    return empty;
}

IntList *intlist_new(int64_t value)
{
    IntList *new = intlist_empty();
    if (!new)
        return NULL;

    new->value = value;

    return new;
}

void intlist_push(IntList *list, uint64_t value)
{
    // special case of the root node being empty
    if (list->value == INTLIST_NULL) {
        list->value = value;
        return;
    }

    while(list->next)
        list = list->next;

    list->next = intlist_new(value);
}

uint64_t intlist_get_unchecked(IntList *list, size_t nth)
{
    while (nth > 0) {
        list = list->next;
        nth--;
    }
    return list->value;
}

void intlist_free(IntList *list)
{
    if (!list)
        return;

    IntList *curr, *nxt;
    curr = list;
    do {
        nxt = curr->next;
        free(curr);
    } while ((curr = nxt));
}

void intlist_print(IntList *list)
{
    if (!list) {
        fputs("<nullptr>", stdout);
        return;
    }

    if(list->value == INTLIST_NULL) {
        fputs("[ <empty> ]", stdout);
        return;
    }

    fputs("[ ", stdout);
    while(list) {
        printf("%ld ", list->value);
        list = list->next;
    }
    fputs("]", stdout);
}

bool intlist_contains(IntList *list, int64_t value)
{
    while (list && list->value != INTLIST_NULL)  {
        if (list->value == value)
            return true;
        list = list->next;
    }
    return false;
}

// yes, this is slooooow. We should be using a hashset or whatever for this kind of thing
IntList *intlist_union(IntList *left, IntList *right)
{
    if (left == right)
        return NULL;

    IntList *new = intlist_empty();

    // push all the elements from the first list first
    while (left && left->value != INTLIST_NULL)  {
        if (!intlist_contains(new, left->value)) // filter duplicates
            intlist_push(new, left->value);
        left = left->next;
    }

    // ..and push all the elements of the second list, if they
    // are not already present in the first one
    while (right && right->value != INTLIST_NULL)  {
        if (!intlist_contains(new, right->value))
            intlist_push(new, right->value);
        right = right->next;
    }

    return new;
}

IntList *intlist_intersection(IntList *left, IntList *right)
{
    if (left == right)
        return NULL;

    IntList *new = intlist_empty();

    // push all the elements from the left that are also in the right + filter duplicates
    while (left && left->value != INTLIST_NULL)  {
        if (intlist_contains(right, left->value) && !intlist_contains(new, left->value)) 
            intlist_push(new, left->value);
        left = left->next;
    }
    
    return new;
}
