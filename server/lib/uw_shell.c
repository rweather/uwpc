/*
 *	uw library - uw_shell
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

char *uwshellname = "/bin/sh";	/* can be patched by caller if desired */

uwid_t
uw_shell(wtype, cmd)
uwtype_t wtype;
char *cmd;
{
	register uwid_t uwid;

	/*
	 * Create a new window (using uw_fork()) and execute the specified
	 * shell command in it.  Returns the window ID of the new window
	 * (or -1 if the window creation failed)  There is no way to
	 * determine if the executed command failed.
	 */
	if ((uwid = uw_fork(wtype, (int *)0)) == 0) {
		(void)execl(uwshellname, uwshellname, "-c", cmd, (char *)0);
		_exit(1);	/* we'd better not reach this point */
		/*NOTREACHED*/
	} else
		return(uwid);
}
