CC = g++
CFLAGS = -g -Wall -Wextra
BIN = glimpse

SRC = $(wildcard src/*.c* src/tabs/*.c*)
HDR = $(wildcard src/*.h* src/tabs/*.h* src/id_lists/*.h*)

.PHONY: all
all: $(BIN)

$(BIN): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses -lpanel

.PHONY: clean
clean:
	rm -f $(BIN)