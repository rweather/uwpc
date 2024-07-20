/*
 *	uw library - uw_options
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>

#include "uwlib.h"

#ifndef FD_SET
#define	FD_SET(n,p)	((p)->fds_bits[0] |= (1 << (n)))
#define	FD_CLR(n,p)	((p)->fds_bits[0] &= ~(1 << (n)))
#define	FD_ISSET(n,p)	((p)->fds_bits[0] & (1 << (n)))
#define	FD_ZERO(p)	((p)->fds_bits[0] = 0)
#define	FD_SETSIZE	(NBBY*sizeof(long))
#endif

#ifndef sigmask
#define sigmask(m)	(1 << ((m)-1))
#endif

#ifndef htons
/* These should have been defined in <netinet/in.h>, but weren't (in 4.2BSD) */
extern unsigned short htons(), ntohs();
extern unsigned long htonl(), ntohl();
#endif

static UWIN *fdmap;
static int (*oldsigio)();
static struct fd_set fdmask;
static int nfds;

extern char *malloc();

uw_optinit(fd, uwin)
int fd;
UWIN uwin;
{
	register int i, flags;
	static int first = 1;
	extern uw_optinput();

	/*
	 * The first time through, allocate the file descriptor map and
	 * bitmask, and cause SIGIO traps to be handled by uw_optinput.
	 */
	if (first) {
		first = 0;
		nfds = getdtablesize();
		fdmap = (UWIN *)malloc((unsigned)(sizeof(UWIN)*nfds));
		if (fdmap != (UWIN *)0)
			for (i = 0; i < nfds; i++)
				fdmap[i] = (UWIN)0;
		oldsigio = signal(SIGIO, uw_optinput);
		FD_ZERO(&fdmask);
	}

	/*
	 * Add the new control fd to the map and mask.  Set the owner
	 * to this process
	 */
	if (fd >= 0 && fd < nfds && uwin != (UWIN)0 && fdmap != (UWIN *)0) {
		fdmap[fd] = uwin;
		FD_SET(fd, &fdmask);
#ifdef SETOWN_BUG
		(void)fcntl(fd, F_SETOWN, -getpid());
#else
		(void)fcntl(fd, F_SETOWN, getpid());
#endif
		if ((flags = fcntl(fd, F_GETFL, 0)) >= 0)
			(void)fcntl(fd, F_SETFL, flags|FASYNC|FNDELAY);
		uwin->uwi_ipclen = 0;
	}
}

uw_optdone(fd)
{
	register int flags;

	/*
	 * Turn off asynchronous I/O notification and remove the
	 * map and mask information for "fd".  We do not close the
	 * file descriptor, however -- the caller is expected to
	 * take care of that.
	 */
	if (fd >= 0 && fd < nfds && fdmap != (UWIN *)0) {
		if ((flags = fcntl(fd, F_GETFL, 0)) >= 0)
			(void)fcntl(fd, F_SETFL, flags&~FASYNC);
		else
			(void)fcntl(fd, F_SETFL, 0);
		(void)fcntl(fd, F_SETFL, 0);
		fdmap[fd] = (UWIN)0;
		FD_CLR(fd, &fdmask);
	}
}

static
uw_optinput(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	register int k, n, fd;
	register UWIN uwin;
	register struct uwoption *uwop;
	register union uwoptval *uwov;
	uwopt_t optnum;
	uwoptcmd_t optcmd;
	uwfnptr_t userfn;
	int oldmask;
	struct timeval timeo;
	struct fd_set ready;
	extern int errno;

	/*
	 * This routine is called when input is waiting on a control
	 * file descriptor.
	 */
	oldmask = sigblock(sigmask(SIGALRM));
	do {
		ready = fdmask;
		timeo.tv_sec = 0;
		timeo.tv_usec = 0;
		n = select(nfds, &ready, (struct fd_set *)0,
			   (struct fd_set *)0, &timeo);
		if (n < 0 && errno == EBADF) {
			/*
			 * One of the file descriptors that we asked for
			 * is no longer valid.  This isn't supposed to
			 * happen; however, we try to handle it by testing
			 * each bit individually and eliminating the bad
			 * ones.
			 */
			for (fd=0; fd < nfds; fd++) {
				if (FD_ISSET(fd, &fdmask)) {
					do {
						ready = fdmask;
						timeo.tv_sec = 0;
						timeo.tv_usec = 0;
						k = select(nfds, &ready,
						    (struct fd_set *)0,
						    (struct fd_set *)0, &timeo);
						if (k < 0 && errno == EBADF) {
							fdmap[fd] = (UWIN)0;
							FD_CLR(fd, &fdmask);
						}
					} while (n < 0 && errno == EINTR);
				}
			}
		}
	} while (n < 0 && errno == EINTR);

	for (fd=0; n > 0 && fd < nfds; fd++) {
		if (FD_ISSET(fd, &ready)) {
			n--;
			uwin = fdmap[fd];
			while ((k = getmesg(fd, uwin)) > 0) {
				uwin->uwi_ipclen = 0;	/* for next time */
				if (uwin->uwi_ipcbuf.uwip_cmd == UWC_OPTION) {
					uwop = &uwin->uwi_ipcbuf.uwip_option;
					uwov = &uwop->uwop_val;
					optnum = ntohs(uwop->uwop_opt);
					optcmd = ntohs(uwop->uwop_cmd);
					if (optcmd == UWOC_SET)
						uw_ntoh(uwin->uwi_type, optnum,
						    (char *)uwov);
					if (optcmd == UWOC_SET) switch(optnum) {
					case UWOP_VIS:
						uwin->uwi_vis = !!uwov->uwov_6bit;
						break;
					case UWOP_TYPE:
						if (uwov->uwov_6bit<UW_NWTYPES)
							uwin->uwi_type=uwov->uwov_6bit;
						break;
					case UWOP_POS:
						uwin->uwi_pos.uwp_v = uwov->uwov_point.v;
						uwin->uwi_pos.uwp_h = uwov->uwov_point.h;
						break;
					case UWOP_TITLE:
						(void)strncpy(uwin->uwi_title,
						    uwov->uwov_string,
						    sizeof uwin->uwi_title);
						break;
					case UWOP_WSIZE:
						uwin->uwi_wsize.uwp_v = uwov->uwov_point.v;
						uwin->uwi_wsize.uwp_h = uwov->uwov_point.h;
						break;
					}
					if (optnum == UWOP_TYPE &&
					    optcmd == UWOC_SET &&
					    uwov->uwov_6bit < UW_NWTYPES)
						uwin->uwi_type=uwov->uwov_6bit;
					userfn = uwin->uwi_options[optnum].uwi_optfn;
					if (userfn != (uwfnptr_t)0)
						(*userfn)(uwin, optnum,
							  optcmd, uwov);
				}
			}
			if (k < 0)
				(void)uw_detach(uwin);	/* I/O error or EOF */
		}
	}
	(void)sigsetmask(oldmask);

	/*
	 * Finally, if "oldsigio" is not SIG_DFL, call it.
	 */
	if (oldsigio != SIG_DFL)
		(*oldsigio)(sig, code, scp);
}

static
getmesg(fd, uwin)
register int fd;
register UWIN uwin;
{
	register int len;
	register char *cp;

	/*
	 * Read some more bytes from control socket "fd" into the input
	 * buffer.  Return 1 if the message is now complete, -1 if an
	 * EOF was reached, or 0 otherwise.  Before returning 1, the byte
	 * order of the common parameters (command, length) is changed
	 * from network to host order.
	 */
	cp = (char *)&uwin->uwi_ipcbuf + uwin->uwi_ipclen;
	if (uwin->uwi_ipclen < sizeof(uwin->uwi_ipcbuf.uwip_len)) {
		len = read(fd, cp, sizeof uwin->uwi_ipcbuf.uwip_len - uwin->uwi_ipclen);
		if (len == 0 || (len < 0 && errno != EWOULDBLOCK))
			return(-1);
		if (len < 0)
			return(0);
		if ((uwin->uwi_ipclen +=len) < sizeof uwin->uwi_ipcbuf.uwip_len)
			return(0);
		uwin->uwi_ipcbuf.uwip_len = ntohs(uwin->uwi_ipcbuf.uwip_len);
		if (uwin->uwi_ipcbuf.uwip_len==sizeof uwin->uwi_ipcbuf.uwip_len)
			return(1);
		cp += len;
	}
	if (uwin->uwi_ipcbuf.uwip_len > sizeof(struct uwipc))
		uwin->uwi_ipcbuf.uwip_len = sizeof(struct uwipc);
	len = read(fd, cp, uwin->uwi_ipcbuf.uwip_len - uwin->uwi_ipclen);
	if (len == 0 || (len < 0 && errno != EWOULDBLOCK))
		return(-1);
	if ((uwin->uwi_ipclen += len) == uwin->uwi_ipcbuf.uwip_len) {
		uwin->uwi_ipcbuf.uwip_cmd = ntohs(uwin->uwi_ipcbuf.uwip_cmd);
		return(1);
	} else
		return(0);
}
