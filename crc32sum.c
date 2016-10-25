#include <stdio.h>
#include <stdlib.h>	/* exit(), malloc(), free() */
#include <zlib.h>	/* crc32 */
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>	/* S_ISREG */
#include <string.h>
#include <errno.h>
#include <getopt.h>	/* getopt */
#include <unistd.h>	/* getpagesize() */
#include <limits.h>	/* LINE_MAX */

#include "crc32sum.h"

static const struct option long_options[] = 
{
	{"check", required_argument, NULL, 'c'},
	{"help", no_argument, NULL, -2},
	{"version", no_argument, NULL, -3},
	{0}
};


/**
 * usage: print usage message
 */
void usage()
{
	printf("Usage: %s [FILE]...\n"
	       "Print CRC (32-bit) checksums.\n"
	       "\n"
	       "With no FILE, or when FILE is -, read standard input.\n",
	       PROGRAM_NAME);
	printf("\n"
	       "      --help\t display this help and exit\n");
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
uLong digest_bytestream(int fd)
{
	struct stat sb;
	uLong crc = 0;
	char *buf = NULL;
	ssize_t ret;
	size_t len = getpagesize();

	if (fd == STDIN_FILENO)
		goto db_digest;

	/* Get file information and perform checks */
	if (fstat(fd, &sb) == -1)
		goto db_out;

	if (S_ISDIR(sb.st_mode)) {
		errno = EISDIR;
		goto db_out;
	}

	if (!S_ISREG(sb.st_mode)) {
		errno = EINVAL;
		goto db_out;
	}

db_digest:
	buf = calloc(len, sizeof(char));
	if (buf == NULL)
		goto db_out;

	crc = crc32(0L, Z_NULL, 0);

	while ((ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			break;
		}

		crc = crc32(crc, (Bytef *) buf, ret);
	}

	free(buf);
db_out:
	return crc;
}

uLong digest_file(const char *filename)
{
	int fd;
	uLong crc;

	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return 1;

	crc = digest_bytestream(fd);

	close(fd);
	return crc;
}

int sum_file(char *filename)
{
	uLong crc;
	errno = 0;

	if (STREQ(filename, "-"))
		crc = digest_bytestream(STDIN_FILENO);
	else
		crc = digest_file(filename);

	if (errno)
		if (errno == EISDIR)
			fprintf(stderr, "%s: %s: Is a directory\n",
				PROGRAM_NAME, filename);
		else
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
		return sum_file("-");

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
