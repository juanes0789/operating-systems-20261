CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC = src/main.c src/ticket/ticket.c src/utils/utils.c
OBJ = $(SRC:.c=.o)
TARGET = ticket_app

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

clean:
	rm -rf $(OBJ) $(TARGET) assets/*.txt

run: all
	./$(TARGET)