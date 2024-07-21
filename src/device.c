/*-------------------------------------------------------------------------

  DEVICE.C - Links for UW to the actual transmission devices.

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
     1.0    14/12/90  RW  Original Version of DEVICE.C
     1.1    26/01/91  RW  Add "uw" configuration handling.
     1.2    17/03/91  RW  Add "FixComDevice" for serial port fixups.
     1.3    21/03/91  RW  Add high bit stripping in Protocol 0 and
     			  support for swapping the BS and DEL keys.

-------------------------------------------------------------------------*/

#include "device.h"		/* Declarations for this module */
#include "comms.h"		/* Turbo C/C++ COM port routines */
#include "keys.h"		/* Keyboard accessing functions */
#include "uw.h"			/* UW protocol definitions and functions */
#include <dos.h>		/* "delay" is defined here */
#include <stdio.h>		/* Standard I/O routines */

/* The following variable points to a string description */
/* of the current communications parameters for display. */
char	_Cdecl	DeviceParameters[20]="COM1 2400 N-8-1";

/* Define the configurable communications parameters */
/* The default is COM1, 2400 N-8-1.		     */
int	_Cdecl	ComPort=1;
int	_Cdecl	ComParams=BAUD_2400 | BITS_8;

/* Define the special modem configuration strings */
char	_Cdecl	ModemInit[MODEM_STR_SIZE + 1]
		 = "ATZ^M~~~AT S7=45 S0=0 V1 X1^M~";
int	_Cdecl	ModemCInit=1;
char	_Cdecl	ModemHang[MODEM_STR_SIZE + 1] = "~~~+++~~~ATH0^M";
char	_Cdecl	ModemDial[MODEM_STR_SIZE + 1] = "ATDT";
char	_Cdecl	UWCommandString[MODEM_STR_SIZE + 1] = "uw^M";

/* Define some other sundry flags */
int	_Cdecl	StripHighBit=0;
int	_Cdecl	SwapBackSpaces=0;

/* Get the string representation of the communications parameters */
static	void	_Cdecl	GetStringParams (void)
{
  static char *BaudRates[] = {"110","150","300","600","1200","2400",
			      "4800","9600","19200","38400","57600",
			      "115200"};
  static char Parities[] = "NOE?";
  sprintf (DeviceParameters,"COM%d %s %c-%c-%c",
		ComPort,
		BaudRates[ComParams & BAUD_RATE],
		Parities[(ComParams & PARITY_GET) >> 6],
		((ComParams & BITS_8) ? '8' : '7'),
		((ComParams & STOP_2) ? '2' : '1'));
} /* GetStringParams */

/* Initialise the communication device - returns non-zero if OK */
int	_Cdecl	InitComDevice (void)
{
  comenable (ComPort);
  comparams (ComPort,ComParams);
  GetStringParams ();
  delay (10);			/* Synchronise Turbo C's delay timer */
  return (1);
} /* InitComDevice */

/* Terminate the handling of the communications device */
void	_Cdecl	TermComDevice (int disconnect)
{
  comdisable (ComPort,!disconnect);
} /* TermComDevice */

/* Read the next character from the communications device */
/* Returns the character (0-255), or -1 if not ready.	  */
int	_Cdecl	ReadComDevice (void)
{
  return (comreceive (ComPort));
} /* ReadComDevice */

/* Write a character to the communications device */
void	_Cdecl	WriteComDevice (int ch)
{
  comsend (ComPort,ch);
} /* WriteComDevice */

/* Fix a communications device after a DOS shell-out */
void	_Cdecl	FixComDevice	(void)
{
  comfix (ComPort);
} /* FixComDevice */

/* Test to see if the connection to the remote device exists. */
int	_Cdecl	TestConnection (void)
{
  return (comcarrier (ComPort));
} /* TestConnection */

/* Send a modem control string to the communications device */
static	void	_Cdecl	modem (char *str,int direct)
{
  while (*str)
    {
      if (*str == '~')		/* '~' is a half-second delay */
	delay (500);
       else if (*str == '^')	/* '^' is control-code escape */
	{
	  ++str;
	  if (*str == '\0')	/* Abort if unexpected end of string */
	    return;
	  if (direct)
	    comsend (ComPort,(*str) & 0x1F); /* Send the control code */
	   else
	    UWProcessChar ((*str) & 0x1F);   /* Send character thru window */
	} /* then */
       else if (direct)
	comsend (ComPort,*str);	/* Send the character direct */
       else
        UWProcessChar (*str);   /* Send character using UW protocol */
      ++str;
    } /* while */
} /* modem */

/* Send the modem initialisation string to the device */
void	_Cdecl	ModemInitString	(void)
{
  modem (ModemInit,1);
} /* ModemInitString */

/* Hangup the modem, by sending the necessary control sequences */
void	_Cdecl	ModemHangup (void)
{
  comdropdtr (ComPort);		/* Drop DTR signal to hangup modem */
  delay (100);			/* Give a short delay for DTR to take effect */
  comraisedtr (ComPort);	/* Raise the DTR signal again */
  if (comcarrier (ComPort))	/* If not hung-up, send control string */
    modem (ModemHang,1);	/* to try to hangup that way */
} /* ModemHangup */

/* Send the modem dialing string to the modem */
void	_Cdecl	DialModem (void)
{
  modem (ModemDial,1);		/* Send the dialing string */
} /* DialModem */

/* Send the string for a function key over the link */
/* for the current window.  func is between 0 and 9 */
void	_Cdecl	SendFunction (int func)
{
  modem (FunctionKeys[func],0); /* Send the function string through UW */
} /* SendFunction */

/* Send a sustained BREAK pulse to the communications device */
void	_Cdecl	DeviceBreak (void)
{
  combreak (ComPort,1);
  delay (500);
  combreak (ComPort,0);
} /* DeviceBreak */

/* Send the string for the UW server command name */
/* stored in the variable UWCommandString.	  */
void	_Cdecl	CommandString (void)
{
  modem (UWCommandString,0);	/* Send the command name string */
} /* CommandString */
