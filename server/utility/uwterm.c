/*
 *	uwterm
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#include "openpty.h"
#include "uwlib.h"

#ifndef UWTERM
#define	UWTERM	"uwterm"
#endif

#define	CTL(c)		((c)&037)

#ifndef FD_SET
/* 4.2 retrofit: better definitions for these are in 4.3BSD's <sys/types.h> */
#define	FD_SET(n,p)	((p)->fds_bits[0] |= (1 << (n)))
#define	FD_CLR(n,p)	((p)->fds_bits[0] &= ~(1 << (n)))
#define	FD_ISSET(n,p)	((p)->fds_bits[0] & (1 << (n)))
#define	FD_ZERO(p)	((p)->fds_bits[0] = 0)
#define	FD_SETSIZE	(NBBY*sizeof(long))
#endif

extern int optind;
extern char *optarg;
extern char *getenv();
extern char *malloc();
extern deadkid();
extern int errno;

#ifndef htons
/* These should have been defined in <netinet/in.h>, but weren't (in 4.2BSD) */
extern unsigned short htons(), ntohs();
extern unsigned long htonl(), ntohl();
#endif

char *argv0;

main(argc, argv)
int argc;
char **argv;
{
	register char *cp;
	register int c;
	int wflag;
	uwtype_t wtype;
	char *term, *title, *server, *login;
	struct sockaddr_in sa, *sin;
	char hostname[32];

	/*
	 * If called with no arguments, create a new window using the
	 * current shell according to the SHELL environment variable
	 * (or "/bin/sh" if that doesn't exist).
	 *
	 * Options which are recognized directly are:
	 *
	 *	-ninet	connect to server at address "inet"
	 *	-wtype	create window with emulation "type"
	 *	-ttitle	label window with "title"
	 *	-llogin	use login name "login" on remote machine
	 *
	 * If no explicit title is specified, the command name is used.
	 */
	argv0 = argv[0];
	sin = (struct sockaddr_in *)0;
	server = (char *)0;
	login = (char *)0;
	title = (char *)0;
	wflag = 0;
	term = (char *)0;
	while ((c = getopt(argc, argv, "l:n:t:w:")) != EOF) {
		switch (c) {
		case 'l':
			if (optarg[0] == '\0') {
				fprintf(stderr,
				    "%s: \"-l\" requires user name\n", argv0);
			} else
				login = optarg;
			break;
		case 'n':
			server = optarg;
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = 0;
			sa.sin_port = 0;
			bzero(sa.sin_zero, sizeof sa.sin_zero);
			for (cp=optarg; isxdigit(c = *cp); cp++) {
				/* Pyramid compiler botch */
				/* sa.sin_addr.s_addr *= 16; */
				sa.sin_addr.s_addr <<= 4;
				if (isdigit(c))
					sa.sin_addr.s_addr += c - '0';
				else if (islower(c))
					sa.sin_addr.s_addr += c-'a' + 10;
				else
					sa.sin_addr.s_addr += c-'A' + 10;
			}
			if (c == '.')
				for (cp++; isdigit(c = *cp); cp++)
					sa.sin_port = sa.sin_port*10 + c-'0';
			if (sa.sin_addr.s_addr == 0 || sa.sin_port == 0) {
				fprintf(stderr,
				    "%s: bad Internet address: %s\n",
				    argv0, optarg);
				return(1);
			}
			sa.sin_addr.s_addr = htonl(sa.sin_addr.s_addr);
			sa.sin_port = htons(sa.sin_port);
			sin = &sa;
			break;
		case 'w':
			wflag++;
			term = optarg;
			wtype = uw_ttype(optarg);
			break;
		case 't':
			title = optarg;
			break;
		}
	}

	gethostname(hostname, sizeof hostname);
	if (title == (char *)0) {
		/*
		 * If there was no "-t" argument, then "title" will still
		 * be NULL.  In this case we use the host name.
		 */
		if (optind == argc)
			title = hostname;
		else
			title = argv[optind];
	}
	
	if (!term) {
		/*
		 * If there was no "-w" argument, fetch the window
		 * type from the environment.  If that fails, use
		 * a default.
		 */
		if ((term=getenv("TERM")) != (char *)0)
			wtype = uw_ttype(term);
		else
			wtype = UWT_ADM31;
	}

	if (optind == argc-1) {
		/*
		 * The remaining argument is the host name.  Fork an "rsh"
		 * to execute this on the remote machine.
		 */
		return(doremote(argv[optind], server, title, term, login));
	} else if (optind == argc) {
		/*
		 * There are no other arguments.  Set up the connection
		 * to this machine.
		 */
		return(dolocal(sin, title, wtype, term));
	} else {
		fprintf(stderr,
		    "Usage: \"%s [-ttitle] [-wtype] [-naddr] [-llogin] host\"\n",
		    argv0);
		return(1);
	}
}

doremote(host, server, title, term, login)
char *host;
char *server;
char *title;
char *term;
char *login;
{
	register int fd, i, pid;
	register char *cp;
	char *av[16];

	/*
	 * Invoke a remote "uwterm" via "rsh".
	 */
	i = 0;
	av[i++] = "rsh";
	av[i++] = host;
	av[i++] = "-n";
	if (login != NULL) {
		av[i++] = "-l";
		av[i++] = login;
	}
	av[i++] = UWTERM;
	if (server == (char *)0) {
		if ((server = getenv("UW_INET")) == (char *)0) {
			fprintf(stderr,"%s: Can't find window server\n",argv0);
			return(1);
		}
	}
	if ((cp = malloc(3+strlen(server))) == (char *)0) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		return(1);
	}
	(void)strcat(strcpy(cp, "-n"), server);
	av[i++] = cp;

	if (title != (char *)0) {
		if ((cp = malloc(3+strlen(title))) == (char *)0) {
			fprintf(stderr, "%s: out of memory\n", argv0);
			return(1);
		}
		(void)strcat(strcpy(cp, "-t"), title);
		av[i++] = cp;
	}

	if (term != (char *)0) {
		if ((cp = malloc(3+strlen(term))) == (char *)0) {
			fprintf(stderr, "%s: out of memory\n", argv0);
			return(1);
		}
		(void)strcat(strcpy(cp, "-w"), term);
		av[i++] = cp;
	}

	av[i] = (char *)0;

	for (fd=getdtablesize()-1; fd > 2; fd--)
		(void)fcntl(fd, F_SETFD, 1);
	(void)execvp(av[0], av);
	(void)execv("/usr/ucb/rsh", av);	/* last-ditch try */
	perror(av[0]);
	return(1);
}

			
dolocal(sin, title, wtype, term)
struct sockaddr_in *sin;
char *title;
uwtype_t wtype;
char *term;
{
	register UWIN uwin;
	register int fd;
	register int s;
	struct ptydesc pt;

	/*
	 * Create and initialize a pseudo-terminal.
	 */
	if (openpty(&pt) < 0) {
		fprintf(stderr, "No pseudo-terminals are available\n");
		return(1);
	}
	ttyinit(pt.pt_tfd);


	/*
	 * Make fd's 0 and 1 be "/dev/null".  We'd like to force a known
	 * definition for fd 2 at this point, but we may need it for
	 * uw_perror() if uw_new() fails.
	 */
	if ((fd = open("/dev/null", O_RDWR)) >= 0) {	/* should be zero */
		if (fd != 0 && pt.pt_tfd != 0 && pt.pt_pfd != 0)
			dup2(fd, 0);
		if (fd != 1 && pt.pt_tfd != 1 && pt.pt_pfd != 1)
			dup2(fd, 1);
		if (fd > 2)
			(void)close(fd);
	}

	/*
	 * Create and title the window.  Make it visible.
	 */
	if ((uwin = uw_new(wtype, sin)) == (UWIN)0) {
		uw_perror(argv0, uwerrno, errno);
		return(1);
	}
	(void)uw_stitle(uwin, title);
	(void)uw_svis(uwin, 1);

	/*
	 * We no longer have use for fd 2, so make it "/dev/null" (the
	 * same as fd 0.
	 */
	(void)dup2(0, 2);

	/*
	 * Adjust the environment to contain the correct values of TERM,
	 * UW_ID, and UW_INET.  These will be inherited by the child
	 * we will create next.
	 */
	adjenv(term, sin, UW_ID(uwin));

	/*
	 * Create a process to execute the command connected to the pty.
	 */
	runcmd(pt.pt_tfd, pt.pt_tname);

	/*
	 * Ignore signals that might cause us trouble.  We do NOT ignore
	 * SIGTSTP so that the user can move us from the foreground into
	 * the background if desired.
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCHLD, deadkid);

#if defined(TIOCSWINSZ) || defined(TIOCSSIZE)
	/*
	 * Install an option handling routine to catch window size
	 * changes from the Mac and make the appropriate changes to
	 * the pseudo-terminal.
	 */
	setresize(uwin, pt.pt_pfd);
#endif

	/*
	 * Close the slave side of the pty.  Copy data between the pty
	 * and the window.  The return value from copy() is the exit
	 * status.
	 */
	(void)close(pt.pt_tfd);
	s = copy(pt.pt_pfd, UW_DATAFD(uwin));
	uw_kill(uwin);
	return(s);
}

ttyinit(ptyfd)
register int ptyfd;
{
	register int ttyfd;
	struct sgttyb sg;
	struct tchars tc;
	struct ltchars ltc;
	int ldisc;
	int lmode;

	/*
	 * Initialize the modes of the terminal whose file descriptor
	 * is "ptyfd" to the same modes as the current terminal.  If there
	 * isn't a "current terminal" handy, then use hardcoded defaults.
	 */
	for (ttyfd=0; ttyfd < 3 && ioctl(ttyfd, TIOCGETD, &ldisc) < 0; ttyfd++)
		;
	if (ttyfd < 3) {
		(void)ioctl(ttyfd, TIOCGETP, &sg);
		(void)ioctl(ttyfd, TIOCGETC, &tc);
		(void)ioctl(ttyfd, TIOCGLTC, &ltc);
		(void)ioctl(ttyfd, TIOCLGET, &lmode);
	} else {
		ldisc = NTTYDISC;

		sg.sg_ispeed = sg.sg_ospeed = 13; /* doesn't really matter */
		sg.sg_erase = 0177;	/* ugh */
		sg.sg_kill = CTL('u');	/* ugh */
		sg.sg_flags = ECHO|CRMOD|ANYP;

		tc.t_intrc = CTL('c');	/* yuck, should be 0177 */
		tc.t_quitc = CTL('\\');
		tc.t_startc = CTL('q');
		tc.t_stopc = CTL('s');
		tc.t_eofc = CTL('d');
		tc.t_brkc = -1;

		ltc.t_suspc = CTL('z');
		ltc.t_dsuspc = CTL('y');
		ltc.t_rprntc = CTL('r');
		ltc.t_flushc = CTL('o');
		ltc.t_werasc = CTL('w');
		ltc.t_lnextc = CTL('v');

		lmode = LCRTBS|LCRTERA|LCRTKIL|LCTLECH;
	}
	(void)ioctl(ptyfd, TIOCSETD, &ldisc);
	(void)ioctl(ptyfd, TIOCSETP, &sg);
	(void)ioctl(ptyfd, TIOCSETC, &tc);
	(void)ioctl(ptyfd, TIOCSLTC, &ltc);
	(void)ioctl(ptyfd, TIOCLSET, &lmode);
}

adjenv(term, sin, wid)
char *term;
struct sockaddr_in *sin;
uwid_t wid;
{
	char *env[4];
	static char ttype[sizeof "TERM=" + 16];
	static char inet[sizeof INET_ENV + 16];
	static char idstr[sizeof "UW_ID=" + 20];

	/*
	 * Redefine the environment variable UW_ID.  Redefine UW_INET
	 * if "sin" is non-NULL.  Redefine TERM.
	 */
	(void)sprintf(ttype, "TERM=%.15s", term);
	env[0] = ttype;
	
	(void)sprintf(idstr, "UW_ID=%ld", wid);
	env[1] = idstr;

	if (sin != NULL) {
		(void)sprintf(inet, "%s=%08lx.%d", INET_ENV,
		    ntohl(sin->sin_addr.s_addr), ntohs(sin->sin_port));
		env[2] = inet;
		env[3] = (char *)0;
	}  else
		env[2] = (char *)0;
	env_set(env);
}

runcmd(fd, tname)
int fd;
char *tname;
{
	register int pid;
	register char *shell;

	/*
	 * Figure out the name of the user's shell.  If unknown,
	 * use a default.
	 */
	if ((shell = getenv("SHELL")) == (char *)0)
		shell = "/bin/sh";

	/*
	 * Fork a new process and attach "fd" to fd's 0, 1, and 2 of
	 * that new process.  Disassociate the current controlling
	 * terminal and attach the new one (whose name is "tname").
	 */
	while ((pid = fork()) < 0)
		sleep(5);
	if (pid == 0) {
		if (fd != 0)
			dup2(fd, 0);
		if (fd != 1)
			dup2(fd, 1);
		if (fd != 2)
			dup2(fd, 2);
		if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
			(void)ioctl(fd, TIOCNOTTY, (char *)0);
			(void)close(fd);
		} else
			setpgrp(0, 0);
		(void)open(tname, O_RDWR);
		for (fd=getdtablesize()-1; fd > 2; fd--)
			(void)fcntl(fd, F_SETFD, 1);
		execlp(shell, "-", (char *)0);
		execl(shell, "-", (char *)0);
		_exit(1);
	}
}

copy(fd1, fd2)
int fd1, fd2;
{
	struct fdinfo {
		int	fi_fd;		/* associated file descriptor */
		int	fi_size;	/* amount of data in buffer */
		char	*fi_ptr;	/* pointer to data in fi_buf */
		char	fi_buf[1024];
	};
	register struct fdinfo *fi, *fo;
	register int n, nfds, len;
	struct fdinfo fdinfo[2];
	struct fd_set rdmask[2], wtmask[2], exmask[2];
	struct timeval tv;

	/*
	 * Copy data between file descriptors fd1 and fd2.  Return when an
	 * EOF is read or an I/O error (other than an interrupted system
	 * call or non-blocking I/O message) is encountered.
	 */
	FD_ZERO(&rdmask[1]);
	FD_ZERO(&wtmask[1]);
	FD_ZERO(&exmask[1]);

	fdinfo[0].fi_fd = fd1;
	fdinfo[0].fi_size = 0;
	fdinfo[1].fi_fd = fd2;
	fdinfo[1].fi_size = 0;

	FD_SET(fd1, &rdmask[1]);
	FD_SET(fd2, &rdmask[1]);

	(void)fcntl(fd1, F_SETFL, FNDELAY);
	(void)fcntl(fd2, F_SETFL, FNDELAY);

	nfds = ((fd1 > fd2) ? fd1 : fd2) + 1;

	while (1) {
		rdmask[0] = rdmask[1];
		wtmask[0] = wtmask[1];
		exmask[0] = exmask[1];
		errno = 0;
		if (fdinfo[0].fi_size != 0 || fdinfo[1].fi_size != 0) {
			/*
			 * Select does not work correctly for writes on
			 * some machines, so we must fake it.  If a write
			 * is pending, we time out after 1/50 second and
			 * pretend that select told us that writes could
			 * now be performed.  The code below will do the
			 * correct thing if the write would still block.
			 */
			tv.tv_sec = 0;
			tv.tv_usec = 1000000 / 50;
			n = select(nfds, rdmask, wtmask, exmask, &tv);
			wtmask[0] = wtmask[1];
		} else
			n = select(nfds, rdmask, wtmask, exmask, (struct timeval *)0);
		if (n < 0 && errno == EINTR) {
			continue;
		} else if (n <= 0) {
			perror("select");
			return(1);
		}
		for (fi=fdinfo; fi < fdinfo+2; fi++) {
			fo = fdinfo + !(fi - fdinfo);
			if (FD_ISSET(fi->fi_fd, rdmask)) {
				/* data available for reading */
				len = read(fi->fi_fd, fi->fi_buf,
				    sizeof fi->fi_buf);
				if (len > 0) {
					fi->fi_size = len;
					fi->fi_ptr = fi->fi_buf;
					FD_CLR(fi->fi_fd, &rdmask[1]);
					FD_SET(fo->fi_fd, &wtmask[1]);
					FD_SET(fo->fi_fd, &wtmask[0]);
				} else if (len == 0) {
					/* EOF, exit */
					return(0);
				} else if (errno != EWOULDBLOCK &&
				    errno != EINTR) {
					/* error, exit */
					return(1);
				}
			}
			if (FD_ISSET(fo->fi_fd, wtmask)) {
				/* data ready for writing */
				errno = 0;
				len = write(fo->fi_fd, fi->fi_ptr, fi->fi_size);
				if (len > 0) {
					fi->fi_ptr += len;
					fi->fi_size -= len;
					if (fi->fi_size == 0) {
						FD_SET(fi->fi_fd, &rdmask[1]);
						FD_CLR(fo->fi_fd, &wtmask[1]);
					}
				} else if (errno != EWOULDBLOCK &&
				    errno != EINTR) {
					/* error, exit */
					return(1);
				}
			}
		}
	}
}

deadkid()
{
	register int pid;

	/*
	 * Collect dead children.  Don't bother with their exit status
	 * or resource usage.
	 */
	while ((pid = wait3((union wait *)0, WNOHANG, (struct rusage *)0)) > 0)
		;
}

#if defined(TIOCSWINSZ) || defined(TIOCSSIZE)
static int ptyfd;

#ifdef TIOCSWINSZ
static struct winsize winsz;

void
doresize(uwin, optnum, optcmd, uwoptval)
UWIN uwin;
uwopt_t optnum;
uwoptcmd_t optcmd;
union uwoptval *uwoptval;
{
	uwtype_t wtype;

	/*
	 * 4.3BSD-style window resizing
	 */
	if (uw_gtype(uwin, &wtype) < 0)
		wtype = UWT_ADM31;	/* probably wrong to do this here */
	if (optcmd == UWOC_SET) {
		switch (optnum) {
		case UWOP_WSIZE:
			winsz.ws_ypixel = uwoptval->uwov_point.v;
			winsz.ws_xpixel = uwoptval->uwov_point.h;
			break;
		case UWOP_TSIZE:
			if (wtype <= UWT_ANSI) {
				winsz.ws_row = uwoptval->uwov_point.v;
				winsz.ws_col = uwoptval->uwov_point.h;
			}
			break;
		}
		if (wtype <= UWT_ANSI &&
		    (optnum == UWOP_WSIZE || optnum == UWOP_TSIZE))
			(void)ioctl(ptyfd, TIOCSWINSZ, &winsz);
	}
}

setresize(uwin, fd)
UWIN uwin;
int fd;
{
	struct uwpoint pt;
	uwtype_t wtype;

	/*
	 * Set up the option-handling routine "doresize".
	 */
	ptyfd = fd;
	uw_optfn(uwin, UWOP_TSIZE, doresize);
	uw_optfn(uwin, UWOP_WSIZE, doresize);
	winsz.ws_row = 24;	/* default to standard terminal size */
	winsz.ws_col = 80;
	if (uw_gwsize(uwin, &pt) == 0) {
		winsz.ws_ypixel = pt.uwp_v;
		winsz.ws_xpixel = pt.uwp_h;
	} else {
		/* make up something plausible */
		winsz.ws_ypixel = 8 * winsz.ws_row;
		winsz.ws_xpixel = 8 * winsz.ws_col;
	}

	if (uw_gtype(uwin, &wtype) == 0 && wtype <= UWT_ANSI)
		(void)uw_optcmd(uwin, UWOP_TSIZE, UWOC_DO, (union uwoptval *)0);
}

#else
#ifdef TIOCSSIZE
void
doresize(uwin, optnum, optcmd, uwoptval)
UWIN uwin;
uwopt_t optnum;
uwoptcmd_t optcmd;
union uwoptval *uwoptval;
{
	struct ttysize ts;
	uwtype_t wtype;

	/*
	 * Sun-style window resizing
	 */
	if (uw_gtype(uwin, &wtype) < 0)
		wtype = UWT_ADM31;	/* probably wrong to do this here */
	if (wtype <= UWT_ANSI && optnum == UWOP_TSIZE && optcmd == UWOC_SET) {
		ts.ts_lines = uwoptval->uwov_point.v;
		ts.ts_cols = uwoptval->uwov_point.h;
		(void)ioctl(ptyfd, TIOCSSIZE, &ts);
	}
}

setresize(uwin, fd)
UWIN uwin;
int fd;
{
	uwtype_t wtype;

	/*
	 * Set up the option-handling routine "doresize".
	 */
	ptyfd = fd;
	uw_optfn(uwin, UWOP_TSIZE, doresize);

	if (uw_gtype(uwin, &wtype) == 0 && wtype <= UWT_ANSI)
		(void)uw_optcmd(uwin, UWOP_TSIZE, UWOC_DO, (union uwoptval *)0);
}
#endif
#endif
#endif
