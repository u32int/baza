#include "result.h"

#include <stddef.h>

const char *result_str(BazaResult result)
{
    switch (result)  {
        case RESULT_OK: return "OK";
        case RESULT_ERR: return "generic error";
        case RESULT_ALLOC: return "memory allocation failed";
        case RESULT_INDEX_OUT_OF_BOUNDS: return "index out of bounds";
        case RESULT_COLUMN_NOT_FOUND: return "column not found";
        case RESULT_TABLE_NOT_EMPTY: return "table not empty";
        case RESULT_TABLE_NOT_FOUND: return "table not found";
        case RESULT_DUPLICATE_COLUMN_NAME: return "duplicate column name";
        case RESULT_ERR_SQL_PARSE: return "SQL parse error";
        case RESULT_SERVER_ERROR: return "server error";
        case RESULT_FILE_NOT_FOUND: return "file not found";
        case RESULT_IO_ERROR: return "io error";
        case RESULT_INVALID_CSV: return "io error";
        case RESULT_VALUE_TYPE: return "value type error";
        case RESULT_FILTER_VALUE_TYPE: return "filter value type error";
        case RESULT_INVALID_QUERY: return "invalid query";
    }
    return NULL; 
}

