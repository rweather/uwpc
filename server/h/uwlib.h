/*
 *	uw library definitions
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include "uw_err.h"
#include "uw_ipc.h"

#ifndef NBBY
#define	NBBY		8		/* defined in <sys/types.h> in 4.3BSD */
#endif

#define	UW_NUMOPTS	32		/* number of window options */
#define	UW_NWTYPES	6		/* number of window emulation types */

typedef char uwtitle_t[256];

struct uwpoint {
	unsigned	uwp_v;		/* vertical component */
	unsigned	uwp_h;		/* horizontal component */
};

struct uw_info {
	uwid_t		uwi_id;		/* unique window ID */
	int		uwi_datafd;	/* file descriptor for data */
	int		uwi_ctlfd;	/* file descriptor for control */
	uwerr_t		uwi_uwerr;	/* last error from UW */
	int		uwi_errno;	/* last error from system call */
	int		uwi_vis;	/* visiblility */
	uwtype_t	uwi_type;	/* window type */
	struct uwpoint	uwi_pos;	/* window position (in pixels) */
	uwtitle_t	uwi_title;	/* window title */
	struct uwpoint	uwi_wsize;	/* window size (in pixels) */
	struct {
		void	(*uwi_optfn)();	/* option handler */
	}		uwi_options[UW_NUMOPTS];
	int		uwi_ipclen;	/* length of data in IPC buffer */
	struct uwipc	uwi_ipcbuf;	/* buffer for IPC messages */
};

#define	UW_DATAFD(uwin)		(uwin)->uwi_datafd
#define	UW_ID(uwin)		(uwin)->uwi_id
#define	UW_PERROR(uwin, mesg)	\
	uw_perror(mesg, (uwin)->uwi_uwerr, (uwin)->uwi_errno)

typedef struct uw_info *UWIN;
typedef void (*uwfnptr_t)();

extern uwid_t uw_cmd();
extern UWIN uw_new();
extern uw_close(), uw_detach();
extern uw_optcmd();
extern uw_kill();
extern uwfnptr_t uw_optfn();
extern uw_rsetopt();
extern void uw_perror();
extern uwid_t uw_fork(), uw_cmd(), uw_shell();

extern uw_gvis(), uw_svis();
extern uw_gtype(), uw_stype();
extern uw_gtitle(), uw_stitle();
extern uw_gwsize(), uw_swsize();
extern uw_gpos(), uw_spos();

extern uwerr_t uwerrno;
extern char *uwerrlist[];
extern unsigned uwnerr;
