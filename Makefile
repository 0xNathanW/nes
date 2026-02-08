CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LIBS = `pkg-config --libs sdl2`
INCLUDES = `pkg-config --cflags sdl2`
TARGET = bin/nes
TEST_TARGET = bin/nestest

# Common sources (everything except main files)
COMMON_SRC = $(filter-out src/main.c src/nestest.c, $(wildcard src/*.c))
COMMON_OBJ = $(COMMON_SRC:src/%.c=build/%.o)

all: $(TARGET) $(TEST_TARGET)

# Create directories.
build:
	mkdir -p build
bin:
	mkdir -p bin

# Linking.
$(TARGET): $(COMMON_OBJ) build/main.o | bin
	$(CC) $(COMMON_OBJ) build/main.o -o $(TARGET) $(LIBS)

$(TEST_TARGET): $(COMMON_OBJ) build/nestest.o | bin
	$(CC) $(COMMON_OBJ) build/nestest.o -o $(TEST_TARGET) $(LIBS)

# All headers (changing any header triggers full rebuild)
HEADERS = $(wildcard src/*.h)

# Compile src files.
build/%.o: src/%.c $(HEADERS) | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build bin

rebuild: clean all

install-deps:
	sudo apt-get update
	sudo apt-get install libsdl2-dev pkg-config

# Tells make that these targets don't create files with the same name.
.PHONY: all clean rebuild install-deps
