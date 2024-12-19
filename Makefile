CC = gcc
CFLAGS = -Wall -Wextra 
CFLAGS_DEBUG = -fsanitize=address,undefined

SRC = $(wildcard src/*.c) $(wildcard src/util/*.c)
OBJ = $(SRC:.c=.o)
TARGET = baza

build: $(OBJ) 
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET)

debug: $(OBJ)
	$(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(OBJ) $(LDFLAGS) -o $(TARGET)

clean:
	rm $(OBJ) $(TARGET)

.PHONY: clean build
