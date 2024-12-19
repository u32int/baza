# baza
baza is a database engine accepting a subset of SQL. It does not depend on any external libraries (except libc) and has been written completely
from scratch. Query types that are implemented as of now (some only partially):

 - SELECT
 - CREATE
 - INSERT
 - DELETE
 - UPDATE 

## quickstart
No dependencies, all you need is a C compiler and GNU Make. `make build` (`make debug` - address sanitizer)
should yield an executable called 'baza' in the current directory. 
For prototyping reasons, the cli reads in tables from 'tables/' and queries './queries.sql'.

## Codebase organization
The project is divided into a parser, an interpreter and a storage backend. The parser takes in raw SQL in textual form
and turns it into a struct Query (defined in parser.h). Afterwards, the interpreter executes the structured query,
checking the request for validity along the way and sending the appropriate requests to the storage backend through its 'API'.

## What remains to be done
In its current state, this project is definitely at a prototype stage. Some parts of the code need to be rethought and rewritten.
In particular:

 - There needs to exist a generic list data structure. The current approach of having a separate implementation for each type is unacceptable.
   A generic vector implementation is work in progress.
 - The storage API needs to be redesigned with hindsight.
 - The parser might benefit from a more general parsing approach combined with a formal grammar of some sort.
 - Error messages need to be more precise, especially in the interpreter.
 - Optimizations that are generally present in most database engines (such as BTrees, etc.) should be, if not implemented, at the very least
      accomodated for.
 - ... the list could go on and on

## Examples of working queries
```sql
CREATE TABLE tabela (
    column1 int32,
    column2 string,
    column3 int64
);

INSERT INTO tabela VALUES (1, test, 321);
INSERT INTO tabela VALUES (2, dwa, 321);
INSERT INTO tabela VALUES (4, cztery, 123);
INSERT INTO tabela VALUES (7, siedem, 400);

SELECT column1, column2 FROM tabela
WHERE column3 > 200;

UPDATE tabela SET column3 = 0 WHERE column3 = 321;

DELETE FROM tabela WHERE column2 = "dwa" OR column2 = "cztery";

SELECT * FROM tabela;
```
