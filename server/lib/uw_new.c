/*
 *	uw library - uw_new
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <strings.h>
#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include "openpty.h"

#include "uwlib.h"

extern char *malloc();
extern char *getenv();

#ifndef htons
/* These should have been defined in <netinet/in.h>, but weren't (in 4.2BSD) */
extern unsigned short htons(), ntohs();
extern unsigned long htonl(), ntohl();
#endif

UWIN
uw_new(uwtype, sin)
uwtype_t uwtype;
struct sockaddr_in *sin;
{
	register UWIN uwin;
	register char *cp, c;
	register int len;
	register int ctlfd;
	int rdsz;
	auto int namelen;
	auto struct sockaddr_in sa, datasin, ctlsin;
	auto struct uwipc uwip;
	extern int errno;

	/*
	 * If our caller didn't supply an address for us to contact,
	 * look in the environment to find it.
	 */
	if (sin == (struct sockaddr_in *)0) {
		if ((cp = getenv(INET_ENV)) == (char *)0) {
			uwerrno = UWE_NXSERV;
			return((UWIN)0);
		}
		sin = &sa;
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = 0;
		sa.sin_port = 0;
		bzero(sa.sin_zero, sizeof sa.sin_zero);
		for ( ; isxdigit(c = *cp); cp++) {
			/* Pyramid compiler blows this, must use left shift */
			/* sa.sin_addr.s_addr *= 16; */
			sa.sin_addr.s_addr <<= 4;
			if (isdigit(c))
				sa.sin_addr.s_addr += c-'0';
			else if (islower(c))
				sa.sin_addr.s_addr += c-'a' + 10;
			else
				sa.sin_addr.s_addr += c-'A' + 10;
		}
		if (c == '.') {
			for (cp++; isdigit(c = *cp); cp++)
				sa.sin_port = sa.sin_port*10 + c-'0';
		}
		if (sa.sin_addr.s_addr == 0 || sa.sin_port == 0 ||
		    c != '\0') {
			/* bad address */
			uwerrno = UWE_INVAL;
			return((UWIN)0);
		}
		sa.sin_addr.s_addr = htonl(sa.sin_addr.s_addr);
		sa.sin_port = htons(sa.sin_port);
	}

	/*
	 * Allocate space for a new window structure.
	 */
	if ((uwin = (UWIN)malloc(sizeof(*uwin))) == (UWIN)0) {
		uwerrno = UWE_NOMEM;
		return((UWIN)0);
	}
	uwin->uwi_type = uwtype;
	for (len=0; len < UW_NUMOPTS; len++) /* "len" is a convenient "int" */
		uwin->uwi_options[len].uwi_optfn = (uwfnptr_t)0;
	
	/*
	 * Create sockets for the data and control file descriptors.
	 */
	if ((uwin->uwi_datafd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
	    (uwin->uwi_ctlfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		if (uwin->uwi_datafd >= 0)
			(void)close(uwin->uwi_datafd);
		return((UWIN)0);
	}

	/*
	 * Bind these sockets to a local address.  We figure out the
	 * local machine's host number and use it if possible; otherwise,
	 * we fall back to 127.0.0.1 (loopback device).  After binding,
	 * we determine the port number for the control socket, since we
	 * must send that to the server.  Connect to the server.
	 */
	datasin.sin_family = AF_INET;
	datasin.sin_port = 0;
	bzero(datasin.sin_zero, sizeof datasin.sin_zero);
	getmyaddr(&datasin.sin_addr);
	ctlsin.sin_family = AF_INET;
	ctlsin.sin_port = 0;
	bzero(ctlsin.sin_zero, sizeof ctlsin.sin_zero);
	getmyaddr(&ctlsin.sin_addr);
	if (bind(uwin->uwi_datafd, (struct sockaddr *)&datasin, sizeof datasin) < 0 ||
	    bind(uwin->uwi_ctlfd, (struct sockaddr *)&ctlsin, sizeof ctlsin) < 0 ||
	    listen(uwin->uwi_ctlfd, 1) < 0) {
		uwerrno = UWE_ERRNO;
		goto error;
	}
	namelen = sizeof ctlsin;
	(void)getsockname(uwin->uwi_ctlfd, (char *)&ctlsin, &namelen);

	if (connect(uwin->uwi_datafd, sin, sizeof(struct sockaddr_in)) < 0) {
		uwerrno = UWE_ERRNO;
		goto error;
	}

	/*
	 * Now we have enough information to build the new-window command
	 * and send it to the server.  The initial command is sent to the
	 * data port.  Next, we wait for a connection from the server to
	 * our data socket.  Finally, we expect the server to send us a
	 * new window status message on the data fd.
	 */
	len = sizeof uwip.uwip_neww + (char *)&uwip.uwip_neww - (char *)&uwip;
	uwip.uwip_len = htons(len);
	uwip.uwip_cmd = htons(UWC_NEWW);
	uwip.uwip_neww.uwnw_id = 0;	/* let server choose this */
	uwip.uwip_neww.uwnw_type = htons(uwtype);
	uwip.uwip_neww.uwnw_ctlport = ctlsin.sin_port;/* byte order correct */
	if (write(uwin->uwi_datafd, (char *)&uwip, len) < 0) {
		uwerrno = UWE_ERRNO;
		goto error;
	}
	
	namelen = sizeof ctlsin;
	if ((ctlfd = accept(uwin->uwi_ctlfd, (struct sockaddr_in *)&ctlsin, &namelen)) < 0) {
		uwerrno = UWE_ERRNO;
		goto error;
	}
	(void)close(uwin->uwi_ctlfd);
	uwin->uwi_ctlfd = ctlfd;
	uw_optinit(ctlfd, uwin);

	cp = (char *)&uwip.uwip_len;
	rdsz = sizeof uwip.uwip_len;
	while (rdsz > 0 && (len=read(uwin->uwi_datafd, cp, rdsz)) > 0) {
		cp += len;
		rdsz -= len;
	}
	if (len > 0) {
		rdsz = htons(uwip.uwip_len) - sizeof uwip.uwip_len;
		while (rdsz > 0 && (len=read(uwin->uwi_datafd, cp, rdsz)) > 0) {
			cp += len;
			rdsz -= len;
		}
	}
	if (len <= 0) {
		uwerrno = UWE_ERRNO;
		goto error;
	}
	uwerrno = uwin->uwi_uwerr = ntohs(uwip.uwip_status.uwst_err);
	errno = uwin->uwi_errno = ntohs(uwip.uwip_status.uwst_errno);
	if (uwin->uwi_uwerr != UWE_NONE)
		goto error;
	uwin->uwi_id = ntohl(uwip.uwip_status.uwst_id);
	return(uwin);

error:
	(void)close(uwin->uwi_datafd);
	(void)close(uwin->uwi_ctlfd);
	free((char *)uwin);
	return((UWIN)0);
}

static
getmyaddr(addr)
struct in_addr *addr;
{
	register struct hostent *h;
	char hostname[32];
	static int once = 1;
	static struct in_addr myaddr;

	if (once) {
		if (gethostname(hostname, sizeof hostname) < 0) {
			(void)strncpy(hostname, "localhost", sizeof hostname-1);
			hostname[sizeof hostname-1] = '\0';
		}
		if ((h = gethostbyname(hostname)) != (struct hostent *)0)
			myaddr = *(struct in_addr *)h->h_addr;
		else
			myaddr.s_addr = htonl(0x7f000001L);
		once = 0;
	}
	*addr = myaddr;
}
