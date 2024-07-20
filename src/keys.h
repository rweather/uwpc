/*-------------------------------------------------------------------------

  KEYS.H - Declarations for the keyboard handling for Unix Windows.

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
     1.0    20/12/90  RW  Original Version of KEYS.H
     1.1    01/01/91  RW  Clean up and remove __PROTO__
     1.2    01/01/91  RW  Add trapping of INT 1B to disable CTRL-BREAK

-------------------------------------------------------------------------*/

#ifndef __KEYS_H__
#define	__KEYS_H__

/* Force C calling conventions */
#ifdef	__STDC__
#define _Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Define some special key codes that will be returned from GetKeyPress */
#define ALT_WIND_NUM(n)		((0x77 + (n)) << 8)
#define ALT_GET_NUM(key)	(((key >> 8) & 255) - 0x77)
#define QUIT_KEY		0x2D00	/* ALT-X */
#define QUIT_KEY2		0x1000	/* ALT-Q */
#define HELP_KEY		0x2C00	/* ALT-Z */
#define EXIT_KEY		0x1200	/* ALT-E - exit UW */
#define HANGUP_KEY		0x2300	/* ALT-H */
#define BREAK_KEY		0x3000	/* ALT-B */
#define NEW_KEY			0x3100	/* ALT-N */
#define KILL_KEY		0x2500	/* ALT-K */
#define JUMP_DOS_KEY		0x2400	/* ALT-J */
#define INIT_KEY		0x1700	/* ALT-I */
#define START_KEY		0x1600	/* ALT-U */
#define UPLOAD_KEY		0x1F00	/* ALT-S */
#define DOWNLOAD_KEY		0x1300	/* ALT-R */
#define CURSOR_UP		0x4800
#define CURSOR_DOWN		0x5000
#define CURSOR_LEFT		0x4B00
#define CURSOR_RIGHT		0x4D00
#define CURSOR_HOME		0x4700
#define CURSOR_END		0x4F00
#define CURSOR_PGUP		0x4900
#define CURSOR_PGDN		0x5100
#define CURSOR_INS		0x5200
#define CURSOR_DEL		0x7F	/* This module translates this key */
#define DIAL_KEY		0x2000	/* ALT-D */

/* Define the strings for the 10 function keys */
#define	FUNC_STR_SIZE	50
extern	char	FunctionKeys[10][FUNC_STR_SIZE + 1];

/* Initialise the keyboard handling system */
void	_Cdecl	InitKeyboard	(void);

/* Terminate the keyboard handling system */
void	_Cdecl	TermKeyboard	(void);

/* Return the user's next keypress, or -1 if none available */
/* Returns 0-255 for the ASCII values, or extended keycode. */
int	_Cdecl	GetKeyPress	(void);

#ifdef	__cplusplus
}
#endif

#endif	/* __KEYS_H__ */
