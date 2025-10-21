CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -D_DEFAULT_SOURCE
TARGET = p1-dataProgram
SOURCES = p1-dataProgram.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)
	-ipcrm -a 2>/dev/null || true

.PHONY: clean
