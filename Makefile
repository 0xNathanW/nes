CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LIBS = `pkg-config --libs sdl2`
INCLUDES = `pkg-config --cflags sdl2`
TARGET = bin/nes

SRC_FILES = $(wildcard src/*.c)
OBJ_FILES = $(SRC_FILES:src/%.c=build/%.o)

all: $(TARGET)

# Create directories.
build:
	mkdir -p build
bin:
	mkdir -p bin

# Linking.
$(TARGET): $(OBJ_FILES) | bin
	$(CC) $(OBJ_FILES) -o $(TARGET) $(LIBS)

# Compile src files.
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build bin

rebuild: clean all

install-deps:
	sudo apt-get update
	sudo apt-get install libsdl2-dev pkg-config

# Tells make that these targets don't create files with the same name.
.PHONY: all clean rebuild install-deps 