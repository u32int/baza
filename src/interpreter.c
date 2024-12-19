#include "interpreter.h"
#include "storage.h"
#include "parser.h"

#include "util/intlist.h"
#include "util/result.h"
#include "util/str.h"

#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <fcntl.h>

bool filter_func_equals(BaseType type, const void *left, const void *right)
{
    switch (type) {
        case BTYPE_INT32: {
            return (*(const int32_t*)left) == (*(const int32_t*)right);
        } break;
        case BTYPE_INT64: {
            return (*(const int64_t*)left) == (*(const int64_t*)right);
        } break;
        case BTYPE_STRING: {
            return !strcmp(*(char**)left, right);
        } break;
        case BTYPE_INVALID:
            return false;
    }
    return false;
}

bool filter_func_greater(BaseType type, const void *left, const void *right)
{
    switch (type) {
        case BTYPE_INT32: {
            return (*(const int32_t*)left) > (*(const int32_t*)right);
        } break;
        case BTYPE_INT64: {
            return (*(const int64_t*)left) > (*(const int64_t*)right);
        } break;
        case BTYPE_STRING: {
            return strcmp(*(char**)left, right) > 0;
        } break;
        case BTYPE_INVALID:
            return false;
    }
    return false;
}

bool filter_func_greater_equal(BaseType type, const void *left, const void *right)
{
    return filter_func_equals(type, left, right) || filter_func_greater(type, left, right);
}

bool filter_func_lesser(BaseType type, const void *left, const void *right)
{
    switch (type) {
        case BTYPE_INT32: {
            return (*(const int32_t*)left) < (*(const int32_t*)right);
        } break;
        case BTYPE_INT64: {
            return (*(const int64_t*)left) < (*(const int64_t*)right);
        } break;
        case BTYPE_STRING: {
            return strcmp(*(char**)left, right) < 0;
        } break;
        case BTYPE_INVALID:
            return false;
    }
    return false;
}

bool filter_func_lesser_equal(BaseType type, const void *left, const void *right)
{
    return filter_func_equals(type, left, right) || filter_func_lesser(type, left, right);
}

bool filter_func_like(BaseType type, const void *left, const void *right)
{
    switch (type) {
        case BTYPE_INT32: {
            return (*(const int32_t*)left) == (*(const int32_t*)right);
        } break;
        case BTYPE_INT64: {
            return (*(const int64_t*)left) == (*(const int64_t*)right);
        } break;
        case BTYPE_STRING: {
            const char *str = *(const char**)left;
            const char *pattern = right;


            while (*str && *pattern) {
                // %: zero or more characters
                if (*pattern == '%') {
                    pattern++;
                    
                    // pattern ending with a %, since we are already here
                    // this must match
                    if (!*pattern)
                        return true;

                    while (*str && *str != *pattern)
                        str++;

                    // no match
                    if (!*str)
                        return false;
                }

                // _: exactly one character
                if (*pattern == '_') {
                    pattern++;
                    if (!pattern)
                        return true;
                    str++;
                    continue;
                }

                if (*str != *pattern)
                    return false;
            
                str++;
                pattern++;
            }

            // one of the strings ends prematurely
            if (*str + *pattern > 0 && *pattern != '%')
                return false;

            return true;
        } break;
        case BTYPE_INVALID:
            return false;
    }
    return false;
}

findfunc_t *filter_func_table[] = {
    [FILTER_EQUAL] = filter_func_equals,
    [FILTER_GREATER] = filter_func_greater,
    [FILTER_GREATER_EQUAL] = filter_func_greater_equal,
    [FILTER_LESSER] = filter_func_lesser,
    [FILTER_LESSER_EQUAL] = filter_func_lesser_equal,
    [FILTER_LIKE] = filter_func_like,
};


typedef struct FilterInterpResult {
    BazaResult res;
    IntList *rows;
} FilterInterpResult;

// interpret a list of filters performing the appropriate set operations and return
// the final list of rows which passed the filters
FilterInterpResult filter_interpret(TableMeta table, Filter *filter_list)
{
    Filter *filter = filter_list;

    IntList *rowset = NULL;
    FilterRelation rel = FILTER_REL_NONE;
    
    while (filter) {
        ColumnResult colres = table_column_get(table.id, filter->column);
        if (colres.result != RESULT_OK) {
            return (FilterInterpResult) {
                .res = RESULT_COLUMN_NOT_FOUND,
            };
        }
        ColumnMeta column = colres.meta;

        // Convert to appropriate type and get the matching columns
        TableFindResult tfres;
        switch (column.type) {
            case BTYPE_INT32:
            case BTYPE_INT64: {
                IntConvResult icres = str_to_int(filter->value);
                if (icres.result != RESULT_OK) {
                    intlist_free(rowset);

                    return (FilterInterpResult) {
                        .res = RESULT_FILTER_VALUE_TYPE,
                    };
                }

                tfres = table_find(table.id, column.id, filter_func_table[filter->op], &icres.value);
            } break;
            case BTYPE_STRING:
                tfres = table_find(table.id, column.id, filter_func_table[filter->op], filter->value);
                break;
            case BTYPE_INVALID:
                return (FilterInterpResult) {
                    .res = RESULT_SERVER_ERROR,
                };
        }

        if (tfres.res != RESULT_OK) {
            intlist_free(rowset);

            return (FilterInterpResult) {
                .res = tfres.res,
            };
        }
        
        // the lookup was successful, check if we have any relations with previous queries to deal with
        if (rel != FILTER_REL_NONE) {
            IntList *new = NULL;

            if (rel == FILTER_REL_AND) {
                new = intlist_intersection(rowset, tfres.matches);
            } else if (rel == FILTER_REL_OR) {
                new = intlist_union(rowset, tfres.matches);
            } 

            if (!new) {
                intlist_free(rowset);
                intlist_free(tfres.matches);

                return (FilterInterpResult) {
                    .res = RESULT_SERVER_ERROR,
                };
            }

            intlist_free(rowset); // we no longer need the previous iteration's set
            intlist_free(tfres.matches); // ..nor do we need current iteration's set, since we are allocing new
            rowset = new;
        } else {
            // .. this is the first filter, just set the rowset
            rowset = tfres.matches;
        }

        rel = filter->next_relation;
        filter = filter->next;
    }

    return (FilterInterpResult) {
        .res = RESULT_OK,
        .rows = rowset,
    };
}

// check if all the values successfuly convert to their designated types
BazaResult validate_value_types(ColumnMetaList *columns, StrList *values)
{
    ColumnMetaList *col = columns;
    StrList *value = values;

    while (col) {
        switch (col->meta->type) {
            case BTYPE_INT32: {
                IntConvResult ires = str_to_int(value->str);
                if (ires.result != RESULT_OK)
                    return ires.result;
            } break;
            case BTYPE_INT64: {
                IntConvResult ires = str_to_int(value->str);
                if (ires.result != RESULT_OK)
                    return ires.result;
            } break;
            case BTYPE_STRING: 
                break;
            default: {} // NOP - shouldn't happen
        }

        col = col->next;
        value = value->next;
    }

    return RESULT_OK;
}

QueryResponse interpret_select_filter(const Query *query, TableMeta table,
                                      ColumnMetaList *columns)
{
    FilterInterpResult fres = filter_interpret(table, query->select_filters);

    if (fres.res != RESULT_OK) {
        columnlist_free(columns);
        return (QueryResponse) { .result = fres.res };
    }

    IntList *column_ids = intlist_empty();
    ColumnMetaList *col = columns;
    while (col) {
        intlist_push(column_ids, col->meta->id);
        col = col->next;
    }

    IntList *row = fres.rows;
    while (row && row->value != INTLIST_NULL) {
        // TODO: fetch data into bintable
        table_row_print(table.id, column_ids, row->value);
        puts("");

        row = row->next;
    }

    intlist_free(fres.rows);
    intlist_free(column_ids);
    columnlist_free(columns);

    // TODO: return data
    return (QueryResponse) {
        .result = RESULT_OK,
        .data = NULL,
    };
}

QueryResponse interpret_select_all(const Query *query, TableMeta table,
                                   ColumnMetaList *columns)
{
    for (uint64_t row = 0; row < table.row_count; row++) {
        table_row_print(table.id, NULL, row);
        puts("");
    }

    columnlist_free(columns);

    // TODO: return data
    return (QueryResponse) {
        .result = RESULT_OK,
        .data = NULL,
    };
}

QueryResponse interpret_select(const Query *query)
{
    TableResult tabres = db_table_get(query->table_name);
    if (tabres.result != RESULT_OK) {
        return (QueryResponse) {
            .result = tabres.result,
        };
    }
    TableMeta table = tabres.meta;

    // fetch column meta
    ColumnMetaList *columns = table_column_get_list(table.id, query->select_columns);
    if (!columns) {
        return (QueryResponse) {
            .result = RESULT_COLUMN_NOT_FOUND,
        };
    }

    if (query->select_filters) {
        return interpret_select_filter(query, table, columns);
    } else {
        return interpret_select_all(query, table, columns);
    }
}

QueryResponse interpret_create(const Query *query)
{
    TableResult tabres = db_table_new(query->table_name);
    if (tabres.result != RESULT_OK) {
        return (QueryResponse) {
            .result = tabres.result,
        };
    }
    TableMeta table = tabres.meta;

    // convert and check the validity of all the types
    IntList *type_list = intlist_empty();

    StrList *strtype = query->create_types;
    while (strtype && strtype->str) {
        BaseType btype = basetype_from_str(strtype->str);

        if (btype == BTYPE_INVALID) {
            intlist_free(type_list);

            return (QueryResponse) {
                .result = RESULT_INVALID_QUERY,
            };
        }

        intlist_push(type_list, btype);
        strtype = strtype->next;
    }

    // create the columns
    StrList *column = query->create_columns;
    IntList *type = type_list;

    while (column && column->str) {
        BazaResult res = table_column_new(table.id, type->value, column->str);
        if (res != RESULT_OK) {
            FATAL("failed to create a new column: %s", result_str(res));
        }

        type = type->next;
        column = column->next;
    }

    intlist_free(type_list);

    return (QueryResponse) {
        .result = RESULT_OK,
        .data = NULL,
    };
}

// insert values into [colums] at [row]
BazaResult insert_values(const Query *query, TableMeta table, 
                         ColumnMetaList *columns, StrList *values, uint64_t row,
                         bool replace)
{
    ColumnMetaList *col = columns;
    StrList *value = values;

    while (col) {
        switch (col->meta->type) {
            case BTYPE_INT32: {
                IntConvResult ires = str_to_int(value->str);
                if (ires.result != RESULT_OK)
                    return ires.result;

                uint32_t *slot = table_column_get_row(table.id, col->meta->id, row);
                *slot = ires.value;
            } break;
            case BTYPE_INT64: {
                IntConvResult ires = str_to_int(value->str);
                if (ires.result != RESULT_OK)
                    return ires.result;

                uint64_t *slot = table_column_get_row(table.id, col->meta->id, row);
                *slot = ires.value;

            } break;
            case BTYPE_STRING:  {
                char **slot = table_column_get_row(table.id, col->meta->id, row);
                if (replace)
                    free(*slot);
                *slot = strdup(value->str);
            } break;
            default: {} // NOP - shouldn't happen
        }

        col = col->next;
        value = value->next;
    }

    return RESULT_OK;
}

QueryResponse interpret_insert(const Query *query)
{
    TableResult tabres = db_table_get(query->table_name);
    if (tabres.result != RESULT_OK) {
        return (QueryResponse) {
            .result = tabres.result,
        };
    }
    TableMeta table = tabres.meta;

    // TODO: check for duplicate columns

    // fetch column meta
    ColumnMetaList *columns = table_column_get_list(table.id, NULL);
    if (!columns) {
        return (QueryResponse) {
            .result = RESULT_COLUMN_NOT_FOUND,
        };
    }

    // check if the type conversions succeed
    BazaResult res = validate_value_types(columns, query->insert_values);

    if (res != RESULT_OK) {
        return (QueryResponse) {
            .result = res,
        };
    }

    table_row_add(table.id);
    res = insert_values(query, table, columns, 
                        query->insert_values, table.row_count, false);

    columnlist_free(columns);

    if (res != RESULT_OK) {
        return (QueryResponse) {
            .result = RESULT_SERVER_ERROR,
        };
    }

    return (QueryResponse) {
        .result = RESULT_OK,
    };
}

QueryResponse interpret_delete_filtered(const Query *query, TableMeta table)
{
    FilterInterpResult fres = filter_interpret(table, query->delete_filters);
    if (fres.res != RESULT_OK)
        return (QueryResponse) { .result = fres.res };

    IntList *filter_rows = fres.rows;

    uint64_t delete_count = 0;
    IntList *row = filter_rows;
    while (row && row->value != INTLIST_NULL) {
        BazaResult res = table_row_delete(table.id, row->value - delete_count);
        if (res != RESULT_OK) {
            return (QueryResponse) {
                .result = res,
            };
        }
        delete_count++;
        row = row->next;
    }

    intlist_free(filter_rows);

    return (QueryResponse) {
        .result = RESULT_OK,
    };
}

QueryResponse interpret_delete_all(const Query *query, TableMeta table)
{
    for (uint64_t row = 0; row < table.row_count; row++) {
        BazaResult res = table_row_delete(table.id, 0);
        if (res != RESULT_OK) {
            return (QueryResponse) {
                .result = res,
            };
        }
    }

    return (QueryResponse) {
        .result = RESULT_OK,
    };
}

QueryResponse interpret_delete(const Query *query)
{
    TableResult tabres = db_table_get(query->table_name);
    if (tabres.result != RESULT_OK) {
        return (QueryResponse) {
            .result = tabres.result,
        };
    }
    TableMeta table = tabres.meta;

    if (query->delete_filters) {
        return interpret_delete_filtered(query, table);
    }
    return interpret_delete_all(query, table);
}

QueryResponse interpret_update_filtered(const Query *query, TableMeta table, ColumnMetaList *columns)
{
    FilterInterpResult fres = filter_interpret(table, query->update_filters);
    if (fres.res != RESULT_OK)
        return (QueryResponse) { .result = fres.res };
    IntList *filter_rows = fres.rows;

    BazaResult validate_res = validate_value_types(columns, query->update_values);
    if (validate_res != RESULT_OK) {
        return (QueryResponse) {
            .result = RESULT_VALUE_TYPE, 
        };
    }

    IntList *row = filter_rows;
    while (row && row->value != INTLIST_NULL) {
        BazaResult res = insert_values(query, table, columns, query->update_values, row->value, true);
        if (res != RESULT_OK) {
            return (QueryResponse) {
                .result = res,
            };
        }
        row = row->next;
    }

    intlist_free(filter_rows);
    
    return (QueryResponse) {
        .result = RESULT_OK,
    };
}

QueryResponse interpret_update_all(const Query *query, TableMeta table, ColumnMetaList *columns)
{
    BazaResult validate_res = validate_value_types(columns, query->update_values);
    if (validate_res != RESULT_OK) {
        return (QueryResponse) {
            .result = RESULT_VALUE_TYPE, 
        };
    }

    for (uint64_t row = 0; row < table.row_count; row++) {
        BazaResult res = insert_values(query, table, columns, query->update_values, row, true);
        if (res != RESULT_OK) {
            return (QueryResponse) {
                .result = res,
            };
        }
    }

    return (QueryResponse) {
        .result = RESULT_OK,
    };
}

QueryResponse interpret_update(const Query *query)
{
    TableResult tabres = db_table_get(query->table_name);
    if (tabres.result != RESULT_OK) {
        return (QueryResponse) {
            .result = tabres.result,
        };
    }
    TableMeta table = tabres.meta;

    // fetch column meta
    ColumnMetaList *columns = table_column_get_list(table.id, query->update_columns);
    if (!columns) {
        return (QueryResponse) {
            .result = RESULT_COLUMN_NOT_FOUND,
        };
    }

    QueryResponse resp;

    if (query->update_filters) {
        resp = interpret_update_filtered(query, table, columns);
    } else {
        resp = interpret_update_all(query, table, columns);
    }

    columnlist_free(columns);

    return resp;
}

QueryResponse interpret_query(const Query *query)
{
    switch (query->type) {
        case QUERY_SELECT:
            return interpret_select(query);
        case QUERY_CREATE:
            return interpret_create(query);
        case QUERY_INSERT:
            return interpret_insert(query);
        case QUERY_DELETE:
            return interpret_delete(query);
        case QUERY_UPDATE:
            return interpret_update(query);
    }
    FATAL("UNIMPLEMENTED");
}

#ifdef BAZATEST_INTERPRETER
void do_query(const char *q)
{
    QueryParseResult res = query_parse(q);

    if (res.result != RESULT_OK) {
        printf("%s: %s", result_str(res.result), res.error_msg);
    } else {
        printf("SQL INPUT: '%s'\n", q);
        query_print(res.query);
    }

    QueryResponse resp = interpret_query(res.query);
    if (resp.result != RESULT_OK) {
        printf("INTERP ERR: %s\n", result_str(resp.result));
    }

    query_free(res.query);
}

int main()
{
    storage_init();

    const char *queries[] = {
        "CREATE TABLE testing ("
        "    one String, "
        "    two int32, "
        "    three String "
        ")",

        "INSERT INTO testing VALUES (hello, 50, there)",
        "INSERT INTO testing VALUES (different, 50, row)",
        "INSERT INTO testing VALUES (hi, 20, there)",
        "INSERT INTO testing VALUES (between, 35, values)",
        "INSERT INTO testing VALUES (hello, 35, values)",
        "INSERT INTO testing VALUES (hello, 50, lastone)",

        "SELECT * FROM testing WHERE two = 50",
    };

    for (int i = 0; i < sizeof(queries)/sizeof(queries[0]); i++) {
        do_query(queries[i]);
        puts("");
    }

    storage_deinit();

    return 0;
}
#endif
