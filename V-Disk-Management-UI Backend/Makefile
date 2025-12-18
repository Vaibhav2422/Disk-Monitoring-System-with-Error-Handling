# Disk Management Simulator - Makefile

CC := gcc
CFLAGS := -std=c99 -O2 -Wall -Wextra -Wno-unused-parameter -Iinclude
LDFLAGS := 
SRC := src/main.c src/server.c src/disk.c src/utils.c
OBJ := $(SRC:.c=.o)
TESTS := tests/test_runner

.PHONY: all build run test clean

all: build

build: bin/server

bin/server: $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

run: build
	@echo "Starting server on PORT=$${PORT:-8080}..."
	@PORT=$${PORT:-8080} DATA_FILE=$${DATA_FILE:-disk_state.json} ./bin/server

test: build tests/test_runner
	@echo "Running tests..."
	./tests/test_runner && echo "All tests passed."

tests/test_runner: tests/test_runner.c src/disk.c src/disk.h src/utils.c src/utils.h
	$(CC) $(CFLAGS) -o $@ tests/test_runner.c src/disk.c src/utils.c

clean:
	rm -rf bin
	find . -name "*.o" -delete
	rm -f tests/test_runner
