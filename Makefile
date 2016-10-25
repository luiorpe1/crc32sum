ARCH = $(shell uname -m)

# GCC Warning Options Reference:
# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html

CC = gcc
CFLAGS += -std=c99 -Wall -Wextra -pedantic -Werror -D_DEFAULT_SOURCE -D_GNU_SOURCE
LDFLAGS = -lz

DESTDIR	 =

ifneq ("$(ARCH)", "x86_64")
	CFLAGS += -D_FILE_OFFSET_BITS=64
endif

crc32sum: crc32sum.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

.PHONY: install clean

install:
	install -m755 crc32sum "${DESTDIR}"/usr/bin

clean:
	rm crc32sum crc32sum.o
