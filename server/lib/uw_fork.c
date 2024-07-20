/*
 *	uw library - uw_fork
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

uwid_t
uw_fork(wtype, pidp)
uwtype_t wtype;
int *pidp;
{
	register int pid;
	register int sd;
	register struct uwipc *uwip;
	register uwid_t wid;
	auto int fd;
	char *portal;
	int lmode, ldisc;
	struct sgttyb sg;
	struct tchars tc;
	struct ltchars ltc;
	struct iovec iov;
	struct msghdr msg;
	struct sockaddr_un sa;
	struct ptydesc pt;
	char idstr[20];
	char *env[2];
	extern char *ltoa();

	/*
	 * Create a new window attached to a pseudo-terminal.  This routine
	 * returns twice -- once in the parent and once in the (new) child.
	 * The parent receives the window ID of the child (or -1 if the
	 * window creation failed).  Zero is returned to the child.
	 * If "pidp" is a non-NULL pointer, the process ID from the fork()
	 * is stored there.
	 */

	/*
	 * Get the terminal configuration for this tty.  For now we
	 * assume that "/dev/tty" is defined.  Eventually we'll have to
	 * provide defaults in case it is not.
	 */
	if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
		(void)ioctl(fd, (int)TIOCGETP, (char *)&sg);
		(void)ioctl(fd, (int)TIOCGETC, (char *)&tc);
		(void)ioctl(fd, (int)TIOCGLTC, (char *)&ltc);
		(void)ioctl(fd, (int)TIOCLGET, (char *)&lmode);
		(void)ioctl(fd, (int)TIOCGETD, (char *)&ldisc);
		(void)close(fd);
	} else {
		/* ... */
	}

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
	 * Obtain a pseudo-tty and construct the datagram we will send later.
	 */
	if (openpty(&pt) < 0) {
		uwerrno = UWE_ERRNO;
		return(-1);
	}
	uwip = (struct uwipc *)malloc(sizeof(struct uwipc)+strlen(pt.pt_pname));
	env[0] = malloc(sizeof "UW_ID=" + sizeof idstr);
	if (uwip == (struct uwipc *)0 || env[0] == (char *)0) {
		uwerrno = UWE_NOMEM;
		return(-1);
	}
	uwip->uwip_cmd = UWC_NEWT;
	uwip->uwip_len = (char *)&uwip->uwip_newt - (char *)uwip +
	    sizeof(struct uwnewt) + strlen(pt.pt_pname);
	uwip->uwip_newt.uwnt_type = wtype;
	(void)strcpy(uwip->uwip_newt.uwnt_pty, pt.pt_pname);


	/* 
	 * Fork a child process using this pseudo-tty.  Initialize the
	 * terminal modes on the pseudo-tty to match those of the parent
	 * tty.  We really want a fork() here, not a vfork().
	 */
	while ((pid=fork()) < 0)
		sleep(5);
	if (pidp != (int *)0)
		*pidp = pid;
	if (pid) {
		wid = (long)pid << 16;
		uwip->uwip_newt.uwnt_id = wid;
	} else {
		(void)setgid(getgid());
		(void)setuid(getuid());
		wid = (long)getpid() << 16;
		(void)strcat(strcpy(env[0], "UW_ID="),
		     ltoa(wid, idstr, sizeof idstr));
		env[1] = (char *)0;
		env_set(env);
		(void)signal(SIGTSTP, SIG_IGN);
		(void)ioctl(open("/dev/tty", 2), (int)TIOCNOTTY, (char *)0);
		(void)close(open(pt.pt_tname, 0)); /*set new ctrl tty */
		(void)dup2(pt.pt_tfd, 0);
		(void)dup2(0, 1);
		(void)dup2(0, 2);
		fd = getdtablesize();
		while (--fd > 2)
			(void)close(fd);
		(void)ioctl(fd, (int)TIOCSETD, (char *)&ldisc);
		(void)ioctl(0, (int)TIOCSETN, (char *)&sg);
		(void)ioctl(0, (int)TIOCSETC, (char *)&tc);
		(void)ioctl(0, (int)TIOCSLTC, (char *)&ltc);
		(void)ioctl(0, (int)TIOCLSET, (char *)&lmode);
		uwerrno = UWE_NONE;
		return(0);
	}


	/*
	 * Pass the file descriptor to the window server.
	 */
	iov.iov_base = (char *)uwip;
	iov.iov_len = uwip->uwip_len;
	msg.msg_name = (caddr_t)&sa;
	msg.msg_namelen = sizeof sa.sun_family + strlen(sa.sun_path);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_accrights = (caddr_t)&pt.pt_pfd;
	msg.msg_accrightslen = sizeof pt.pt_pfd;
	if (sendmsg(sd, &msg, 0) < 0) {
		free((char *)uwip);
		uwerrno = UWE_ERRNO;
		return(-1);
	}
	free((char *)uwip);
	uwerrno = UWE_NONE;
	return(wid);
}

static
char *
ltoa(l, buf, buflen)
long l;
char *buf;
int buflen;
{
	register char *cp;
	register unsigned long ul;
	static char digits[] = "0123456789";

	/*
	 * This routine replaces a call to sprintf() so that the library
	 * is independent of stdio.
	 */
	cp = buf+buflen;
	*--cp = '\0';
	ul = l;
	if (cp > buf) {
		do {
			*--cp = digits[ul%10];
			ul /= 10;
		} while (cp > buf && ul != 0);
	}
	return(cp);
}
