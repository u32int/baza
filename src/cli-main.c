#include "interpreter.h"
#include "storage.h"
#include "util/str.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <string.h>

// read in a csv file containing a table into the db.
// the file has to be in a pretty specific format; the first line
// must contain the column names and the second line must specify their types
BazaResult csv_read(const char *table_name, const char *path, const char *csv_delim)
{
    FILE *file = fopen(path, "r");
    if (!file)
        return RESULT_FILE_NOT_FOUND;

    BazaResult result = RESULT_OK;

    char *line = NULL; 
    size_t len = 0;

    // fetch first line
    ssize_t nread = getline(&line, &len, file);
    if (nread <= 0)
        return RESULT_INVALID_CSV; 
    line[nread-1] = 0;

    // the first line is supposed to contain the column names
    StrList *columns = strlist_from_split(line, csv_delim);

    // fetch second line
    nread = getline(&line, &len, file);
    if (nread <= 0)
        return RESULT_INVALID_CSV; 
    line[nread-1] = 0;

    // the second line is supposed to contain types
    StrList *types = strlist_from_split(line, csv_delim);

    char *tn_copy = strdup(table_name);

    Query query = (Query) {
        .type = QUERY_CREATE,
        .table_name = tn_copy,
        .create_columns = columns,
        .create_types = types,
    };

    QueryResponse resp = interpret_query(&query);
    if (resp.result != RESULT_OK) {
        result = resp.result;
        goto bail;
    }

    // iterate over the remaining lines issuing an insert query for each one
    while ((nread = getline(&line, &len, file)) != -1) {
        if (nread > 0)
            line[nread-1] = 0;

        StrList *values = strlist_from_split_quoted(line, csv_delim);

        query = (Query) {
            .type = QUERY_INSERT,
            .table_name = tn_copy,
            .insert_values = values,
        };

        QueryResponse resp = interpret_query(&query);

        strlist_free(values);

        if (resp.result != RESULT_OK) {
            result = resp.result;
            goto bail;
        }
    }

bail:
    free(line);
    free(tn_copy);
    strlist_free(columns); 
    strlist_free(types);
    return result;
}

StrList *fs_read_queries(const char *path)
{
    size_t query_count = 0;

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        FATAL("open: %m");

    struct stat statbuf;
    if (fstat(fd, &statbuf) < 0)
        FATAL("stat: %m");

    char *mapped_file = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!mapped_file)
        FATAL("mmap: %m");

    StrList *list = strlist_empty();
    char *clone = strdup(mapped_file);

    char *tok = strtok(clone, ";");
    do {
        while (*tok == ' ' || *tok == '\n')
            tok++;

        if (*tok == '#')
            continue;

        if (*tok)
            strlist_push(list, tok);
    } while ((tok = strtok(NULL, ";")));

    free(clone);
    munmap(mapped_file, statbuf.st_size);

    return list;    
} 

void do_query(const char *q)
{
    QueryParseResult res = query_parse(q);
    printf("SQL INPUT: '%s'\n", q);

    if (res.result != RESULT_OK) {
        printf("%s: %s", result_str(res.result), res.error_msg);
    } else {
        query_print(res.query);
    }

    QueryResponse resp = interpret_query(res.query);
    if (resp.result != RESULT_OK) {
        printf("INTERP ERR: %s\n", result_str(resp.result));
    } else {
        puts("QUERY RESULT: OK");
    }

    fputs("\n\n", stdout);

    query_free(res.query);
}

int main()
{
    storage_init();

    #define READ_CSV_FILE(name) do { \
        BazaResult tres = csv_read(name, "./tables/"name".baza.csv", ","); \
        printf("LOAD CSV "name": %s\n", result_str(tres)); \
        if (tres != RESULT_OK) \
            return 1; } while (0) \

    READ_CSV_FILE("Studenci");
    READ_CSV_FILE("PodstawyProgramowania");

    StrList *queries = fs_read_queries("./queries.sql");

    StrList *q = queries;
    while (q && q->str) {
        do_query(q->str);
        q = q->next;
    }

    strlist_free(queries);
    storage_deinit();

    return 0;
}
