/*
 *	uw library - uw_detach
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_detach(uwin)
UWIN uwin;
{
	/*
	 * Detach the control file descriptor for a window, while still
	 * retaining access to the data file descriptor.
	 */
	if (uwin != (UWIN)0) {
		if (uwin->uwi_ctlfd >= 0) {
			uw_optdone(uwin->uwi_ctlfd);
			(void)close(uwin->uwi_ctlfd);
			uwin->uwi_ctlfd = -1;
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
