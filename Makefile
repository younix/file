CC ?= cc
CFLAGS = -std=c99 -pedantic -Wall -Wextra

file: file.c
	$(CC) $(CFLAGS) -o $@ file.c

clean:
	rm -f file
