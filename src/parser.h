// SQL Query parser - takes in raw text SQL statements and turns them into
// a Query struct, which is in turn understood 
// by the interpreter (see interpreter.h).
#ifndef _PARSER_H
#define _PARSER_H

#include "util/str.h"
#include "util/result.h"

typedef enum {
    QUERY_SELECT,
    QUERY_CREATE,
    QUERY_INSERT,
    QUERY_DELETE,
    QUERY_UPDATE,
} QueryType;

const char *querytype_str(QueryType type);

typedef enum FilterOp {
    FILTER_INVALID,
    FILTER_NONE,
    FILTER_EQUAL,
    FILTER_NOT_EQUAL,
    FILTER_GREATER,
    FILTER_GREATER_EQUAL,
    FILTER_LESSER,
    FILTER_LESSER_EQUAL,
    FILTER_LIKE,
} FilterOp;

const char *filterop_to_str(FilterOp op);
FilterOp filterop_from_str(const char *str);

typedef enum FilterRelation {
    FILTER_REL_INVALID,
    FILTER_REL_NONE,
    FILTER_REL_AND,
    FILTER_REL_OR,
} FilterRelation;

const char *filterrel_to_str(FilterRelation relation);
FilterRelation filterrel_from_str(const char *str);

typedef struct Filter {
    FilterOp op;
    char *value;
    char *column;
    FilterRelation next_relation;
    struct Filter *next;
} Filter;

void filter_print(Filter *filter);

typedef enum SortDirection {
    SORT_ASCENDING,
    SORT_DESCENDING,
    SORT_INVALID,
} SortDirection;

SortDirection sortdirection_from_str(const char *str);
const char *sortdirection_to_str(SortDirection direction);

/// Internal server-side representation of a query.
/// In order to avoid free-releated errors, we assume that
/// the query struct owns all the memory/pointers in its body
/// and is responsible for freeing them.
typedef struct Query {
    QueryType type; 
    char *table_name;
    union {
        struct { // QUERY_SELECT
            Filter *select_filters;
            StrList *select_columns;
            char *select_sort_column;
            SortDirection select_sort_direction;
        };
        struct { // QUERY_CREATE
            // names and types of columns, might be merged
            // into a single list in the future
            StrList *create_columns;
            StrList *create_types;
        };
        struct { // QUERY_CREATE
            StrList *insert_values;
            // columns will be added in future versions where
            // we will support null values. Since we do not, all
            // columns need to filled anyways
            // StringList *insert_columns;
        };
        struct { // QUERY_DELETE
            Filter *delete_filters;
        };
        struct { // QUERY_UPDATE
            Filter *update_filters;
            StrList *update_columns;
            StrList *update_values;
        };
    };
} Query;

void query_print(Query *query);
void query_free(Query *query);

typedef struct {
    BazaResult result;
    union {
        Query *query;
        const char *error_msg;
    };
} QueryParseResult;

QueryParseResult query_parse(const char *query_string);

#endif /* _PARSER_H */
