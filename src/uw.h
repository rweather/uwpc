/*-------------------------------------------------------------------------

  UW.H - Declarations for the Unix Windows protocol handling routines.

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
     1.0    15/12/90  RW  Original Version of UW.H
     1.1    01/01/91  RW  Clean up code and remove __PROTO__

-------------------------------------------------------------------------*/

#ifndef __UW_H__
#define	__UW_H__

#include "uwlib.h"		/* UW library declarations */

/* Force C calling conventions */
#ifdef	__STDC__
#define _Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Define the structure of a window process descriptor, */
/* defining the process to be run within a window.	*/
typedef	struct	{
		  int used;		/* Non-zero if in use */
		  int terminal;		/* Non-zero for terminal windows */

		  /* Initialise the window handling for the window */
		  void	_Cdecl	(*init) (int window,uwtype_t emul);

		  /* Output a character to the window */
		  void	_Cdecl	(*output) (int window,int ch);

		  /* Send a time slice tick to the window process */
		  void	_Cdecl	(*tick) (int window);

		  /* Kill the window - no longer required */
		  void	_Cdecl	(*kill) (int window);

		  /* Make the window the top-most (current) window */
		  void	_Cdecl	(*top) (int window);

		  /* Translate keypresses according to the terminal type */
		  /* Calls UWProcWindow to send keycodes as necessary.   */
		  void	_Cdecl	(*key) (int window,int key);

		} UWProcess;

/* Define the list of all window process descriptors */
#define	NUM_UW_WINDOWS		8	/* Window 0 is Protocol 0 terminal */
extern	UWProcess _Cdecl UWProcList[];

/* Define the currently displayed window number */
extern	int	_Cdecl	UWCurrWindow;

/* Define the UW protocol that is currently in use: 0-2 */
extern	int	_Cdecl	UWProtocol;

/* Initialise the UW protocol handling routines */
void	_Cdecl	InitUWProtocol	(uwtype_t emul);

/* Terminate the UW protocol handling routines */
void	_Cdecl	TermUWProtocol	(void);

/* Process a character typed in at the keyboard, sending */
/* it to the host for the current window if necessary.   */
void	_Cdecl	UWProcessChar	(int ch);

/* Process a character for a particular window */
void	_Cdecl	UWProcWindow	(int window,int ch);

/* Send a special key sequence through the current window */
void	_Cdecl	UWSendKey	(int key);

/* Create a new terminal processing window - returns number or -1 */
int	_Cdecl	UWTerminal	(uwtype_t emul);

/* Create a new window, with specified functions - returns number or -1 */
int	_Cdecl	UWCreateWindow (void _Cdecl (*init) (int window,uwtype_t emul),
			      void _Cdecl (*output) (int window,int ch),
			      void _Cdecl (*tick) (int window),
			      void _Cdecl (*kill) (int window),
			      void _Cdecl (*top) (int window),
			      void _Cdecl (*key) (int window,int key),
			      int terminal,uwtype_t emul);

/* Kill a numbered window if it is in use */
void	_Cdecl	UWKillWindow	(int window);

/* Select a new window to be made the "top" window */
void	_Cdecl	UWTopWindow	(int window);

/* Do some processing for the protocol handling, and */
/* send "tick" messages to all active windows.	     */
void	_Cdecl	UWTick		(void);

/* Send the "exit" sequence to the UW server and clean up */
/* The "kill" message will be sent to all active windows  */
/* first, to allow graceful exit of running processes.	  */
void	_Cdecl	UWExit		(void);

#ifdef	__cplusplus
}
#endif

#endif	/* __UW_H__ */
