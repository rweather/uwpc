/*
 *	uw library - uw_netadj
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <strings.h>
#include <signal.h>
#include "openpty.h"

#include "uw_opt.h"	/* I had hoped to avoid including this */
#include "uwlib.h"

static woptarg_t woa_vis[] = { WOA_UDATA(1), WOA_END };
static woptarg_t woa_type[] = { WOA_UDATA(6), WOA_END };
static woptarg_t woa_pos[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };
static woptarg_t woa_title[] = { WOA_STRING(255), WOA_END };
static woptarg_t woa_size[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };
static woptarg_t woa_tsize[] = { WOA_UDATA(12), WOA_UDATA(12), WOA_END };
static woptarg_t woa_fontsz[] = { WOA_UDATA(6), WOA_END };
static woptarg_t woa_clipb[] = { WOA_UDATA(1), WOA_END };
static woptarg_t woa_bell[] = { WOA_UDATA(2), WOA_END };
static woptarg_t woa_curs[] = { WOA_UDATA(1), WOA_END };
static woptarg_t woa_chgsz[] = { WOA_UDATA(1), WOA_END };

static woptarg_t *optargs[][WONUM_MAX+1] = {
	/* window type 0 == adm31 */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
		woa_tsize, woa_fontsz, woa_clipb, woa_bell, woa_curs, woa_chgsz
	},
	/* window type 1 == vt52 */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
		woa_tsize, woa_fontsz, woa_clipb, woa_bell, woa_curs, woa_chgsz
	},
	/* window type 2 == ansi */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
		woa_tsize, woa_fontsz, woa_clipb, woa_bell, woa_curs, woa_chgsz
	},
	/* window type 3 = tek4010 */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
	},
	/* window type 4 = file transfer */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
	},
	/* window type 5 = printer */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
	},
	/* window type 6 = plot */
	{
		0, woa_vis, woa_type, woa_pos, woa_title, woa_size, 0, 0,
	},
};

#ifdef htons
uw_hton(wtype, optnum, data)
uwtype_t wtype;
uwopt_t optnum;
char *data;
{
}

uw_ntoh(wtype, optnum, data)
uwtype_t wtype;
uwopt_t optnum;
char *data;
{
}

#else
/* These should have been defined in <netinet/in.h> but weren't (in 4.2BSD) */
extern unsigned short htons(), ntohs();
extern unsigned long htonl(), ntohl();

uw_hton(wtype, optnum, data)
uwtype_t wtype;
uwopt_t optnum;
char *data;
{
	static struct netadj na = {
		(short (*)())htons, (long (*)())htonl, htons, htonl
	};
	if (data != (char *)0 && wtype < sizeof optargs / sizeof optargs[0] &&
	    optnum <= WONUM_MAX && optargs[wtype][optnum] != (woptarg_t *)0) {
		netadj(optargs[wtype][optnum], data, &na);
	}
}

uw_ntoh(wtype, optnum, data)
uwtype_t wtype;
uwopt_t optnum;
char *data;
{
	static struct netadj na = {
		(short (*)())ntohs, (long (*)())ntohl, ntohs, ntohl
	};
	if (data != (char *)0 && wtype < sizeof optargs / sizeof optargs[0] &&
	    optnum <= WONUM_MAX && optargs[wtype][optnum] != (woptarg_t *)0) {
		netadj(optargs[wtype][optnum], data, &na);
	}
}

static
netadj(woa, data, na)
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
	 * Convert an option between host byte order and network byte order.
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
#endif
