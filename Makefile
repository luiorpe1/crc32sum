crc32sum: crc32sum.c
	gcc -std=c99 -Wall -o crc32sum -lz -D_FILE_OFFSET_BITS=64 -D_BSD_SOURCE crc32sum.c

.PHONY: clean
clean:
	rm crc32sum
