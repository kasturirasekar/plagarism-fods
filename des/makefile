CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = plagiarism_checker
SOURCES = main.c plagiarism.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) -lm

clean:
	rm -f $(TARGET)

.PHONY: clean