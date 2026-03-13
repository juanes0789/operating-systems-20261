CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
SRC = src/main.c src/scheduler.c src/io.c src/metrics.c
OBJ = $(SRC:.c=.o)
TARGET = mlfq_sim

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c include/scheduler.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

