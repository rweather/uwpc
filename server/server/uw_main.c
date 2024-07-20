/*
 *	uw - UNIX windows program for the Macintosh (host end)
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <stdio.h>

#include "uw_param.h"
#include "uw_clk.h"
#include "uw_opt.h"
#include "uw_win.h"
#include "uw_fd.h"
#include "uw_pcl.h"
#include "uw_ipc.h"
#include "openpty.h"

int nflag;			/* no startup file */
int sflag;			/* "secure" (hee hee) -- no network requests */
int errflag;			/* argument error */
char *rcfile;			/* ".uwrc" file name */

extern void rc_kludge();	/* horrible hack (see rc_kludge()) */

main(argc, argv)
char **argv;
{
	register int c;
	register fildes_t fd;
	extern int calloptscan;
	extern int errno;
	extern int optind;
	extern char *optarg;

	/*
	 * Make sure we don't accidentally try to run this inside itself.
	 */
	if (getenv(UIPC_ENV)) {
		fprintf(stderr, "%s is already running\n", *argv);
		exit(1);
	}

	/*
	 * Process command-line arguments.
	 */
	while ((c=getopt(argc, argv, "f:ns")) != EOF) {
		switch (c) {
		case 'f':
			if (nflag) {
				fprintf(stderr,
				    "Cannot specify both \"-f\" and \"-n\"\n");
				nflag = 0;
			}
			rcfile = optarg;
			break;
		case 'n':
			if (rcfile != (char *)0) {
				fprintf(stderr,
				    "Cannot specify both \"-f\" and \"-n\"\n");
				rcfile = (char *)0;
			}
			nflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			errflag = 1;
			break;
		}
	}
	if (errflag) {
		fprintf(stderr, "Usage: \"%s [-f file] [-n] [-s]\"\n", *argv);
		exit(1);
	}

	/*
	 * Initialize the file descriptor table.
	 */
	fd_init();
	FD_SET(0, &selmask[0].sm_rd);

	/*
	 * If we can open the "/etc/utmp" for write, do so.
	 * Immediately afterwards, we lose any magic powers that
	 * might have allowed us to do this.
	 */
#ifdef UTMP
	fd = open("/etc/utmp", O_WRONLY);
	(void)setgid(getgid());
	(void)setuid(getuid());
	if (fd >= 0)
		fdmap[fd].f_type = FDT_OTHER;
	utmp_init(fd);
#endif

	/*
	 * Initialize the window structures.
	 */
	win_init();

	/*
	 * Initialize timeouts.
	 */
	clk_init();


	/*
	 * Create a UNIX-domain network address, and put its name into
	 * the environment so that descendents can contact us with new
	 * window requests.  If we want to be "secure", we don't allow
	 * any UNIX-domain messages to come in.
	 */
	ipc_init(!sflag);
	if (!sflag)
		clk_timeout(5, rc_kludge, (toarg_t)0);


	/*
	 * Ignore interrupts, quits, and terminal stops.  Clean up and exit
	 * if a hangup or termination is received.  Also catch changes in
	 * child status (so that we can wait for them).  Set up the terminal
	 * modes.
	 */
	(void)signal(SIGHUP, done);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGTERM, done);
	(void)signal(SIGTSTP, SIG_IGN);
	(void)signal(SIGCHLD, cwait);

	tty_mode(1);


	/*
	 * Tell the Macintosh to initialize.
	 */
	pcl_entry(0);

	
	/*
	 * Create window 1 (to start things off) and wait for input.
	 * When input is available, process it.
	 */
	if (!nflag)
		finduwrc();

	while (1) {
		CLK_CHECK();
		if (calloptscan && protocol->p_chkopt) {
			calloptscan = 0;
			(*protocol->p_chkopt)(0);
		}
		selmask[1] = selmask[0];
		if (select(nfds, &selmask[1].sm_rd, &selmask[1].sm_wt,
		    &selmask[1].sm_ex, (struct timeval *)0) < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			done(1);	/* for now -- fix this! */
		}
		for (fd=0; fd < nfds; fd++) {
			if (FD_ISSET(fd, &selmask[1].sm_rd)) {
				switch (fdmap[fd].f_type) {
				case FDT_MAC:
					PCL_RECV(0, (char *)0, 0);
					break;
				case FDT_UDSOCK:
					ipc_udrecv(fd);
					break;
				case FDT_ISSOCK:
					ipc_isrecv(fd);
					break;
				case FDT_DATA:
					PCL_XMIT(0, fdmap[fd].f_win);
					break;
				case FDT_CTL:
					ipc_ctlrecv(0, fd, fdmap[fd].f_win);
					break;
				default:
					/* "can't happen" */
					FD_CLR(fd, &selmask[0].sm_rd);
					break;
				}
			}
			if (FD_ISSET(fd, &selmask[1].sm_wt)) {
				/* "can't happen" */
				FD_CLR(fd, &selmask[0].sm_wt);
				break;
			}
			if (FD_ISSET(fd, &selmask[1].sm_ex)) {
				/* "can't happen" */
				FD_CLR(fd, &selmask[0].sm_ex);
				break;
			}
		}
	}
}

finduwrc()
{
	register struct passwd *pw;
	register char *homedir;

	/*
	 * If the global variable "rcfile" is non-NULL, then it specifies
	 * the name of the startup file.  Otherwise, the name of the startup
	 * file is "$HOME/.uwrc".  If $HOME is undefined or null, the password
	 * file is consulted.  The ".uwrc" file is an executable program or
	 * "/bin/sh" command file.  (For "csh" (ugh) use "#! /bin/csh".)
	 *
	 * Returns 0 if the ".uwrc" file doesn't exist, 1 if it does.  As
	 * a side-effect, this routine sets the global variable "rcfile"
	 * to the name of the ".uwrc" file.
	 */
	if (rcfile == (char *)0) {
		if ((homedir=getenv("HOME")) == NULL || !*homedir) {
			if ((pw = getpwuid(getuid())) != NULL)
				homedir = pw->pw_dir;
			else
				return;
		}
		rcfile = malloc((unsigned)(strlen(homedir) + sizeof "/.uwrc"));
		if (rcfile == (char *)0)
			return;
		(void)strcpy(rcfile, homedir);
		(void)strcat(rcfile, "/.uwrc");
	}
	if (access(rcfile, F_OK) < 0)
		rcfile = (char *)0;
}

runuwrc()
{
	register int pid;
	register fildes_t fd;
	struct ptydesc pt;

	/*
	 * We use a real fork (rather than a vfork()) because the parent
	 * doesn't wait for the child.  The caller knows that the file
	 * exists; however, it cannot determine whether or not it is
	 * successfully executed.
	 *
	 * We acquire a pseudo-terminal for rather convoluted reasons.
	 * Programs such as "uwtool" expect to be able to inherit tty
	 * modes from their controlling terminal.  By the time that we
	 * reach this point, we've already changed our controlling
	 * terminal to use cbreak mode with no special characters except
	 * XON/XOFF.  Therefore, we obtain a pseudo-terminal and
	 * restore our original modes onto it.  We double-fork (sigh,
	 * another miserable kludge) so that the server does not have
	 * to wait for the completion of the ".uwrc" file.  (The child
	 * waits for the grandchild so that the master side of the pty
	 * remains open until the grandchild is finished.)
	 */
	if (openpty(&pt) < 0)
		return;
	while ((pid = fork()) < 0)
		sleep(5);
	if (pid > 0) {
		(void)close(pt.pt_pfd);
		(void)close(pt.pt_tfd);
	} else {
		/* child */
		while ((pid = fork()) < 0)
			sleep(5);
		if (pid > 0) {
			while (wait((int *)0) < 0 && errno == EINTR)
				;
			_exit(1);
			/*NOTREACHED*/
		} else {
			/* grandchild */
			(void)setgid(getgid());
			(void)setuid(getuid());
			(void)close(pt.pt_pfd);
			if (pt.pt_tfd != 0)
				(void)dup2(pt.pt_tfd, 0);
			if (pt.pt_tfd != 1);
				(void)dup2(pt.pt_tfd, 1);
			if (pt.pt_tfd != 2)
				(void)dup2(pt.pt_tfd, 2);
			win_envinit(defwtype, (long)0);
			(void)signal(SIGHUP, SIG_DFL);
			(void)signal(SIGINT, SIG_DFL);
			(void)signal(SIGQUIT, SIG_DFL);
			(void)signal(SIGTERM, SIG_DFL);
			(void)signal(SIGTSTP, SIG_IGN);
			(void)signal(SIGCHLD, SIG_DFL);
			(void)ioctl(open("/dev/tty",O_RDWR),
			    (int)TIOCNOTTY, (char *)0);
			(void)open(pt.pt_tname, O_RDONLY);
			for (fd=3; fd < nfds; fd++)
				(void)close(fd);
			tty_mode(0);
			(void)execlp(rcfile, rcfile, (char *)0);
			(void)execl("/bin/sh", "sh", rcfile, (char *)0);
			_exit(1);
			/*NOTREACHED*/
		}
	}
}

void
rc_kludge()
{
	static int firsttime = 1;

	/*
	 * A problem which occurs with ".uwrc" file handling is that
	 * the "rc" file is interpreted immediately after the server
	 * begins, i.e. before it and the Macintosh have (possibly)
	 * changed from the default protocol to an extended one.
	 *
	 * To get around this problem, if a ".uwrc" file exists, it
	 * is not executed immediately.  Instead, it will be executed
	 * when this routine is called, either directly by pcl_newpcl()
	 * when the protocol changes, or after an initial timeout.
	 *
	 * It is most unfortunate that "pcl_newpcl" must call "upwards"
	 * into this source file.
	 */
	if (firsttime) {
		firsttime = 0;
		if (rcfile != (char *)0)
			runuwrc();
		else
			(void)PCL_NEWW(0, WC_INTERNAL, defwtype, (nwin_t)1, 0L,
			    (fildes_t)-1, (fildes_t)-1);
	}
}

void
done(s)
{
	/*
	 * Clean up and exit.  It is overkill to close all of the file
	 * descriptors, but it causes no harm.
	 */
	pcl_exit(0);
	utmp_exit();
	fd_exit();
	ipc_exit();
	tty_mode(0);
	exit(s);
}

void
cwait()
{
	register int pid;
	union wait status;
	struct rusage rusage;

	/*
	 * Collect dead children.  Restart any children that have stopped.
	 */
	while ((pid=wait3(&status, WNOHANG|WUNTRACED, &rusage)) > 0)
		if (WIFSTOPPED(status))
			(void)kill(pid, SIGCONT);
}
