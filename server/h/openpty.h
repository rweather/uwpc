/*
 *	This file defines the "ptydesc" structure which is returned
 *	by the routine "openpty".
 */

struct ptydesc {
	int		pt_pfd;		/* file descriptor of master side */
	int		pt_tfd;		/* file descriptor of slave side */
	char		*pt_pname;	/* master device name */
	char		*pt_tname;	/* slave device name */
};
