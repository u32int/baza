#include "parser.h"

#include "util/result.h"
#include "util/defs.h"
#include "util/str.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/// Checks if [tok]->str matches [keyword] and if so skips to the next token.
/// must be called from within a function returning QueryParseResult
#define EXPECT_KEYWORD(tok, keyword, error) do { \
    if (!tok || !str_ieq(tok->str, keyword)) { \
        return (QueryParseResult) { \
            .result = RESULT_ERR_SQL_PARSE, \
            .error_msg = error \
        }; \
    } \
    tok = tok->next; \
} while(0)

/// Checks if [tok] exists and if so assigns [value] to [variable], otherwise
/// returning an error with [error] as the message.
/// must be called from within a function returning QueryParseResult
#define EXPECT_VARIABLE(tok, variable, value, error) do { \
    if (!tok) { \
        return (QueryParseResult) { \
            .result = RESULT_ERR_SQL_PARSE, \
            .error_msg = error \
        }; \
    } \
    variable = value; \
    tok = tok->next; \
} while(0)

/// Checks if [tok] exists, returning an error if it doesn't.
/// must be called from within a function returning QueryParseResult
#define EXPECT_TOKEN(tok, error) do { \
    if (!tok) { \
        return (QueryParseResult) { \
            .result = RESULT_ERR_SQL_PARSE, \
            .error_msg = error \
        }; \
    } \
} while(0)

typedef struct ListParseResult {
    BazaResult res;
    union {
        const char *error_msg;
        struct {
            StrList *list;
            StrList *outer_last;  // the last element of the list as understood by the caller
        };
    };
} ListParseResult;

#define LIST_DELIMITER ","

/// Extract an SQL list from tokens starting at [start].
/// If [delim] is not NULL, it is assumed to contain at least two chars,
/// signifying the start and end delimiter. For instance:
/// brack_delim should = "()" for a list like "(1, 2, 3)"
/// Invalid lists:
///  1,2,3)  (IFF delim is _not_ NULL of course)
/// Extra commas are ignored for now but might throw some sort of an error
/// in a later revision
ListParseResult extract_sql_list(StrList *start, const char *delim)
{
    StrList *final;
    StrList *cur = start;

    if (delim) {
        if (start->str[0] != delim[0]) {
            return (ListParseResult) {
                .res = RESULT_ERR_SQL_PARSE,
                .error_msg = "Expected a list "
                             "to start with a valid delimiter"
            };
        }

        bool bail = false;
        if (start->str[start->strlen-1] == delim[1]) {
            start->str[start->strlen-1] = 0;
            bail = true;
        }

        // seek the start string one char to step over the opening delimiter
        final = strlist_from_split(start->str + 1, LIST_DELIMITER);

        cur = cur->next;

        if (bail)
            goto bail;
    } else {
        final = strlist_empty();
    }

    for (;;) {
        bool last;

        // TODO: sort of unclean and maybe redundant to check the strlen?
        if (delim) {
            last = cur->strlen > 0 && cur->str[cur->strlen-1] == delim[1];
            if (last) {
                cur->str[cur->strlen-1] = 0;
            }
        } else {
            last = !(cur->strlen > 0 && cur->str[cur->strlen-1] == ',');
        }

        StrList *sub_splt = strlist_from_split(cur->str, ",");
        final = strlist_merge(final, sub_splt);
        cur = cur->next;

        if (last)
            break;
    }

bail:
    return (ListParseResult) {
        .res = RESULT_OK,
        .list = final,
        .outer_last = cur,
    };
}

const char *querytype_str(QueryType type)
{
    switch (type) {
        case QUERY_SELECT: return "SELECT";
        case QUERY_CREATE: return "CREATE";
        case QUERY_INSERT: return "INSERT";
        case QUERY_DELETE: return "DELETE";
        case QUERY_UPDATE: return "UPDATE";
    }
    return NULL;
}

#define STRFILTER_EQUAL "="
#define STRFILTER_NOT_EQUAL "!="
#define STRFILTER_GREATER ">"
#define STRFILTER_GREATER_EQUAL ">="
#define STRFILTER_LESSER "<"
#define STRFILTER_LESSER_EQUAL "<="
#define STRFILTER_LIKE "like"

const char *filterop_to_str(FilterOp op)
{
    switch (op) {
        case FILTER_INVALID: return "!invalid filter!";
        case FILTER_NONE: return "none";
        case FILTER_EQUAL: return STRFILTER_EQUAL;
        case FILTER_NOT_EQUAL: return STRFILTER_NOT_EQUAL;
        case FILTER_GREATER: return STRFILTER_GREATER;
        case FILTER_GREATER_EQUAL: return STRFILTER_GREATER_EQUAL;
        case FILTER_LESSER: return STRFILTER_LESSER;
        case FILTER_LESSER_EQUAL: return STRFILTER_LESSER_EQUAL;
        case FILTER_LIKE: return STRFILTER_LIKE;
    }
    return NULL;
}

FilterOp filterop_from_str(const char *str)
{
    if (str_ieq(str, STRFILTER_EQUAL))
        return FILTER_EQUAL;
    else if (str_ieq(str, STRFILTER_NOT_EQUAL))
        return FILTER_NOT_EQUAL;
    else if (str_ieq(str, STRFILTER_GREATER))
        return FILTER_GREATER;
    else if (str_ieq(str, STRFILTER_GREATER_EQUAL))
        return FILTER_GREATER_EQUAL;
    else if (str_ieq(str, STRFILTER_LESSER))
        return FILTER_LESSER;
    else if (str_ieq(str, STRFILTER_LESSER_EQUAL))
        return FILTER_LESSER_EQUAL;
    else if (str_ieq(str, STRFILTER_LIKE))
        return FILTER_LIKE;

    return FILTER_INVALID;
}

#define STRFILTERREL_AND "and"
#define STRFILTERREL_OR  "or"

const char *filterrel_to_str(FilterRelation rel)
{
    switch (rel) {
        case FILTER_REL_INVALID: return "!invalid filter relation!";
        case FILTER_REL_NONE: return "none";
        case FILTER_REL_AND: return STRFILTERREL_AND;
        case FILTER_REL_OR: return STRFILTERREL_OR;
    }
    return NULL;
}

FilterRelation filterrel_from_str(const char *str)
{
    if (str_ieq(str, STRFILTERREL_AND))
        return FILTER_REL_AND;
    else if (str_ieq(str, STRFILTERREL_OR))
        return FILTER_REL_OR;
    return FILTER_REL_INVALID;
}

void filter_print(Filter *filter)
{
    printf("Filter { column: '%s' op: '%s' value: '%s' next_rel: '%s' }", filter->column,
           filterop_to_str(filter->op), filter->value, filterrel_to_str(filter->next_relation));
}

Filter *filter_empty()
{
    Filter *filter = malloc(sizeof(Filter));
    if (!filter)
        return NULL;

    *filter = (Filter) {
        .value = NULL,
        .next = NULL,
        .op = FILTER_NONE,
        .next_relation = FILTER_REL_NONE,
    };

    return filter;
}

void filter_free(Filter *filter)
{
    Filter *cur = filter, *nxt;
    while (cur) {
        nxt = cur->next;
        if (cur->value)
            free(cur->value);
        if (cur->column)
            free(cur->column);
        free(cur);
        cur = nxt;
    }
}

typedef struct FilterParseResult {
    BazaResult res;
    union {
        const char *err_msg;
        struct {
            Filter *filters;
            StrList *last_tok;
        };
    };
} FilterParseResult;

FilterParseResult parse_filters(StrList *tok)
{
    Filter *filters = filter_empty();
    Filter *cur = filters;

    if (!tok) {
        return (FilterParseResult) { 
            .res = RESULT_ERR_SQL_PARSE, 
            .err_msg = "expected a filter after WHERE"
        };
    }

    for (;;) {
        // WHERE <column> <filterop> <value> [FILTER_REL..]
        //       ^      ^
        
        // we know this token exists because we checked either before 
        // the loop or at the end of last iter.
        cur->column = strdup(tok->str);
        tok = tok->next;

        // WHERE <column> <filterop> <value> [FILTER_REL..]
        //                ^        ^

        FilterOp op = filterop_from_str(tok->str);
        if (op == FILTER_INVALID) {
            filter_free(filters);

            return (FilterParseResult) { 
                .res = RESULT_ERR_SQL_PARSE, 
                .err_msg = "invalid operator in a filter (where clause)"
            };
        }
        cur->op = op;
        tok = tok->next;

        // WHERE <column> <filterop> <value> [FILTER_REL..]
        //                           ^     ^

        if (!tok) {
            filter_free(filters);

            return (FilterParseResult) { 
                .res = RESULT_ERR_SQL_PARSE, 
                .err_msg = "expected a value after an operator in a filter (where clause)"
            };
        }

        cur->value = strdup(tok->str);
        tok = tok->next;

        if (!tok) {
            // .. then we are done
            return (FilterParseResult) { 
                .res = RESULT_OK, 
                .filters = filters,
                .last_tok = tok,
            };
        }

        // .. else we expect an 'AND' or an 'OR'
        FilterRelation rel = filterrel_from_str(tok->str);
        if (rel == FILTER_REL_INVALID)  {
            // We still return OK because a WHERE clause can be followed 
            // by other things like 'GROUP BY' etc.
            return (FilterParseResult) { 
                .res = RESULT_OK, 
                .filters = filters,
                .last_tok = tok,
            };
        }

        cur->next_relation = rel;
        tok = tok->next;

        if (!tok) {
            return (FilterParseResult) { 
                .res = RESULT_ERR_SQL_PARSE, 
                .err_msg = "expected another filter condition following 'AND' or 'OR'",
            };
        }

        // we expect another filter following this one
        cur->next = filter_empty();
        cur = cur->next;
    }
}

#define SORT_ASCENDING_STR "ASC"
#define SORT_DESCENDING_STR "DESC"
#define SORT_INVALID_STR "!INVALID SORT DIRECTION!"

SortDirection sortdirection_from_str(const char *str)
{
    if (str_ieq(str, SORT_ASCENDING_STR))
        return SORT_ASCENDING;
    else if (str_ieq(str, SORT_DESCENDING_STR))
        return SORT_DESCENDING;
    return SORT_INVALID;
}

const char *sortdirection_to_str(SortDirection direction)
{
    switch (direction) {
        case SORT_ASCENDING: return SORT_ASCENDING_STR;
        case SORT_DESCENDING: return SORT_DESCENDING_STR;
        default: return SORT_INVALID_STR;
    }
}

void query_print(Query *query)
{
    printf("Query {\n"
           "  type: %s\n"
           "  table_name: %s\n",
           querytype_str(query->type),
           query->table_name);

    switch(query->type) {
        case QUERY_SELECT:
            fputs("  columns: ", stdout);
            strlist_print(query->select_columns);
            Filter *fcur = query->select_filters;
            fputs("\n  filters: ", stdout);
            while (fcur) {
                fputs("\n   ", stdout);
                filter_print(fcur);
                fcur = fcur->next;
            }
            if (query->select_sort_column) {
                printf("\n  order by: %s (%s)", 
                       query->select_sort_column, 
                       sortdirection_to_str(query->select_sort_direction));
            }
            break;
        case QUERY_CREATE:
            fputs("  column names: ", stdout);
            strlist_print(query->create_columns);
            putchar('\n');
            fputs("  column types: ", stdout);
            strlist_print(query->create_types);
            break;
        case QUERY_INSERT:
            fputs("  values: ", stdout);
            strlist_print(query->insert_values);
            break;
        case QUERY_DELETE: {
            Filter *fcur = query->delete_filters;
            fputs("  filters: ", stdout);
            while (fcur) {
                fputs("\n   ", stdout);
                filter_print(fcur);
                fcur = fcur->next;
            }
        } break;
        case QUERY_UPDATE: {
            fputs("  column names: ", stdout);
            strlist_print(query->update_columns);
            putchar('\n');
            fputs("  column values: ", stdout);
            strlist_print(query->update_values);
            if (query->update_filters) {
                Filter *fcur = query->update_filters;
                fputs("\n  filters: ", stdout);
                while (fcur) {
                    fputs("\n   ", stdout);
                    filter_print(fcur);
                    fcur = fcur->next;
                }
            }
        } break;
    }
    puts("\n}");
}

Query *query_new()
{
    Query *query = malloc(sizeof(Query));
    if (!query)
        return NULL;
    
    *query = (Query) {
        .table_name = NULL,
    };

    return query;
}

void query_free(Query *query)
{
    free(query->table_name);

    switch(query->type) {
        case QUERY_SELECT:
            if (query->select_columns)
                strlist_free(query->select_columns);
            if (query->select_filters)
                filter_free(query->select_filters);
            if (query->select_sort_column)
                free(query->select_sort_column);
            break;
        case QUERY_CREATE:
            if (query->create_columns)
                strlist_free(query->create_columns);
            if (query->create_types)
                strlist_free(query->create_types);
            break;
        case QUERY_INSERT:
            if (query->insert_values)
                strlist_free(query->insert_values);
            break;
        case QUERY_DELETE:
            if (query->delete_filters)
                filter_free(query->delete_filters);
            break;
        case QUERY_UPDATE:
            if (query->update_values)
                strlist_free(query->update_values);
            if (query->update_columns)
                strlist_free(query->update_columns);
            if (query->update_filters)
                filter_free(query->update_filters);
            break; 
    }

    free(query);
}

/// Try to parse a 'WHERE' clause. Returns NULL if the token stream
/// doesn't contain a valid WHERE
Filter *try_parse_where(StrList **split)
{
    StrList *tok = *split;

    //  WHERE <filters> 
    //  ^   ^

    if (!tok || !str_ieq(tok->str, "WHERE")) {
        // not a valid WHERE
        return NULL;
    } 
    tok = tok->next;

    //  WHERE <filters> 
    //        ^       ^

    // TODO: this filter parsing is _very_ primitive. We should honor parenthesis 
    // and build a tree from this in the future.

    FilterParseResult fpr = parse_filters(tok);
    if (fpr.res != RESULT_OK) {
        // TODO remove this silly unwrap
        FATAL("filter parse error: %s", fpr.err_msg);
    }

    *split = fpr.last_tok;
    return fpr.filters;
}

typedef struct OrderParseResult {
    BazaResult result;
    SortDirection direction;
    char *column;
} OrderParseResult;

OrderParseResult try_parse_order(StrList **split)
{
    StrList *tok = *split;

    OrderParseResult ret = (OrderParseResult) {
        .direction = SORT_INVALID,
        .column = NULL,
        .result = RESULT_ERR_SQL_PARSE
    };

    //  ORDER BY <column> <direction>
    //  ^      ^
    if (!tok || !str_ieq(tok->str, "ORDER"))
        return ret;
    tok = tok->next;

    if (!tok || !str_ieq(tok->str, "BY"))
        return ret;
    tok = tok->next;

    //  ORDER BY <column> <direction>
    //           ^      ^
    if (!tok)
        return ret;
    ret.column = strdup(tok->str);
    tok = tok->next;

    //  ORDER BY <column> <direction>
    //                    ^         ^
    if (!tok) {
        free(ret.column);
        return ret;
    }
    
    ret.direction = sortdirection_from_str(tok->str);
    tok = tok->next;
    
    *split = tok;
    ret.result = RESULT_OK;
    return ret;
}

/// Parse a "SELECT" query
/// examples of valid queries:
///    SELECT * FROM table;
///    SELECT * FROM table WHERE name = 'Bob';
///    SELECT name, age FROM table WHERE name = 'Bob';
QueryParseResult query_parse_select(Query *query, StrList *split)
{
    query->type = QUERY_SELECT;
    query->select_columns = NULL;
    query->select_filters = NULL;
    query->select_sort_column = NULL;

    StrList *tok = split;
    EXPECT_TOKEN(tok, "Empty SELECT clause, no column names provided");

    // SELECT <columns> FROM <table>
    //        ^       ^
    if (!strcmp(tok->str, "*")) {
        query->select_columns = NULL;
        tok = tok->next;
    } else {
        ListParseResult lpr = extract_sql_list(tok, NULL);

        if (lpr.res == RESULT_OK) {
            query->select_columns = lpr.list;
        } else {
            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = lpr.error_msg,
            };
        }

        tok = lpr.outer_last;
    }

    // SELECT <columns> FROM <table>
    //                  ^  ^
    EXPECT_KEYWORD(tok, "from", "expected FROM after a column list");
    
    // SELECT <columns> FROM <table>
    //                       ^     ^

    EXPECT_VARIABLE(tok, query->table_name, strdup(tok->str), "expected a table name after FROM in a SELECT");


    // [Optional] scan for WHERE and ORDER BY (might appear in any order)
    // SELECT (...) WHERE/ORDER BY
    //              ^            ^
    while (tok) {
        Filter *filters = try_parse_where(&tok);
        // TODO: check for duplicate matches?
        if (filters)
            query->select_filters = filters;

        OrderParseResult opres = try_parse_order(&tok);
        if (opres.result == RESULT_OK) {
            if (opres.direction == SORT_INVALID) {
                free(opres.column);
                return (QueryParseResult) {
                    .result = RESULT_ERR_SQL_PARSE,
                    .error_msg = "invalid sort direction, must be one of (ASC/DESC)",
                };
            }

            query->select_sort_column = opres.column;
            query->select_sort_direction = opres.direction;
        }
    }

    return (QueryParseResult) {
        .result = RESULT_OK,
        .query = query,
    };
}

/// Examples of valid CREATE queries:
/// CREATE TABLE TableName (
///     Name string,
///     FavoriteNumber int64
/// )
///
/// CREATE TABLE TableName 
/// (
///     Name string,
///     FavoriteNumber int64
/// )
QueryParseResult query_parse_create(Query *query, StrList *split)
{
    query->type = QUERY_CREATE;
    query->create_columns = NULL;
    query->create_types = NULL;

    StrList *tok = split;

    // CREATE TABLE TableName (
    //        ^   ^
    EXPECT_KEYWORD(tok, "table", "expected TABLE after CREATE");

    // CREATE TABLE TableName (
    //              ^       ^
    // (...)
    //    Column type{,}
    // (...)
    // )
    //
    EXPECT_VARIABLE(tok, query->table_name, strdup(tok->str), "expected a table name after TABLE");

    // TODO: replace the following section by an extract_sql_list()?

    // CREATE TABLE TableName (
    //                        ^

    // sure.. "(" is a keyword!
    EXPECT_KEYWORD(tok, "(", "expected a '(' after the table name. Forgot a space?");

    // CREATE TABLE TableName (
    // (...)
    //    Column type{,}
    //    ^            ^
    // (...)
    // )

    query->create_columns = NULL;
    query->create_types = NULL;

    for (;;) {
        // 1) parse the column name
        if (!tok) {
            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = "inside of column definition section of a"
                             "CREATE statement; expected a column name"
            };
        }

        if (!query->create_columns)
            query->create_columns = strlist_new(tok->str);
        else
            strlist_push(query->create_columns, tok->str);

        tok = tok->next;

        // 2) parse the type name
        if (!tok) {
            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = "inside of column definition section of a "
                             "CREATE statement; expected a column name. "
                             "Perhaps you forgot to remove a comma from the "
                             "last Column-Type pair?"
            };
        }

        bool has_comma = false;
        if (tok->strlen > 0 && tok->str[tok->strlen-1] == ',') {
            tok->str[tok->strlen-1] = 0;
            has_comma = true;
        }

        if (!query->create_types)
            query->create_types = strlist_new(tok->str);
        else
            strlist_push(query->create_types, tok->str);
        
        if (!has_comma) {
            tok = tok->next;
            break;
        }
        tok = tok->next;
    }

    // CREATE TABLE TableName (
    // (...)
    //    Column type{,}
    // (...)
    // )
    // ^

    // sure.. ")" is a keyword!
    EXPECT_KEYWORD(tok, ")", "expected a ')' after the column definition section");

    return (QueryParseResult) {
        .result = RESULT_OK,
        .query = query,
    };
}

/// Examples of valid INSERT queries:
/// INSERT INTO table_name VALUES (5, "witam", 7)
/// INSERT INTO table_name VALUES ( 5, "witam", 7 )
QueryParseResult query_parse_insert(Query *query, StrList *split)
{
    query->type = QUERY_INSERT;
    query->insert_values = NULL;

    StrList *tok = split;

    // INSERT INTO table_name VALUES (...)
    //        ^  ^
    EXPECT_KEYWORD(tok, "into", "expected INTO after INSERT");

    // INSERT INTO table_name VALUES (...)
    //             ^        ^
    EXPECT_VARIABLE(tok, query->table_name, strdup(tok->str), 
                    "expected a table name after INTO in an INSERT statement");

    // INSERT INTO table_name VALUES (...)
    //                        ^    ^
    EXPECT_KEYWORD(tok, "values", "expected VALUES after the table name");

    // INSERT INTO table_name VALUES (...)
    //                               ^   ^
    ListParseResult lpr = extract_sql_list(tok, "()");
    if (lpr.res == RESULT_OK) {
        query->insert_values = lpr.list;
    } else {
        return (QueryParseResult) {
            .result = RESULT_ERR_SQL_PARSE,
            .error_msg = lpr.error_msg,
        };
    }

    return (QueryParseResult) {
        .result = RESULT_OK,
        .query = query,
    };
}

QueryParseResult query_parse_delete(Query *query, StrList *split)
{
    query->type = QUERY_DELETE;
    query->delete_filters = NULL;

    StrList *tok = split;

    // DELETE FROM table WHERE conditions 
    //        ^  ^
    EXPECT_KEYWORD(tok, "from", "expected FROM after DELETE");

    // DELETE FROM table WHERE conditions 
    //             ^   ^
    EXPECT_VARIABLE(tok, query->table_name, strdup(tok->str), "expected a table name after FROM in DELETE");

    // [Optional] (if no where clause is specified, this query deletes all rows)
    // DELETE FROM table WHERE conditions 
    //                   ^   ^

    Filter *filters = try_parse_where(&tok);
    if (filters)
        query->delete_filters = filters;

    return (QueryParseResult) {
        .result = RESULT_OK,
        .query = query,
    };
}

QueryParseResult query_parse_update(Query *query, StrList *split)
{
    query->type = QUERY_UPDATE;
    query->update_filters = NULL;
    query->update_columns = NULL;
    query->update_values = NULL;

    StrList *tok = split;

    // UPDATE table SET column = value, ... WHERE condition/filter
    //        ^   ^ 
    EXPECT_VARIABLE(tok, query->table_name, strdup(tok->str), "expected a table name UPDATE");

    // UPDATE table SET column = value, ... WHERE condition/filter
    //              ^ ^ 
    EXPECT_KEYWORD(tok, "set", "expected SET after table name in UPDATE");

    StrList *columns = strlist_empty();
    StrList *values = strlist_empty();

    for (;;) {
        // SET column = value, ... WHERE condition/filter
        //     ^    ^ 
        if (!tok) {
            strlist_free(columns);
            strlist_free(values);

            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = "expected a column name in a SET assignment",
            };
        }

        strlist_push(columns, tok->str);
        tok = tok->next;

        // SET column = value, ... WHERE condition/filter
        //            ^ 

        if (!tok || !str_ieq(tok->str, "=")) {
            strlist_free(columns);
            strlist_free(values);

            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = "expected '=' after column name in a SET assignment",
            };
        }

        tok = tok->next;

        // SET column = value, ... WHERE condition/filter
        //              ^   ^ 

        if (!tok) {
            strlist_free(columns);
            strlist_free(values);

            return (QueryParseResult) {
                .result = RESULT_ERR_SQL_PARSE,
                .error_msg = "expected a column name in a SET assignment",
            };
        }

        strlist_push(values, tok->str);

        if (tok->strlen && tok->str[tok->strlen-1] != ',') {
            // no comma, break
            tok = tok->next;
            break;
        }

        tok = tok->next;
    }

    query->update_columns = columns;
    query->update_values = values;

    // [Optional]
    // UPDATE table SET column = value, ... WHERE condition/filter
    //                                      ^   ^ 

    Filter *filter = try_parse_where(&tok);
    if (filter)
        query->update_filters = filter;

    return (QueryParseResult) {
        .result = RESULT_OK,
        .query = query,
    };
}

#define PARSE_SPLIT_STRING " \t\n"

QueryParseResult query_parse(const char *query_string)
{
    Query *query = query_new();
    if (!query)
        return (QueryParseResult) { .result = RESULT_ALLOC };

    // split the string on whitespace (and newlines)
    StrList *split = strlist_from_split_quoted(query_string, PARSE_SPLIT_STRING);

    QueryParseResult parse_result;
    char *verb = split->str;

    if (str_ieq(verb, "select")) {
        parse_result = query_parse_select(query, split->next);
    } else if (str_ieq(verb, "create")) {
        parse_result = query_parse_create(query, split->next);
    } else if (str_ieq(verb, "insert")) {
        parse_result = query_parse_insert(query, split->next);
    } else if (str_ieq(verb, "delete")) {
        parse_result = query_parse_delete(query, split->next);
    } else if (str_ieq(verb, "update")) {
        parse_result = query_parse_update(query, split->next);
    } else {
        parse_result = (QueryParseResult) {
            .result = RESULT_ERR_SQL_PARSE,
            .error_msg = "Unknown SQL command"
        };
    }

    if (parse_result.result != RESULT_OK) {
        // free the query since we are not returning it
        query_free(query);
    }

    // the query_parse_<whatever> functions create their own copies
    // of the strings that they require, so it is ok the free
    // the entire thing
    strlist_free(split);

    return parse_result;
}

#ifdef BAZATEST_PARSER
void run_tests();

int main()
{
    run_tests();

    return 0;
}

void run_tests()
{
    const char *queries[] = {
        // "SELECT name, age FROM table WHERE age = 50 AND name like pozdrawia%",

        "SELECT name, age FROM table "
        "WHERE age = 50 AND name like pozdrawia% "
        "ORDER BY age DESC "

        // "CREATE TABLE witam (\n"
        // "       first_name string,\n"
        // "       last_name string,\n"
        // "       age int32_t\n"
        // ")"
        // ,

        // "DELETE FROM table WHERE first_name = Wlodzimierz AND last_name = Bialy",

        // "UPDATE sometable SET a = 5, b = 4 WHERE c = 7 OR d >= 10",
    };

    for (int i = 0; i < sizeof(queries)/sizeof(queries[0]); i++) {
        const char *input = queries[i];
        printf("input: '%s'\n", input);

        QueryParseResult res = query_parse(input);

        if (res.result == RESULT_OK) {
            query_print(res.query);
            query_free(res.query);
        } else {
            printf("ERROR: %s\n", res.error_msg);
        }
    }
}

#endif /* BAZATEST_PARSER */
