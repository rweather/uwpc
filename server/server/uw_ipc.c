/*
 *	uw IPC
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <strings.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#include "uw_param.h"
#include "uw_err.h"
#include "uw_opt.h"
#include "uw_win.h"
#include "uw_fd.h"
#include "uw_pcl.h"
#include "uw_ipc.h"

#ifndef ntohs
/* declaring these as one-element arrays or as NULL pointers is a HACK */
extern unsigned long ntohl(), htonl();
extern unsigned short ntohs(), htons();
static struct netadj na_ntoh[1] = {
	(short (*)())ntohs, (long (*)())ntohl, ntohs, ntohl
};
static struct netadj na_hton[1] = {
	(short (*)())htons, (long (*)())htonl, htons, htonl
};
#else
static struct netadj *na_ntoh = NULL;
static struct netadj *na_hton = NULL;
#endif

static int have_udport;
static char uipc_port[] = "/tmp/uwXXXXXX";

static int inet_sd;
static struct ipcmsg {
	int		im_len;
	struct uwipc	im_msg;
} *inet_buf;

extern int errno;

ipc_init(use_uipc)
{
	ipc_isinit();
	if (use_uipc)
		ipc_udinit();
}


/*
 * UNIX-domain
 */

static
ipc_udinit()
{
	register int len;
	register char *cp;
	register fildes_t sd;
	auto struct sockaddr_un sa;
	auto char *env[2];
	extern char *mktemp();

	len = strlen(UIPC_ENV) + sizeof uipc_port + 1;
	if ((cp = malloc(len)) != NULL) {
		(void)sprintf(cp, "%s=%s", UIPC_ENV, mktemp(uipc_port));
		env[0] = cp;
		env[1] = (char *)0;
		env_set(env);

		sa.sun_family = AF_UNIX;
		(void)strncpy(sa.sun_path, uipc_port, sizeof sa.sun_path-1);
		sa.sun_path[sizeof sa.sun_path-1] = '\0';
		if ((sd = socket(AF_UNIX, SOCK_DGRAM, 0)) >= 0 &&
		    bind(sd,&sa,sizeof sa.sun_family+strlen(sa.sun_path)) >= 0){
			have_udport = 1;
			(void)chmod(uipc_port, S_IREAD|S_IWRITE);
			(void)fcntl(sd, F_SETFL, FNDELAY);
			fdmap[sd].f_type = FDT_UDSOCK;
			FD_SET(sd, &selmask[0].sm_rd);
		}
	}
}

ipc_exit()
{
	if (have_udport)
		(void)unlink(uipc_port);
}

ipc_udrecv(sd)
register fildes_t sd;
{
	register struct window *w;
	register int cnt;
	struct msghdr msg;
	auto int fd;
	struct iovec iov;
	struct stat st1, st2;
	union {
		struct uwipc uwip;
		char data[1024];
	} buf;


	/*
	 * main() calls this routine when there is a message waiting on
	 * the UNIX-domain socket.  The message's access rights are
	 * expected to contain the file descriptor for the "master" side
	 * of a pseudo-tty.  The message contains the name of the pty.
	 * The sender is expected to start up a process on the slave side
	 * of the pty.  This allows the host end to create windows which
	 * run something other than the shell.
	 */
	fd = -1;

	iov.iov_base = (caddr_t)buf.data;
	iov.iov_len = sizeof buf - 1;

	msg.msg_name = (caddr_t)0;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = (caddr_t)&fd;
	msg.msg_accrightslen = sizeof fd;

	if ((cnt=recvmsg(sd, &msg, 0)) < 0 || cnt != buf.uwip.uwip_len)
		return;
	switch (buf.uwip.uwip_cmd) {
	case UWC_NEWT:
		if (msg.msg_accrightslen > 0 && fd >= 0) {
			/*
			 * We can't trust the process which connected to us,
			 * so we verify that it really passed us a pseudo-tty's
			 * file descriptor by checking the device name and its
			 * inode number.  [Of course, if someone else wants to
			 * hand us a terminal session running under their
			 * uid....]
			 */
			if (cnt == sizeof buf)
				cnt--;
			buf.data[cnt] = '\0';
			if (strncmp(buf.uwip.uwip_newt.uwnt_pty,
			    "/dev/pty", sizeof "/dev/pty"-1) ||
			    fstat(fd, &st1) < 0 ||
			    stat(buf.uwip.uwip_newt.uwnt_pty, &st2) < 0 ||
			    st1.st_dev != st2.st_dev ||
			    st1.st_ino != st2.st_ino) {
				(void)close(fd);
				return;
			}
			/*
			 * OK, we believe the sender.  We allocate a window and
			 * tell the Macintosh to create that window on its end.
			 * If we have no free windows, then we close the file
			 * descriptor (which will terminate the slave process).
			 */
			w = PCL_NEWW(0, WC_INTERNAL,
			      buf.uwip.uwip_newt.uwnt_type,
			      (nwin_t)0, buf.uwip.uwip_newt.uwnt_id,
			      fd, (fildes_t)-1);
			if (w != NULL) {
				(void)strncpy(w->w_tty,
				    buf.uwip.uwip_newt.uwnt_pty,
				    sizeof w->w_tty-1);
				w->w_tty[5] = 't'; /* switch to "/dev/ttyp?" */
				utmp_add(w->w_tty);
			} else
				(void)close(fd);
		}
		break;
	case UWC_OPTION:
		w = win_search(buf.uwip.uwip_option.uwop_id,
		    protocol->p_maxwin);
		if (w != NULL) {
			opt_extopt((caddr_t)w, &w->w_optdefn,
			    (woptcmd_t)buf.uwip.uwip_option.uwop_cmd,
			    (woption_t)buf.uwip.uwip_option.uwop_opt,
			    (char *)&buf.uwip.uwip_option.uwop_val,
			    (struct netadj *)0);
		}
		break;
	}
}


/*
 * Internet domain
 */

static
ipc_isinit()
{
	register fildes_t sd;
	register char *cp;
	struct hostent *h;
	struct sockaddr_in sin;
	auto int sinlen;
	char hostname[32];
	char *env[2];

	/*
	 * Allocate enough buffers for each file descriptor to have one.
	 * This is overkill.
	 */
	inet_buf = (struct ipcmsg *)malloc(nfds * sizeof(struct ipcmsg));
	if (inet_buf == NULL)
		return;

	/*
	 * Determine our host name and get an Internet stream socket.
	 * We really should specify the protocol here (rather than 0)
	 * but we "know" that it defaults to TCP.
	 */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return;
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	bzero(sin.sin_zero, sizeof sin.sin_zero);
	if (gethostname(hostname, sizeof hostname) < 0 || hostname[0] == '\0')
		(void)strcpy(hostname, "localhost");
	if ((h = gethostbyname(hostname)) != NULL)
		bcopy(h->h_addr, (char *)&sin.sin_addr, h->h_length);
	else
		sin.sin_addr.s_addr = htonl(0x7f000001L); /* 128.0.0.1 (lo0) */
	if (bind(sd, &sin, sizeof sin) < 0) {
		/*
		 * Unable to bind to unspecified port -- try once more with
		 * loopback device.  If we already were using the loopback
		 * device we just suffer the inefficiency of doing this twice.
		 */
		sin.sin_addr.s_addr = htonl(0x7f000001L);
		if (bind(sd, &sin, sizeof sin) < 0) {
			(void)close(sd);
			return;
		}
	}

	/*
	 * Listen for incoming connections
	 */
	if (listen(sd, NWINDOW) < 0) {
		(void)close(sd);
		return;
	}

	/*
	 * Determine our port number and put our address in the environment.
	 */
	sinlen = sizeof sin;
	if (getsockname(sd, (char *)&sin, &sinlen) < 0) {
		/* huh?  Oh well, give up */
		(void)close(sd);
		return;
	}
	if ((cp = malloc(sizeof INET_ENV + 1 + 8 + 1 + 5)) == NULL) {
		/* no memory, give up */
		(void)close(sd);
		return;
	}
	sprintf(cp, "%s=%08lx.%05u", INET_ENV,
	    ntohl(sin.sin_addr.s_addr), ntohs(sin.sin_port));
	env[0] = cp;
	env[1] = (char *)0;
	env_set(env);

	inet_sd = sd;
	fdmap[sd].f_type = FDT_ISSOCK;
	FD_SET(sd, &selmask[0].sm_rd);
}

ipc_isrecv(sd)
register fildes_t sd;
{
	register fildes_t fd;
	register struct uwipc *uwip;
	register struct window *w;
	register uwerr_t uwerr;
	register int len;
	struct sockaddr sin;
	struct uwipc reply;
	auto int sinlen;

	/*
	 * This routine is called when one of two conditions occur.  It is
	 * called when an outside process tries to establish a steam
	 * Internet connection.
	 *
	 * Later, as soon as data is available, this routine will be
	 * called again to handle the external message (which must be
	 * a "new window" command).
	 */
	if (sd == inet_sd) {
		sinlen = sizeof sin;
		if ((fd = accept(sd, &sin, &sinlen)) >= 0) {
			(void)fcntl(fd, F_SETFL, FNDELAY);
			fdmap[fd].f_type = FDT_ISSOCK;
			fdmap[fd].f_win = (struct window *)0;
			FD_SET(fd, &selmask[0].sm_rd);
			inet_buf[fd].im_len = 0;
		}
	} else {
		switch (ipc_getmsg(sd, inet_buf + sd)) {
		case -1:
			(void)close(sd);
			fdmap[sd].f_type = FDT_NONE;
			FD_CLR(sd, &selmask[0].sm_rd);
			FD_CLR(sd, &selmask[0].sm_wt);
			FD_CLR(sd, &selmask[0].sm_ex);
			break;
		case 1:
			uwip = &inet_buf[sd].im_msg;
			uwerr = UWE_NONE;
			if ((uwip->uwip_len < sizeof(struct uwneww) + 
			    ((char *)&uwip->uwip_neww - (char *)uwip)) ||
			    uwip->uwip_cmd != UWC_NEWW) {
				uwerr = UWE_NXTYPE;
			} else {
				fd = ipc_ctlopen(sd,
				    (unsigned)uwip->uwip_neww.uwnw_ctlport);
				w = PCL_NEWW(0, WC_EXTERNAL,
				      ntohs(uwip->uwip_neww.uwnw_type),
				      (nwin_t)0, ntohl(uwip->uwip_neww.uwnw_id),
				      sd, fd);
				if (w == (struct window *)0)
					uwerr = UWE_NXTYPE;	/* for now */
				else
					uwerr = UWE_NONE;
			}
			len = sizeof(struct uwstatus) +
			    ((char *)&reply.uwip_status - (char *)&reply);
			reply.uwip_len = htons(len);
			reply.uwip_cmd = htons(UWC_STATUS);
			reply.uwip_status.uwst_err = htons(uwerr);
			reply.uwip_status.uwst_errno = htons(errno);
			if (uwerr == UWE_NONE)
				reply.uwip_status.uwst_id = htonl(w->w_id);
			else
				reply.uwip_status.uwst_id = 0;
			(void)write(sd, (char *)&reply, len);
			if (uwerr != UWE_NONE) {
				(void)close(sd);
				fdmap[sd].f_type = FDT_NONE;
				FD_CLR(sd, &selmask[0].sm_rd);
				FD_CLR(sd, &selmask[0].sm_wt);
				FD_CLR(sd, &selmask[0].sm_ex);
			}
			inet_buf[sd].im_len = 0;
		}
	}
}

ipc_ctlopen(sd, port)
fildes_t sd;
unsigned port;
{
	register int fd;
	auto struct sockaddr_in sin;
	auto int sinlen;

	/*
	 * Create a control socket and connect it to the same host as
	 * "sd" on the specified port.
	 */
	sinlen = sizeof sin;
	if (port == 0 ||
	    getpeername(sd, (struct sockaddr *)&sin, &sinlen) < 0 ||
	    (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return(-1);
	} else {
		sin.sin_port = port;
		(void)fcntl(fd, F_SETFL, FNDELAY);
		if (connect(fd, &sin, sinlen) < 0 && errno != EINPROGRESS) {
			(void)close(fd);
			return(-1);
		} else
			return(fd);
	}
}

void
ipc_optmsg(win, optcmd, optnum, data, datalen)
caddr_t win;
woptcmd_t optcmd;
woption_t optnum;
char *data;
unsigned datalen;
{
	register struct window *w;
	register int len;
	struct uwipc uwip;

	/*
	 * Propagate a window option message (WILL, WONT, SET) from the Mac
	 * to the remote process (external windows only).
	 */
	if ((w = (struct window *)win) != NULL && w->w_alloc &&
	    w->w_class == WC_EXTERNAL && w->w_ctlfd >= 0 &&
	    optnum <= WONUM_MAX && (optcmd == WOC_WILL || optcmd == WOC_WONT ||
	     (optcmd == WOC_SET && data != NULL))) {
		len = datalen +
		    ((char *)&uwip.uwip_option.uwop_val - (char *)&uwip);
		uwip.uwip_len = htons(len);
		uwip.uwip_cmd = htons(UWC_OPTION);
		uwip.uwip_option.uwop_id = htonl(w->w_id);
		uwip.uwip_option.uwop_opt = htons(optnum);
		uwip.uwip_option.uwop_cmd = htons(optcmd);
		if (optcmd == WOC_SET) {
			bcopy(data, (char *)&uwip.uwip_option.uwop_val,
			    (int)datalen);
			opt_netadj(w->w_optdefn.wod_optlst[optnum].wol_argdefn,
			    (char *)&uwip.uwip_option.uwop_val, na_hton);
		}
		(void)write(w->w_ctlfd, (char *)&uwip, len);
	}
}

ipc_ctlrecv(mfd, sd, win)
fildes_t mfd;
register fildes_t sd;
register struct window *win;
{
	register struct window *w;
	register struct uwipc *uwip;

	switch (ipc_getmsg(sd, inet_buf + sd)) {
	case -1:
		(void)close(sd);
		fdmap[sd].f_type = FDT_NONE;
		FD_CLR(sd, &selmask[0].sm_rd);
		FD_CLR(sd, &selmask[0].sm_wt);
		FD_CLR(sd, &selmask[0].sm_ex);
		break;
	case 1:
		uwip = &inet_buf[sd].im_msg;
		switch (uwip->uwip_cmd) {
		case UWC_KILLW:
			if ((uwip->uwip_len == sizeof(struct uwkillw) + 
			    ((char *)&uwip->uwip_killw - (char *)uwip))) {
				w = win_search(ntohl(uwip->uwip_killw.uwkw_id),
				    protocol->p_maxwin);
				if (w == win)
					PCL_KILLW(mfd, w);
			}
			break;
		case UWC_OPTION:
			/* hope the message is long enough... sigh */
			if (uwip->uwip_len > 
			 ((char *)&uwip->uwip_option.uwop_val - (char *)uwip)) {
				w = win_search(ntohl(uwip->uwip_option.uwop_id),
				    protocol->p_maxwin);
				if (w == win) {
					opt_extopt((caddr_t)w, &w->w_optdefn,
					    (woptcmd_t)ntohs(uwip->uwip_option.uwop_cmd),
					    (woption_t)ntohs(uwip->uwip_option.uwop_opt),
					    (char *)&uwip->uwip_option.uwop_val,
					    na_ntoh);
				}
			}
			break;
		}
		inet_buf[sd].im_len = 0;
	}
}

ipc_getmsg(sd, im)
register fildes_t sd;
register struct ipcmsg *im;
{
	register int len;
	register char *cp;

	/*
	 * Read some more bytes from socket "sd" into the message buffer
	 * contained in "im".  Return 1 if the message is now complete,
	 * -1 if an EOF was reached, or 0 otherwise.  Before returning 1,
	 * the byte order of the common parameters (command, length) is
	 * changed from network to host order.
	 *
	 * This routine expects the socket to use non-blocking I/O (which
	 * is enabled by ipc_isrecv() when the connection is accepted).
	 */
	cp = (char *)&im->im_msg + im->im_len;
	if (im->im_len < sizeof(im->im_msg.uwip_len)) {
		len = read(sd, cp, sizeof im->im_msg.uwip_len - im->im_len);
		if (len == 0 || (len < 0 && errno != EWOULDBLOCK))
			return(-1);
		if ((im->im_len += len) < sizeof im->im_msg.uwip_len)
			return(0);
		im->im_msg.uwip_len = ntohs(im->im_msg.uwip_len);
		if (im->im_msg.uwip_len == sizeof im->im_msg.uwip_len)
			return(1);
		cp += len;
	}
	if (im->im_msg.uwip_len > sizeof(struct ipcmsg))
		im->im_msg.uwip_len = sizeof(struct ipcmsg);
	len = read(sd, cp, im->im_msg.uwip_len - im->im_len);
	if (len == 0)
		return(-1);
	if (len < 0)
		return((errno==EWOULDBLOCK) ? 0 : -1);
	if ((im->im_len += len) == im->im_msg.uwip_len) {
		im->im_msg.uwip_cmd = ntohs(im->im_msg.uwip_cmd);
		return(1);
	} else
		return(0);
}
