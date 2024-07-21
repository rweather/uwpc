//-------------------------------------------------------------------------
//
// TIMER.H - Timer declarations for UW/PC.
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
//    1.0    07/04/91  RW  Original Version of TIMER.H
//
//-------------------------------------------------------------------------

#ifndef __TIMER_H__
#define	__TIMER_H__

//
// Define various system timer values for UW/PC.
//
extern	int	CurrentTime;	// Current 24-hour time in minutes.
extern	int	OnlineTime;	// Time in minutes since logon (mod 6000).
extern	int	TimerCount;	// Incremented every 18.2th of a second.

//
// Initialise the system timer routines.
//
void	InitTimers	(void);

//
// Terminate the system timer routines.
//
void	TermTimers	(void);

//
// Reset the current online time to zero.
//
void	ResetOnline	(void);

#endif	/* __TIMER_H__ */
