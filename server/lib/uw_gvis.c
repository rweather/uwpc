/*
 *	uw library - uw_gvis, uw_svis
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_gvis(uwin, vp)
register UWIN uwin;
register int *vp;
{
	/*
	 * Get the visibility status of the window "uwin".  "vp" is a
	 * pointer to the integer where the status is returned.
	 */
	if (uwin != (UWIN)0) {
		if (vp != (int *)0) {
			*vp = uwin->uwi_vis;
			if (uwin->uwi_ctlfd > 0) {
				return(0);
			} else {
				uwerrno = uwin->uwi_uwerr = UWE_NOCTL;
				return(-1);
			}
		} else {
			uwerrno = uwin->uwi_uwerr = UWE_INVAL;
			return(-1);
		}
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}

uw_svis(uwin, v)
register UWIN uwin;
int v;
{
	union uwoptval optval;

	/*
	 * Make window "uwin" visible (v != 0) or invisible (v == 0).
	 */
	if (uwin != (UWIN)0) {
		uwin->uwi_vis = (v != 0);
		optval.uwov_1bit = uwin->uwi_vis;
		return(uw_optcmd(uwin, UWOP_VIS, UWOC_SET, &optval));
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
