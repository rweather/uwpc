/*
 *	uw window data definition
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_WIN
#define	UW_WIN

#include "uw_opt.h"

/*
 * A "point" is a pair of 16-bit integers.  This may specify the horizontal
 * and vertical position or size of a window.
 */
typedef short npixel_t;			/* number of pixels */
struct point {
	npixel_t	v,h;
};

/*
 * The type of a window determines how it responds to I/O and which
 * window options it supports.  I'd like to declare these with an "enum",
 * but the stupid PCC screams if I use enums as array indices, so they
 * are defined via #define's instead.
 */
typedef unsigned int wtype_t;		/* window type: */
#define	WT_ADM31	0		/*	ADM-31 terminal emulation */
#define	WT_VT52		1		/*	VT52 terminal emulation */
#define	WT_ANSI		2		/*	ANSI terminal emulation */
#define	WT_TEK4010	3		/*	Tek4010 terminal emulation */
#define	WT_FTP		4		/*	file transfer */
#define	WT_PRINT	5		/*	output to printer */
#define	WT_PLOT		6		/*	plot window */
#define	WT_MAXTYPE	6		/* maximum window type */

extern wtype_t defwtype;		/* default window type */

/*
 * There are two basic classes of windows -- those which are processed
 * directly by the server and those which are processed by outside
 * programs.  Directly-handled windows are always terminal emulations.
 * Externally-handled windows may be any window type.
 */
typedef enum {				/* window class: */
	WC_INTERNAL,			/*	processed directly */
	WC_EXTERNAL,			/*	processed externally */
} wclass_t;

struct window {
	int		w_alloc;	/* window allocated if nonzero */
	long		w_id;		/* window unique ID */
	wtype_t		w_type;		/* window emulation type */
	wclass_t	w_class;	/* window class */
	fildes_t	w_datafd;	/* data file descriptor */
	union {
		struct winint {
			char wi_tty[32];	/* terminal name */
		}	wu_int;
		struct winext {
			fildes_t we_ctlfd;	/* control file descriptor */
		}	wu_ext;
	}		w_un;
	struct woptdefn	w_optdefn;	/* window option definitions */
	int		w_visible;	/* nonzero if window is visible */
	struct point	w_position;	/* position of window on screen */
	struct point	w_size;		/* size of window in pixels */
	char		w_title[256];	/* window title */
	char		*w_private;	/* storage private to emulation type */
};
#define	w_tty		w_un.wu_int.wi_tty
#define	w_ctlfd		w_un.wu_ext.we_ctlfd

typedef int nwin_t;			/* window index data type */

/*
 * Some operations upon windows depend upon the window type.  For each
 * emulation type there is a "emulation" structure which specifies
 * emulation-specific data.
 */
struct emulation {
	struct woptdefn	we_optdefn;	/* window option definitions */
	int		(*we_start)();	/* emulation setup code */
	void		(*we_stop)();	/* emulation shutdown code */
	void		(*we_setext)();	/* make changes req'd for extern win */
};

extern struct window *win_neww();	/* create new window */
extern struct window *win_search();	/* convert window ID to window ptr */

/*
 * The following macros convert between a window number and a pointer to
 * the corresponding window structure (and vice versa).
 *
 * NWINDOW *must* be >= P1_NWINDOW and >= P2_NWINDOW (in "uw_pcl.h").
 */
#define	NWINDOW		7		/* maximum number of windows */
#define	WIN_NUM(wptr)	((wptr)-window+1)
#define	WIN_PTR(wnum)	(window+(wnum)-1)
extern struct window window[];		/* window data structures */
#endif
