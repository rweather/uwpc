/*-------------------------------------------------------------------------

  DEVICE.H - Links for UW to the actual transmission devices.

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
     1.0    14/12/90  RW  Original Version of DEVICE.H
     1.1    01/01/91  RW  Clean up and remove __PROTO__
     1.2    26/01/91  RW  Add "uw" configuration handling.
     1.3    17/03/91  RW  Add "FixComDevice" for serial port fixups.
     1.4    21/03/91  RW  Add high bit stripping in Protocol 0 and
     			  support for swapping the BS and DEL keys.

-------------------------------------------------------------------------*/

#ifndef __DEVICE_H__
#define	__DEVICE_H__

/* Force C calling conventions if necessary */
#ifdef	__STDC__
#define	_Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* The following variable points to a string description */
/* of the current communications parameters for display. */
extern	char	_Cdecl	DeviceParameters[];

/* Define the configurable communications parameters */
/* The default is COM1, 2400 N-8-1.		     */
extern	int	_Cdecl	ComPort;
extern	int	_Cdecl	ComParams;

/* Define the special modem configuration strings */
/* If ModemCInit is non-zero the initialisation   */
/* string is still sent if a carrier is present.  */
#define	MODEM_STR_SIZE	100	/* Maximum number of chars for strings */
extern	char	_Cdecl	ModemInit[];
extern	int	_Cdecl	ModemCInit;
extern	char	_Cdecl	ModemHang[];
extern	char	_Cdecl	ModemDial[];
extern	char	_Cdecl	UWCommandString[];

/* Define some sundry flags */
extern	int	_Cdecl	StripHighBit;
extern	int	_Cdecl	SwapBackSpaces;

/* Initialise the communications device - returns non-zero if OK */
int	_Cdecl	InitComDevice	(void);

/* Terminate the handling of the communications device */
void	_Cdecl	TermComDevice	(int disconnect);

/* Read the next character from the communications device */
/* Returns the character (0-255), or -1 if not ready.	  */
int	_Cdecl	ReadComDevice	(void);

/* Write a character to the communications device */
void	_Cdecl	WriteComDevice	(int ch);

/* Fix a communications device after a DOS shell-out */
void	_Cdecl	FixComDevice	(void);

/* Test to see if the connection to the remote device exists. */
int	_Cdecl	TestConnection	(void);

/* Send the modem initialisation string to the device */
void	_Cdecl	ModemInitString	(void);

/* Hangup the modem, by sending the necessary control sequences */
void	_Cdecl	ModemHangup	(void);

/* Send the modem dialing string to the modem */
void	_Cdecl	DialModem	(void);

/* Send the string for a function key over the link */
/* for the current window.  func is between 0 and 9 */
void	_Cdecl	SendFunction	(int func);

/* Send a sustained BREAK pulse to the communications device */
void	_Cdecl	DeviceBreak	(void);

/* Send the string for the UW server command name */
/* stored in the variable UWCommandString.	  */
void	_Cdecl	CommandString	(void);

#ifdef	__cplusplus
}
#endif

#endif	/* __DEVICE_H__ */
