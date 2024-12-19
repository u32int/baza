/// Internal functions called by storage.c to implement the storage api from storage.h
#ifndef _STORAGE_INTERNAL_H
#define _STORAGE_INTERNAL_H

#include "storage.h"

/// A linked list of columns belonging to the same table
typedef struct Column {
    ColumnMeta meta;
    void *data; // an array of row values interpreted based on column type
    struct Column *next;
} Column;

/// Creates a new column [name] with [type], !without! allocating any backing storage.
Column *icolumn_new(const char *name, BaseType type);

void icolumn_free(Column *column, uint64_t row_count);

/// (Re)allocated data for a given column to fit the supplied capacity.
BazaResult icolumn_realloc_data(Column *column, uint64_t capacity);

/// Get the value at [index] inside [column].
void *icolumn_row_get(Column *column, uint64_t index);

/// Delete row data at [index], shifting all the other rows up to [size] one slot down
void icolumn_row_delete(Column *column, uint64_t index, size_t size);

/// Find all matching rows i.e. ones for which func(value, column[i]) returns true.
TableFindResult icolumn_find(Column *column, size_t size, findfunc_t func, void *value);



/// The main structure describing a single table
typedef struct Table {
    TableMeta meta;
    size_t row_capacity;  // the max number of rows that this table can currently store (i.e. are allocated)
    Column *columns;      // linked list of columns
    struct Table *next;
} Table;

#define BAZA_DEFAULT_ROW_CAPACITY 64

/// Return an empty table with the [name] and row capacity of {BAZA_DEFAULT_CAPACITY}
Table *itable_new(const char *name);

/// Free [table]. As with column_free, we assume exclusive ownership 
/// of all the data pointed to from inside the structure
void itable_free(Table *table);

/// Realloc all columns in a table to fit the new_size
void itable_realloc(Table *table, uint64_t new_size);

/// Get a struct Column* from a ColumnID
Column *itable_column_byid(Table *table, ColumnID_t cid);

/// Get a struct Column* by its string name
Column *itable_column_find(Table *table, const char *name);

/// Add a new column [name] with [type] to [table]
BazaResult itable_column_new(Table *table, BaseType type, const char *name);

/// Delete table row at [index], with bounds checking
BazaResult itable_row_delete(Table *table, size_t index);

/// Return a list of all row ids matching value in column 
TableFindResult itable_find(Table *table, Column *column,
                            findfunc_t func, void *value);

/// print a row to stdout
void itable_row_print(Table *table, IntList *ColumnIDs, uint64_t row);

// Get table from database by table_name
Table *idb_table_get(const char *table_name);

// Get table from database by id
Table *idb_table_get_byid(TableID_t tid);

TableResult idb_table_new(const char *table_name);

#endif
