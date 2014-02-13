#include <stdio.h>
#include <stdlib.h>	/* exit(), malloc(), free() */
#include <zlib.h>	/* crc32 */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>	/* S_ISREG */
#include <string.h>
#include <errno.h>
#include <getopt.h>	/* getopt */
#include <unistd.h>	/* getpagesize() */
#include <limits.h>	/* LINE_MAX */

#include "crc32sum.h"

#define PROGRAM_NAME "crc32sum"
#define AUTHORS "Luis Ortega Perez de Villar"

#define STREQ(a, b) (strcmp((a), (b)) == 0)


static const struct option long_options[] = 
{
	{"check", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, -2},
	{"version", no_argument, NULL, -3},
	{}
};


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
uLong digest_filestream(int fd)
{
	ssize_t ret;
	size_t len = getpagesize();

	/* char *buf = calloc(len, sizeof(char)); */
	char *buf = malloc(len);

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
 * pre: set errno to 0
 * post: if errno != 0 discard result
 */
uLong digest_file(const char *filename)
{
	int fd;
	struct stat sb;
	char *data;
	uLong crc = 0;

	/* Open file */
	/* To open files >2GB in 32bit systems look at "man 2 open"
	 * and search for O_LARGEFILE */
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return 1;

	/* Get file information and perform checks */
	if (fstat(fd, &sb) == -1)
		goto df_out;

	if (!S_ISREG(sb.st_mode)) {
		errno = EINVAL;
		goto df_out;
	}

	/* Map file into memory */
	data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
		goto df_out;

	#ifdef _BSD_SOURCE
	/* Advise the kernel about paging policy for this mapping */
	madvise(data, sb.st_size, MADV_SEQUENTIAL);
	#endif

	/* Compute checksum */
	crc = crc32(0L, (Bytef *) data, sb.st_size);

	/* Unmap and close file, ignore ret values */
	munmap(data, sb.st_size);

df_out:
	close(fd);

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
		fprintf(stderr, "%s: %s\n", filename, strerror(errno));
	else
		fprintf(stdout, "%8lX  %s\n", crc, filename);

	return 0;

}

int digest_check(char *checkfile)
{
	char *buf;
	size_t lmaxsize = LINE_MAX;
	FILE *fp;
	int ret = 0;
	ssize_t n;
	uLong crc;
	unsigned nfailed = 0;

	buf = malloc(LINE_MAX);

	if (!buf)
		return 1;

	fp = fopen(checkfile, "r");
	if (!fp) {
		ret = 1;
		goto check_out;
	}

	while ((n = getline(&buf, &lmaxsize, fp)) > 0) {
		if (n == 1 || buf[0] == '#')
			continue;

		if (buf[n-1] == '\n')
			buf[n-1] = '\0';

		char *chkcrc;
		char *filename;
		char strcrc[9] = {0};

		chkcrc = strtok(buf, " ");
		filename = strtok(NULL, " ");

		errno = 0;
		crc = digest_file(filename);

		if (errno) {
			++nfailed;
			ret = 1;
			fprintf(stderr, "%s: %s\n", filename, strerror(errno));
			continue;
		}

		sprintf(strcrc, "%8lX", crc);
		if (STREQ(chkcrc, strcrc))
			fprintf(stdout, "%s: OK\n", filename);
		else {
			++nfailed;
			fprintf(stderr, "%s: FAILED\n", filename);
		}

		pr_debug("read: %s\ncomputed: %s\n", chkcrc, strcrc);
	}

	if (nfailed)
		fprintf(stderr, "%s: WARNING: "
			"%u computed checksum did NOT match\n",
			PROGRAM_NAME, nfailed);

	if (fclose(fp) == EOF)
		ret = 1;

check_out:
	free(buf);
	return ret;
}

int main(int argc, char **argv)
{
	extern int optind, optopt;
	extern char *optarg;
	int c;

	if (argc == 1)
		/* Sum data from stdin */
		return	sum_file("-");

	while ((c = getopt_long(argc, argv, "c:", long_options, NULL)) != -1) {
		switch (c) {
		case 'c':
			digest_check(optarg);
			return 0;
		case -2:
			usage();
			return 0;
		case -3:
			version();
			return 0;
		case '?':
			/* Automatically prints message */
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
