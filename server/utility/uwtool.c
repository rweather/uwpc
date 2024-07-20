/*
 *	uwtool
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <strings.h>
#include <stdio.h>

#include "uwlib.h"

main(argc, argv)
int argc;
char **argv;
{
	register uwid_t uwid;
	register char *fname, *term;
	register int c;
	register uwtype_t wtype;
	char *argv0;
	char *av[2];
	int vflag;
	int wflag;
	char *title;
	union uwoptval uwoptval;
	extern int errno;
	extern int optind;
	extern char *optarg;
	extern char *getenv();

	/*
	 * If called with no arguments, create a new window using the
	 * current shell according to the SHELL environment variable
	 * (or "/bin/sh" if that doesn't exist).  If called with
	 * arguments, argv[optind] through argv[argc-1] are the arguments
	 * to the command.
	 *
	 * Options which are recognized directly are:
	 *
	 *	-v	(verbose) print new window ID on stdout
	 *	-wtype	create window with emulation "type"
	 *	-ttitle	label window with "title"
	 *
	 * If no explicit title is specified, the command name is used.
	 */
	argv0 = argv[0];
	wflag = 0;
	vflag = 0;
	title = (char *)0;
	while ((c = getopt(argc, argv, "vw:t:")) != EOF) {
		switch (c) {
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			wtype = uw_ttype(optarg);
			break;
		case 't':
			title = optarg;
			break;
		}
	}
			
	if (optind < argc) {
		/*
		 * Adjust the "argv" pointer according to the number of
		 * arguments we've processed.
		 */
		argv += optind;
		fname = *argv;
	} else {
		/*
		 * No (non-option) arguments -- use SHELL
		 */
		if ((fname = getenv("SHELL")) == (char *)0)
			fname = "/bin/sh";
		av[0] = fname;
		av[1] = (char *)0;
		argv = av;
	}

	if (title == (char *)0) {
		/*
		 * If there was no "-t" argument, then "title" will still
		 * be NULL.  In this case we use the command name as
		 * the title.
		 */
		title = fname;
	}
	
	if (!wflag) {
		/*
		 * If there was no "-w" argument, fetch the window
		 * type from the environment.  If that fails, use
		 * a default.
		 */
		if ((term=getenv("TERM")) != (char *)0)
			wtype = uw_ttype(term);
		else
			wtype = UWT_ADM31;
	}
	
	if ((uwid = uw_cmd(wtype, fname, argv)) > 0) {
		(void)strncpy(uwoptval.uwov_string, title,
		    sizeof uwoptval.uwov_string);
		(void)uw_rsetopt(uwid, UWOP_TITLE, &uwoptval);
		if (vflag)
			printf("%d\n", uwid);
		return(0);
	} else {
		if (uwerrno != UWE_NXSERV)
			uw_perror(fname, uwerrno, errno);
		else
			uw_perror(argv0, uwerrno, errno);
		return(1);
	}
}
