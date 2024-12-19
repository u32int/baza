#ifndef _UTIL_RESULT_H
#define _UTIL_RESULT_H

/// A global result enum, to be used for propagating errors up the call chain
typedef enum {
    RESULT_OK,
    RESULT_ERR,
    RESULT_ALLOC,
    RESULT_ERR_SQL_PARSE,
    RESULT_INDEX_OUT_OF_BOUNDS,
    RESULT_COLUMN_NOT_FOUND,
    RESULT_TABLE_NOT_EMPTY,
    RESULT_TABLE_NOT_FOUND,
    RESULT_DUPLICATE_COLUMN_NAME,
    RESULT_INVALID_QUERY,
    RESULT_FILE_NOT_FOUND,
    RESULT_IO_ERROR,
    RESULT_INVALID_CSV,
    RESULT_VALUE_TYPE,
    RESULT_FILTER_VALUE_TYPE,
    RESULT_SERVER_ERROR,
} BazaResult;

const char *result_str(BazaResult result);

#endif /* _UTIL_RESULT_H */
