/*
 *	uw library - uw_cmd
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include "uwlib.h"

uwid_t
uw_cmd(wtype, file, argv)
uwtype_t wtype;
char *file;
char **argv;
{
	register uwid_t uwid;

	/*
	 * Create a new window (using uw_fork()) and run the specified
	 * command in it.  Returns the window ID of the new window
	 * (or -1 if the window creation failed).  There is no way to
	 * determine if the executed command failed (e.g. if the
	 * executable file did not exist).
	 */
	if ((uwid = uw_fork(wtype, (int *)0)) == 0) {
		(void)execvp(file, argv);
		uwerrno = UWE_ERRNO;
		perror(file);
		_exit(1);
		/*NOTREACHED*/
	} else
		return(uwid);
}
