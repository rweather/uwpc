/*
 *	uw protocol
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_PCL
#define	UW_PCL

#include "uw_win.h"

/* UW may operate over connections which speak one of several protocols.
 * Internally these protocols are assigned numbers starting at zero.
 * Three such protocols are currently defined:
 *
 *	0: no special protocol
 *	1: original UW (v1.6, v2.10) protocol
 *	2: extended protocol (v3.x)
 */

/*
 * Protocol 0:
 *
 * The connection between the Macintosh and the host is simply a serial
 * line.  Flow control may be enabled, but no special commands are
 * recognized.  Only one active window is supported.  This "protocol"
 * does not require the UW server; hence, there is no need to support it.
 */

/*
 * Protocol 1: (original UW protocol)
 *
 * Two types of information are exchanged through the 7-bit serial line:
 * ordinary data and command bytes.  Command bytes are preceeded by
 * an IAC byte.  IAC bytes and literal XON/XOFF characters (those which
 * are not used for flow control) are sent by a P1_FN_CTLCH command.
 * Characters with the eighth bit set (the "meta" bit) are prefixed with
 * a P1_FN_META function.
 *
 * The next most-significant bit in the byte specifies the sender and
 * recipient of the command.  If this bit is clear (0), the command byte
 * was sent from the host computer to the Macintosh; if it is set (1)
 * the command byte was sent from the Macintosh to the host computer.
 * This prevents confusion in the event that the host computer
 * (incorrectly) echos a command back to the Macintosh.
 *
 * The remaining six bits are partitioned into two fields.  The low-order
 * three bits specify a window number from 1-7 (window 0 is reserved for
 * other uses) or another type of command-dependent parameter.  The next
 * three bits specify the operation to be performed by the recipient of
 * the command byte.
 *
 * Note that the choice of command bytes prevents the ASCII XON (021) and
 * XOFF (023) characters from being sent as commands.  P1_FN_ISELW commands
 * are only sent by the Macintosh (and thus are tagged with the P1_DIR_MTOH
 * bit).  Since XON and XOFF data characters are handled via P1_FN_CTLCH,
 * this allows them to be used for flow control purposes.
 */
#define	P1_IAC		0001		/* interpret as command */
#define	P1_DIR		0100		/* command direction: */
#define	P1_DIR_HTOM	0000		/*	from host to Mac */
#define	P1_DIR_MTOH	0100		/*	from Mac to host */
#define	P1_FN		0070		/* function code: */
#define	P1_FN_NEWW	0000		/*	new window */
#define	P1_FN_KILLW	0010		/*	kill (delete) window */
#define	P1_FN_ISELW	0020		/*	select window for input */
#define	P1_FN_OSELW	0030		/*	select window for output */
#define	P1_FN_META	0050		/*	add meta to next data char */
#define	P1_FN_CTLCH	0060		/*	low 3 bits specify char */
#define	P1_FN_MAINT	0070		/*	maintenance functions */
#define	P1_WINDOW	0007		/* window number mask */
#define	P1_CC		0007		/* control character specifier: */
#define	P1_CC_IAC	1		/*	IAC */
#define	P1_CC_XON	2		/*	XON */
#define	P1_CC_XOFF	3		/*	XOFF */
#define	P1_MF		0007		/* maintenance functions: */
#define	P1_MF_ENTRY	0		/*	beginning execution */
#define	P1_MF_ASKPCL	2		/*	request protocol negotiation */
#define	P1_MF_CANPCL	3		/*	suggest protocol */
#define	P1_MF_SETPCL	4		/*	set current protocol */
#define	P1_MF_EXIT	7		/*	execution terminating */
#define	P1_NWINDOW	7		/* maximum number of windows */

/*
 * Protocol 2: (extended UW protocol)
 *
 * Protocol 2 is an extension of protocol 1.  The P2_FN_NEWW command and
 * the new command P2_FN_WOPT communicate window options between the host
 * and the Macintosh.  (See "uw_opt.h" for details.)
 */
#define	P2_IAC		P1_IAC		/* interpret as command */
#define	P2_DIR		P1_DIR		/* command direction: */
#define	P2_DIR_HTOM	P1_DIR_HTOM	/*	from host to Mac */
#define	P2_DIR_MTOH	P1_DIR_MTOH	/*	from Mac to host */
#define	P2_FN		P1_FN		/* function code: */
#define	P2_FN_NEWW	P1_FN_NEWW	/*	new window */
#define	P2_FN_KILLW	P1_FN_KILLW	/*	kill (delete) window */
#define	P2_FN_ISELW	P1_FN_ISELW	/*	select window for input */
#define	P2_FN_OSELW	P1_FN_OSELW	/*	select window for output */
#define	P2_FN_WOPT	0040		/*	communicate window options */
#define	P2_FN_META	P1_FN_META	/*	add meta to next data char */
#define	P2_FN_CTLCH	P1_FN_CTLCH	/*	low 3 bits specify char */
#define	P2_FN_MAINT	P1_FN_MAINT	/*	maintenance functions */
#define	P2_WINDOW	P1_WINDOW	/* window number mask */
#define	P2_CC		P1_CC		/* control character specifier: */
#define	P2_CC_IAC	P1_CC_IAC	/*	IAC */
#define	P2_CC_XON	P1_CC_XON	/*	XON */
#define	P2_CC_XOFF	P1_CC_XOFF	/*	XOFF */
#define	P2_MF		P1_MF		/* maintenance functions: */
#define	P2_MF_ENTRY	P1_MF_ENTRY	/*	beginning execution */
#define	P2_MF_ASKPCL	P1_MF_ASKPCL	/*	request protocol negotiation */
#define	P2_MF_CANPCL	P1_MF_CANPCL	/*	suggest protocol */
#define	P2_MF_SETPCL	P1_MF_SETPCL	/*	set current protocol */
#define	P2_MF_EXIT	P1_MF_EXIT	/*	execution terminating */
#define	P2_NWINDOW	P1_NWINDOW	/* maximum number of windows */

/*
 * Protocol negotiation
 *
 * The server is not used for protocol 0.  For the other protocols, the
 * Macintosh and the server negotiate to select the active protocol.  The
 * basic idea is that the Macintosh will express its desire for a protocol
 * and the server will attempt to satisfy that desire.  Until negotiations
 * are complete, protocol 1 is used.
 *
 * Protocols are identified by single-character names which are formed by
 * adding the ASCII code for a space (040) to the protocol number minus 1
 * (i.e. protocol 1 is ' ', protocol 2 is '!').
 *
 * P1_FN_CANPCL and P1_FN_SETPCL are three-byte commands: P1_IAC,
 * P1_FN_XXXPCL, protocol-name.
 *
 * Macintosh:
 *	If UW v2.10 is used on the Macintosh or if a newer Macintosh program
 *	wishes to use protocol 1, it will never initiate protocol negotiation.
 *	Hence, all interaction will use protocol 1 by default.
 *
 *	If the Macintosh program is capable of supporting protocol 2 and the
 *	user requests its use, the Mac will remember this fact but will
 *	continue to use protocol 1.  The Mac program will assume that no
 *	server is present until instructed otherwise by the user or until a
 *	P1_FN_ENTRY command is received (e.g. when the server starts up).
 *	At this time, the Mac program issues P1_FN_ASKPCL.  If the server
 *	cannot support protocol 2 (i.e. it is an old server), then it will
 *	ignore the P1_FN_ASKPCL.  The Macintosh will retry the P1_FN_ASKPCL
 *	a couple of times (about five seconds apart) and, if there is no
 *	response from the server, will abandon negotiations.  Protocol 1
 *	will be used for the remainder of the session.
 *
 *	If the server recognizes the P1_FN_ASKPCL command it will respond
 *	with the name of the most complex protocol it can support (currently
 *	'!').  If this is acceptable to the Macintosh, it will instruct the
 *	server to use this protocol.  If the Macintosh cannot support this
 *	protocol it will respond with a P1_FN_CANPCL suggesting a less-complex
 *	protocol.  If the server agrees to this it will answer establish the
 *	protocol; otherwise, it will suggest an even weaker protocol.
 *	Eventually someone will suggest protocol 1 (which is universal) and
 *	the other side will issue a P1_FN_SETPCL command to establish its use.
 *
 * Host:
 *	If the host receives a P1_FN_ASKPCL it will respond with the most
 *	complex protocol it can support (using the P1_FN_CANPCL command).
 *	Negotiations will proceed as described above until one side
 *	establishes a new protocol with P1_FN_SETPCL.  At this time, the
 *	host will switch to the new protocol.
 *
 *	If the host receives a P1_FN_ENTRY (P2_FN_ENTRY) command, it will
 *	switch back to protocol 1.  Receipt of this command indicates that
 *	the Macintosh program was restarted.  The Macintosh must initiate
 *	protocol negotiations again.
 */

/*
 * Although many of the functions are identical (and the code is shared
 * between them), each protocol is accessed through a (struct protocol)
 * which specifies the functions for various operations.
 *
 * In theory, the main program knows nothing about the protocol in use.
 * In practice, the externally-visible functions are accessed as macros
 * for greater efficiency.
 *
 * The protocol layer is aware of the (struct window) data structures.
 */

struct protocol {
	char		p_name;		/* single-character protocol name */
	nwin_t		p_maxwin;	/* maximum window number */
	int		*p_ctlch;	/* control character map table */
	unsigned	p_szctlch;	/* size (# of entries) in ctlch table */
	void		(*p_entry)();	/* start up (ENTRY maintenance fn) */
	void		(*p_exit)();	/* shut down (EXIT maintenance fn) */
	void		(*p_renew)();	/* renew (re-init) */
	struct window	*(*p_neww)();	/* create new window */
	void		(*p_killw)();	/* kill window */
	void		(*p_xmit)();	/* transmit to specified window */
	void		(*p_recv)();	/* receive from Macintosh */
	void		(*p_chkopt)();	/* check for pending option output */
	void		(*p_sendopt)();	/* send option string to Macintosh */
	void		(*p_askpcl)();	/* send an ASKPCL maintenance command */
	void		(*p_canpcl)();	/* send a CANPCL maintenance command */
	void		(*p_setpcl)();	/* send a SETPCL maintenance command */
};

extern struct protocol *protocol;

#define	PCL_NEWW(mfd,class,wtype,wnum,wid,dfd,cfd) \
	(*protocol->p_neww)(mfd,class,wtype,wnum,(long)wid,dfd,cfd)
#define	PCL_KILLW(mfd,w)	(*protocol->p_killw)(mfd,w)
#define	PCL_RECV(mfd,buf,len)	(*protocol->p_recv)(mfd,buf,len)
#define	PCL_XMIT(mfd,w)	(*protocol->p_xmit)(mfd,w)
#define	PCL_SENDOPT(mfd,fn,buf,len) \
	(protocol->p_sendopt ? (*protocol->p_sendopt)(mfd,fn,buf,len) : 0)
#endif
