/*
 *	uw_pcl - protocol handling for UW
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "uw_param.h"
#include "uw_clk.h"
#include "uw_opt.h"
#include "uw_win.h"
#include "uw_pcl.h"
#include "openpty.h"

#define	XON	0021			/* ASCII XON */
#define	XOFF	0023			/* ASCII XOFF */
#define	RUB	0177			/* ASCII RUBout */
#define	META	0200			/* "meta" bit for whatever it's worth */

/*
 * Protocol negotiation is performed by a finite state machine (implemented
 * in pcl_haggle()).  The states are defined as the enumerated type
 * "pnstate_t".  The inputs have type "pnreq_t".
 */
typedef enum {
	PNST_NOP,			/* no protocol negotiation */
	PNST_AWAIT,			/* timing out an ASKPCL */
	PNST_CWAIT,			/* timing out a CANPCL */
	PNST_OK,			/* negotiations completed */
	PNST_FAIL			/* negotiation failed */
} pnstate_t;

typedef unsigned short pnreq_t;		/* finite state machine requests */
#define	PNRQ_PCL	0000377		/* protocol mask */
#define	PNRQ_CMD	0177400		/* command mask: */
#define	PNRQ_NONE	(pnreq_t)(0<<8)	/*	no request */
#define	PNRQ_START	(pnreq_t)(1<<8)	/*	start negotiation */
#define	PNRQ_AWAIT	(pnreq_t)(2<<8)	/*	timeout waiting for ASKPCL */
#define	PNRQ_ASK	(pnreq_t)(3<<8)	/*	process received ASKPCL */
#define	PNRQ_CAN	(pnreq_t)(4<<8)	/*	process received CANPCL */
#define	PNRQ_SET	(pnreq_t)(5<<8)	/*	process received SETPCL */
#define	PNRQ_CWAIT	(pnreq_t)(6<<8)	/*	timeout waiting for CANPCL */
#define	PNRQ_INIT	(pnreq_t)(7<<8)	/*	initialize everything */

static int p1_ctlch[] = { -1, P1_IAC, XON, XOFF, -1, -1, -1, -1 };

extern void p1_entry(), p1_renew(), p2_renew();
extern struct window *p1_neww(), *p2_neww();
extern void p1_killw(), p1_xmit(), p1_recv();
extern void p1_askpcl(), p1_canpcl(), p1_setpcl();
extern void p2_recv(), p2_chkopt(), p2_sendopt();

static struct protocol pcl_table[] = {
	{
	  ' ',
	  P1_NWINDOW,
	  p1_ctlch, sizeof p1_ctlch / sizeof p1_ctlch[0],
	  p1_entry, NULL, p1_renew,
	  p1_neww, p1_killw,
	  p1_xmit, p1_recv, NULL, NULL,
	  p1_askpcl, p1_canpcl, p1_setpcl
	},
	{
	  '!',
	  P2_NWINDOW,
	  p1_ctlch, sizeof p1_ctlch / sizeof p1_ctlch[0],
	  p1_entry, NULL, p2_renew,
	  p2_neww, p1_killw,
	  p1_xmit, p2_recv, p2_chkopt, p2_sendopt,
	  p1_askpcl, p1_canpcl, p1_setpcl
	},
};

struct protocol *protocol = pcl_table;

/*
 * Two "current" windows are defined: the current input window (for
 * input from the Macintosh) and the current output window (for output
 * to the Macintosh).
 */
static struct {
	struct window	*in;
	struct window	*out;
} curwin;


pcl_entry(mfd)
register fildes_t mfd;
{
	/*
	 * This routine is called to start up protocol handling.  We always
	 * start with protocol 1 (the original UW protocol).
	 */
	protocol = pcl_table;
	pcl_haggle(mfd, PNRQ_INIT);
	if (protocol->p_entry)
		(*protocol->p_entry)(mfd);
}

pcl_exit(mfd)
register fildes_t mfd;
{
	/*
	 * This routine is called when we shut down (just before the server
	 * exits).
	 */
	if (protocol->p_exit)
		(*protocol->p_exit)(mfd);
	protocol = pcl_table;
}

static
pcl_newpcl(newproto)
struct protocol *newproto;
{
	extern void rc_kludge();

	/*
	 * Switch to new protocol "newproto".  Right now we can get away
	 * with just changing the value of "protocol".  Eventually we will
	 * probably want to call protocol-dependent functions to shut down
	 * the old protocol and start up the new one.
	 */
	protocol = newproto;

	/*
	 * This is a horrible kludge.  See rc_kludge() in "main.c" for
	 * further details.
	 */
	rc_kludge();
}

static
void
pcl_tohaggle(arg)
register toarg_t arg;
{
	/*
	 * This is a kludge to get around the single-argument restriction
	 * on clk_timeout.  We split the argument "arg" into two 16-bit
	 * pieces and invoke pcl_haggle.
	 */
	pcl_haggle((fildes_t)((arg>>16)&0177777), (pnreq_t)(arg&0177777));
}

static
pcl_haggle(mfd, req)
fildes_t mfd;
register pnreq_t req;
{
	register struct protocol *p, *q;
	register char pname;
	register int request;
	static pnstate_t pnstate;
	static int waitcnt;

	/*
	 * This routine implements the finite-state machine that handles
	 * protocol negotiation.  This routine is called by routines which
	 * recognize incoming protocol commands and at 5 second intervals
	 * when negotiations are in progress.  The current protocol is
	 * described by the variable "protocol".
	 */
	if (req == PNRQ_INIT) {
		waitcnt = 0;
		pnstate = PNST_NOP;
		req = PNRQ_NONE;
	}
	if (!(p = protocol) || !p->p_askpcl || !p->p_canpcl || !p->p_setpcl) {
		req = PNRQ_NONE;
	} else {
		pname = req & PNRQ_PCL;
		request = req & PNRQ_CMD;
		switch (request) {
		case PNRQ_START:	/* start protocol negotiation */
			/*
			 * The Macintosh is responsible for starting protocol
			 * negotiation (if it wants something other than the
			 * standard protocol).  This code is present for
			 * purposes of exposition only.
			 */
			(*p->p_askpcl)(mfd);
			req = PNRQ_AWAIT | pname;
			waitcnt = 0;
			pnstate = PNST_AWAIT;
			break;
		case PNRQ_AWAIT:	/* timeout an ASKPCL */
			/*
			 * This state also is not reached on the host.
			 */
			if (pnstate == PNST_AWAIT) {
				if (++waitcnt > 3) {
					pnstate = PNST_FAIL;
					req = PNRQ_NONE;
				} else
					(*p->p_askpcl)(mfd);
			} else
				req = PNRQ_NONE;
			break;
		case PNRQ_ASK:		/* handle received ASKPCL */
			q = pcl_table+sizeof pcl_table/sizeof pcl_table[0] - 1;
			(*p->p_canpcl)(mfd, q->p_name);
			pnstate = PNST_CWAIT;
			req = PNRQ_CWAIT | q->p_name;
			waitcnt = 0;
			break;
		case PNRQ_CAN:		/* handle received CANPCL */
			for (q=pcl_table+sizeof pcl_table/sizeof pcl_table[0]-1;
			     q > pcl_table && q->p_name > pname;
			     q--)
				;
			if (q->p_name == pname || q == pcl_table) {
				(*p->p_setpcl)(mfd, q->p_name);
				pcl_newpcl(q);
				pnstate = PNST_OK;
				req = PNRQ_NONE;
			} else {
				(*p->p_canpcl)(mfd, q->p_name);
				pnstate = PNST_CWAIT;
				req = PNRQ_CWAIT | q->p_name;
				waitcnt = 0;
			}
			break;
		case PNRQ_CWAIT:	/* timeout a CANPCL */
			if (pnstate == PNST_CWAIT) {
				if (++waitcnt > 3) {
					pnstate = PNST_FAIL;
					req = PNRQ_NONE;
				} else
					(*p->p_canpcl)(mfd, pname);
			} else
				req = PNRQ_NONE;
			break;
		case PNRQ_SET:		/* handle a received SETPCL */
			for (q=pcl_table+sizeof pcl_table/sizeof pcl_table[0]-1;
			     q > pcl_table && q->p_name != pname;
			     q--)
				;
			if (q->p_name == pname) {
				pcl_newpcl(q);
				pnstate = PNST_OK;
				req = PNRQ_NONE;
			} else {
				/*
				 * We are in trouble now -- the Mac has
				 * instructed us to switch to a protocol
				 * that we can't support.  We switch back
				 * to protocol 1 and hope that our message
				 * to the Mac (telling it to switch to
				 * protocol 1) will be interpreted correctly.
				 */
				pnstate = PNST_FAIL;
				req = PNRQ_NONE;
				(*p->p_setpcl)(mfd, ' ');
				if (p != pcl_table)
					pcl_newpcl(pcl_table);
			}
			break;
		}
		if (req != PNRQ_NONE)
			(void)clk_timeout(5*CLK_HZ,
			    pcl_tohaggle, (toarg_t)(((long)mfd<<16)|req));
	}
}

static
void
p1_entry(mfd)
fildes_t mfd;
{
	static char cmdbuf[2] = { P1_IAC };

	cmdbuf[1] = P1_DIR_HTOM|P1_FN_MAINT|P1_MF_ENTRY;
	(void)write(mfd, cmdbuf, sizeof cmdbuf);
}

static
struct window *
p1_neww(mfd, wclass, wtype, wnum, wid, datafd, ctlfd)
fildes_t mfd;
wclass_t wclass;
wtype_t wtype;
nwin_t wnum;
long wid;
fildes_t datafd;
fildes_t ctlfd;
{
	register struct window *w;
	static char cmdbuf[2] = { P1_IAC, 0 };

	/*
	 * Create a new window for the host.  This routine is not called when
	 * the Macintosh creates a window.
	 */
	w = win_neww(wclass, wtype, wnum, protocol->p_maxwin, wid,
	    datafd, ctlfd, (struct woptdefn *)0);
	if (w) {
		cmdbuf[1] = P1_DIR_HTOM|P1_FN_NEWW|WIN_NUM(w);
		(void)write(mfd, cmdbuf, sizeof cmdbuf);
	}
	return(w);
}

static
void
p1_killw(mfd, w)
register struct window *w;
{
	static char cmdbuf[] = { P1_IAC, P1_DIR_HTOM|P1_FN_KILLW };

	/*
	 * Kill window "w" and tell the Macintosh to do the same.
	 */
	if (w && w->w_alloc) {
		if (curwin.in == w)
			curwin.in = (struct window *)0;
		if (curwin.out == w)
			curwin.out = (struct window *)0;
		cmdbuf[1] = P1_DIR_HTOM|P1_FN_KILLW|WIN_NUM(w);
		(void)write(mfd, cmdbuf, sizeof cmdbuf);
		win_killw(w);
	}
}

static
void
p1_xmit(mfd, w)
fildes_t mfd;
register struct window *w;
{
	register char *cp, *cq;
	register int i, len;
	char ibuf[32], obuf[32];
	static char refresh;
	static char cmdbuf[] = { P1_IAC, 0 };
	extern int errno;

	/*
	 * Transmit data to the Macintosh (via file descriptor "mfd)
	 * on behalf of window "w".  Be sure to convert any embedded
	 * control characters and meta characters.
	 *
	 * Note that the input/output buffers should NOT be very large.
	 * It is undesirable to perform large reads and effectively
	 * "lock out" all other file descriptors.  The chosen size
	 * should preserve a reasonable amount of efficiency.
	 *
	 * The UW protocol only requires an OSELW command when the
	 * output window changes.  We issue this command more often
	 * to "refresh" the Mac's idea of what the output window is.
	 * This helps (slightly) to overcome spurious output redirects
	 * caused by a noisy line.
	 */
	if (w && w->w_alloc) {
		if (curwin.out != w || ++refresh == 0) {
			refresh = 0;
			curwin.out = w;
			cmdbuf[1] = P1_DIR_HTOM|P1_FN_OSELW|WIN_NUM(w);
			(void)write(mfd, cmdbuf, sizeof cmdbuf);
		}
		cq = obuf;
		if ((len = read(w->w_datafd, ibuf, sizeof ibuf)) < 0 &&
		    errno != EWOULDBLOCK)
			(*protocol->p_killw)(mfd, w);
		for (cp=ibuf; cp < ibuf+len; cp++) {
			if (*cp&META) {
				if (cq > obuf) {
					(void)write(mfd, obuf, cq-obuf);
					cq = obuf;
				}
				cmdbuf[1] = P1_DIR_HTOM|P1_FN_META;
				(void)write(mfd, cmdbuf, sizeof cmdbuf);
				*cp &= ~META;
			}
			i = -1;
			if (*cp == RUB || *cp < ' ') {
				i = protocol->p_szctlch - 1;
				while (i >= 0 && protocol->p_ctlch[i] != *cp)
					i--;
			}
			if (i >= 0) {
				if (cq > obuf) {
					(void)write(mfd, obuf, cq-obuf);
					cq = obuf;
				}
				cmdbuf[1] = P1_DIR_HTOM|P1_FN_CTLCH|i;
				(void)write(mfd, cmdbuf, sizeof cmdbuf);
			} else {
				*cq++ = *cp;
				if (cq >= obuf+sizeof obuf) {
					(void)write(mfd, obuf, cq-obuf);
					cq = obuf;
				}
			}
		}
		if (cq > obuf)
			(void)write(mfd, obuf, cq-obuf);
	} else
		(void)read(w->w_datafd, ibuf, sizeof ibuf);
}

static
void
p1_recv(mfd, cbuf, clen)
fildes_t mfd;
char *cbuf;
int clen;
{
	register int len;
	register char *buf, *cp, *cq;
	register struct window *w;
	nwin_t wnum;
	auto int nready;
	char ibuf[512], obuf[512];
	static int seen_iac, seen_meta;
	static pnreq_t pnrq_cmd;
	static char cmdbuf[2] = { P1_IAC };

	/*
	 * The received bytestream is examined.  Non-command bytes are
	 * written to the file descriptor corresponding to the current
	 * "input" window (relative to the Macintosh -- the window the
	 * user types input to).
	 *
	 * If "clen" is nonzero, then the contents of the buffer "cbuf"
	 * are processed before any input is read.
	 */
	if (ioctl(mfd, (int)FIONREAD, (char *)&nready) < 0) {
		perror("FIONREAD");
		return;
	}
	nready += clen;

	for (cq = obuf; nready > 0; nready -= len) {
		if (clen > 0) {
			len = clen;
			buf = cbuf;
			clen = 0;
		} else {
			if (nready > sizeof ibuf)
				len = read(mfd, ibuf, sizeof ibuf);
			else
				len = read(mfd, ibuf, nready);
			if (len <= 0) {
				perror("read");
				return;
			}
			buf = ibuf;
		}
		for (cp=buf; cp < buf+len; cp++) {
			if (pnrq_cmd) {
				pcl_haggle(mfd, pnrq_cmd|*cp);
				pnrq_cmd = 0;
				/* pcl_haggle may have changed the protocol */
				if (protocol != pcl_table) {
					if (protocol->p_recv)
						(*protocol->p_recv)(mfd,
						    cp+1, buf+len-cp-1);
					return;
				}
			} else if (seen_iac) {
				if ((*cp&P1_DIR) == P1_DIR_MTOH) {
					if (cq > obuf) {
						(void)write(curwin.in->w_datafd,
							    obuf, cq-obuf);
						cq = obuf;
					}
					switch (*cp & P1_FN) {
					case P1_FN_NEWW:
						wnum = *cp & P1_WINDOW;
						if (!wnum)
							break;
						w = WIN_PTR(wnum);
						if (w->w_alloc)
							break;
						if (!win_neww(WC_INTERNAL,
						    defwtype, wnum,
						    protocol->p_maxwin, 0L,
						    (fildes_t)-1, (fildes_t)-1,
						    (struct woptdefn *)0)) {
							cmdbuf[1] = P1_DIR_HTOM|
							    P1_FN_KILLW|wnum;
							(void)write(mfd, cmdbuf,
							    sizeof cmdbuf);
						}
						break;
					case P1_FN_KILLW:
						wnum = *cp & P1_WINDOW;
						if (!wnum)
							break;
						win_killw(WIN_PTR(wnum));
						break;
					case P1_FN_ISELW:
						wnum = *cp & P1_WINDOW;
						if (!wnum)
							break;
						w = WIN_PTR(wnum);
						if (w->w_alloc)
							curwin.in = w;
						else
							curwin.in = NULL;
						break;
					case P1_FN_META:
						seen_meta = 1;
						break;
					case P1_FN_CTLCH:
						*cq = protocol->p_ctlch[*cp&P1_CC];
						if (seen_meta) {
							seen_meta = 0;
							*cq |= META;
						}
						if (curwin.in)
							cq++;
						break;
					case P1_FN_MAINT:
						switch (*cp & P1_MF) {
						case P1_MF_ENTRY:
							(*protocol->p_renew)(mfd);
							break;
						case P1_MF_ASKPCL:
							pcl_haggle(mfd,PNRQ_ASK);
							break;
						case P1_MF_CANPCL:
							pnrq_cmd = PNRQ_CAN;
							break;
						case P1_MF_SETPCL:
							pnrq_cmd = PNRQ_SET;
							break;
						case P1_MF_EXIT:
							done(0);
							break;
						}
						break;
					}
				}
				seen_iac = 0;
			} else if (*cp == P1_IAC)
				seen_iac++;
			else {
				if (seen_meta) {
					seen_meta = 0;
					*cq = *cp | META;
				} else
					*cq = *cp;
				if (curwin.in) {
					if (++cq >= obuf+sizeof obuf) {
						(void)write(curwin.in->w_datafd,
							    obuf, cq-obuf);
						cq = obuf;
					}
				}
			}
		}
	}
	if (cq > obuf)
		(void)write(curwin.in->w_datafd, obuf, cq-obuf);
}

static
void
p1_askpcl(mfd)
fildes_t mfd;
{
	static char cmdbuf[2] = { P1_IAC,P1_DIR_HTOM|P1_FN_MAINT|P1_MF_ASKPCL };

	(void)write(mfd, cmdbuf, sizeof cmdbuf);
}

static
void
p1_canpcl(mfd, pname)
fildes_t mfd;
char pname;
{
	static char cmdbuf[3] = { P1_IAC,P1_DIR_HTOM|P1_FN_MAINT|P1_MF_CANPCL };

	cmdbuf[2] = pname;
	(void)write(mfd, cmdbuf, sizeof cmdbuf);
}

static
void
p1_setpcl(mfd, pname)
fildes_t mfd;
char pname;
{
	static char cmdbuf[3] = { P1_IAC,P1_DIR_HTOM|P1_FN_MAINT|P1_MF_SETPCL };

	cmdbuf[2] = pname;
	(void)write(mfd, cmdbuf, sizeof cmdbuf);
}

static
void
p1_renew(mfd)
fildes_t mfd;
{
	register struct window *w;
	static char cmdbuf[2] = { P1_IAC };

	/*
	 * Re-init (re-NEW) an existing connection.  Send a NEWW command
	 * for each existing window.  This function is invoked when the
	 * Macintosh sends an ENTRY maintenance command.
	 */
	for (w=window; w < window+protocol->p_maxwin; w++) {
		if (w->w_alloc) {
			win_renew(w, 0);
			cmdbuf[1] = P1_DIR_HTOM|P1_FN_NEWW|WIN_NUM(w);
			(void)write(mfd, cmdbuf, sizeof cmdbuf);
		}
	}
}

static
void
p2_renew(mfd)
fildes_t mfd;
{
	register struct window *w;
	static char cmdbuf[3] = { P2_IAC };

	/*
	 * Re-init (re-NEW) an existing connection.  Send a NEWW command
	 * for each existing window.  This function is invoked when the
	 * Macintosh sends an ENTRY maintenance command.
	 */
	for (w=window; w < window+protocol->p_maxwin; w++) {
		if (w->w_alloc) {
			win_renew(w, 1);
			cmdbuf[1] = P2_DIR_HTOM|P2_FN_NEWW|WIN_NUM(w);
			cmdbuf[2] = w->w_type + ' ';
			(void)write(mfd, cmdbuf, sizeof cmdbuf);
		}
	}
}

static
struct window *
p2_neww(mfd, wclass, wtype, wnum, wid, datafd, ctlfd)
fildes_t mfd;
wclass_t wclass;
wtype_t wtype;
nwin_t wnum;
long wid;
fildes_t datafd;
fildes_t ctlfd;
{
	register struct window *w;
	static char cmdbuf[3] = { P2_IAC };

	/*
	 * Create a new window as requested by the host.  This routine is not
	 * called when the Macintosh creates a window.
	 */
	w = win_neww(wclass, wtype, wnum, protocol->p_maxwin, wid,
	    datafd, ctlfd, (struct woptdefn *)0);
	if (w) {
		cmdbuf[1] = P2_DIR_HTOM|P2_FN_NEWW|WIN_NUM(w);
		cmdbuf[2] = ' ' + wtype;
		(void)write(mfd, cmdbuf, sizeof cmdbuf);
	}
	return(w);
}

static
void
p2_chkopt(mfd)
fildes_t mfd;
{
	register struct window *w;
	nwin_t maxwin;

	/*
	 * Ideally, this routine would call a routine in the window
	 * module (perhaps win_chkopt()), passing the maximum window
	 * number as one argument.  The "for" loop would be in that
	 * routine.  However, I'm not willing to accept the overhead
	 * for that conceptual nicety.
	 */
	maxwin = protocol->p_maxwin;
	for (w=window; w < window+maxwin; w++)
		if (w->w_alloc)
			opt_scan((caddr_t)w, &w->w_optdefn, p2_sendopt, mfd,
			    P2_FN_WOPT|WIN_NUM(w));
}

static
void
p2_sendopt(mfd, fn, buf, len)
fildes_t mfd;
int fn;
register char *buf;
register int len;
{
	register char *cp;
	register int i;
	char outbuf[512];

	/*
	 * Encode and transmit the option string contained in "buf".  The
	 * initial command (which will be P2_FN_WOPT|WIN_NUM(w)) is
	 * contained in "fn".
	 *
	 * The caller is responsible for handing us a correctly-formed
	 * option string.  This routine merely performs the protocol encoding
	 * which is required for control and meta characters.
	 */
	curwin.out = NULL;
	outbuf[0] = P2_IAC;
	outbuf[1] = fn|P2_DIR_HTOM;
	for (cp=outbuf+2; len > 0; buf++,len--) {
		if (cp > outbuf+sizeof outbuf - 4) {
			(void)write(mfd, outbuf, cp-outbuf);
			cp = outbuf;
		}
		if (*buf & META) {
			*cp++ = P2_IAC;
			*cp++ = P2_DIR_HTOM|P2_FN_META;
			*buf &= ~META;
		}
		i = -1;
		if (*buf == RUB || *buf < ' ') {
			i = protocol->p_szctlch - 1;
			while (i >= 0 && protocol->p_ctlch[i] != *buf)
				i--;
		}
		if (i >= 0) {
			*cp++ = P2_IAC;
			*cp++ = P2_DIR_HTOM|P2_FN_CTLCH|(i&7);
		} else
			*cp++ = *buf;
	}
	if (cp > outbuf)
		(void)write(mfd, outbuf, cp-outbuf);
}

static
void
p2_recv(mfd, cbuf, clen)
fildes_t mfd;
char *cbuf;
int clen;
{
	register int len;
	register char *buf, *cp, *cq;
	register struct window *w;
	register char c;
	nwin_t wnum;
	auto int nready;
	char ibuf[512], obuf[512];
	static int seen_iac, seen_meta, is_option;
	static pnreq_t pnrq_cmd;
	static nwin_t neww;
	static char cmdbuf[2] = { P2_IAC };

	/*
	 * The received bytestream is examined.  Non-command bytes are
	 * written to the file descriptor corresponding to the current
	 * "input" window (relative to the Macintosh -- the window the
	 * user types input to).
	 *
	 * If "clen" is nonzero, then the contents of the buffer "cbuf"
	 * are processed before any input is read.
	 */
	if (ioctl(mfd, (int)FIONREAD, (char *)&nready) < 0) {
		perror("FIONREAD");
		return;
	}
	nready += clen;

	for (cq = obuf; nready > 0; nready -= len) {
		if (clen > 0) {
			len = clen;
			buf = cbuf;
			clen = 0;
		} else {
			if (nready > sizeof ibuf)
				len = read(mfd, ibuf, sizeof ibuf);
			else
				len = read(mfd, ibuf, nready);
			if (len <= 0) {
				perror("read");
				return;
			}
			buf = ibuf;
		}
		for (cp=buf; cp < buf+len; cp++) {
			if (pnrq_cmd) {
				pcl_haggle(mfd, pnrq_cmd|*cp);
				pnrq_cmd = 0;
				/* pcl_haggle may have changed the protocol */
				if (protocol != pcl_table) {
					if (protocol->p_recv)
						(*protocol->p_recv)(mfd,
						    cp+1, buf+len-cp-1);
					return;
				}
			} else if (neww) {
				w = WIN_PTR(neww);
				if (!w->w_alloc &&
				    !win_neww(WC_INTERNAL, (wtype_t)(*cp-' '),
				    neww, protocol->p_maxwin, 0L,
				    (fildes_t)-1, (fildes_t)-1,
				    (struct woptdefn *)0)) {
					cmdbuf[1] = P2_DIR_HTOM|
					    P2_FN_KILLW|neww;
					(void)write(mfd, cmdbuf,
					    sizeof cmdbuf);
				}
				neww = 0;
			} else if (seen_iac) {
				if ((*cp&P2_DIR) == P2_DIR_MTOH) {
					c = *cp & P2_FN;
					if (is_option &&
					    c!=P2_FN_META && c!=P2_FN_CTLCH) {
						opt_iflush();
						is_option = 0;
					}
					if (cq > obuf) {
						(void)write(curwin.in->w_datafd,
							    obuf, cq-obuf);
						cq = obuf;
					}
					switch (*cp & P2_FN) {
					case P2_FN_NEWW:
						neww = *cp & P2_WINDOW;
						break;
					case P2_FN_WOPT:
						wnum = *cp & P2_WINDOW;
						if (!wnum)
							break;
						w = WIN_PTR(wnum);
						if (!w->w_alloc) {
							curwin.in = NULL;
							break;
						}
						is_option = 1;
						opt_istart((caddr_t)w, &w->w_optdefn);
						break;
					case P2_FN_KILLW:
						wnum = *cp & P2_WINDOW;
						if (!wnum)
							break;
						win_killw(WIN_PTR(wnum));
						break;
					case P2_FN_ISELW:
						wnum = *cp & P2_WINDOW;
						if (!wnum)
							break;
						w = WIN_PTR(wnum);
						if (w->w_alloc)
							curwin.in = w;
						else
							curwin.in = NULL;
						break;
					case P2_FN_META:
						seen_meta = 1;
						if ((*cp&P2_CC) == 0)
							break;
						/* no break */
					case P2_FN_CTLCH:
						c=protocol->p_ctlch[*cp&P2_CC];
						if (seen_meta) {
							seen_meta = 0;
							c |= META;
						}
						if (is_option)
							is_option=opt_input(c);
						else
							if (curwin.in)
								*cq++ = c;
						break;
					case P2_FN_MAINT:
						switch (*cp & P2_MF) {
						case P2_MF_ENTRY:
							(*protocol->p_setpcl)(mfd, protocol->p_name);
							(*protocol->p_renew)(mfd);
							break;
						case P2_MF_ASKPCL:
							pcl_haggle(mfd,PNRQ_ASK);
							break;
						case P2_MF_CANPCL:
							pnrq_cmd = PNRQ_CAN;
							break;
						case P2_MF_SETPCL:
							pnrq_cmd = PNRQ_SET;
							break;
						case P2_MF_EXIT:
							done(0);
							break;
						}
						break;
					}
				}
				seen_iac = 0;
			} else if (*cp == P2_IAC)
				seen_iac++;
			else {
				if (seen_meta) {
					c = *cp | META;
					seen_meta = 0;
				} else
					c = *cp;
				if (is_option)
					is_option = opt_input(c);
				else
					if (curwin.in)
						*cq++ = c;
				if (cq >= obuf+sizeof obuf) {
					(void)write(curwin.in->w_datafd,
						    obuf, cq-obuf);
					cq = obuf;
				}
			}
		}
	}
	if (cq > obuf)
		(void)write(curwin.in->w_datafd, obuf, cq-obuf);
}
