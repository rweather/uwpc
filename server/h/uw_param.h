/*
 *	uw parameters
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

/*
 * This file exists because #include file definitions aren't in the same
 * place on all machines.  Also, it seems pointless to drag in all of
 * <stdio.h> just to define NULL.  Finally, a few declarations are
 * sufficiently global that this is the most logical place to put them.
 *
 * This file should be #included after all of the system include files
 * (e.g. <sys/types.h>) but before any other UW include files.
 */
#ifndef UW_PARAM
#define	UW_PARAM

typedef int fildes_t;		/* this really should be in <sys/types.h> */

#ifndef NBBY			/* this is in <sys/types.h> in 4.3BSD */
#define	NBBY	8		/* (number of bits/byte) */
#endif

#ifndef NULL
#define	NULL	0
#endif

extern char *malloc();
extern char *mktemp();
extern char *getenv();
extern void done();
extern void cwait();

#endif
