.RECIPEPREFIX := >

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
TARGET := uart_protocol_demo
SOURCES := src/main.c src/protocol.c src/uart_rx.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES)
>$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
>./$(TARGET)

clean:
>rm -f $(TARGET)
