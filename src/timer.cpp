//-------------------------------------------------------------------------
//
// TIMER.CPP - Timer declarations for UW/PC.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1992  Rhys Weatherley
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
//    1.0    07/04/91  RW  Original Version of TIMER.CPP
//    1.1    24/04/92  RW  Add CurrSystemTime call.
//
//-------------------------------------------------------------------------

#include "timer.h"		// Declarations for this module.
#include <dos.h>		// DOS/BIOS manipulation routines.

//
// Define various system timer values for UW/PC.
//
int	CurrentTime;	// Current 24-hour time in minutes.
int	OnlineTime;	// Time in minutes since logon (mod 6000).
int	TimerCount;	// Incremented every 18.2th of a second.

//
// Global data needed for this module.
//
static	void	interrupt (far *saveint) (...);
static	int	OnlineOffset;

//
// Define the number of system ticks per minute.  The actual
// value is 1092.266666, but this is close enough for UW/PC.
//
#define	TICKS_PER_MINUTE	1092

//
// Get the current time from the BIOS data area.
//
#define	SYSTEM_TIME	(*((long far *)0x46C))

//
// Convert a system time to a number of minutes.
//
#define	TO_MINUTES(x)	((x) / TICKS_PER_MINUTE)

// Interrupt routine to intercept system timer ticks.
static	void	interrupt TimerInt (...)
{
  (*saveint) ();		// Call old timer routine.
  enable ();			// Enable interrupts.
  ++TimerCount;
  CurrentTime = TO_MINUTES (SYSTEM_TIME) % 1440;
  ++OnlineOffset;
  if (OnlineOffset > TICKS_PER_MINUTE)
    {
      OnlineTime = (OnlineTime + 1) % 6000;
      OnlineOffset = 0;
    }
} // TimerInt //

//
// Initialise the system timer routines.
//
void	InitTimers (void)
{
  saveint = getvect (0x1C);
  setvect (0x1C,TimerInt);
  CurrentTime = TO_MINUTES (SYSTEM_TIME) % 1440;
  TimerCount = 0;
  ResetOnline ();
} // InitTimers //

//
// Terminate the system timer routines.
//
void	TermTimers (void)
{
  setvect (0x1C,saveint);
} // TermTimers //

//
// Reset the current online time to zero.
//
void	ResetOnline (void)
{
  OnlineTime = 0;
  OnlineOffset = 0;
} // ResetOnline //

//
// Get the current system time as a number of minutes since midnight.
// This uses operating system calls rather than timer ticks to get
// the time more accurately.
//
int	CurrSystemTime (void)
{
  union REGS regs;
  regs.h.ah = 0x2C;
  int86 (0x21,&regs,&regs);	// Call DOS to get the current time.
  return (regs.h.ch * 60 + regs.h.cl);
} // CurrSystemTime //
