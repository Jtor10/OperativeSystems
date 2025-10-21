CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE -D_BSD_SOURCE
TARGET = p1-dataProgram
SOURCES = p1-dataProgram.c database.c
HEADERS = database.h

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) songs_database.bin
	rm -f /tmp/music_db_pipe

# Crear base de datos directamente desde línea de comandos
create-db: $(TARGET)
	./$(TARGET) --create

.PHONY: clean create-db
