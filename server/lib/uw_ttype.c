/*
 *	uw library - uw_ttype
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <strings.h>
#include "uwlib.h"

struct table {
	char		*tname;
	uwtype_t	wtype;
};

/* The following table must be sorted */
static struct table table[] = {
	{ "aaa-24", UWT_ANSI },
	{ "adm3", UWT_ADM31 },
	{ "adm31", UWT_ADM31 },
	{ "adm3a", UWT_ADM31 },
	{ "ansi", UWT_ANSI },
	{ "tek", UWT_TEK4010 },
	{ "tek4010", UWT_TEK4010 },
	{ "tek4012", UWT_TEK4010 },
	{ "vt52", UWT_VT52 },
};

uwtype_t
uw_ttype(name)
char *name;
{
	register struct table *t, *lo, *hi;
	register int cmp;

	/*
	 * Map a terminal name string to a UW window emulation type.
	 */
	lo = table;
	hi = table + sizeof table / sizeof table[0] - 1;
	while (lo <= hi) {
		t = lo + (hi-lo) / 2;
		cmp = strcmp(name, t->tname);
		if (cmp == 0)
			return(t->wtype);
		if (cmp < 0)
			hi = t-1;
		else
			lo = t+1;
	}
	return(UWT_ADM31);	/* default if no match */
}
