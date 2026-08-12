
CC = gcc
# -ffast-math for avoiding checking Nan and infinities when calculation
#  of complex numbers (IEEE-754 specificaions_
CFLAGS = -Wall -Wextra -O3 -ffast-math -fopenmp -march=native -Iinclude $(shell pkg-config --cflags sdl3 sdl3-ttf sdl3-image)
LDFLAGS = $(shell pkg-config --libs sdl3 sdl3-ttf sdl3-image) -lm -fopenmp

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/mandelbrot
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
