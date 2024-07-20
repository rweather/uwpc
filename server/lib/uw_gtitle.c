/*
 *	uw library - uw_gtitle, uw_stitle
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <strings.h>
#include "uwlib.h"

uw_gtitle(uwin, ttl)
register UWIN uwin;
uwtitle_t ttl;
{
	/*
	 * Get the title of window "uwin" and put it in "ttl".
	 */
	if (uwin != (UWIN)0) {
		(void)strncpy(ttl, uwin->uwi_title, sizeof(uwtitle_t));
		if (uwin->uwi_ctlfd > 0) {
			return(0);
		} else {
			uwerrno = uwin->uwi_uwerr = UWE_NOCTL;
			return(-1);
		}
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}

uw_stitle(uwin, ttl)
register UWIN uwin;
uwtitle_t ttl;
{
	union uwoptval optval;

	/*
	 * Set the title of window "uwin" to "ttl".
	 */
	if (uwin != (UWIN)0) {
		(void)strncpy(uwin->uwi_title, ttl, sizeof uwin->uwi_title);
		uwin->uwi_title[sizeof uwin->uwi_title - 1] = '\0';
		(void)strncpy(optval.uwov_string,ttl,sizeof optval.uwov_string);
		optval.uwov_string[sizeof optval.uwov_string - 1] = '\0';
		return(uw_optcmd(uwin, UWOP_TITLE, UWOC_SET, &optval));
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
