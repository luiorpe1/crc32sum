#include <stdio.h>
#include <stdlib.h>	/* exit(), malloc(), free() */
#include <zlib.h>	/* crc32 */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>	/* S_ISREG */
#include <string.h>
#include <errno.h>
#include <unistd.h>	/* getopt */


#define PROGRAM_NAME "crc32sum"
#define AUTHORS "Luis Ortega Perez de Villar"

#define STREQ(a, b) (strcmp((a), (b)) == 0)


/**
 * usage: print usage message
 */
void usage()
{
	printf("Usage: %s [FILE]...\n"
	       "Print CRC (32-bit) checksums.\n"
	       "With no FILE, or when FILE is -, read standard input.\n",
	       PROGRAM_NAME);
	printf("\n"
	       "\t--help\tdisplay this help and exit\n");
}


/**
 * version: print version message
 */
void version()
{
	printf("Copyright (C) 2013.\n"
	       "License GPLv2+: GNU GPL version 2 or later"
	       " <http://gnu.org/licenses/old-licenses/gpl-2.0.html>.\n"
	       "This is free software: you are free to change and"
	       " redistribute it.\n"
	       "There is NO WARRANTY, to the extent permitted by law.\n"
	       "\n"
	       "Written by %s.\n", AUTHORS);
}


/**
 * digest_filestream: compute crc32 cheksum for data stream in fd
 */
int digest_filestream(int fd)
{
	ssize_t ret;
	size_t len = 4096; /* 4KB */

	/* char *buf = calloc(4096, sizeof(char)); */
	char *buf = malloc(4096);

	if (buf == NULL)
		return 1;

	uLong crc = crc32(0L, Z_NULL, 0);

	while ((ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		crc = crc32(crc, (Bytef *) buf, ret);
	}

	free(buf);
	return crc;
}


/**
 * digest_file: compute the crc32 checksum for file "filename"
 */
int digest_file(const char *filename)
{
	int fd;
	struct stat sb;
	char *data;
	uLong crc;

	/* Open file */
	/* To open files >2GB in 32bit systems look at "man 2 open"
	 * and search for O_LARGEFILE */
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return 1;

	/* Get file information and perform checks */
	if (fstat(fd, &sb) == -1)
		return 1;

	if (!S_ISREG(sb.st_mode)) {
		errno = EINVAL;
		return 1;
	}

	/* Map file into memory */
	data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
		return 1;

	#ifdef _BSD_SOURCE
	/* Advise the kernel about paging policy for this mapping */
	madvise(data, sb.st_size, MADV_SEQUENTIAL);
	#endif

	/* Compute checksum */
	crc = crc32(0L, (Bytef *) data, sb.st_size);

	/* Unmap and close file */
	if (munmap(data, sb.st_size) == -1)
		return 1;

	if (close(fd) == -1)
		return 1;

	return crc;
}


int sum_file(char *filename)
{
	uLong crc;
	errno = 0;

	if (STREQ(filename, "-"))
		crc = digest_filestream(0);
	else
		crc = digest_file(filename);

	if (errno)
		printf("%s: %s\n", filename, strerror(errno));
	else
		printf("%8lX  %s\n", crc, filename);

	return 0;

}

int main(int argc, char **argv)
{
	extern int optind, optopt;
	int c;

	if (argc == 1)
		/* Sum data from stdin */
		return	sum_file("-");

	while ((c = getopt(argc, argv, "hV")) != -1) {
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 'V':
			version();
			return 0;
		case '?':
			fprintf(stderr, "Unknown option '-%c'.\n", optopt);
			return 1;
		default:
			abort();
		}
	}

	/* Sum files */
	for (int i = optind; i < argc; i++)
		sum_file(argv[i]);

	return 0;
}
