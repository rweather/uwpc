/*
 *	uw window options
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 *
 * Some protocols support the transmission of window options.  A window
 * option is a parameter (or collection of related parameters) which
 * describes the layout, appearance, or other characteristic of a
 * window.  Some options are common to all window types, while others
 * are window emulation-specific.
 *
 * Window options may be "set" by one side on its own initiative or in
 * response to an "inquiry" from the other side.  In addition, one side
 * may request that the other side "report" changes in options.
 *
 * Options are passed as part of a "new window" command or as part of
 * an "option" command (as defined by the protocol, above).  The option
 * format has been chosen to minimize the need for protocol encoding
 * of special or meta characters.
 */

#ifndef	UW_OPT
#define	UW_OPT

typedef unsigned int woptcmd_t;		/* window option command: */
#define	WOC_SET		0		/*	request current option value */
#define	WOC_INQUIRE	2		/*	report current option value */
#define	WOC_DO		4		/*	do report changes to option */
#define	WOC_DONT	5		/*	don't report changes */
#define	WOC_WILL	6		/*	will report changes */
#define	WOC_WONT	7		/*	won't report changes */
#define	WOC_MASK	7		/*	mask */
#define	WOC_BADCMD(n)	((n)==1 || (n)==3)

/*
 * Option commands include an option number specifier.  If the option
 * number is in the range 1-14 a short-form specifier can be used;
 * otherwise, a long-form specifier must be used.  Option (sub)command
 * bytes consist of 7 bits of data.  The lower order 3 bits specify the
 * option command.  The next higher 4 bits specify the option number.
 * The value zero is reserved (as described below).  If the option
 * number is greater than 14, these four bits specify 017 (15) and the
 * option number is specified in a second byte.  The value is encoded
 * by adding ' ' to the option number.  Multiple options may be specified
 * in one command -- the last option is followed by a reference to
 * "option" 0 (as an endmarker).
 */

typedef unsigned int woption_t;		/* window option number: */
#define	WONUM_MIN	1		/*	minimum option number */
#define	WONUM_GENERIC	7		/*	maximum generic option number */
#define	WONUM_SHORT	14		/*	maximum short option number */
#define	WONUM_MAX	31		/*	maximum option number */
#define	WONUM_MASK	(017<<3)	/*	mask for extraction */
#define	WONUM_USELONG(n) ((unsigned)(n) > WONUM_SHORT)
#define	WONUM_SENCODE(n) (((n)&017)<<3)	/* 	short encoding function */
#define	WONUM_SDECODE(b) (((b)>>3)&017)	/* 	short decoding function */
#define	WONUM_LPREFIX	(017<<3)	/*	long encoding prefix */
#define	WONUM_LENCODE(n) ((n)+' ')	/* 	long encoding function */
#define	WONUM_LDECODE(c) (((c)&0177)-' ') /* 	long decoding function */


/*
 * The following option numbers are generic (recognized for all window
 * types):
 */
#define	WOG_END		0		/* [endmarker] */
#define	WOG_VIS		1		/* 0=invisible, 1=visible */
#define	WOG_TYPE	2		/* window emulation type (see below) */
#define	WOG_POS		3		/* window position on screen */
#define	WOG_TITLE	4		/* window title */
#define	WOG_SIZE	5		/* window size (in bits) */
#define	WOG_6		6		/* unassigned, reserved */
#define	WOG_7		7		/* unassigned, reserved */

/*
 * Option arguments immediately follow option (sub)command bytes.  They are
 * encoded to prevent interference with flow-control and IAC recognition.
 * Three types of options are recognized: non-graphic character strings of
 * fixed length, general character strings of variable length, and binary
 * numbers of fixed width.
 *
 * Non-graphic character strings are transmitted directly.  They CANNOT
 * include IAC, XON, or XOFF and should not include "meta" characters.
 *
 * General character strings are encoded in the UW protocol fashion: "meta"
 * characters and special characters are escaped.  The string is terminated
 * with a null byte.  The string may not exceed some predetermined maximum
 * number of characters (which may be less than or equal to 256, including
 * the terminating null byte).
 *
 * Binary numbers are transmitted in 6-bit chunks, least-significant bits
 * first.  The number of 6-bit chunks required depends upon the width of
 * the number.  The 0100 bit in each byte is always set to prevent
 * collisions with special characters (such as flow control and IAC).
 */

/*
 * Implementation:
 *
 * Arrays of type "woptarg_t" are used to describe the arguments associated
 * with each option.  (Note that arguments are associated only with
 * the "set" option subcommand.)
 */

typedef unsigned woptarg_t;		/* option argument type: */
#define	WOA_END		0		/*	endmarker */
#define	WOA_CHARS(n)	((1<<8)|(n))	/*	"n" untranslated characters */
#define	WOA_STRING(m)	((2<<8)|(m))	/*	string of max length "m" */
#define	WOA_UDATA(b)	((3<<8)|(b))	/*	binary number "b" bits wide */
#define	WOA_CMDMASK	0177400		/* command mask */

typedef long woptbmask_t;		/* option bitmask (>= 32 bits wide) */
#define	WOPT_SET(mask,bit)	((mask) |= (1<<(bit)))
#define	WOPT_CLR(mask,bit)	((mask) &= ~(1<<(bit)))
#define	WOPT_ISSET(mask,bit)	((mask) & (1<<(bit)))

struct woptdefn {
	woptbmask_t	wod_pending;	/* pending notifications to Mac */
	woptbmask_t	wod_inquire;	/* pending inquiries from Mac */
	woptbmask_t	wod_do;		/* pending DO commands to Mac */
	woptbmask_t	wod_dont;	/* pending DONT commands to Mac */
	woptbmask_t	wod_askrpt;	/* reports (of changes) we ask for */
	struct woptlst {
		woptarg_t	*wol_argdefn;	/* option argument definition */
		char		*(*wol_get)();	/* called to get option value */
		void		(*wol_set)();	/* called to set option value */
		void		(*wol_ext)();	/* called for external window */
	}		wod_optlst[WONUM_MAX+1];
};

/*
 * The following structure is used by routines that fetch and set option
 * values.
 */
union optvalue {
	unsigned char	ov_udata1;
	unsigned char	ov_udata2;
	unsigned char	ov_udata6;
	unsigned short	ov_udata12;
	struct {
		unsigned short	v,h;
	}		ov_point;
	char		ov_string[256];
};

/*
 * When it is necessary to convert between host byte order and network
 * byte order, opt_netadj() is called.  A pointer to the following
 * structure is passed.
 */
struct netadj {
	short		(*na_short)();
	long		(*na_long)();
	unsigned short	(*na_ushort)();
	unsigned long	(*na_ulong)();
};

#endif
