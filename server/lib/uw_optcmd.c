/*
 *	uw library - uw_optcmd
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

uw_optcmd(uwin, optnum, optcmd, optval)
UWIN uwin;
uwopt_t optnum;
uwoptcmd_t optcmd;
union uwoptval *optval;
{
	register int len;
	struct uwipc uwip;
	extern int errno;

	/*
	 * Send an option command string to the server (and eventually
	 * to the Macintosh).
	 */
	if (uwin != (UWIN)0) {
		if (uwin->uwi_ctlfd >= 0) {
			if (optnum < UW_NUMOPTS) {
				len = sizeof uwip;
				uwip.uwip_len = htons(len);
				uwip.uwip_cmd = htons(UWC_OPTION);
				uwip.uwip_option.uwop_id = htonl(uwin->uwi_id);
				uwip.uwip_option.uwop_opt = htons(optnum);
				uwip.uwip_option.uwop_cmd = htons(optcmd);
				switch (optcmd) {
				case UWOC_SET:
					if (optval == (union uwoptval *)0) {
						uwin->uwi_uwerr = UWE_INVAL;
						break;
					} 
					uwip.uwip_option.uwop_val = *optval;
					uw_hton(uwin->uwi_type, optnum,
					    (char *)&uwip.uwip_option.uwop_val);
					/* no break */
				case UWOC_ASK:
				case UWOC_DO:
				case UWOC_DONT:
				case UWOC_WILL:
				case UWOC_WONT:
					if (write(uwin->uwi_ctlfd, (char *)&uwip,
					    len) < 0) {
						uwin->uwi_uwerr = UWE_ERRNO;
						uwin->uwi_errno = errno;
					} else
						uwin->uwi_uwerr = UWE_NONE;
					break;
				default:
					uwin->uwi_uwerr = UWE_INVAL;
					break;
				}
			} else
				uwin->uwi_uwerr = UWE_INVAL;
		}
		uwerrno = uwin->uwi_uwerr;
		if (uwin->uwi_uwerr == UWE_NONE)
			return(0);
		else
			return(-1);
	} else {
		uwerrno = UWE_INVAL;
		return(-1);
	}
}
