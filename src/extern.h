//-------------------------------------------------------------------------
//
// EXTERN.H - External DOS and Windows 3.0 declarations for UW/PC.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1991  Rhys Weatherley
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 1, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Revision History:
// ================
//
//  Version  DD/MM/YY  By  Description
//  -------  --------  --  --------------------------------------
//    1.0    05/05/91  RW  Original Version of EXTERN.H
//
//-------------------------------------------------------------------------

#ifndef __EXTERN_H__
#define	__EXTERN_H__

#if !defined(_Windows)

// Define the declarations necessary for the DOS version of UW/PC //
#define	UWPC_DOS	1

// Use the Turbo C++ "delay" function under DOS //
#define	DELAY_FUNC	delay

// Define the far memory allocation routines //
#include <alloc.h>
#define	UWAlloc(size)	(farmalloc (size))
#define UWFree(ptr)	(farfree (ptr))

#else /* _Windows */

// Define the declarations necessary for the Windows 3.0 version of UW/PC //
#define	UWPC_WINDOWS	1

#ifndef	NO_WINDOWS_H
#include <windows.h>
#endif	/* NO_WINDOWS_H */

// Define the replacement for the DOS "delay" function //
void	UWDelay	(unsigned ms);
#define	DELAY_FUNC	UWDelay

// Define some timer values for setting up timers for file transfers.
#define	TIMER_ELAPSED	250		// Elapsed time between timer events.
#define	TIMER_FREQ	4		// Number of timer pulses per second.
#define	TIMER_NUMBER	1		// Value attached to the timer.

// Declare the far memory allocation routines for Windows //
void far *UWAlloc (unsigned size);
void	  UWFree  (void far *ptr);

#endif /* _Windows */

#endif	/* __EXTERN_H__ */
