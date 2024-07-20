/*
 *	uw library - uw_optfn
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include "uwlib.h"

uwfnptr_t
uw_optfn(uwin, optnum, optfn)
UWIN uwin;
uwopt_t optnum;
uwfnptr_t optfn;
{
	uwfnptr_t oldfn;

	/*
	 * Establish an option-processing function (defined by the host).
	 * The specified function will be called whenever an option message
	 * is received from the server.  The previous function is returned.
	 */
	oldfn = (uwfnptr_t)0;
	if (uwin != (UWIN)0) {
		if (optnum < UW_NUMOPTS) {
			oldfn = uwin->uwi_options[optnum].uwi_optfn;
			uwin->uwi_options[optnum].uwi_optfn = optfn;
			uwin->uwi_uwerr = UWE_NONE;
		} else
			uwin->uwi_uwerr = UWE_INVAL;
	}
	uwerrno = uwin->uwi_uwerr;
	return(oldfn);
}
