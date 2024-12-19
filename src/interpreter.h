// Interprets a Query struct (see parser.h) and talks to the storage
// backend via its API to apply the query and return a result.
#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include "parser.h"

#include "util/defs.h"
#include "util/result.h"

typedef struct QueryRespose {
    BazaResult result;
    void *data;
} QueryResponse;

QueryResponse interpret_query(const Query *query);

#endif /* _INTERPRETER_H */
