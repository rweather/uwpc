/*
 *	uw_fd - file-descriptor/select data
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>

#include "uw_param.h"
#include "uw_fd.h"

struct selmask selmask[2];
struct fdmap fdmap[FD_SETSIZE];
fildes_t nfds;				/* number of file descriptors */

fd_init()
{
	register fildes_t fd;

	nfds = getdtablesize();
	if (nfds > FD_SETSIZE)
		nfds = FD_SETSIZE;
	fdmap[0].f_type = FDT_MAC;
	fdmap[1].f_type = FDT_MAC;
	fdmap[2].f_type = FDT_DEBUG;
	for (fd=3; fd < FD_SETSIZE; fd++) {
		fdmap[fd].f_type = FDT_NONE;
		(void)close(fd);
	}
	FD_ZERO(&selmask[0].sm_rd);
	FD_ZERO(&selmask[0].sm_wt);
	FD_ZERO(&selmask[0].sm_ex);
}

fd_exit()
{
	register fildes_t fd;

	for (fd=3; fd < nfds; fd++)
		(void)close(fd);
}
