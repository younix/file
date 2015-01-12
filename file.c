/*
 * Copyright (c) 2014 Jan Klemkow <j.klemkow@wemelug.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define jump_to_nonspace(str) \
	for (; (str)[0] == ' ' || (str)[0] == '\t'; (str)++)

enum type {UNKNOWN = 0, CHAR, SHORT, INT, LONG};	/* type of value */


struct magic {
	bool cont;	/* continue line */
	off_t offset;	/* file offset */
	enum type type;	/* type of value */
	union {
		char c;
		short s;
		int i;
		long l;
	} value;	/* magic value */
	char *msg;	/* message */
};

void
print_msg(char *path, char *type, char *link)
{
	if (link == NULL)
		printf("%s: %s\n", path, type);
	else
		printf("%s: %s %s\n", path, type, link);
}

char *
stat_type(struct stat *sb)
{
	if (S_ISBLK(sb->st_mode)) return "block special";
	if (S_ISCHR(sb->st_mode)) return "character special";
	if (S_ISDIR(sb->st_mode)) return "directory";
	if (S_ISFIFO(sb->st_mode)) return "fifo";
	if (S_ISSOCK(sb->st_mode)) return "socket";
	return NULL;
}

void
file_stdout(char *path)
{
	struct stat sb;
	char *type = NULL;

	switch (lstat(path, &sb)) {
	case 0:
		if ((type = stat_type(&sb)) != NULL) {
			print_msg(path, type, NULL);
		} else if (S_ISLNK(sb.st_mode)) {
			char buf[BUFSIZ];
			ssize_t size;
			if ((size = readlink(path, buf, sizeof buf)) == -1)
				print_msg(path, "cannot open", NULL);
			buf[size] = '\0';
			print_msg(path, "symbolic link to", buf);
		} else if (S_ISREG(sb.st_mode)) {
			if (sb.st_blocks == 0)
				print_msg(path, "empty", NULL);
			else
				print_msg(path, "regular file", NULL);
		}
		break;
	case ENOENT:
	case EACCES:
		print_msg(path, "cannot open", NULL); break;
	default:
		err(EXIT_FAILURE, "stat");
	}
}

char *
magic_type_str(enum type type)
{
	if (type == UNKNOWN) return "unkown";
	if (type == CHAR) return "char";
	if (type == SHORT) return "short";
	if (type == INT) return "int";
	if (type == LONG) return "long";
	return "unknown";
}

enum type
get_magic_type(char *str)
{
	if (strncmp(str, "char", 4) == 0) return CHAR;
	if (strncmp(str, "short", 5) == 0) return SHORT;
	if (strncmp(str, "int", 3) == 0) return INT;
	if (strncmp(str, "long", 4) == 0) return LONG;

	return UNKNOWN;
}

void
print_magic(struct magic *magic)
{
	printf("%s%" PRIu64 "\t%s\n",
	    magic->cont ? ">" : "",
	    magic->offset,
	    magic_type_str(magic->type));
}

void
read_magic(char *path)
{
	struct magic magic;
	FILE *fh = NULL;
	char buf[BUFSIZ];

	if ((fh = fopen(path, "r")) == NULL)
		err(EXIT_FAILURE, "fopen");

	for (size_t line = 0; fgets(buf, sizeof buf, fh) != NULL; line++) {
		if (buf[0] == '\n' || buf[0] == '#')
			continue;

		memset(&magic, 0, sizeof magic);

		/* parse continue character */
		char *str = &buf[0];
		if (buf[0] == '>') {
			magic.cont = true;
			str = &buf[1];
		}

		/* parse offset */
		errno = 0;
		magic.offset = strtol(str, &str, 0);
		if (errno != 0) {
			fprintf(stderr, "line %zu: %s", line, strerror(errno));
			continue;
		}

		/* parse offset */
		//printf("str: %s\n", str);
		/* looking for first non-space char */
		jump_to_nonspace(str);
		//printf("str: %s\n", str);
		if ((magic.type = get_magic_type(str)) == UNKNOWN)
			continue;

		print_magic(&magic);
	}

	if (fclose(fh) == EOF)
		err(EXIT_FAILURE, "fclose");
}

void
usage(void)
{
	fprintf(stderr, "file [-dh] [-M file] [-m file] file...\n");
	fprintf(stderr, "file [-ih] file...\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	bool dflag = false;
	bool iflag = false;
	bool hflag = false;
	char *magic_path = "/etc/magic";
	int ch;

	while ((ch = getopt(argc, argv, "dihM:m:")) != -1) {
		switch (ch) {
		case 'd':
			if (iflag)
				usage();
			dflag = true;
			break;
		case 'i':
			if (dflag)
				usage();
			iflag = true;
			break;
		case 'h':
			hflag = true;
			break;
		case 'm':
			dflag = true;
			/* FALL THROUGH */
		case 'M':
			if (iflag)
				usage();
			if ((magic_path = strdup(optarg)) == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if (dflag)
		read_magic(magic_path);

	for (int i = 0; i < argc; i++)
		file_stdout(argv[i]);

	return EXIT_SUCCESS;
}
