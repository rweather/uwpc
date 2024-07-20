/*
 *	uw library - uw_gpos, uw_spos
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_gpos(uwin, pp)
register UWIN uwin;
register struct uwpoint *pp;
{
	/*
	 * Get the position of window "uwin" and store it in the point
	 * whose address is "pp".
	 */
	if (uwin != (UWIN)0) {
		if (pp != (struct uwpoint *)0) {
			*pp = uwin->uwi_pos;
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

uw_spos(uwin, pp)
register UWIN uwin;
struct uwpoint *pp;
{
	union uwoptval optval;

	/*
	 * Set the position of window "uwin" to "pp".
	 */
	if (uwin != (UWIN)0) {
		if (pp != (struct uwpoint *)0) {
			uwin->uwi_pos = *pp;
			optval.uwov_point.v = pp->uwp_v;
			optval.uwov_point.h = pp->uwp_h;
			return(uw_optcmd(uwin, UWOP_POS, UWOC_SET, &optval));
		} else {
			uwerrno = uwin->uwi_uwerr = UWE_INVAL;
			return(-1);
		}
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
