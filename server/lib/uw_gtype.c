/*
 *	uw library - uw_gtype, uw_stype
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uw_gtype(uwin, tp)
register UWIN uwin;
register uwtype_t *tp;
{
	/*
	 * Get the type of the window "uwin".  The window type is stored
	 * in the variable whose address is passed in "tp".
	 */
	if (uwin != (UWIN)0) {
		if (tp != (uwtype_t *)0) {
			*tp = uwin->uwi_type;
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

uw_stype(uwin, t)
register UWIN uwin;
int t;
{
	union uwoptval optval;

	/*
	 * Set the type of window "uwin" to "t".
	 */

	if (uwin != (UWIN)0) {
		if (t < UW_NWTYPES) {
			uwin->uwi_type = t;
			optval.uwov_6bit = uwin->uwi_type;
			return(uw_optcmd(uwin, UWOP_TYPE, UWOC_SET, &optval));
		} else {
			uwerrno = uwin->uwi_uwerr = UWE_INVAL;
			return(-1);
		}
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
