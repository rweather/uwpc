/*
 *	uw error codes
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_ERR
#define	UW_ERR

typedef int uwerr_t;

#define	UWE_NONE	0		/* no error */
#define	UWE_ERRNO	1		/* system call error, consult errno */
#define	UWE_NXTYPE	2		/* nonexistent window type */
#define	UWE_DUPID	3		/* window ID duplicated (in use) */
#define	UWE_NOTIMPL	4		/* operation not implemented yet */
#define	UWE_NXSERV	5		/* non-existent server */
#define	UWE_NOMEM	6		/* unable to allocate required memory */
#define	UWE_INVAL	7		/* invalid argument to function */
#define	UWE_NOCTL	8		/* no control file descriptor */

#endif
