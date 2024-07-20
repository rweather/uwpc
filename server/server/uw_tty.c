/*
 *	uw_tty - terminal support for UW
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "uw_param.h"
#include "uw_win.h"
#include "uw_opt.h"

#define	XON	021	/* ASCII XON (ASR-33 paper-tape reader on) */
#define	XOFF	023	/* ASCII XOFF (ASR-33 paper-tape reader off) */

static char *envinfo[][3] = {
	{
		"TERM=adm31",
		"TERMCAP=adm31:cr=^M:do=^J:nl=^J:al=\\EE:am:le=^H:bs:ce=\\ET:cm=\\E=%+ %+ :cl=^Z:cd=\\EY:co#80:dc=\\EW:dl=\\ER:ei=\\Er:ho=^^:im=\\Eq:li#24:mi:nd=^L:up=^K:MT:km:so=\\EG1:se=\\EG0:",
		(char *)0
	},
	{
		"TERM=vt52",
		(char *)0
	},
	{
		"TERM=ansi",
		(char *)0
	},
	{
		"TERM=tek4010",
		(char *)0
	}
};

/* private (emulation-specific) data */
struct tty {
	struct {
		unsigned short	h,v;
	}		t_size;
	unsigned	t_fontsz;
	unsigned	t_clipb;
	unsigned	t_bell;
	unsigned	t_curs;
	unsigned	t_chgsz;
};

#define	WOTTY_SIZE	8		/* terminal size in (row, col) */
#define	WOTTY_FONTSZ	9		/* font size index (0=7pt, 1=9pt) */
#define	WOTTY_CLIPB	10		/* 0=clipboard, 1=encode mouse clicks */
#define	WOTTY_BELL	11		/* bell: bit 0=visible, bit 1=audible */
#define	WOTTY_CURSOR	12		/* cursor type: 0=block, 1=underscore */
#define	WOTTY_CHGSZ	13		/* change actual size (not view size) */

static woptarg_t size_xdr[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };
static woptarg_t fontsz_xdr[] = { WOA_UDATA(6), WOA_END };
static woptarg_t clipb_xdr[] = { WOA_UDATA(1), WOA_END };
static woptarg_t bell_xdr[] = { WOA_UDATA(2), WOA_END };
static woptarg_t curs_xdr[] = { WOA_UDATA(1), WOA_END };
static woptarg_t chgsz_xdr[] = { WOA_UDATA(1), WOA_END };

/* TIOCSWINSZ is in 4.3BSD, TIOCSSIZE is in Sun UNIX */
#if defined(TIOCSWINSZ) || defined(TIOCSSIZE)
#define	RPTWINSZ	(1<<WOTTY_SIZE)
#else
#define	RPTWINSZ	0
#endif

#define	TTY_WOPT	{					\
	0, 0, 0, 0,						\
	RPTWINSZ|(1<<WOTTY_FONTSZ)|(1<<WOTTY_CLIPB)|		\
	 (1<<WOTTY_BELL)|(1<<WOTTY_CURSOR)|(1<<WOTTY_CHGSZ),	\
	{							\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ NULL, NULL, NULL },				\
		{ size_xdr, tty_getopt, tty_setopt },		\
		{ fontsz_xdr, tty_getopt, tty_setopt },		\
		{ clipb_xdr, tty_getopt, tty_setopt },		\
		{ bell_xdr, tty_getopt, tty_setopt },		\
		{ curs_xdr, tty_getopt, tty_setopt },		\
		{ chgsz_xdr, tty_getopt, tty_setopt }		\
	}							\
}
extern int tty_start();
extern void tty_stop(), tty_setopt(), tty_setext();
extern char *tty_getopt();

struct emulation adm31_emul = { TTY_WOPT, tty_start, tty_stop, tty_setext };
struct emulation vt52_emul = { TTY_WOPT, tty_start, tty_stop, tty_setext };
struct emulation ansi_emul = { TTY_WOPT, tty_start, tty_stop, tty_setext };
struct emulation tek_emul = { { 0, 0, 0, 0, 0 }, tty_start, tty_stop };

tty_mode(f)
int f;
{
	static struct sgttyb ostty, nstty;
	static struct tchars otchars, ntchars;
	static struct ltchars oltchars, nltchars;
	static int olmode, nlmode;
	static int operm;
	static saved;
	static int inout = FREAD|FWRITE;
	struct stat st;

	/*
	 * This routine either saves the current terminal modes and then
	 * sets up the terminal line or resets the terminal modes (depending
	 * upon the value of "f").  The terminal line is used in "cbreak"
	 * mode with all special characters except XON/XOFF disabled.  The
	 * hated (by me) LDECCTQ mode is required for the Macintosh to
	 * handle flow control properly.
	 */
	if (f == 1) {
		if (fstat(0, &st) == 0)
			operm = st.st_mode & 06777;
		else
			operm = -1;
		if (ioctl(0, (int)TIOCGETP, (char *)&ostty) < 0) {
			perror("ioctl((int)TIOCGETP)");
			done(1);
		}
		if (ioctl(0, (int)TIOCGETC, (char *)&otchars) < 0) {
			perror("ioctl((int)TIOCGETC)");
			done(1);
		}
		if (ioctl(0, (int)TIOCGLTC, (char *)&oltchars) < 0) {
			perror("ioctl((int)TIOCGLTC)");
			done(1);
		}
		if (ioctl(0, (int)TIOCLGET, (char *)&olmode) < 0) {
			perror("ioctl((int)TIOCLGET)");
			done(1);
		}
		nstty = ostty;
		nstty.sg_erase = nstty.sg_kill = -1;
		nstty.sg_flags |= CBREAK;
		nstty.sg_flags &= ~(RAW|CRMOD|ECHO|LCASE|XTABS|ALLDELAY);
		ntchars.t_intrc = ntchars.t_quitc = -1;
		ntchars.t_eofc = ntchars.t_brkc = -1;
		ntchars.t_startc = XON;
		ntchars.t_stopc = XOFF;
		nltchars.t_suspc = nltchars.t_dsuspc = -1;
		nltchars.t_rprntc = nltchars.t_flushc = -1;
		nltchars.t_werasc = nltchars.t_lnextc = -1;
		nlmode = olmode | LDECCTQ;
		if (operm != -1 && fchmod(0, S_IREAD|S_IWRITE) < 0)
			operm = -1;
		if (ioctl(0, (int)TIOCSETN, (char *)&nstty) < 0) {
			perror("ioctl((int)TIOCSETN)");
			done(1);
		}
		if (ioctl(0, (int)TIOCSETC, (char *)&ntchars) < 0) {
			perror("ioctl((int)TIOCSETC");
			done(1);
		}
		if (ioctl(0, (int)TIOCSLTC, (char *)&nltchars) < 0) {
			perror("ioctl((int)TIOCSLTC");
			done(1);
		}
		if (ioctl(0, (int)TIOCLSET, (char *)&nlmode) < 0) {
			perror("ioctl((int)TIOCLSET)");
			done(1);
		}
		saved = 1;
	} else if (saved) {
		(void)ioctl(0, (int)TIOCFLUSH, (char *)&inout);
		(void)ioctl(0, (int)TIOCSETP, (char *)&ostty);
		(void)ioctl(0, (int)TIOCSETC, (char *)&otchars);
		(void)ioctl(0, (int)TIOCSLTC, (char *)&oltchars);
		(void)ioctl(0, (int)TIOCLSET, (char *)&olmode);
		if (operm != -1)
			(void)fchmod(0, operm);
	}
}

static
tty_start(w)
register struct window *w;
{
	register struct tty *t;
	extern char *malloc();

	/*
	 * Start up a terminal emulation.  Establish reasonable defaults
	 * for the terminal-specific window options.
	 */
	if (w->w_type != WT_TEK4010) {
		if ((w->w_private = malloc(sizeof(struct tty))) != NULL) {
			t = (struct tty *)w->w_private;
			t->t_size.h = 80;
			t->t_size.v = 24;
			t->t_fontsz = 0;
			t->t_clipb = 0;
			t->t_bell = 3;
			t->t_curs = 0;
			t->t_chgsz = 0;
			return(1);
		} else
			return(0);
	} else {
		w->w_private = (char *)0;
		return(1);
	}
}

static
void
tty_stop(w)
register struct window *w;
{
	/*
	 * Shut down (stop) a terminal emulation.
	 */
	free(w->w_private);
	w->w_private = (char *)0;
}

static
void
tty_setext(wod)
register struct woptdefn *wod;
{
	/*
	 * This routine makes adjustments to the window option definitions
	 * for external windows.  Basically, we turn off reporting for
	 * WOTTY_SIZE.  (If the external process wants to handle this, it
	 * can turn it back on.)
	 */
	WOPT_CLR(wod->wod_do, WOTTY_SIZE);
	WOPT_CLR(wod->wod_askrpt, WOTTY_SIZE);
}

tty_envinit(wtype)
register wtype_t wtype;
{
	/*
	 * Set up environment variables corresponding to the window type
	 * "wtype".
	 */
	env_set(envinfo[wtype]);
}

static
char *
tty_getopt(win, num)
caddr_t win;
woption_t num;
{
	register struct tty *t;
	static union optvalue ov;
	struct window *w;

	if ((w=(struct window *)win) != NULL && w->w_alloc &&
	    (t=(struct tty *)w->w_private) != NULL) {
		switch (num) {
		case WOTTY_SIZE:
			ov.ov_point.h = t->t_size.h;
			ov.ov_point.v = t->t_size.v;
			break;
		case WOTTY_FONTSZ:
			ov.ov_udata6 = t->t_fontsz;
			break;
		case WOTTY_CLIPB:
			ov.ov_udata1 = t->t_clipb;
			break;
		case WOTTY_BELL:
			ov.ov_udata2 = t->t_bell;
			break;
		case WOTTY_CURSOR:
			ov.ov_udata1 = t->t_curs;
			break;
		case WOTTY_CHGSZ:
			ov.ov_udata1 = t->t_chgsz;
			break;
		}
	}
	return((char *)&ov);
}

static
void
tty_setopt(win, num, value)
caddr_t win;
woption_t num;
char *value;
{
	register struct tty *t;
	register union optvalue *ovp;
	register struct window *w;

	if ((w=(struct window *)win) != NULL && w->w_alloc &&
	    (t=(struct tty *)w->w_private) != NULL &&
	    (ovp = (union optvalue *)value) != NULL) {
		switch (num) {
		case WOTTY_SIZE:
			t->t_size.h = ovp->ov_point.h;
			t->t_size.v = ovp->ov_point.v;
#ifdef TIOCSWINSZ
			if (w->w_class == WC_INTERNAL) {
				/* set window size on pty (4.3BSD) */
				struct winsize ws;
				ws.ws_row = t->t_size.v;
				ws.ws_col = t->t_size.h;
				ws.ws_xpixel = w->w_size.h;
				ws.ws_ypixel = w->w_size.v;
				(void)ioctl(w->w_datafd, (int)TIOCSWINSZ,
				    (char *)&ws);
			}
#else
#ifdef TIOCSSIZE
			if (w->w_class == WC_INTERNAL) {
				/* set window size on pty (Sun) */
				struct ttysize ts;
				ts.ts_lines = t->t_size.v;
				ts.ts_cols = t->t_size.h;
				(void)ioctl(w->w_datafd, (int)TIOCSSIZE,
				    (char *)&ts);
			}
#endif
#endif
			break;
		case WOTTY_FONTSZ:
			t->t_fontsz = ovp->ov_udata6;
			break;
		case WOTTY_CLIPB:
			t->t_clipb = ovp->ov_udata1;
			break;
		case WOTTY_BELL:
			t->t_bell = ovp->ov_udata2;
			break;
		case WOTTY_CURSOR:
			t->t_curs = ovp->ov_udata1;
			break;
		case WOTTY_CHGSZ:
			t->t_chgsz = ovp->ov_udata1;
			break;
		}
	}
}
