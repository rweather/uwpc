/*-----------------------------------------------------------------------------

   COMMS.H - Serial Communications Routines for Turbo C/C++

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
      1.0    ??/09/90  RW  Original version of COMMS.H
      1.1    10/11/90  RW  Added ability to call comcarrier before comenable,
      			   support for 57600 baud and automatic raising of
			   the DTR signal.
      1.2    17/11/90  RW  Add 'leavedtr' parameter to "comrestore".
      1.3    17/03/91  RW  Create 'comfix' to fix DOS shell-out bug.

-----------------------------------------------------------------------------*/

#ifndef	__COMMS_H__
#define	__COMMS_H__

/* Defines for various port parameters */

#define BAUD_RATE	0x0F		/* Mask to extract baud rate */
#define	BAUD_110	0		/* Baud rates */
#define	BAUD_150	1
#define BAUD_300	2
#define	BAUD_600	3
#define	BAUD_1200	4
#define BAUD_2400	5
#define	BAUD_4800	6
#define	BAUD_9600	7
#define	BAUD_19200	8
#define BAUD_38400	9
#define BAUD_57600	10
#define	BAUD_115200	11

#define	BITS_7		0x00		/* Data bits */
#define	BITS_8		0x10

#define STOP_1		0x00		/* Stop bits */
#define STOP_2		0x20

#define PARITY_GET	0xC0		/* Mask to extract parity */
#define PARITY_NONE	0x00		/* Parity modes */
#define	PARITY_ODD	0x40
#define PARITY_EVEN	0x80

#ifdef	__STDC__
#define	_Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Enable a COM port for Interrupt Driven I/O by this module */
void	_Cdecl	comenable (int port);

/* Save the current setting of a COM port to be restored later */
/* Call this function before calling 'comenable' on the port.  */
void	_Cdecl	comsave (int port);

/* Set the communications parameters for a COM port */
void	_Cdecl	comparams (int port,int params);

/* Disable the interrupt I/O for a COM port */
/* If 'leavedtr' != 0, then leave DTR up    */
void	_Cdecl	comdisable (int port,int leavedtr);

/* Restore the setting of a COM port - after 'comdisable'    */
/* If 'leavedtr' is non-zero the keep DTR set no matter what */
void	_Cdecl	comrestore (int port,int leavedtr);

/* Turn a COM port's test loop-back mode on or off */
/* NOTE: comenable always turns the loop-mode off  */
void	_Cdecl	comtest (int port,int on);

/* Return the number of available input chars on a COM port */
/* Will raise the DTR signal if it is not already raised.   */
int	_Cdecl	comavail (int port);

/* Flush all input from the COM port */
void	_Cdecl	comflush (int port);

/* Receive a character from the COM port: -1 if not ready */
/* Will raise the DTR signal if it is not already raised. */
int	_Cdecl	comreceive (int port);

/* Test to see if the COM port is ready for a new */
/* character to transmit.  Will raise the DTR     */
/* signal if it is not already raised.		  */
int	_Cdecl	comready (int port);

/* Output a character to a COM port.  Will raise */
/* the DTR signal if it is not already raised.   */
void	_Cdecl	comsend (int port,int ch);

/* Test to see if a carrier is present - can be called before comenable */
int	_Cdecl	comcarrier (int port);

/* Test to see if the DSR (Data Set Ready) signal is present */
int	_Cdecl	comdsr (int port);

/* Drop the DTR signal on a COM port */
void	_Cdecl	comdropdtr (int port);

/* Raise the DTR signal on a COM port */
void	_Cdecl	comraisedtr (int port);

/* Set the BREAK pulse on a COM port to 0 or 1 */
void	_Cdecl	combreak (int port,int value);

/* Restore a COM port after a DOS shell-out, since */
/* a program may have disabled interrupts, etc.    */
void	_Cdecl	comfix (int port);

#ifdef	__cplusplus
}
#endif

#endif	/* __COMMS_H__ */
