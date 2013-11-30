CC = gcc
CFLAGS = -std=c99 -Wall -Werror -O2 -lz -D_FILE_OFFSET_BITS=64 -D_BSD_SOURCE

crc32sum: crc32sum.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm crc32sum
