/*
 *	uw_fd - file-descriptor/select data
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_FD
#define	UW_FD

#include "uw_param.h"

/*
 * If FD_SET and friends aren't defined in <sys/types.h>, then we
 * provide simple definitions here.
 */
#ifndef FD_SET
#define	FD_SET(n,p)	((p)->fds_bits[0] |= (1 << (n)))
#define	FD_CLR(n,p)	((p)->fds_bits[0] &= ~(1 << (n)))
#define	FD_ISSET(n,p)	((p)->fds_bits[0] & (1 << (n)))
#define	FD_ZERO(p)	((p)->fds_bits[0] = 0)
#define	FD_SETSIZE	(NBBY*sizeof(long))
#endif

/*
 * We use file descriptors for several different things.  "fdmap" associates
 * a file descriptor number with its use.
 */
typedef enum {				/* file descriptor type */
	FDT_NONE,			/*	not in use */
	FDT_DATA,			/*	data connection for window */
	FDT_CTL,			/*	control connection for window */
	FDT_MAC,			/*	tty line which talks to Mac */
	FDT_UDSOCK,			/*	UNIX-domain datagram socket */
	FDT_ISSOCK,			/*	Internet-domain stream sock */
	FDT_DEBUG,			/*	debugging use */
	FDT_OTHER			/*	other uses */
} fdtype_t;

struct fdmap {
	fdtype_t	f_type;		/* file descriptor type */
	struct window	*f_win;		/* associate window (if any) */
};

struct selmask {
	struct fd_set	sm_rd;
	struct fd_set	sm_wt;
	struct fd_set	sm_ex;
};

extern struct fdmap fdmap[FD_SETSIZE];
extern fildes_t nfds;
extern struct selmask selmask[2];
#endif
