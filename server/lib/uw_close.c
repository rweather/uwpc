/*
 *	uw library - uw_close
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_close(uwin)
UWIN uwin;
{
	/*
	 * Close all connections to an existing window, but do not kill it.
	 */
	if (uwin != (UWIN)0) {
		if (uwin->uwi_ctlfd >= 0)
			(void)uw_detach(uwin);
		if (uwin->uwi_datafd >= 0)
			(void)close(uwin->uwi_datafd);
		free((char *)uwin);
		return(0);
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
