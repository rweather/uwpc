/*
 *	uw_win - window handling for UW
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <strings.h>
#include <stdio.h>

#include "openpty.h"
#include "uw_param.h"
#include "uw_opt.h"
#include "uw_win.h"
#include "uw_fd.h"

/*
 * "defwtype" specifies the default window type.  This type is used when
 * more specific information is not available.
 */
wtype_t defwtype = WT_ADM31;

/*
 * "window" is declared in "uw_win.h"  Here we define it.
 */
struct window window[NWINDOW];		/* window data structures */

/*
 * "emulation" describes window emulation-specific data.  "generic_emul"
 * describes emulations which do not require special server attention
 * (e.g. file transfer, all of whose real work is done by a separate process).
 */
extern struct emulation adm31_emul, vt52_emul, ansi_emul, tek_emul;
static struct emulation generic_emul;
static struct emulation *emulation[WT_MAXTYPE+1] = {
	&adm31_emul,
	&vt52_emul,
	&ansi_emul,
	&tek_emul,
	&generic_emul,
	&generic_emul,
	&generic_emul,
};

extern char *win_getopt();
extern void win_setopt();

static woptarg_t woa_vis[] = { WOA_UDATA(1), WOA_END };
static woptarg_t woa_type[] = { WOA_UDATA(6), WOA_END };
static woptarg_t woa_pos[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };
static woptarg_t woa_title[] = { WOA_STRING(255), WOA_END };
static woptarg_t woa_size[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };

static struct woptdefn genwinopt = {
	(1<<WOG_VIS), 0, 0, 0,
	(1<<WOG_VIS)|(1<<WOG_TYPE)|(1<<WOG_POS)|(1<<WOG_TITLE)|(1<<WOG_SIZE),
	{
/* WOG_END */	{ NULL, NULL, NULL },
/* WOG_VIS */	{ woa_vis, win_getopt, win_setopt },
/* WOG_TYPE */	{ woa_type, win_getopt, win_setopt },
/* WOG_POS */	{ woa_pos, win_getopt, win_setopt },
/* WOG_TITLE */	{ woa_title, win_getopt, win_setopt },
/* WOG_SIZE */	{ woa_size, win_getopt, win_setopt },
/* WOG_6 */	{ NULL, NULL, NULL },
/* WOG_7 */	{ NULL, NULL, NULL }
	}
};

/*
 * This is a violation of the level structure, but it is expedient.
 */
extern void ipc_optmsg();

win_init()
{
	register struct window *w;

	/*
	 * Initialize.  Mark all windows unallocated.
	 */
	for (w=window; w < window+NWINDOW; w++)
		w->w_alloc = 0;
}

long
win_mkid()
{
	static unsigned short i = 0;
	static long pid = -1;

	if (pid == -1)
		pid = getpid();
	return((pid << (NBBY*(sizeof(long)/sizeof(short)))) | i++);
}

struct window *
win_search(wid, maxwin)
long wid;
nwin_t maxwin;
{
	register struct window *w;

	for (w=window; w < window+maxwin; w++)
		if (w->w_alloc && w->w_id == wid)
			return(w);
	return((struct window *)0);
}

struct window *
win_neww(wclass, wtype, wnum, maxwin, wid, datafd, ctlfd, options)
wclass_t wclass;
wtype_t wtype;
nwin_t wnum;
nwin_t maxwin;
long wid;
fildes_t datafd;
fildes_t ctlfd;
struct woptdefn *options;
{
	fildes_t fd;
	int pid;
	struct window *w;
	char *tty, *shell;
	auto struct ptydesc pt;
	extern char *getenv();

	/*
	 * Create a new window.  "wclass" specifies the window wclass.
	 * If "wnum" is negative, choose a window number; otherwise,
	 * "wnum" is the window number.  "datafd" and "ctlfd" are the
	 * data and control file descriptors to be associated with
	 * this window.  If "datafd" is negative and "wclass" is
	 * WC_INTERNAL, allocate a pseudo-terminal.
	 *
	 * If "options" is non-NULL it specifies the address of an
	 * option definition structure; otherwise, a new one is constructed
	 * from the generic and emulation-specific prototype structures.
	 *
	 * If "wid" is nonzero it is a proposed window ID.  It must be
	 * unique (not in use).  If "wid" is zero, a new ID is assigned.
	 *
	 * The window type "wtype" will always be a terminal emulation
	 * if the wclass is WC_INTERNAL.
	 *
	 * Internal-class windows are visible by default, while external
	 * ones are initially invisible.
	 *
	 * Return the address of the window structure or NULL if
	 * none could be created.
	 */
	tty = (char *)0;
	if (wtype > WT_MAXTYPE)
		return((struct window *)0);
	if (wid == 0) {
		while (win_search(wid=win_mkid(), maxwin) != NULL)
			;
	} else if (win_search(wid, maxwin) != NULL)
		return((struct window *)0);
	if (datafd < 0 && wclass == WC_INTERNAL) {
		if (!openpty(&pt)) {
			datafd = pt.pt_pfd;
			tty = pt.pt_tname;
			while ((pid = fork()) < 0)
				sleep(5);
			if (!pid) {
				win_envinit(wtype, wid);
				(void)signal(SIGHUP, SIG_DFL);
				(void)signal(SIGINT, SIG_DFL);
				(void)signal(SIGQUIT, SIG_DFL);
				(void)signal(SIGTERM, SIG_DFL);
				(void)signal(SIGTSTP, SIG_IGN);
				(void)signal(SIGCHLD, SIG_DFL);
				(void)ioctl(open("/dev/tty",O_RDWR),
				    (int)TIOCNOTTY, (char *)0);
				(void)close(open(pt.pt_tname, O_RDONLY));
				(void)setuid(getuid());
				if (!(shell = getenv("SHELL")))
					shell = "/bin/sh";
				if (pt.pt_tfd != 0)
					(void)dup2(pt.pt_tfd, 0);
				if (pt.pt_tfd != 1);
					(void)dup2(pt.pt_tfd, 1);
				if (pt.pt_tfd != 2)
					(void)dup2(pt.pt_tfd, 2);
				for (fd=3; fd < nfds; fd++)
					(void)close(fd);
				tty_mode(0);	/* HACK! */
				execl(shell, shell, (char *)0);
				_exit(1);
			} else {
				utmp_add(tty);
				(void)close(pt.pt_tfd);
			}
		}
	}

	if (datafd >= 0) {
		if (wnum > 0) {
			w = WIN_PTR(wnum);
			if (w->w_alloc)
				w = (struct window *)0;
		} else {
			for (w=window; w < window+maxwin && w->w_alloc; w++)
				;
			if (w >= window+maxwin)
				w = (struct window *)0;
		}
	} else
		w = (struct window *)0;

	if (w) {
		w->w_alloc = 1;
		w->w_id = wid;
		w->w_class = wclass;
		w->w_type = wtype;
		w->w_visible = (w->w_class == WC_INTERNAL);
		w->w_position.h = w->w_position.v = 0;
		w->w_size.h = w->w_size.v = 0;
		w->w_title[0] = '\0';
		if (emulation[wtype]->we_start &&
		    !(*emulation[wtype]->we_start)(w)) {
			if (options)
				w->w_optdefn = *options;
			else
				opt_new(&w->w_optdefn, &genwinopt,
				    (struct woptdefn *)0);
		} else {
			if (options)
				w->w_optdefn = *options;
			else
				opt_new(&w->w_optdefn, &genwinopt,
				    &emulation[wtype]->we_optdefn);
		}
		w->w_datafd = datafd;
		(void)fcntl(datafd, F_SETFL, FNDELAY);
		FD_SET(datafd, &selmask[0].sm_rd);
		fdmap[datafd].f_type = FDT_DATA;
		fdmap[datafd].f_win = w;
		if (w->w_class == WC_INTERNAL) {
			if (tty)
				(void)strncpy(w->w_tty, tty, sizeof w->w_tty);
		} else {
			w->w_ctlfd = ctlfd;
			if (ctlfd >= 0) {
				(void)fcntl(ctlfd, F_SETFL, FNDELAY);
				FD_SET(ctlfd, &selmask[0].sm_rd);
				fdmap[ctlfd].f_type = FDT_CTL;
				fdmap[ctlfd].f_win = w;
				if (emulation[wtype]->we_setext)
					(*emulation[wtype]->we_setext)(&w->w_optdefn);
				opt_setext(&w->w_optdefn, ipc_optmsg);
			}
		}
	}
			
	return(w);
}

win_killw(w)
register struct window *w;
{
	/*
	 * Kill the window "w".  This is pretty simple; we just close
	 * the data and control file descriptors and mark the structure
	 * inactive.
	 */
	if (w && w->w_alloc) {
		if (w->w_datafd >= 0) {
			if (w->w_class == WC_INTERNAL)
				utmp_rm(w->w_tty);
			FD_CLR(w->w_datafd, &selmask[0].sm_rd);
			FD_CLR(w->w_datafd, &selmask[0].sm_wt);
			FD_CLR(w->w_datafd, &selmask[0].sm_ex);
			fdmap[w->w_datafd].f_type = FDT_NONE;
			(void)close(w->w_datafd);
		}
		if (w->w_class == WC_EXTERNAL && w->w_ctlfd >= 0) {
			FD_CLR(w->w_ctlfd, &selmask[0].sm_rd);
			FD_CLR(w->w_ctlfd, &selmask[0].sm_wt);
			FD_CLR(w->w_ctlfd, &selmask[0].sm_ex);
			fdmap[w->w_ctlfd].f_type = FDT_NONE;
			(void)close(w->w_ctlfd);
		}
		w->w_alloc = 0;
	}
}

win_renew(w, report)
struct window *w;
int report;
{
	/*
	 * Reinitialize (re-NEW) the window "w".  Report the state of the
	 * window to the Mac if "report" is nonzero.
	 */
	opt_renew(&w->w_optdefn, report);
}

win_newtype(w, wtype)
register struct window *w;
register wtype_t wtype;
{
	/*
	 * Change the window emulation type to "wtype".
	 */
	if (wtype <= WT_MAXTYPE && wtype != w->w_type) {
		if (emulation[w->w_type]->we_stop)
			(*emulation[w->w_type]->we_stop)(w);
		w->w_type = wtype;
		if (emulation[wtype]->we_start &&
		    !(*emulation[wtype]->we_start)(w)) {
			opt_newtype(&w->w_optdefn, &genwinopt,
			    (struct woptdefn *)0);
		} else {
			opt_newtype(&w->w_optdefn, &genwinopt,
			    &emulation[wtype]->we_optdefn);
		}
		if (w->w_class == WC_EXTERNAL && w->w_ctlfd >= 0) {
			if (emulation[wtype]->we_setext)
				(*emulation[wtype]->we_setext)(&w->w_optdefn);
			opt_setext(&w->w_optdefn, ipc_optmsg);
		}
	}
}

win_envinit(wtype, wid)
register wtype_t wtype;
long wid;
{
	register char *widstr;
	auto char *env[2];

	/*
	 * Set up the environment according to the new window type and
	 * window ID.
	 *
	 * A 64-bit integer will fit in 20 digits.  If a "long" is wider
	 * than this, then this code will have to be adjusted.
	 */
	if (wtype <= WT_TEK4010)
		tty_envinit(wtype);
	if ((widstr = malloc(sizeof "UW_ID=" + 20)) != NULL) {
		sprintf(widstr, "UW_ID=%ld", wid);
		env[0] = widstr;
		env[1] = (char *)0;
		env_set(env);
	}
}

static
char *
win_getopt(win, num)
caddr_t win;
woption_t num;
{
	register struct window *w;
	static union optvalue ov;

	/*
	 * Get the value of window option "num".  It is arguably wrong to
	 * always return the address of "ov" (even if the window isn't
	 * allocated or an unknown option type was requested); however,
	 * we're already in trouble and there is no good way to recover
	 * at this point.
	 */
	if ((w = (struct window *)win) != NULL && w->w_alloc) {
		switch (num) {
		case WOG_VIS:
			ov.ov_udata1 = w->w_visible;
			break;
		case WOG_TYPE:
			ov.ov_udata6 = w->w_type;
			break;
		case WOG_POS:
			ov.ov_point.h = w->w_position.h;
			ov.ov_point.v = w->w_position.v;
			break;
		case WOG_TITLE:
			(void)strncpy(ov.ov_string, w->w_title,
			    sizeof ov.ov_string);
			ov.ov_string[sizeof ov.ov_string-1] = '\0';
			break;
		case WOG_SIZE:
			ov.ov_point.h = w->w_size.h;
			ov.ov_point.v = w->w_size.v;
			break;
		}
	}
	return((char *)&ov);
}

static
void
win_setopt(win, num, value)
caddr_t win;
woption_t num;
char *value;
{
	register struct window *w;
	register union optvalue *ov;

	/*
	 * Set window option "num" to "value"
	 */
	if ((w = (struct window *)win) != NULL && w->w_alloc &&
	    (ov = (union optvalue *)value) != NULL) {
		switch (num) {
		case WOG_VIS:
			w->w_visible = ov->ov_udata1;
			break;
		case WOG_TYPE:
			win_newtype(w, (wtype_t)ov->ov_udata6);
			break;
		case WOG_POS:
			w->w_position.h = ov->ov_point.h;
			w->w_position.v = ov->ov_point.v;
			break;
		case WOG_TITLE:
			(void)strncpy(w->w_title, ov->ov_string,
			    sizeof w->w_title);
			w->w_title[sizeof w->w_title-1] = '\0';
			break;
		case WOG_SIZE:
			w->w_size.h = ov->ov_point.h;
			w->w_size.v = ov->ov_point.v;
			break;
		}
	}
}
