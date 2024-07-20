/*
 *	uw_opt - window option handling for UW
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>

#include "uw_param.h"
#include "uw_opt.h"

/*
 * The following variable is a kludge for efficiency.  It is set to
 * a nonzero value when a bitmask is changed, indicating that opt_scan()
 * should be called.
 */
int calloptscan;			/* pending,do,dont awaits opt_scan */

/* option input state variables */
static caddr_t optwin;			/* window */
static struct woptdefn *optwod;		/* window option definition */
static woptcmd_t optcmd;		/* option command */
static woption_t optnum;		/* current option number */
static woptarg_t *optarg;		/* current encoding */
static int optcnt;			/* count in current encoding */
static char *optout;			/* decoded option */
static char optbuf[512];		/* buffer for decoded option */

opt_new(wod, generic, unique)
register struct woptdefn *wod, *generic, *unique;
{
	register int n, mask;

	/*
	 * Set up the option definition structure pointed to by "wod" to
	 * correspond with the option definitions in "generic" (common to
	 * all window types) and "unique" (per-window).
	 */
	mask = (1<<(WONUM_GENERIC+1))-1;
	if (unique) {
		wod->wod_askrpt = unique->wod_askrpt & ~mask;
		wod->wod_pending = unique->wod_pending & ~mask;
		for (n=WONUM_GENERIC+1; n <= WONUM_MAX; n++)
			wod->wod_optlst[n] = unique->wod_optlst[n];
	} else {
		wod->wod_askrpt = 0;
		wod->wod_pending = 0;
		for (n=WONUM_GENERIC+1; n <= WONUM_MAX; n++)
			wod->wod_optlst[n].wol_argdefn = (woptarg_t *)0;
	}
	if (generic) {
		wod->wod_askrpt |= generic->wod_askrpt & mask;
		wod->wod_pending |= generic->wod_pending & mask;
		for (n=1; n <= WONUM_GENERIC; n++)
			wod->wod_optlst[n] = generic->wod_optlst[n];
	} else {
		for (n=1; n <= WONUM_GENERIC; n++)
			wod->wod_optlst[n].wol_argdefn = (woptarg_t *)0;
	}
	wod->wod_do = wod->wod_askrpt;
	wod->wod_dont = 0;
	wod->wod_inquire = 0;
	calloptscan = 1;
}

opt_renew(wod, report)
register struct woptdefn *wod;
int report;
{
	/*
	 * Reset "wod_do" for all window options that we want the Macintosh
	 * to report to us.  If "report" is nonzero, send the Mac the
	 * current values of these options.
	 */
	wod->wod_do = wod->wod_askrpt;
	wod->wod_dont = 0;
	if (report)
		wod->wod_pending = wod->wod_askrpt;
	calloptscan = 1;
}

opt_newtype(wod, generic, unique)
register struct woptdefn *wod, *generic, *unique;
{
	register int n, bit;
	register woptbmask_t oldask;

	/*
	 * Change the window options to reflect a new window emulation type.
	 * The new emulation may not support all of the events that the old
	 * one did, and in any event they may not mean the same thing.
	 */
	oldask = wod->wod_askrpt;
	opt_new(wod, generic, unique);
	for (n=1, bit=2; n <= WONUM_GENERIC; n++,bit<<=1) {
		if ((oldask&bit) && !(wod->wod_askrpt&bit))
			wod->wod_dont |= bit;
		if (!(oldask&bit) && (wod->wod_askrpt&bit))
			wod->wod_do |= bit;
	}
	for ( ; n <= WONUM_MAX; n++, bit<<=1)
		if (wod->wod_askrpt&bit)
			wod->wod_do |= bit;
	calloptscan = 1;
}

opt_setext(wod, fn)
register struct woptdefn *wod;
register void (*fn)();
{
	register int n;

	/*
	 * Set "wol_ext" to "fn" for each option that has a defined "wol_set".
	 */
	for (n=1; n <= WONUM_MAX; n++)
		if (wod->wod_optlst[n].wol_set)
			wod->wod_optlst[n].wol_ext = fn;
}

opt_scan(w, wod, fn, mfd, cmd)
caddr_t w;
register struct woptdefn *wod;
void (*fn)();
fildes_t mfd;
int cmd;
{
	register struct woptlst *wol;
	register char *cp;
	register int n, bit, maxsize;
	char buf[512];

	/*
	 * Scan the entire list of options for pending ones.  For each
	 * one, call "fn".  "cmd" is the command that we are to pass as the
	 * first argument to "fn".
	 *
	 * Note that we must send data (wod_pending) before processing
	 * DO commands (wod_do); otherwise, the host and Mac may not
	 * agree upon the value of a window option.  (The Mac might
	 * respond to the "do" command before it sees the new value.)
	 */
	cp = buf;
#ifdef notdef
	for (n=1,bit=2,wol=wod->wod_optlst+1; n<=WONUM_MAX; n++,bit<<=1,wol++) {
#else
	for (n=WONUM_MAX, bit=(1<<WONUM_MAX), wol=wod->wod_optlst+WONUM_MAX;
	     n > 0;
	     n--, bit >>= 1, wol--) {
#endif
		if (wod->wod_pending&bit) {
			wod->wod_pending &= ~bit;
			if (wol->wol_argdefn) {
				maxsize = 2 +
				    opt_size(wol->wol_argdefn);
				if (cp > buf + sizeof buf - maxsize - 1) {
					*cp++ = 0;
					(*fn)(mfd, cmd, buf, cp-buf);
					cp = buf;
				}
				if (WONUM_USELONG(n)) {
					*cp++ = WOC_SET|WONUM_LPREFIX;
					*cp++ = WONUM_LENCODE(n);
				} else
					*cp++ = WOC_SET|WONUM_SENCODE(n);
				cp += opt_encode(cp, wol->wol_argdefn,
				    (*wol->wol_get)(w, n));
			}
		}
		if (wod->wod_inquire&bit) {
			wod->wod_inquire &= ~bit;
			if (cp > buf + sizeof buf - 3) {
				*cp++ = 0;
				(*fn)(mfd, cmd, buf, cp-buf);
				cp = buf;
			}
			if (wol->wol_argdefn) {
				if (WONUM_USELONG(n)) {
					*cp++ = WOC_INQUIRE|WONUM_LPREFIX;
					*cp++ = WONUM_LENCODE(n);
				} else
					*cp++ = WOC_INQUIRE|WONUM_SENCODE(n);
			}
		}
		if ((wod->wod_do|wod->wod_dont)&bit) {
			if (cp > buf + sizeof buf - 3) {
				*cp++ = 0;
				(*fn)(mfd, cmd, buf, cp-buf);
				cp = buf;
			}
			if (wod->wod_do&bit) {
				wod->wod_do &= ~bit;
				wod->wod_dont &= ~bit;
				if (wol->wol_argdefn) {
					if (WONUM_USELONG(n)) {
						*cp++ = WOC_DO|WONUM_LPREFIX;
						*cp++ = WONUM_LENCODE(n);
					} else
						*cp++ = WOC_DO|WONUM_SENCODE(n);
				}
			} else if (wod->wod_dont&bit) {
				wod->wod_do &= ~bit;
				wod->wod_dont &= ~bit;
				if (wol->wol_argdefn) {
					if (WONUM_USELONG(n)) {
						*cp++ = WOC_DONT|WONUM_LPREFIX;
						*cp++ = WONUM_LENCODE(n);
					} else
						*cp++=WOC_DONT|WONUM_SENCODE(n);
				}
			}
		}
		if (cp > buf) {
			*cp++ = 0;
			(*fn)(mfd, cmd, buf, cp-buf);
			cp = buf;
		}
	}
}

opt_size(woa)
register woptarg_t *woa;
{
	register int size, cnt;

	/*
	 * Determine the maximum size of an option whose argument encoding
	 * is specified by "woa".  This does NOT include additional encoding
	 * (e.g. for meta characters) at the protocol level.
	 */
	if (woa) {
		for (size=0; *woa != WOA_END; woa++) {
			cnt = *woa & ~WOA_CMDMASK;
			switch (*woa & WOA_CMDMASK) {
			case WOA_CHARS(0):
			case WOA_STRING(0):
				size += cnt;
				break;
			case WOA_UDATA(0):
				size += (cnt + 5) / 6;
				break;
			}
		}
	} else
		size = 0;
	return(size);
}

opt_encode(buf, woa, data)
char *buf;
register woptarg_t *woa;
char *data;
{
	register char *cp, *cq;
	register int n, cnt;
	register unsigned long ival;
	union {
		struct {
			char	c1;
			short	s;
		}	cs;
		struct {
			char	c2;
			long	l;
		}	cl;
	} u;

	/*
	 * Encode "data" according to the option argument specifier "woa"
	 * into the buffer "buf".  Return the number of bytes of "buf"
	 * actually used.  The caller has already verified that "buf" is
	 * large enough.
	 */
	if (!data)
		return(0);
	for (cp=buf,cq=data; *woa != WOA_END; woa++) {
		cnt = *woa & ~WOA_CMDMASK;
		switch (*woa & WOA_CMDMASK) {
		case WOA_CHARS(0):
			for (n=0; n < cnt; n++)
				*cp++ = *cq++;
			break;
		case WOA_STRING(0):
			for (n=0; n < cnt-1 && *cq; n++)
				*cp++ = *cq++;
			if (n < cnt)
				cq += cnt-n;
			*cp++ = '\0';
			break;
		case WOA_UDATA(0):
			if (cnt <= NBBY) {
				ival = (unsigned char)*cq++;
			} else if (cnt <= sizeof(short)*NBBY) {
				while ((int)cq & ((char *)&u.cs.s-&u.cs.c1-1))
					cq++;
				ival = *(unsigned short *)cq;
				cq += sizeof(short);
			} else {
				while ((int)cq & ((char *)&u.cl.l-&u.cl.c2-1))
					cq++;
				ival = *(unsigned long *)cq;
				cq += sizeof(long);
			}
			if (cnt != sizeof(long)*NBBY)
				ival &= (1<<cnt) - 1;
			for (n=0; n < cnt; n += 6, ival >>= 6)
				*cp++ = (ival & 077) | 0100;
			break;
		}
	}
	return(cp-buf);
}
			
opt_istart(w, wod)
caddr_t w;
struct woptdefn *wod;
{
	/*
	 * Start collecting input for a window option specification.
	 */
	optwin = w;
	optwod = wod;
	optnum = 0;
}

opt_input(c)
char c;
{
	register int cnt, bit;
	register struct woptdefn *wod;
	register struct woptlst *wol;
	register unsigned long ival;
	union {
		struct {
			char	c1;
			short	s;
		}	cs;
		struct {
			char	c2;
			long	l;
		}	cl;
	} u;

	/*
	 * Add the received character "c" to the current option specification.
	 * If it is complete, take the appropriate action.  If option 0
	 * (the endmarker) is received, return 0 (to notify the caller that
	 * we are done).  Otherwise, return 1 -- more option data remains
	 * to be processed.
	 *
	 * This code isn't as readable as it should be; there are far too
	 * many return statements floating around.  Sorry about that.
	 */
	if (optwin) {
		wod = optwod;
		if (optnum == 0 || optnum == WONUM_MAX+1) {
			/* start (or continue) decoding a new option */
			if (optnum == 0) {
				/* start new option (or decode endmarker) */
				if (c & WONUM_MASK) {
					/* new option */
					optcmd = c & WOC_MASK;
					if (WOC_BADCMD(optcmd)) {
						opt_iflush();
						return(0);
					}
					if (c == WONUM_LPREFIX) {
						optnum = WONUM_MAX+1;
						return(1);
					} else
						optnum = WONUM_SDECODE(c);
				} else {
					/* end of options */
					opt_iflush();
					return(0);
				}
			} else {
				/* read second byte of long option number */
				optnum = WONUM_LDECODE(c);
				if (optnum > WONUM_MAX) {
					opt_iflush();
					return(0);
				}
			}
			/*
			 * This point is reached when the option number has
			 * been completely decoded.  If the command is not
			 * WOC_SET, then it has no arguments and we can
			 * process it immediately.
			 */
			wol = &wod->wod_optlst[optnum];
			bit = 1<<optnum;
			if (optcmd == WOC_SET) {
				optout = optbuf;
				optcnt = 0;
				optarg = wol->wol_argdefn;
				if (!optarg) {
					opt_iflush();
					return(0);
				}
			} else {
				if (wol->wol_ext &&
				    (optcmd == WOC_WILL || optcmd == WOC_WONT))
					(*wol->wol_ext)(optwin, optcmd,
					    optnum, (char *)0, 0);
				switch (optcmd) {
				case WOC_INQUIRE:
					wod->wod_pending |= bit;
					calloptscan = 1;
					break;
				case WOC_DO:
				case WOC_DONT:
					break;
				case WOC_WILL:
					wod->wod_askrpt |= bit;
					wod->wod_do &= ~bit;
					break;
				case WOC_WONT:
					wod->wod_askrpt &= ~bit;
					wod->wod_dont &= ~bit;
					break;
				}
				optnum = 0;
			}
			return(1);
		} else {
			/* continue processing argument to option */
			wol = &wod->wod_optlst[optnum];
			bit = 1<<optnum;
			cnt = *optarg & ~WOA_CMDMASK;
			switch (*optarg & WOA_CMDMASK) {
			case WOA_CHARS(0):
				*optout++ = c;
				optcnt++;
				break;
			case WOA_STRING(0):
				*optout++ = c;
				optcnt++;
				if (!c) {
					optout += cnt - optcnt;
					optcnt = cnt;
				} else if (optcnt == cnt-1) {
					*optout++ = '\0';
					optcnt = cnt;
				}
				break;
			case WOA_UDATA(0):
				if (optcnt == 0) {
					if (cnt <= NBBY) {
						*optout = 0;
					} else if (cnt <= sizeof(short)*NBBY) {
						while ((int)optout & ((char *)&u.cs.s-&u.cs.c1-1))
							optout++;
						*(short *)optout = 0;
					} else {
						while ((int)optout & ((char *)&u.cl.l-&u.cl.c2-1))
							optout++;
						*(long *)optout = 0;
					}
				}
				ival = (c & 077) << optcnt;
				if (cnt != NBBY*sizeof(long))
					ival &= (1<<cnt) - 1;
				optcnt += 6;
				if (cnt <= NBBY) {
					*(unsigned char *)optout |= (unsigned char)ival;
					if (optcnt >= cnt)
						optout++;
				} else if (cnt <= sizeof(short)*NBBY) {
					*(unsigned short *)optout |= (unsigned short)ival;
					if (optcnt >= cnt)
						optout += sizeof(short);
				} else {
					*(unsigned long *)optout |= ival;
					if (optcnt >= cnt)
						optout += sizeof(long);
				}
				break;
			}
			if (optcnt >= cnt) {
				optcnt = 0;
				if (*++optarg == WOA_END) {
					wod->wod_pending &= ~bit;
					(*wol->wol_set)(optwin, optnum, optbuf);
					if (wol->wol_ext) {
						(*wol->wol_ext)(optwin, WOC_SET,
						    optnum, optbuf,
						    optout-optbuf);
					}
					optnum = 0;
				}
			}
			return(1);
		}
		/*NOTREACHED*/
	}
	return(0);
}

opt_iflush()
{
	optwin = (caddr_t)0;
}

opt_extopt(w, wod, cmd, num, data, na)
caddr_t w;
register struct woptdefn *wod;
woptcmd_t cmd;
woption_t num;
char *data;
struct netadj *na;
{
	register struct woptlst *wol;

	if (w != NULL && wod != NULL && num <= WONUM_MAX) {
		wol = wod->wod_optlst + num;
		if (wol->wol_argdefn) {
			switch (cmd) {
			case WOC_SET:
				if (data && wol->wol_set) {
					if (na) {
						opt_netadj(wol->wol_argdefn,
						    data, na);
					}
					/*
					 * Set the new value and notify the Mac.
					 * Because of a race condition (the Mac
					 * might concurrently be sending us its
					 * value for this option), we ask the
					 * Mac to send back the value after it
					 * is set.
					 */
					(*wol->wol_set)(w, num, data);
					WOPT_SET(wod->wod_pending, num);
					WOPT_SET(wod->wod_inquire, num);
					calloptscan = 1;
				}
				break;
			case WOC_INQUIRE:
				WOPT_SET(wod->wod_inquire, num);
				calloptscan = 1;
				break;
			case WOC_DO:
				WOPT_SET(wod->wod_do, num);
				WOPT_SET(wod->wod_askrpt, num);
				calloptscan = 1;
				break;
			case WOC_DONT:
				WOPT_SET(wod->wod_dont, num);
				WOPT_CLR(wod->wod_askrpt, num);
				calloptscan = 1;
				break;
			}
		}
	}
}

opt_netadj(woa, data, na)
register woptarg_t *woa;
char *data;
register struct netadj *na;
{
	register char *cp;
	register int cnt;
	union {
		struct {
			char	c1;
			short	s;
		}	cs;
		struct {
			char	c2;
			long	l;
		}	cl;
	} u;

	/*
	 * Convert an option (in internal format) from host byte order
	 * to network byte order.  If the two are the same then this is
	 * a NOP.
	 */
	if (data && na) {
		for (cp=data; *woa != WOA_END; woa++) {
			cnt = *woa & ~WOA_CMDMASK;
			switch (*woa & WOA_CMDMASK) {
			case WOA_CHARS(0):
			case WOA_STRING(0):
				cp += cnt;
				break;
			case WOA_UDATA(0):
				if (cnt <= NBBY) {
					cp++;
				} else if (cnt <= sizeof(short)*NBBY) {
					while ((int)cp & ((char *)&u.cs.s-&u.cs.c1-1))
						cp++;
					*(u_short *)cp =
					    (*na->na_ushort)(*(u_short *)cp);
					cp += sizeof(short);
				} else {
					while ((int)cp & ((char *)&u.cl.l-&u.cl.c2-1))
						cp++;
					*(u_short *)cp =
					    (*na->na_ushort)(*(u_short *)cp);
					cp += sizeof(long);
				}
			}
		}
	}
}
