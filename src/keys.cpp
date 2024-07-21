/*-------------------------------------------------------------------------

  KEYS.CPP - Declarations for the keyboard handling for Unix Windows.

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
     1.0    20/12/90  RW  Original Version of KEYS.CPP
     1.1    01/01/91  RW  Clean up and remove __PROTO__
     1.2    01/01/91  RW  Add trapping of INT 1B to disable CTRL-BREAK
     1.3    23/01/91  RW  Convert to C++ for Version 2.00 of UW/PC.
     1.4    05/05/91  RW  Process function key expansions.

-------------------------------------------------------------------------*/

#include "keys.h"		/* Declarations for this module */
#include "config.h"		/* Configuration routines for UW/PC */
#include <bios.h>		/* Turbo C/C++ BIOS handling routines */
#include <dos.h>		/* DOS/Interrupt handling routines */
#include <ctype.h>		/* Character typing macros */

/*
 * Global declarations for function key expansions.
 */
static	char	*ExpandKey;
static	int	AltKeys[26] =
	  {0x1E00,0x3000,0x2E00,0x2000,0x1200,0x2100,0x2200,0x2300,
	   0x1700,0x2400,0x2500,0x2600,0x3200,0x3100,0x1800,0x1900,
	   0x1000,0x1300,0x1F00,0x1400,0x1600,0x2F00,0x1100,0x2D00,
	   0x1500,0x2C00};
#define	FIRST_DIGIT	0x7800

/* Define the saved interrupt address for INT 1B */
static	void	interrupt (far *saveint) (...);

/* Interrupt routine to ignore the BREAK routine */
static	void	interrupt IgnoreBreak (...)
{
  /* Don't do anything - want to ignore the BREAK */
} /* IgnoreBreak */

/* Initialise the keyboard handling system */
void	InitKeyboard (void)
{
  saveint = getvect (0x1B);
  setvect (0x1B,IgnoreBreak);
  ExpandKey = 0;
} /* InitKeyboard */

/* Terminate the keyboard handling system */
void	TermKeyboard (void)
{
  setvect (0x1B,saveint);
} /* TermKeyboard */

/* Return the user's next keypress, or -1 if none available */
/* Returns 0-255 for the ASCII values, or extended keycode. */
int	GetKeyPress (void)
{
  int key;
  while (1)
    {
      if (ExpandKey)
        {
	  switch (*ExpandKey)
	    {
	      case '\0':break;	/* At end of expansion */
	      case '~':	++ExpandKey;
	      		return (PAUSE_KEY);
	      case '^':	++ExpandKey;
	      		if (*ExpandKey)
			  return ((*ExpandKey++) & 0x1F);
			break;
	      case '#':	++ExpandKey;
	      		if (isdigit (*ExpandKey) && *ExpandKey != '0')
			  return (FIRST_DIGIT + (((*ExpandKey++) - '1') << 8));
			 else if (isalpha (*ExpandKey))
			  return (AltKeys[((*ExpandKey++) & 0xDF) - 'A']);
			 else if (*ExpandKey)
			  return (*ExpandKey++);
			break;
	      default:	return (*ExpandKey++);
	    }
          ExpandKey = 0;	/* Function key expansion is finished */
	}
      if (!bioskey (1))
        return (-1);		/* There are no keys ready */
      key = bioskey (0);
      if (key & 255)		/* Normal ASCII keystroke */
        return (key & 255);
       else if (key == 0x300)	/* CTRL-@ */
        return (0);
       else if (key == 0)	/* CTRL-BREAK */
        return (BREAK_KEY);
       else if (key == 0x7500)	/* CTRL-END */
        return (BREAK_KEY);
       else if (key == 0x8100)	/* ALT-0 -> ALT-W */
        return (NEXTWIN_KEY);
       else if (key == 0x5300)
        return (0x7F);		/* Delete key => ASCII 'DEL' character */
       else if (key == 0x9200)	/* CTRL-INSERT -> ALT-C */
        return (CUT_KEY);
       else if (key >= 0x3B00 && key <= 0x4400 &&
       		UWConfig.FKeys[(key >> 8) - 0x3B][0] != '\0')
        ExpandKey = UWConfig.FKeys[(key >> 8) - 0x3B];
       else
        return (key);		/* Return extended keycode direct */
    }
} /* GetKeyPress */
