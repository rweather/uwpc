/*
 *	uw library - uw_rsetopt
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <strings.h>
#include <signal.h>
#include "openpty.h"

#include "uwlib.h"

extern char *malloc();
extern char *getenv();

uw_rsetopt(uwid, optnum, optval)
uwid_t uwid;
uwopt_t optnum;
union uwoptval *optval;
{
	register int sd;
	register struct uwipc *uwip;
	char *portal;
	struct iovec iov;
	struct msghdr msg;
	struct sockaddr_un sa;

	/*
	 * Set a window option on a remote window (that is, one for which
	 * we do not have a control fd).
	 */

	/*
	 * Create a UNIX-domain socket.
	 */
	if (!(portal=getenv("UW_UIPC"))) {
		uwerrno = UWE_NXSERV;
		return(-1);
	}

	if ((sd=socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
		uwerrno = UWE_ERRNO;
		return(-1);
	}
	sa.sun_family = AF_UNIX;
	(void)strncpy(sa.sun_path, portal, sizeof sa.sun_path-1);
	sa.sun_path[sizeof sa.sun_path-1] = '\0';


	/*
	 * Construct the datagram we will send later.
	 */
	uwip = (struct uwipc *)malloc(sizeof(struct uwipc));
	if (uwip == (struct uwipc *)0) {
		uwerrno = UWE_NOMEM;
		return(-1);
	}
	uwip->uwip_cmd = UWC_OPTION;
	uwip->uwip_len = sizeof(struct uwipc);
	uwip->uwip_option.uwop_id = uwid;
	uwip->uwip_option.uwop_cmd = UWOC_SET;
	uwip->uwip_option.uwop_opt = optnum;
	uwip->uwip_option.uwop_val = *optval;

	/*
	 * Pass the file descriptor to the window server.
	 */
	iov.iov_base = (char *)uwip;
	iov.iov_len = uwip->uwip_len;
	msg.msg_name = (caddr_t)&sa;
	msg.msg_namelen = sizeof sa.sun_family + strlen(sa.sun_path);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = (caddr_t)0;
	msg.msg_accrightslen = 0;
	if (sendmsg(sd, &msg, 0) < 0) {
		free((char *)uwip);
		uwerrno = UWE_ERRNO;
		return(-1);
	}
	free((char *)uwip);
	uwerrno = UWE_NONE;
	return(0);
}
