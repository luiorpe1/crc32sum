crc32sum: crc32sum.c
	gcc -std=c99 -o crc32sum -lz -D_FILE_OFFSET_BITS=64 crc32sum.c

.PHONY: clean
clean:
	rm crc32sum
