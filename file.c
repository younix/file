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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

	for (int i = 0; i < argc; i++)
		file_stdout(argv[i]);

	return EXIT_SUCCESS;
}
