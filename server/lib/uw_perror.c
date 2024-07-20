/*
 *	uw library - uw_perror
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

char *uwerrlist[] = {
	"no error",
	"system call error",
	"nonexistent window type",
	"window ID duplicated (in use)",
	"operation not implemented",
	"non-existent server",
	"unable to allocate required memory",
	"invalid argument to function",
	"no control file descriptor for window",
};
unsigned uwnerr = sizeof uwerrlist / sizeof uwerrlist[0];

int uwerrno;

/*ARGSUSED*/
void
uw_perror(mesg, uwerr, errno)
char *mesg;
uwerr_t uwerr;
int errno;
{
	register char *errmsg;

	/*
	 * Print a UW error message.  We call write() directly to avoid
	 * making the UW library dependent upon stdio.
	 */
	if (uwerr == UWE_ERRNO) {
		perror(mesg);
	} else {
		if (mesg != (char *)0) {
			(void)write(2, mesg, strlen(mesg));
			(void)write(2, ": ", 2);
		}
		if (uwerr >= uwnerr)
			errmsg = "unknown UW error";
		else
			errmsg = uwerrlist[uwerr];
		(void)write(2, errmsg, strlen(errmsg));
		(void)write(2, "\n", 1);
	}
}
