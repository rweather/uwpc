/*
 *	uw IPC definitions
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_IPC
#define	UW_IPC

/*
 * UW accepts network connections in both the UNIX domain and the
 * Internet domain.  UNIX domain datagrams are used by processes on
 * the local machine to create new windows and to change the value
 * of window parameters (window options).  TCP (Internet stream)
 * connections are used by local and non-local processes which wish
 * to handle their own host activity (e.g. pseudo-terminal handling).
 *
 * Some of the definitions in this file duplicate definitions in the
 * UW server source code, because this file is also intended for use
 * with the UW library.
 *
 * The code which performs byte-order conversions knows the size of the
 * types defined in this file (since there is no typeof() operator).
 */

#define	UIPC_ENV	"UW_UIPC"	/* Unix-domain port environment var */
#define	INET_ENV	"UW_INET"	/* Internet-domain port environ var */

typedef long uwid_t;			/* unique window identifier */

typedef short uwcmd_t;			/* commands: */
#define	UWC_NEWW	0		/*	create new window */
#define	UWC_NEWT	1		/*	create new tty window */
#define	UWC_STATUS	2		/*	creation status message */
#define	UWC_KILLW	3		/*	kill existing window */
#define	UWC_OPTION	4		/*	act upon window option */

typedef short uwoptcmd_t;		/* option subcommands: */
#define	UWOC_SET	0		/*	set value of option */
#define	UWOC_ASK	2		/*	ask for value of option */
#define	UWOC_DO		4		/*	report changes in value */
#define	UWOC_DONT	5		/*	don't report changes */
#define	UWOC_WILL	6		/*	will report changes */
#define	UWOC_WONT	7		/*	won't report changes */

typedef short uwtype_t;			/* window type (see also uw_win.h): */
#define	UWT_ADM31	0		/*	ADM-31 */
#define	UWT_VT52	1		/*	VT-52 */
#define	UWT_ANSI	2		/*	ANSI */
#define	UWT_TEK4010	3		/*	Tektronix 4010 */
#define	UWT_FTP		4		/*	file transfer */
#define	UWT_PRINT	5		/*	output to Macintosh printer */
#define	UWT_PLOT	6		/*	plot window */

typedef short uwopt_t;			/* window option number: */
#define	UWOP_VIS	1		/*	visibility */
#define	UWOP_TYPE	2		/*	window type */
#define	UWOP_POS	3		/*	window position */
#define	UWOP_TITLE	4		/*	window title */
#define	UWOP_WSIZE	5		/*	window size (in bits) */

#define	UWOP_TSIZE	8		/*	terminal size (row,col) */
#define	UWOP_TFONTSZ	9		/*	small/large font size */
#define	UWOP_TCLIPB	10		/*	clipboard/mouse encoding */
#define	UWOP_TBELL	11		/*	audible, visual bell */
#define	UWOP_TCURS	12		/*	cursor shape */

union uwoptval {
	unsigned char	uwov_1bit;
	unsigned char	uwov_6bit;
	unsigned short	uwov_12bit;
	struct {
		unsigned short v,h;
	}		uwov_point;
	char		uwov_string[256];
};


/*
 * UWC_NEWW: create a new window
 *
 * This command is only valid when it is sent as the first message on an
 * Internet stream socket.  The remote port is the data fd for the window.
 * If a control fd is desired, its port number is contained in "uwnt_ctlport"
 */
struct uwneww {
	uwid_t		uwnw_id;	/* unique window identifier */
	uwtype_t	uwnw_type;	/* window type */
	short		uwnw_ctlport;	/* port number of control fd */
};

/*
 * UWC_NEWT: create a new tty window
 *
 * This command is only valid when it is sent as a datagram to the Unix-domain
 * socket.  It must be accompanied by an access right (file descriptor) for
 * the master side of a pty.  The server takes over all responsibilities for
 * this window.  "uwnt_pty" is variable-length.
 */
struct uwnewt {
	uwid_t		uwnt_id;	/* unique window identifier */
	uwtype_t	uwnt_type;	/* window type */
	char		uwnt_pty[1];	/* name of associated pty */
};

/*
 * UWC_STATUS: status report for UWC_NEWW
 *
 * This type of packet is sent by the server to the data fd in response
 * to a UWC_NEWW.  It specifies whether the window was successfully
 * created and what unique ID was assigned.
 */
struct uwstatus {
	uwid_t		uwst_id;	/* unique window identifier */
	short		uwst_err;	/* error status */
	short		uwst_errno;	/* UNIX error code (see <errno.h>) */
};

/*
 * UWC_KILLW: kill the window
 *
 * This command may be sent to either the Unix-domain socket or the control
 * file descriptor of an external window.  In the latter case, "uwkw_id"
 * must match the ID of the window associated with the file descriptor.
 */
struct uwkillw {
	uwid_t		uwkw_id;	/* unique window identifier */
};

/*
 * UWC_OPTION: act upon window option
 *
 * This command may be sent to either the Unix-domain socket or the control
 * file descriptor of an external window.  In the former case, only the
 * UWOC_SET command is processed.
 */
struct uwoption {
	uwid_t		uwop_id;	/* unique window identifier */
	uwopt_t		uwop_opt;	/* option number */
	uwoptcmd_t	uwop_cmd;	/* option subcommand */
	union uwoptval	uwop_val;	/* option value (for UWOC_SET) */
};

struct uwipc {
	unsigned short	uwip_len;	/* length of this message */
	uwcmd_t		uwip_cmd;	/* command (message type) */
	union {
		struct uwneww uwipu_neww;
		struct uwnewt uwipu_newt;
		struct uwstatus uwipu_status;
		struct uwkillw uwipu_killw;
		struct uwoption uwipu_option;
	}		uwip_u;
#define	uwip_neww	uwip_u.uwipu_neww
#define	uwip_newt	uwip_u.uwipu_newt
#define	uwip_status	uwip_u.uwipu_status
#define	uwip_killw	uwip_u.uwipu_killw
#define	uwip_option	uwip_u.uwipu_option
};

#endif
