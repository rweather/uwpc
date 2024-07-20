/*-------------------------------------------------------------------------

  TERM.H - Terminal handling routines for the Unix Windows protocol.

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
   -------  --------  --  --------------------------------------
     1.0    15/12/90  RW  Original Version of TERM.H
     1.1    01/01/91  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#ifndef __TERM_H__
#define	__TERM_H__

#include "uwlib.h"		/* UW library declarations */

/* Force C calling conventions */
#ifdef	__STDC__
#define	_Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Define the configurable terminal types to use for windows */
extern	uwtype_t _Cdecl	DefTermType;
extern	uwtype_t _Cdecl	Def0TermType;

/* The following variable is non-zero to allow the terminal beep sound */
extern	int	_Cdecl	AllowBeep;

/* Initialise a terminal window for use by UW */
/* If 'window' == 0, also setup this module.  */
void	_Cdecl	TermInit	(int window,uwtype_t emul);

/* Output a character to a terminal window */
void	_Cdecl	TermOutput	(int window,int ch);

/* Kill a terminal window - no longer required */
/* If 'window' == 0, clean everything up.      */
void	_Cdecl	TermKill	(int window);

/* Bring a terminal window to the top on the screen */
void	_Cdecl	TermTop		(int window);

/* Translate keypresses according to the terminal type */
/* Calls UWProcWindow to send keycodes as necessary.   */
void	_Cdecl	TermKey		(int window,int key);

/* Save the screen details and jump to a DOS shell */
void	_Cdecl	JumpToDOS	(void);

/* Pop-up the help screen, and wait for the user to press a key */
void	_Cdecl	HelpScreen	(void);

/* Pop-up a box and wait for one of a set of keystrokes */
/* Returns the key that was pressed.			*/
int	_Cdecl	PopupBox	(char *message,char *keys);

/* Prompt the user for a string on the screen.  Returns */
/* non-zero if OK, or zero if ESC was pressed.  The     */
/* previous contents of the buffer may be edited.  The	*/
/* buffer must be 'len' + 1 bytes in length at least.	*/
int	_Cdecl	PromptUser	(char *prompt,char *buf,int len);

#ifdef	__cplusplus
}
#endif

#endif	/* __TERM_H__ */
