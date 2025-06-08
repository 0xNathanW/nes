# Sets compiler to gcc.
CC = gcc
# Compiler flags
# -Wall: Enable all warnings
# -Wextra: Enable extra warnings
# -std=c99: Use C99 standard
# -g: Enable debugging
CFLAGS = -Wall -Wextra -std=c99 -g
# Links to SDL2 library, expands to -lSDL2
LIBS = `pkg-config --libs sdl2`
# Includes SDL2 headers, expands to -I/usr/include/SDL2
INCLUDES = `pkg-config --cflags sdl2`

# Name of the executable.
TARGET = nes

# Source files
SOURCES = main.c
# Object files are created from source files by replacing .c with .o
OBJECTS = $(SOURCES:.c=.o)

# Default target, builds the executable when `make` is run, will run command below.
all: $(TARGET)

# Link the executable, expands to gcc main.o -o nes -lSDL2.
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all

install-deps:
	sudo apt-get update
	sudo apt-get install libsdl2-dev pkg-config

# Tells make that these targets don't create files with the same name.
.PHONY: all clean rebuild install-deps 