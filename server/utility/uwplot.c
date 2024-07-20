/*
 *	uwplot
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <ctype.h>
#include <stdio.h>

#include "uwlib.h"

char *argv0;
UWIN uwin;

main(argc, argv)
char **argv;
{
	register int c, len;
	register char *cp;
	register char *title;
	register struct sockaddr_in *sin;
	auto struct sockaddr_in sa;
	auto char buf[4096];
	extern char *optarg;
	extern int errno;
	extern onintr();

	/*
	 * Options which are recognized directly are:
	 *
	 *	-ninet	connect to server at address "inet"
	 *	-ttitle	label window with "title" (default is argv[0])
	 */
	argv0 = argv[0];
	sin = (struct sockaddr_in *)0;
	title = argv0;
	while ((c = getopt(argc, argv, "n:t:")) != EOF) {
		switch (c) {
		case 'n':
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = 0;
			sa.sin_port = 0;
			bzero(sa.sin_zero, sizeof sa.sin_zero);
			for (cp=optarg; isxdigit(c = *cp); cp++) {
				/* Pyramid compiler botch */
				/* sa.sin_addr.s_addr *= 16; */
				sa.sin_addr.s_addr <<= 4;
				if (isdigit(c))
					sa.sin_addr.s_addr += c - '0';
				else if (islower(c))
					sa.sin_addr.s_addr += c-'a' + 10;
				else
					sa.sin_addr.s_addr += c-'A' + 10;
			}
			if (c == '.')
				for (cp++; isdigit(c = *cp); cp++)
					sa.sin_port = sa.sin_port*10 + c-'0';
			if (sa.sin_addr.s_addr == 0 || sa.sin_port == 0) {
				fprintf(stderr,
				    "%s: bad Internet address: %s\n",
				    argv0, optarg);
				return(1);
			}
			sa.sin_addr.s_addr = htonl(sa.sin_addr.s_addr);
			sa.sin_port = htons(sa.sin_port);
			sin = &sa;
			break;
		case 't':
			title = optarg;
			break;
		}
	}

	/*
	 * Catch hangup, interrupt, quit, and termination signals.  Kill
	 * the window if one of these is received.
	 */
	(void)signal(SIGHUP, onintr);
	(void)signal(SIGINT, onintr);
	(void)signal(SIGQUIT, onintr);
	(void)signal(SIGTERM, onintr);

	/*
	 * Create a new plot window, title it, and make it visible.
	 */
	if ((uwin = uw_new(UWT_PLOT, sin)) == (UWIN)0) {
		uw_perror(argv[0], uwerrno, errno);
		return(1);
	}
	(void)uw_stitle(uwin, title);
	(void)uw_svis(uwin, 1);

	/*
	 * Copy the standard input to the plot window.
	 */
	while ((len = read(0, buf, sizeof buf)) > 0 ||
	    (len < 0 && errno == EINTR)) {
		if (len > 0)
			(void)write(UW_DATAFD(uwin), buf, len);
	}

	/*
	 * This is something of a hack.  We don't expect to be able to
	 * read anything from the window.  The read will hang until the
	 * window is killed.
	 */
	while ((len = read(UW_DATAFD(uwin), buf, sizeof buf)) > 0 ||
	    len < 0 && errno == EINTR)
		;
	return(0);
}

onintr()
{
	uw_kill(uwin);
	exit(0);
}
