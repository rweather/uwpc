/*
 *	uw_clk - timer support for UW
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */

#ifndef UW_CLK
#define	UW_CLK

/*
 * Events which are supposed to occur at a certain time are handled by
 * setting "timeout"s.  The list of timeouts is sorted in order of
 * occurrence.  The "alarm" mechanism is used to send SIGALRM when the
 * first timeout expires.  However, the timeout is not processed
 * immediately.  Instead, it will be processed upon exit from the
 * select() in main().  This prevents timeouts from happening at
 * inappropriate times.
 *
 * The resolution of timeouts is in seconds.  The server doesn't need
 * any better resolution, and this allows all of the hair associated with
 * (struct timeval) and (struct itimerval) types to be avoided.
 */

#define	CLK_HZ		1		/* one tick/second */

typedef long toarg_t;

struct timeout {
	struct timeout	*to_next;
	time_t		to_when;
	void		(*to_fn)();
	toarg_t		to_arg;
};

extern int timer_rdy;

#define	CLK_CHECK()	if (timer_rdy) clk_service(); else
#endif
