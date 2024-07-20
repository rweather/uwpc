/*
 *	uw library - uw_kill
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <netinet/in.h>

#include "uwlib.h"

#ifndef htons
/* These should have been defined in <netinet/in.h>, but weren't (in 4.2BSD) */
extern unsigned short htons(), ntohs();
extern unsigned long htonl(), ntohl();
#endif

uw_kill(uwin)
UWIN uwin;
{
	register int len;
	struct uwipc uwip;
	extern int errno;

	/*
	 * Kill the window "uwin".  After putting out the contract,
	 * destroy the evidence by closing all existing connections
	 * to the window.
	 */
	if (uwin != (UWIN)0) {
		if (uwin->uwi_ctlfd >= 0) {
			len = sizeof uwip.uwip_killw +
			    (char *)&uwip.uwip_killw - (char *)&uwip;
			uwip.uwip_len = htons(len);
			uwip.uwip_cmd = htons(UWC_KILLW);
			uwip.uwip_killw.uwkw_id = htonl(uwin->uwi_id);
			if (write(uwin->uwi_ctlfd, (char *)&uwip, len) < 0) {
				uwin->uwi_errno = errno;
				uwerrno = uwin->uwi_uwerr = UWE_ERRNO;
			} else
				uwerrno = uwin->uwi_uwerr = UWE_NONE;
			(void)uw_detach(uwin);
		} else
			uwerrno = uwin->uwi_uwerr = UWE_NOCTL;
		if (uwin->uwi_uwerr == UWE_NONE)
			return(0);
		else
			return(-1);
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
