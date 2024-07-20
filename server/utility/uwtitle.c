/*
 *	uwtitle
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <strings.h>
#include <stdio.h>

#include "uwlib.h"

extern char *optarg;
extern int optind;
extern int errno;

extern char *getenv();
extern long atol();

main(argc, argv)
int argc;
char **argv;
{
	register int c;
	register char *cp, *cq, **av;
	register uwid_t uwid;
	char *argv0;
	char *cqlimit;
	union uwoptval uwoptval;

	/*
	 * If called with no arguments, print a syntax message.  Otherwise,
	 * set the title of the current window to argv[1..argc-1].  The
	 * window ID is obtained from the environment or the "-i" argument.
	 */
	argv0 = argv[0];
	uwid = 0;
	while ((c = getopt(argc, argv, "i:")) != EOF) {
		switch (c) {
		case 'i':
			if ((uwid = atol(optarg)) == 0) {
				fprintf(stderr,
				    "%s: malformed \"-i\" argument\n", argv0);
				return(1);
			}
			break;
		}
	}
	if (optind >= argc) {
		fprintf(stderr, "Syntax: \"%s [-iID] title ...\"\n", *argv);
		return(1);
	}

	if (uwid == 0) {
		if ((cp = getenv("UW_ID")) == NULL) {
			fprintf(stderr,
			    "%s: can't determine window ID\n", argv0);
			return(1);
		}

		if ((uwid = (uwid_t)atol(cp)) == 0) {
			fprintf(stderr,
			    "%s: garbaged window ID in environment: %s",
			    argv0, cp);
			return(1);
		}
	}

	/*
	 * Copy the argv list into "uwoptval" and change the title.
	 */
	av = argv + optind - 1;
	cq = uwoptval.uwov_string;
	cqlimit = uwoptval.uwov_string + sizeof uwoptval.uwov_string;
	while ((cp = *++av) != NULL && cq < cqlimit) {
		while (cq < cqlimit && (*cq++ = *cp++) != '\0')
			;
		cq[-1] = ' ';
	}
	cq[-1] = '\0';

	if (uw_rsetopt(uwid, UWOP_TITLE, &uwoptval) < 0) {
		uw_perror("uw_rsetopt", uwerrno, errno);
		return(1);
	} else
		return(0);
}
