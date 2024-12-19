#include "storage.h"
#include "storage_internal.h"
#include "util/result.h"

#include <string.h>

size_t basetype_size(BaseType type)
{
    switch (type) {
        // Pointer/structure types
        case BTYPE_STRING: 
            return sizeof(char*);
        // Integral types
        case BTYPE_INT32:
            return sizeof(uint32_t);
        case BTYPE_INT64:
            return sizeof(uint64_t);
        case BTYPE_INVALID:
            return 0;
    }
    FATAL("invalid sqltype, this is a bug");
}

void basetype_print(BaseType type, void *data)
{
    switch (type) {
        case BTYPE_STRING: 
            printf("%s", *(char**)data);
            break;
        case BTYPE_INT32:
            printf("%d", *(uint32_t*)data);
            break;
        case BTYPE_INT64:
            printf("%ld", *(uint64_t*)data);
            break;
        case BTYPE_INVALID:
            fputs("INVALID", stdout);
            break;
    }
}

char *basetype_value_to_str(BaseType type, void *data)
{
#define BT_BUFF_SIZE 256
    char *buff = malloc(BT_BUFF_SIZE);

    switch (type) {
        case BTYPE_STRING: 
            snprintf(buff, BT_BUFF_SIZE, "%s", *(char**)data);
            break;
        case BTYPE_INT32:
            snprintf(buff, BT_BUFF_SIZE, "%d", *(uint32_t*)data);
            break;
        case BTYPE_INT64:
            snprintf(buff, BT_BUFF_SIZE, "%ld", *(uint64_t*)data);
            break;
        case BTYPE_INVALID:
            snprintf(buff, BT_BUFF_SIZE, "INVALID");
            break;
    }

    return buff;
}

BaseType basetype_from_str(const char *type)
{
    if (str_ieq(type, "int32")) {
        return BTYPE_INT32;
    } else if (str_ieq(type, "int64")) {
        return BTYPE_INT64; 
    } else if (str_ieq(type, "string")) {
        return BTYPE_STRING;
    } 

    return BTYPE_INVALID;
}

BazaResult table_column_new(TableID_t tid, BaseType type, const char *name)
{
    Table *tptr = idb_table_get_byid(tid);
    if (!tptr)
        return RESULT_TABLE_NOT_FOUND;

    return itable_column_new(tptr, type, name);
}

ColumnResult table_column_get(TableID_t tid, const char *column_name)
{
    Table *tptr = idb_table_get_byid(tid);
    if (!tptr)
        return (ColumnResult) { .result = RESULT_TABLE_NOT_FOUND };

    Column *column = tptr->columns;
    while (column) {
        if (!strcmp(column->meta.name, column_name)) {
            return (ColumnResult) {
                .result = RESULT_OK,
                .meta = column->meta,
            };
        }
        column = column->next;
    }

    return (ColumnResult) { .result = RESULT_COLUMN_NOT_FOUND };
}

ColumnMetaList *table_column_get_list(TableID_t tid, StrList *list)
{
    // TODO: return in specified order and return an error if one of the list members
    // was not matched to any column
    Table *tptr = idb_table_get_byid(tid);
    if (!tptr)
        return NULL;

    ColumnMetaList *cmlst = columnlist_empty();
    Column *column = tptr->columns;

    if (!list) {
        // No columns specified, return all of them
        while (column) {
            columnlist_push(cmlst, column->meta);
            column = column->next;
        }
    } else {
        while (column) {
            if (strlist_contains(list, column->meta.name))
                columnlist_push(cmlst, column->meta);
            column = column->next;
        }
    }

    return cmlst;
}

void *table_column_get_row(TableID_t tid, ColumnID_t cid, uint64_t nth)
{
    Table *tptr = idb_table_get_byid(tid);
    if (!tptr)
        return NULL;

    if (nth > tptr->meta.row_count)
        return NULL;

    Column *column = itable_column_byid(tptr, cid);
    if (!column)
        return NULL;

    return icolumn_row_get(column, nth);
}

BazaResult table_row_add(TableID_t table)
{
    Table *tptr = idb_table_get_byid(table);
    if (!tptr)
        return RESULT_TABLE_NOT_FOUND;

    tptr->meta.row_count++;

    if (tptr->meta.row_count == tptr->row_capacity)
        itable_realloc(tptr, tptr->row_capacity * 2);

    return RESULT_OK;
}

/// Delete [row] in [table]
BazaResult table_row_delete(TableID_t table, uint64_t row)
{
    Table *tptr = idb_table_get_byid(table);
    if (!tptr)
        return RESULT_TABLE_NOT_FOUND;

    return itable_row_delete(tptr, row);
}

BazaResult table_row_print(TableID_t table, IntList *ColumnIDs, uint64_t row)
{
    Table *tptr = idb_table_get_byid(table);
    if (!tptr)
        return RESULT_TABLE_NOT_FOUND;

    itable_row_print(tptr, ColumnIDs, row);
    
    return RESULT_OK;
}

TableFindResult table_find(TableID_t tid, ColumnID_t cid,
                           findfunc_t func, void *value)
{
    Table *tptr = idb_table_get_byid(tid);
    if (!tptr)
        return (TableFindResult) { .res = RESULT_ALLOC };

    Column *cptr = itable_column_byid(tptr, cid);

    return itable_find(tptr, cptr, func, value);
}

TableResult db_table_get(const char *table_name)
{
    Table *tptr = idb_table_get(table_name);

    if (!tptr) {
        return (TableResult) {
            .result = RESULT_TABLE_NOT_FOUND,
        };
    }

    return (TableResult) {
        .result = RESULT_OK,
        .meta = tptr->meta,
    };
}

TableResult db_table_new(const char *table_name)
{
    return idb_table_new(table_name);
}


// storage_init and storage_deinit are implemented in storage_internal
