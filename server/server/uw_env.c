/*
 *	uw_env - environment manipulation
 *
 * Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#define	MAXENV	128	/* maximum number of arguments in environment */

static char *earray[MAXENV+1];

env_set(env)
char **env;
{
	register char **ep1, **ep2, *cp;
	char **ep3;
	extern char **environ;


	/*
	 * Merge the set of environment strings in "env" into the
	 * environment.
	 */

	/*
	 * The first time through, copy the environment from its
	 * original location to the array "earray".  This makes it a
	 * little easier to change things.
	 */

	if (environ != earray) {
		ep1=environ;
		ep2=earray;
		while(*ep1 && ep2 <= earray+MAXENV)
			*ep2++ = *ep1++;
		*ep2++ = (char *)0;
		environ = earray;
	}


	/*
	 * If "env" is non-NULL, it points to a list of new items to
	 * be added to the environment.  These replace existing items
	 * with the same name.
	 */

	if (env) {
		for (ep1=env; *ep1; ep1++) {
			for (ep2=environ; *ep2; ep2++)
				if (!env_cmp(*ep1, *ep2))
					break;
			if (ep2 < earray+MAXENV) {
				if (!*ep2)
					ep2[1] = (char *)0;
				*ep2 = *ep1;
			}
		}
	}


	/* Finally, use an insertion sort to put things in order. */

	for (ep1=environ+1; cp = *ep1; ep1++) {
		for(ep2=environ; ep2 < ep1; ep2++)
			if (env_cmp(*ep1, *ep2) < 0)
				break;
		ep3 = ep2;
		for(ep2=ep1; ep2 > ep3; ep2--)
			ep2[0] = ep2[-1];
		*ep2 = cp;
	}
}


static
env_cmp(e1, e2)
register char *e1, *e2;
{
	register d;

	do {
		if (d = *e1 - *e2++)
			return(d);
	} while (*e1 && *e1++ != '=');
	return(0);
}
