/*-------------------------------------------------------------------------

  UW.C - Declarations for the Unix Windows protocol handling routines.

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
     1.0    15/12/90  RW  Original Version of UW.C
     1.1    01/01/91  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#include "uw.h"			/* Declarations for this module */
#include "uwproto.h"		/* Unix Windows protocol declarations */
#include "device.h"		/* Communications device handling */
#include "term.h"		/* Terminal handling routines */
#include <stddef.h>		/* NULL is defined here */

#ifdef	__TURBOC__
#pragma	warn -par		/* Turn off annoying warnings */
#endif

/* Define the list of all window process descriptors */
UWProcess	_Cdecl	UWProcList[NUM_UW_WINDOWS];

/* Define the currently displayed window number */
int	_Cdecl	UWCurrWindow;

/* Define the UW protocol that is currently in use: 0-2 */
int	_Cdecl	UWProtocol;

/* Define some global data for this module */
static	int	_Cdecl	LastWindow;	/* Last window used for input */
static	int	_Cdecl	OutputWindow;	/* Current window for output */
static	int	_Cdecl	GotIAC;	    	/* Non-zero if P1_IAC received */
static	int	_Cdecl	GotMETA;	/* Non-zero if META received */
static	int	_Cdecl	ExitFlag=0;	/* Flag used by "UWExit" */

/* Define some useful macros */
#define	IAC	001
#define XON	021
#define	XOFF	023

/* Output a Unix Windows command to the host server */
static	void	_Cdecl	UWCommand (int command)
{
  WriteComDevice (P1_IAC);	/* Send command from client to host */
  WriteComDevice (P1_DIR_CTOH | command);
} /* UWCommand */

/* Send a character to the server, swapping input windows as necessary */
static	void	_Cdecl	UWToServer (int window,int ch)
{
  if (LastWindow != window)
    {
      LastWindow = window;
      if (window)
	UWCommand (P1_FN_ISELW | window);
    } /* if */
  ch &= 0xFF;			/* Make this program more 8-bit clean */
  if (UWProtocol < 1)
    {
      WriteComDevice (ch);	/* Write directly in protocol 0 */
      return;
    } /* if */
  if (ch & 0x80)		/* Test to see if META should be sent */
    {
      UWCommand (P1_FN_META);
      ch &= 0x7F;
    } /* if */
  switch (ch)
    {
      case IAC:  UWCommand (P1_FN_META | P1_CC_IAC); break;
      case XON:  UWCommand (P1_FN_META | P1_CC_XON); break;
      case XOFF: UWCommand (P1_FN_META | P1_CC_XOFF); break;
      default:	 WriteComDevice (ch); break;
    } /* switch */
} /* UWToServer */

/* Do a process tick for a terminal window */
static	void	_Cdecl	TermTick (int window)
{
  /* If there is a file transfer in progress, call it's tick function */
  if (UWProcList[window].transfer != NULL)
    (*(UWProcList[window].transfer -> tick)) (window);
} /* TermTick */

/* Initialise the UW protocol handling routines */
void	_Cdecl	InitUWProtocol (uwtype_t emul)
{
  int wind;
  UWProtocol = 0;		/* Start off in protocol 0 */
  UWCurrWindow = 0;		/* Current window is a dumb terminal */
  LastWindow = 0;
  OutputWindow = 0;
  GotIAC = 0;
  GotMETA = 0;
  for (wind = 1;wind < NUM_UW_WINDOWS;++wind)
    UWProcList[wind].used = 0;	/* Mark windows 1-7 as unused */
  UWProcList[0].used = 1;	/* Setup the Protocol 0 window */
  UWProcList[0].terminal = 1;
  UWProcList[0].transfer = NULL; /* No file transfer is currently in process */
  UWProcList[0].init = TermInit;
  UWProcList[0].output = TermOutput;
  UWProcList[0].tick = TermTick;
  UWProcList[0].kill = TermKill;
  UWProcList[0].top = TermTop;
  UWProcList[0].key = TermKey;
  (*UWProcList[0].init) (0,emul); /* Initialise the protocol 0 window */
} /* InitUWProtocol */

/* Terminate the UW protocol handling routines */
void	_Cdecl	TermUWProtocol (void)
{
  (*UWProcList[0].kill) (0);	/* Kill the protocol 0 window */
} /* TermUWProtocol */

/* Process a character typed in at the keyboard, sending */
/* it to the host for the current window if necessary.   */
void	_Cdecl	UWProcessChar (int ch)
{
  if (UWProcList[UWCurrWindow].transfer != NULL)
    return;		/* Ignore key if a file transfer is in progress */
  UWProcWindow (UWCurrWindow,ch); /* Send to the current window */
} /* UWProcessChar */

/* Process a character for a particular window */
void	_Cdecl	UWProcWindow (int window,int ch)
{
  if (!UWProcList[window].used)
    return;			/* Abort if window is unused */
  if (!UWProcList[window].terminal && UWProcList[window].transfer == NULL)
    return;			/* Not a terminal/transfer window - ignore */
   else
    UWToServer (window,ch);	/* Send character to the server */
} /* UWProcWindow */

/* Send a special key sequence through the current window */
void	_Cdecl	UWSendKey (int key)
{
  /* Send the key to the terminal emulation as necessary */
  if (UWProcList[UWCurrWindow].terminal)
    (*UWProcList[UWCurrWindow].key) (UWCurrWindow,key);
} /* UWSendKey */

/* Get a new window number - returns number or -1 */
static	int	_Cdecl	NewWindow (void)
{
  int wind=1;
  while (wind < NUM_UW_WINDOWS && UWProcList[wind].used)
    ++wind;
  if (wind >= NUM_UW_WINDOWS)
    return (-1);
   else
    {
      UWProcList[wind].used = 1;
      return (wind);
    } /* if */
} /* NewWindow */

/* Create a new terminal processing window - returns number or -1 */
int	_Cdecl	UWTerminal (uwtype_t emul)
{
  return (UWCreateWindow (TermInit,TermOutput,TermTick,TermKill,
			  TermTop,TermKey,1,emul));
} /* UWTerminal */

/* Create a new window, with specified functions - returns number or -1 */
int	_Cdecl	UWCreateWindow (void _Cdecl (*init) (int window,uwtype_t emul),
			      void _Cdecl (*output) (int window,int ch),
			      void _Cdecl (*tick) (int window),
			      void _Cdecl (*kill) (int window),
			      void _Cdecl (*top) (int window),
			      void _Cdecl (*key) (int window,int key),
			      int terminal,uwtype_t emul)
{
  int wind;
  if (UWProtocol < 1)
    return (-1);		/* Cannot create windows in protocol 0 */
  if ((wind = NewWindow ()) < 0)
    return (-1);		/* No more windows available */
  UWProcList[wind].terminal = terminal; /* Setup the window parameters */
  UWProcList[wind].transfer = NULL; /* No file transfer in progress */
  UWProcList[wind].init = init;
  UWProcList[wind].output = output;
  UWProcList[wind].tick = tick;
  UWProcList[wind].kill = kill;
  UWProcList[wind].top = top;
  UWProcList[wind].key = key;
  UWCommand (P1_FN_NEWW | wind);/* Send "create" message to server */
  (*init) (wind,emul);		/* Initialise the window */
  return (wind);		/* Return the window number for use */
} /* UWCreateWindow */

/* Kill a numbered window if it is in use */
void	_Cdecl	UWKillWindow (int window)
{
  if (UWProtocol < 1 || window < 1 || window >= NUM_UW_WINDOWS ||
      !UWProcList[window].used)
    return;			/* Ignore the kill request */
  UWProcList[window].used = 0;	/* Mark the window as unused */
  (*UWProcList[window].kill) (window); /* Kill the window */
  UWCommand (P1_FN_KILLW | window);
  if (UWCurrWindow == window)
    {
      /* Choose another window to be made the top window */
      int wind=1;
      while (wind < NUM_UW_WINDOWS && !UWProcList[wind].used)
	++wind;
      if (wind >= NUM_UW_WINDOWS)
	{
	  /* Send the exit command - no more windows */
	  UWCommand (P1_FN_MAINT | P1_MF_EXIT);
	  UWProtocol = 0;	/* Return to protocol 0 */
	  (*UWProcList[0].top) (0); /* Bring protocol 0 window to top */
	  UWCurrWindow = 0;
	  LastWindow = 0;
	  OutputWindow = 0;
	} /* then */
       else
	{
	  /* Bring the window to the top and make it current */
	  (*UWProcList[wind].top) (wind);
	  UWCurrWindow = wind;
	} /* else */
    } /* if */
} /* UWKillWindow */

/* Select a new window to be made the "top" window */
void	_Cdecl	UWTopWindow (int window)
{
  if (UWProtocol < 1 || window < 1 || window >= NUM_UW_WINDOWS ||
      !UWProcList[window].used)
    return;			/* Ignore the top request */
  (*UWProcList[window].top) (window); /* Bring the window to the top */
  UWCurrWindow = window;	/* Set the current window number */
} /* UWTopWindow */

/* Do some processing for the protocol handling, and */
/* send "tick" messages to all active windows.	     */
void	_Cdecl	UWTick (void)
{
  int ch;

  /* Send "tick" messages to all windows */
  if (UWProtocol == 0)
    (*UWProcList[0].tick) (0);	/* Just call tick function for window 0 */
   else
    {
      int wind;
      for (wind = 1;wind < NUM_UW_WINDOWS;++wind)
	{
	  if (UWProcList[wind].used)
	    (*UWProcList[wind].tick) (wind);
	} /* for */
    } /* if */

  /* Test to see if the connection to the server has dropped */
  if (UWProtocol > 0 && !TestConnection ())
    {
      ExitFlag = 1;
      UWExit ();	/* Exit protocol 1 - no connection present */
      ExitFlag = 0;
    } /* if */

  /* Get any input characters and process the UW protocol */
  if ((ch = ReadComDevice ()) < 0)
    return;			/* No input characters ready */
  if (UWProtocol > 0)
    ch &= 0x7F;			/* Strip high bit in protocol 1 */
  if (ch == P1_IAC && !GotIAC)
    GotIAC = 1;			/* Record that IAC was received */
   else if (GotIAC)
    {
      int arg,count;
      GotIAC = 0;		/* Reset IAC flag */
      arg = ch & P1_FN_ARG;	/* Get the argument of the function */
      /* Determine the function to be performed */
      switch (ch & P1_FN)
	{
	  case P1_FN_NEWW:
		/* Create a new window for the host */
		if (UWProtocol < 1)
		  break;	/* Cannot create in protocol 0 */
		UWProcList[arg].used = 1; /* Setup window parameters */
		UWProcList[arg].terminal = 1;
		UWProcList[arg].transfer = NULL;
		UWProcList[arg].init = TermInit;
		UWProcList[arg].output = TermOutput;
		UWProcList[arg].tick = TermTick;
		UWProcList[arg].kill = TermKill;
		UWProcList[arg].top = TermTop;
		UWProcList[arg].key = TermKey;
		TermInit (arg,DefTermType); /* Init the terminal window */
		if (!UWCurrWindow)
		  UWTopWindow (arg);/* Display the window first time in */
		break;
	  case P1_FN_KILLW:
		/* Kill a window for the host */
		if (UWProtocol < 1 || !UWProcList[arg].used)
		  break;	/* Cannot kill the window */
		UWKillWindow (arg);/* Kill the window: maybe also exit P1 */
		break;
	  case P1_FN_OSELW:
		/* Select a new output window */
		if (UWProtocol > 0)
		  OutputWindow = arg;
		break;
	  case P1_FN_META:
		/* Turn on the META bit of the next character */
		GotMETA = 1;
		return;		/* Exit "tick" service */
	  case P1_FN_CTLCH:
		/* Decode a control character */
		ch &= P1_CC;
		if (ch == P1_CC_IAC)
		  ch = IAC;
		 else if (ch == P1_CC_XON)
		  ch = XON;
		 else if (ch == P1_CC_XOFF)
		  ch = XOFF;
		 else
		  break;
		if (GotMETA)
		  ch |= 0x80;	/* Set the META bit */
		/* Ignore spurious characters when outputting */
		if (UWProcList[OutputWindow].used)
		  (*UWProcList[OutputWindow].output) (OutputWindow,ch);
		break;
	  case P1_FN_MAINT:
		/* Perform a maintenance function */
		if ((ch & P1_MF) == P1_MF_ENTRY)
		  {
		    /* Reset the client by destroying all windows */
		    ExitFlag = 1;
		    UWExit ();
		    ExitFlag = 0;

		    /* Enter Unix Windows Protocol 1 */
		    UWProtocol = 1;
		  }
		 else if (UWProtocol > 0 && (ch & P1_MF) == P1_MF_EXIT)
		  {
		    ExitFlag = 1;	/* Don't send codes back */
		    UWExit ();		/* Kill all of our windows */
		    ExitFlag = 0;	/* Restore the exit flag */
		  }
		break;
	  default: break;	/* Ignore the function - unknown */
	}
      GotMETA = 0;		/* Reset the META flag for a function */
    } /* then */
   else
    {
      /* Send the character verbatim to the output window */
      if (GotMETA)
	ch |= 0x80;		/* Set the META bit */
      GotMETA = 0;
      /* Ignore spurious characters when outputting */
      if (UWProcList[OutputWindow].used)
	(*UWProcList[OutputWindow].output) (OutputWindow,ch);
    } /* else */
} /* UWTick */

/* Send the "exit" sequence to the UW server and clean up */
/* The "kill" message will be sent to all active windows  */
/* first, to allow graceful exit of running processes.	  */
void	_Cdecl	UWExit (void)
{
  if (UWProtocol > 0)
    {
      int wind;
      for (wind = 1;wind < NUM_UW_WINDOWS;++wind)
	{
	  /* Kill the window, and mark it as unused */
	  if (UWProcList[wind].used)
	    {
	      if (!ExitFlag)
		UWCommand (P1_FN_KILLW | wind);
	      UWProcList[wind].used = 0;
	      (*UWProcList[wind].kill) (wind);
	    } /* if */
	} /* for */
      if (!ExitFlag)
	UWCommand (P1_FN_MAINT | P1_MF_EXIT); /* Send UW "exit" command */
    } /* if */
  UWProtocol = 0;		/* Return to protocol 0 */
  LastWindow = 0;
  OutputWindow = 0;
  (*UWProcList[0].top) (0);	/* Display Window 0 again */
  UWCurrWindow = 0;
} /* UWExit */
