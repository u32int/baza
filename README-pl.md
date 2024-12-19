# baza
Silnik bazy danych akceptujący subset SQL. Program nie polega na żadnych zewnętrznych bibliotekach (oprócz libc) i został napisany w całości od zera.
Zaimplementowane rodzaje kwerend (niektóre tylko częściowo):

 - SELECT
 - CREATE
 - INSERT
 - DELETE
 - UPDATE 

## quickstart
Brak dependencies, wystarczy kompilator C i gnu make. `make build` (`make debug` - wersja z address sanitizer)
powinno wyprodukować executable 'baza' w obecnym directory. 
Domyślne program czyta pliki csv (Studenci, PodstawyProgramowania) 
z 'tabele/' oraz kwerendy z pliku './queries.sql'.

queries.sql zawiera kwerendy z lab6.pdf, które powinny wykonywać się poprawnie.

## Podział kodu
Projekt zasadniczo dzieli się na parser, interpreter i storage backend. 
Parser przyjmuje raw sql w formie tekstowej i zamienia go na struct Query (zdefiniowany w parser.h).
Interpreter interpretuje struct Query, validując request (np. istnienie kolumn) i następnie wysyła odpowiednie
zapytania do storage backendu przez jego "API" (storage.h).

Codebase jest zdecydowanie w niedokończonym stanie. Wszystkie dynamiczne struktury danych powinny zostać zunifikowane,
najlepiej w postaci ciągłej w pamięci (odpowiednika vector w innych językach). Funkcje interpretujące powinny zwracać
jakiegoś rodzaju serializowalną strukturę (w QueryResponse, zamiast printować na stdout), i tak dalej.

## Przykłady wspieranych kwerend
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
