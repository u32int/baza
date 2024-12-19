#ifndef _STORAGE_H
#define _STORAGE_H

#include "parser.h"
#include "util/result.h"
#include "util/intlist.h"
#include "util/defs.h"
#include "util/str.h"

#include "util/includes.h"

/// The storage API deals with the following abstract objects and concepts:
///
/// DataBase (db): exposes functions to query the general state of the 
/// database as well as to fetch information about tables. Conceptually
/// the db "object" is a singleton, so no handles/descriptors are necessary
/// to call these functions. 
/// (TODO: in multithreaded scenarios you may need to take a mutex to 
/// perform certain operations)
///
/// Table: a table as generally understood in the context of relational
/// databases. Contains a list of columns.
///
/// Column: a column as generally understood in the context of relational
/// databases. Contains a list of rows.

typedef enum BaseType {
    BTYPE_INVALID,
    BTYPE_STRING,
    BTYPE_INT32,
    BTYPE_INT64,
} BaseType;

/// Returns the size of an SQLType [type] in bytes
size_t basetype_size(BaseType type);
/// Print the [data] interpreting it as [type]
void basetype_print(BaseType type, void *data);
char *basetype_value_to_str(BaseType type, void *value);
BaseType basetype_from_str(const char *type);

typedef uint64_t TableID_t;
#define TABLE_ID_INVALID (-1)

typedef struct TableMeta {
    TableID_t id;
    char *name;
    uint64_t row_count;
} TableMeta;

typedef uint64_t ColumnID_t;
#define COLUMN_ID_INVALID (-1)

typedef struct ColumnMeta {
    ColumnID_t id;
    char *name;
    BaseType type;
} ColumnMeta;

typedef struct ColumnMetaList {
    ColumnMeta *meta;
    struct ColumnMetaList *next;
} ColumnMetaList;

ColumnMetaList *columnlist_empty();
void columnlist_free(ColumnMetaList *list);
void columnlist_push(ColumnMetaList *list, ColumnMeta value);
void columnlist_print(ColumnMetaList *list);

/// Initialize the entire backend
void storage_init();

/// Deinitialize the entire backend
void storage_deinit();

typedef struct TableResult {
    BazaResult result;
    TableMeta meta;
} TableResult;

/// Creates a new table named [table_name]. Returns RESULT_OK and the table metadata 
/// if successful. Otherwise .result describes the error.
TableResult db_table_new(const char *table_name);

/// Get the TableMeta of a table named [table_name], if it exists.
TableResult db_table_get(const char *table_name);

/// Attempts to delete the table reffered to by table_id. If successful,
/// [table_id] can no longer be assumed to reffer to anything useful.
BazaResult db_table_delete(TableID_t table);

typedef struct ColumnCreationResult {
    BazaResult result;
    ColumnMeta meta;
} ColumnResult;

/// Attempts to create a new column in [table].
/// If successful, meta contains the new column's metadata.
BazaResult table_column_new(TableID_t table, 
                            BaseType type, const char *name);

/// Returns the column meta of a given column, if it exists.
ColumnResult table_column_get(TableID_t table, 
                              const char *column_name);

/// Returns the metadata of columns specified in [list]. If even one of the columns
/// is invalid (i.e. no column with that name exists in the table), this function returns NULL.
/// If [list] is NULL, returns metadata for all the columns contained in the table. 
ColumnMetaList *table_column_get_list(TableID_t table, StrList *list);

/// Deletes [column] from [table].
BazaResult table_column_delete(TableID_t table, ColumnID_t column);

/// Get the [nth] row from [column] in [table]. Returns NULL if out of range.
/// Since even in a multithreaded context this is expected to be called only
/// after acquiring a lock (and dropped afterwards), this pointer may also
/// be used to modify the row contents (aka "exclusive raw mutable pointer" in rusty terms).
/// It is up to the caller to interpret the return pointer type.
/// The storage backend guarantees correct alignment.
void *table_column_get_row(TableID_t table, ColumnID_t column, uint64_t nth);

void *table_column_set_row(TableID_t table, ColumnID_t column, uint64_t nth, void *value);

/// Make space for an additional row, incrementing the internal row_count of the table
BazaResult table_row_add(TableID_t table);

/// Delete [row] in [table]
BazaResult table_row_delete(TableID_t table, uint64_t row);

/// Print a row to stdout
BazaResult table_row_print(TableID_t table, IntList *ColumnIDs, uint64_t row);

// NOTE: it is the responsiblity of the caller to free [matches]
typedef struct TableFindResult {
    BazaResult res;
    IntList *matches;
} TableFindResult;

/// Args: 
///    type: type of the column (and thus both of the values)
///    left: value already present in the column 
///    right: value supplied as the [value] arg in table_find
typedef bool (findfunc_t)(BaseType type, const void *left, const void *right);

/// Returns a list of row IDs for which [func] returns true.
TableFindResult table_find(TableID_t table, ColumnID_t column,
                           findfunc_t func, void *value);

#endif /* STORAGE_H */
