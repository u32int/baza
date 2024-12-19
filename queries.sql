SELECT imię, nazwisko, numer_indeksu 
FROM Studenci 
WHERE rok_rozpoczęcia = 2019
ORDER BY numer_indeksu ASC;

SELECT imię, nazwisko, numer_indeksu 
FROM Studenci 
WHERE nazwisko like "K%" OR nazwisko like "W%";

UPDATE Studenci
SET nazwisko = "Anioł"
WHERE imię = "Monika" AND nazwisko = "Duda";

UPDATE Studenci
SET kierunek = "AIR"
WHERE imię = "Adam";

DELETE FROM PodstawyProgramowania 
WHERE Grupa = 6;
