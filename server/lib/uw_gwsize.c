/*
 *	uw library - uw_gwsize, uw_swsize
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_gwsize(uwin, pp)
register UWIN uwin;
register struct uwpoint *pp;
{
	/*
	 * Get the (pixel) size of window "uwin" and store it in the
	 * point whose address is "pp".
	 */
	if (uwin != (UWIN)0) {
		if (pp != (struct uwpoint *)0) {
			*pp = uwin->uwi_wsize;
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

uw_swsize(uwin, pp)
register UWIN uwin;
struct uwpoint *pp;
{
	union uwoptval optval;

	/*
	 * Set the (pixel) size of window "uwin" to "pp".
	 */
	if (uwin != (UWIN)0) {
		if (pp != (struct uwpoint *)0) {
			uwin->uwi_wsize = *pp;
			optval.uwov_point.v = pp->uwp_v;
			optval.uwov_point.h = pp->uwp_h;
			return(uw_optcmd(uwin, UWOP_WSIZE, UWOC_SET, &optval));
		} else {
			uwerrno = uwin->uwi_uwerr = UWE_INVAL;
			return(-1);
		}
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
