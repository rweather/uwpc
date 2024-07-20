/*
 *	uw_clk - timer support for UW
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/resource.h>

#include "uw_param.h"
#include "uw_clk.h"

static struct timeout *pending;
static struct timeout *freelist;

int timer_rdy;			/* nonzero when some timeout is ready to run */

clk_timeout(secs, fn, arg)
int secs;
void (*fn)();
toarg_t arg;
{
	register struct timeout *to, **tol;
	register time_t curtime;
	extern time_t time();

	to = freelist;
	if (!to) {
		if (!(to = (struct timeout *)malloc(sizeof *to)))
			return(-1);
	} else
		freelist = to->to_next;
	to->to_fn = fn;
	to->to_arg = arg;

	if (secs < 0)
		secs = 0;
	curtime = time((time_t *)0);
	to->to_when = curtime + secs;

	tol = &pending;
	while (*tol && to->to_when > (*tol)->to_when)
		tol = &(*tol)->to_next;
	to->to_next = *tol;
	*tol = to;

	clk_service();
	return(0);
}

clk_service()
{
	register struct timeout *to;
	register time_t curtime;

	curtime = time((time_t *)0);
	while ((to=pending) && to->to_when <= curtime) {
		pending = to->to_next;
		if (to->to_fn) {
			(*to->to_fn)(to->to_arg);
			to->to_next = freelist;
			freelist = to;
		}
	}

	timer_rdy = 0;
	if (pending)
		(void)alarm((unsigned)(pending->to_when - curtime));
}

void
clk_alarm()
{
	/*
	 * A SIGALRM has been received.
	 */
	timer_rdy = 1;
}

clk_init()
{
	timer_rdy = 0;
	(void)signal(SIGALRM, clk_alarm);
}
