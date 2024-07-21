/*-----------------------------------------------------------------------------

   COMMTEST.C - Testing version of the Communications Library.  This simulates
   		the COM ports without actually doing anything.

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
    -------  --------  --  ---------------------------------------------------
      1.0    11/04/91  RW  Original version of COMMTEST.C

-----------------------------------------------------------------------------*/

#include "comms.h"		/* Declarations for this module */

#pragma	warn	-par		/* Turn off annoying warnings */

/* Define the port addresses for the 4 serial ports. */
/* These can be changed by the caller if necessary.  */
int	_Cdecl	comports[4] = {0,0,0,0};

/* Enable a COM port for Interrupt Driven I/O by this module */
void	_Cdecl	comenable (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comenable */

/* Save the current setting of a COM port to be restored later */
/* Call this function before calling 'comenable' on the port.  */
void	_Cdecl	comsave (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comsave */

/* Set the communications parameters for a COM port */
void	_Cdecl	comparams (int port,int params)
{
  /* Nothing to be done here - just ignore the request */
} /* comparams */

/* Disable the interrupt I/O for a COM port */
/* If 'leavedtr' != 0, then leave DTR up    */
void	_Cdecl	comdisable (int port,int leavedtr)
{
  /* Nothing to be done here - just ignore the request */
} /* comdisable */

/* Restore the setting of a COM port - after 'comdisable'    */
/* If 'leavedtr' is non-zero the keep DTR set no matter what */
void	_Cdecl	comrestore (int port,int leavedtr)
{
  /* Nothing to be done here - just ignore the request */
} /* comrestore */

/* Turn a COM port's test loop-back mode on or off */
/* NOTE: comenable always turns the loop-mode off  */
void	_Cdecl	comtest (int port,int on)
{
  /* Nothing to be done here - just ignore the request */
} /* comtest */

/* Return the number of available input chars on a COM port */
/* Will raise the DTR signal if it is not already raised.   */
int	_Cdecl	comavail (int port)
{
  return (0);		/* Nothing is ever received by these routines */
} /* comavail */

/* Flush all input from the COM port */
void	_Cdecl	comflush (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comflush */

/* Receive a character from the COM port: -1 if not ready */
/* Will raise the DTR signal if it is not already raised. */
int	_Cdecl	comreceive (int port)
{
  return (-1);		/* Return "no character available" signal */
} /* comreceive */

/* Test to see if the COM port is ready for a new */
/* character to transmit.  Will raise the DTR     */
/* signal if it is not already raised.		  */
int	_Cdecl	comready (int port)
{
  return (1);		/* Return that the COM port is always ready */
} /* comready */

/* Output a character to a COM port.  Will raise */
/* the DTR signal if it is not already raised.   */
void	_Cdecl	comsend (int port,int ch)
{
  /* Nothing to be done here - just ignore the request */
} /* comsend */

/* Test to see if a carrier is present - can be called before comenable */
int	_Cdecl	comcarrier (int port)
{
  return (1);		/* Imitate an available carrier signal */
} /* comcarrier */

/* Test to see if the DSR (Data Set Ready) signal is present */
int	_Cdecl	comdsr (int port)
{
  return (1);		/* Imitate an available DSR signal */
} /* comdsr */

/* Drop the DTR signal on a COM port */
void	_Cdecl	comdropdtr (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comdropdtr */

/* Raise the DTR signal on a COM port */
void	_Cdecl	comraisedtr (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comraisedtr */

/* Set the BREAK pulse on a COM port to 0 or 1 */
void	_Cdecl	combreak (int port,int value)
{
  /* Nothing to be done here - just ignore the request */
} /* combreak */

/* Restore a COM port after a DOS shell-out, since */
/* a program may have disabled interrupts, etc.    */
void	_Cdecl	comfix (int port)
{
  /* Nothing to be done here - just ignore the request */
} /* comfix */
