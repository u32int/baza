#include "storage_internal.h"
#include "util/intlist.h"
#include "util/result.h"
#include "util/str.h"

#include <string.h>

Column *icolumn_new(const char *name, BaseType type)
{
    Column *ret = malloc(sizeof(Column));
    if (!ret)
        return NULL;

    static uint64_t COLUMN_ID = 0;
    
    ret->meta.name = strdup(name);
    ret->meta.type = type;
    ret->meta.id = COLUMN_ID;
    ret->data = NULL;
    ret->next = NULL;

    COLUMN_ID++;

    return ret;
}

// A column is assumed to "own" all of its data, including the data that it points to.
void icolumn_free(Column *column, uint64_t row_count)
{
    // free data structures requiring special care
    switch (column->meta.type) {
        // TODO: could be reduced to a generic thing maybe, basetype_isptr() or something
        case BTYPE_STRING: {
            char **string_data = column->data;

            for(size_t i = 0; i < row_count; i++) {
                free(string_data[i]);
            }
        } break;
        default: { /* typed stored literally - NOP */ }
    }

    free(column->data);
    free(column->meta.name);
    free(column);
}

BazaResult icolumn_realloc_data(Column *column, size_t capacity)
{
    size_t type_size = basetype_size(column->meta.type);

    if (!column->data) {
        // first allocation
        column->data = malloc(type_size * capacity);
        if (!column->data)
            return RESULT_ALLOC;
    } else {
        // subsequent realloc
        column->data = realloc(column->data, type_size * capacity);
        if (!column->data)
            return RESULT_ALLOC;
    }

    return RESULT_OK;
}

/// Get the value at [index] inside [column]
void *icolumn_row_get(Column *column, size_t index)
{
    // ^^ TODO: generate this with a macro?
    switch (column->meta.type) {
        case BTYPE_INT32: {
            uint32_t *u32data = column->data;
            u32data += index;
            return u32data;
        } break;
        case BTYPE_INT64: {
            uint64_t *u64data = column->data;
            u64data += index;
            return u64data;
        } break;
        case BTYPE_STRING: {
            char **strdata = column->data;
            strdata += index;
            return strdata;
        } break;
        default: {
            return NULL;
        } break;
    }
}

/// Set the row [data] at [index]. No bounds checks performed
void icolumn_row_set(Column *column, size_t index, const void *data)
{
    // TODO: generate this with a macro?
    switch (column->meta.type) {
        case BTYPE_INT32: {
            uint32_t *u32data = column->data;
            u32data += index;
            *u32data = *(uint32_t*)data;
        } break;
        case BTYPE_INT64: {
            uint64_t *u64data = column->data;
            u64data += index;
            *u64data = *(uint64_t*)data;
        } break;
        case BTYPE_STRING: {
            char **strdata = column->data;
            strdata += index;
            char *dup = strdup(*(char**)data);
            *strdata = dup;
        } break;
        case BTYPE_INVALID: {
            // nop: this shouldn't ever happen.
        } break;
    }
}

// Set the row [data] at [index]. No bounds checks performed
// DIFFERENCE FROM 'icolumn_row_set': does not copy types that would usually
// be copied to ensure ownership. I figured this the right abstraction layer
// to split this at. This function should probably thus only be called from
// other column_* functions, where we already "own" the pointer
void icolumn_row_set_nocopy(Column *column, size_t index, const void *data)
{
    // TODO: generate this with a macro?
    switch (column->meta.type) {
        case BTYPE_INT32: {
            uint32_t *u32data = column->data;
            u32data += index;
            *u32data = *(uint32_t*)data;
        } break;
        case BTYPE_INT64: {
            uint64_t *u64data = column->data;
            u64data += index;
            *u64data = *(uint64_t*)data;
        } break;
        case BTYPE_STRING: {
            char **strdata = column->data;
            strdata += index;
            *strdata = *(char**)data;
        } break;
        case BTYPE_INVALID: {
            // nop: this shouldn't ever happen.
        } break;
    }
}

void icolumn_row_delete(Column *column, size_t index, size_t size)
{
    // TODO: generalize
    if (column->meta.type == BTYPE_STRING) {
        char *to_delete = *(char**)icolumn_row_get(column, index);
        free(to_delete);
    }

    for(; index < size-1; index++) {
        void *next = icolumn_row_get(column, index + 1);
        icolumn_row_set_nocopy(column, index, next);
    }
}

/// Find all matching rows i.e. ones for which func(value, column[i]) returns true
TableFindResult icolumn_find(Column *column, size_t size, findfunc_t func, void *value)
{
    IntList *lst = intlist_empty();
    if (!lst)
        return (TableFindResult) { .res = RESULT_ALLOC };

    for(size_t i = 0; i < size; i++) {
        void *ith = icolumn_row_get(column, i);
        bool cmp = func(column->meta.type, ith, value);
        if (cmp)
            intlist_push(lst, i);
    }

    return (TableFindResult) { .res = RESULT_OK, .matches = lst };
}


/// Return an empty table with the [name] and row capacity of {BAZA_DEFAULT_CAPACITY}
Table *itable_new(const char *name)
{
    Table *new = malloc(sizeof(Table));
    if (!new)
        return NULL;

    static uint64_t TABLE_ID = 0;
    
    *new = (Table) {
        .meta.name = strdup(name),
        .meta.row_count = 0,
        .meta.id = TABLE_ID,
        .row_capacity = BAZA_DEFAULT_ROW_CAPACITY,
        .columns = NULL,
        .next = NULL,
    };

    TABLE_ID++;

    return new;
}

/// Free [table]. As with column_free, we assume exclusive ownership 
/// of all the data pointed to from inside the structure
void itable_free(Table *table)
{
    Column *cur = table->columns, *nxt;
    while(cur) {
        nxt = cur->next;
        icolumn_free(cur, table->meta.row_count);
        cur = nxt;
    }
    free(table->meta.name);
    free(table);
}

void itable_realloc(Table *table, uint64_t size)
{
    Column *col = table->columns;
    while (col) {
        icolumn_realloc_data(col, size);
        col = col->next;
    }
    table->row_capacity = size;
}

Column *itable_column_byid(Table *table, ColumnID_t cid)
{
    Column *column = table->columns;
    while (column) {
        if (column->meta.id == cid)
            return column;
        column = column->next;
    }
    return NULL;
}

Column *itable_column_find(Table *table, const char *name)
{
    Column *cur = table->columns;
    while (cur) {
        if (!strcmp(cur->meta.name, name))
            return cur;
        cur = cur->next;
    }
    return NULL;
}

/// Add a new column [name] with [type] to [table] (internal)
BazaResult itable_column_new(Table *table, BaseType type, const char *name)
{
    // Since we do not allow for null values yet, disallow adding a new column 
    // if there is already data inside the table
    if (table->meta.row_count > 0)
        return RESULT_TABLE_NOT_EMPTY;

    if (itable_column_find(table, name))
        return RESULT_DUPLICATE_COLUMN_NAME;

    Column *new = icolumn_new(name, type);
    if (!new)
        return RESULT_ALLOC;

    ENSURE(icolumn_realloc_data(new, table->row_capacity));

    if (!table->columns) {
        table->columns = new;
    } else {
        Column *cursor = table->columns;
        while(cursor->next)
            cursor = cursor->next;
        cursor->next = new;
    }

    return RESULT_OK; 
}

BazaResult itable_row_delete(Table *table, size_t index)
{
    if (index >= table->meta.row_count)
        return RESULT_INDEX_OUT_OF_BOUNDS;

    Column *cur = table->columns;
    while(cur) {
        icolumn_row_delete(cur, index, table->meta.row_count);
        cur = cur->next;
    }

    table->meta.row_count--;
    return RESULT_OK;
}

void itable_row_print(Table *table, IntList *ColumnIDs, uint64_t row)
{
    if (row > table->meta.row_count)
        return;

    Column *col = table->columns; 
    while (col) {
        if (ColumnIDs && !intlist_contains(ColumnIDs, col->meta.id)) {
            col = col->next;
            continue;
        }

        void *value = icolumn_row_get(col, row);

        #define PRINT_ROW_PADDING 20

        char *as_str = basetype_value_to_str(col->meta.type, value);
        fputs(as_str, stdout);

        int slen = str_count_utf8_glyphs(as_str);
        printf("%*s", PRINT_ROW_PADDING-slen, " ");

        free(as_str);
        col = col->next;
    }
}

/// Return a list of all row ids matching value in column 
TableFindResult itable_find(Table *table, Column *column,
                            findfunc_t func, void *value)
{
    return icolumn_find(column, table->meta.row_count, func, value);
}

// global database object
struct DataBase {
    Table *tables; 
} DB;

TableResult idb_table_new(const char *table_name)
{
    if (!DB.tables) {
        DB.tables = itable_new(table_name);
        
        if (DB.tables) {
            return (TableResult) {
                .result = RESULT_OK,
                .meta = DB.tables->meta,
            };
        }

        return (TableResult) {
            .result = RESULT_SERVER_ERROR,
        };
    }

    Table *tcur = DB.tables;
    while (tcur->next)
        tcur = tcur->next;

    tcur->next = itable_new(table_name);

    if (tcur->next) {
        return (TableResult) {
            .result = RESULT_OK,
            .meta = tcur->next->meta,
        };
    }

    return (TableResult) { .result = RESULT_SERVER_ERROR };
}

Table *idb_table_get(const char *table_name)
{
    Table *cur = DB.tables; 
    while (cur) {
        if (!strcmp(cur->meta.name, table_name))
            return cur;
        cur = cur->next;
    }
    return NULL;
}

Table *idb_table_get_byid(TableID_t tid)
{
    Table *cur = DB.tables; 
    while (cur) {
        if (cur->meta.id == tid)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void storage_init()
{

}

void storage_deinit()
{
    Table *tab = DB.tables, *next;
    while (tab) {
        next = tab->next; 
        itable_free(tab);
        tab = next;
    }
}
