#include "../storage.h"

#include <stdio.h>
#include <stdlib.h>

ColumnMetaList *columnlist_empty()
{
    ColumnMetaList *empty = malloc(sizeof(ColumnMetaList));
    if (!empty)
        return NULL;

    *empty = (ColumnMetaList) {
        .meta = NULL,
        .next = NULL
    };

    return empty;
}

void columnlist_free(ColumnMetaList *list)
{
    ColumnMetaList *curr, *nxt;
    curr = list;
    do {
        nxt = curr->next;
        if (curr->meta)
            free(curr->meta);
        free(curr);
    } while ((curr = nxt));
}

void columnlist_push(ColumnMetaList *list, ColumnMeta value)
{
    // special case of the root node being empty
    if (!list->meta) {
        list->meta = malloc(sizeof(ColumnMeta));
        *list->meta = value;
        return;
    }

    while(list->next)
        list = list->next;

    list->next = columnlist_empty();
    list->next->meta = malloc(sizeof(ColumnMeta));
    *list->next->meta = value;
}

void columnlist_print(ColumnMetaList *list)
{
    if (!list) {
        fputs("<nullptr>", stdout);
        return;
    }

    if(!list->meta) {
        fputs("[ <empty> ]", stdout);
        return;
    }

    fputs("[ ", stdout);
    while (list) {
        if (!list->meta) {
            fputs("<null> ", stdout);
        } else {
            printf("(id: %lu type: %d) ", list->meta->id, list->meta->type);
        }
        list = list->next;
    }
    fputs("]", stdout);
}
