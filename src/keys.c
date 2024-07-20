/*-------------------------------------------------------------------------

  KEYS.C - Declarations for the keyboard handling for Unix Windows.

    This file is part of UW/PC - a multi-window comms package for the PC.
    Copyright (C) 1990-1991  Rhys Weatherley

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Revision History:
  ================

   Version  DD/MM/YY  By  Description
   -------  --------  --  ----------------------------------------------
     1.0    20/12/90  RW  Original Version of KEYS.C
     1.1    01/01/90  RW  Clean up and remove __PROTO__
     1.2    01/01/91  RW  Add trapping of INT 1B to disable CTRL-BREAK

-------------------------------------------------------------------------*/

#include "keys.h"		/* Declarations for this module */
#include <bios.h>		/* Turbo C/C++ BIOS handling routines */
#include <dos.h>		/* DOS/Interrupt handling routines */

/* Define the strings for the 10 function keys */
char	FunctionKeys[10][FUNC_STR_SIZE + 1] =
	  {"","","","","","","","","",""};

/* Define the saved interrupt address for INT 1B */
static	void	interrupt (far *saveint) ();

/* Interrupt routine to ignore the BREAK routine */
static	void	interrupt IgnoreBreak ()
{
  /* Don't do anything - want to ignore the BREAK */
} /* IgnoreBreak */

/* Initialise the keyboard handling system */
void	_Cdecl	InitKeyboard (void)
{
  saveint = getvect (0x1B);
  setvect (0x1B,IgnoreBreak);
} /* InitKeyboard */

/* Terminate the keyboard handling system */
void	_Cdecl	TermKeyboard (void)
{
  setvect (0x1B,saveint);
} /* TermKeyboard */

/* Return the user's next keypress, or -1 if none available */
/* Returns 0-255 for the ASCII values, or extended keycode. */
int	_Cdecl	GetKeyPress (void)
{
  int key;
  if (!bioskey (1))
    return (-1);		/* There are no keys ready */
  key = bioskey (0);
  if (key & 255)		/* Normal ASCII keystroke */
    return (key & 255);
   else if (key == 0x300)	/* CTRL-@ */
    return (0);
   else if (key == 0)		/* CTRL-BREAK */
    return (BREAK_KEY);
   else if (key == 0x7500)	/* CTRL-END */
    return (BREAK_KEY);
   else if (key == 0x5300)
    return (0x7F);		/* Delete key => ASCII 'DEL' character */
   else
    return (key);		/* Return extended keycode direct */
} /* GetKeyPress */
