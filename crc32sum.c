#include <stdio.h>
#include <stdlib.h>	/* exit(), malloc(), free() */
#include <zlib.h>	/* crc32 */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>	/* S_ISREG */
#include <string.h>
#include <errno.h>


#define PROGRAM_NAME "crc32sum"
#define AUTHORS "Luis Ortega Perez de Villar"

#define STREQ a b = (strcmp((a), (b)) ? 0 : 1)


void usage()
{
	printf("Usage: %s [FILE]...\n"
	       "Print CRC (32-bit) checksums."
	       PROGRAM_NAME);
}


/**
 * digest_filestream: compute crc32 cheksum for data stream in fd
 */
int digest_filestream(int fd)
{
	ssize_t ret;
	size_t len = 4096; /* 4KB */

	//char *buf = calloc(4096, sizeof(char));
	char *buf = malloc(4096);

	if (buf == NULL) {
		//perror("calloc");
		return 1;
	}

	uLong crc = crc32(0L, Z_NULL, 0);

	while ((ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			//perror("read");
			break;
		}

		crc = crc32(crc, buf, ret);
	}

	free(buf);
	return crc;
}


/**
 * sum_file: calculate crc32 cheksum for file "filename"
 */
int sum_file(const char *filename)
{
	int fd;
	struct stat sb;
	char *data; 
	uLong crc;
	
	/* Open file */
	/* To open files >2GB in 32bit systems look at "man 2 open"
	 * and search for O_LARGEFILE */
	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		//fprintf(stderr, "Unable to open %s\n", filename);
		
		//fprintf(stderr, "%s: %s [error code: %d]\n",
		//	filename, strerror(errno), errno);
		//perror(filename);
		
		return 1;
	}

	/* Get file information and perform checks */
	if (fstat(fd, &sb) == -1) {
		//fprintf(stderr,
		//	"%s: Unable to retrieve information\n", filename);
		return 1; 
	}

	if (!S_ISREG (sb.st_mode)) {
		//fprintf(stderr, "%s is not a file\n", filename);
		errno = EINVAL;
		return 1;
	}

	/* Map file into memory */
	data = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		//fprintf(stderr, "unable to map %s\n", filename);
		return 1;
	}

	//TODO Find out why "madvise" doesn't compile
	//madvise(data, sb.st_size, MADV_SEQUENTIAL);

	/* Calculate checksum */
	crc = crc32(0L, (Bytef *) data, sb.st_size);

	/* Unmap and close file */
	if (munmap(data, sb.st_size) == -1) {
		//fprintf(stderr, "unable to unmap %s\n", filename);
		return 1;
	}

	if (close(fd) == -1) {
		//fprintf(stderr, "unable to close %s\n", filename);
		return 1;
	}

	return crc;
}


int main(int argc, char **argv)
{
	if (argc < 2) {
		//fprintf(stderr, "usage: %s file1 [...]\n", argv[0]);
		usage();
		return 1;
	}

	char *crcname;

	for (int i = 1; i < argc; i++) {
		errno = 0;
		uLong crc = sum_file(argv[i]);
		
		if (errno)
			printf("%s: %s\n", argv[i], strerror(errno));
		else
			printf("%8lX  %s\n", crc, argv[i]);
	}

	return 0;
}
