//-------------------------------------------------------------------------
//
// LOGIN.CPP - Declarations for the DOS login facility to allow a user to
//	       login to the PC remotely.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1991  Rhys Weatherley
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 1, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Revision History:
// ================
//
//  Version  DD/MM/YY  By  Description
//  -------  --------  --  --------------------------------------
//    1.0    26/07/91  RW  Original Version of LOGIN.CPP
//
//-------------------------------------------------------------------------

#include "login.h"		// Declarations for this module.
#include "client.h"		// Client handling declarations.
#include "uw.h"			// UW protocol declarations.
#include "config.h"		// Configuration routines.
#include <string.h>		// String handling routines.
#include <dir.h>		// Directory handling routines.
#include <ctype.h>		// Character typing macros.

#define	MODE_PASSWORD	0
#define	MODE_COMMAND	1

// Send a string of characters to the remote host.
void	UWLoginTool::sendstr (char *str)
{
  while (*str)
    send (*str++);
} // UWLoginTool::sendstr //

// Process a character meant for the buffer.  Returns
// zero if OK, 1 if '\r' pressed, -1 if ESC pressed.
int	UWLoginTool::charbuf (int ch)
{
  if (ch == '\r' || ch == '\n')
    {
      buffer[posn] = '\0';
      return (1);
    }
   else if (ch == '\033')
    return (-1);
   else if (ch == '\b' && posn > 0)
    {
      --posn;
      if (mode != MODE_PASSWORD)
        sendstr ("\b \b");	// Backspace over last character.
    }
   else if (ch >= ' ' && posn < 79)
    {
      buffer[posn++] = ch;
      if (mode != MODE_PASSWORD)
        send (ch);			// Echo the character.
    }
  return (0);
} // UWLoginTool::charbuf //

// Output the main login menu.
void	UWLoginTool::menu (void)
{
  static char dir[100];
  getcwd (dir,100);
  sendstr ("\r\n");
  sendstr (dir);
  sendstr ("> ");
} // UWLoginTool::menu //

// Execute the current command.
int	UWLoginTool::execute (void)
{
  int index=0;
  while (buffer[index] && isspace (buffer[index]))
    ++index;
  return (1);
} // UWLoginTool::execute //

UWLoginTool::UWLoginTool (UWDisplay *wind)
	: UWClient (wind)
{
  UWMaster.install (this);
  sendstr ("\032Welcome to the UW/PC remote login client\r\n\r\nPassword: ");
  clrbuf ();
  mode = MODE_PASSWORD;
} // UWLoginTool::UWLoginTool //

void	UWLoginTool::key (int keypress)
{
  if (keypress == 033)
    {
      send ('X' & 0x1F);	// Cancel the login session.
      UWMaster.remove ();	// Remove the login client.
    } /* then */
   else
    defkey (keypress);		// Handle the default keys.
} // UWLoginTool::key //

void	UWLoginTool::remote (int ch)
{
  int value;
  switch (mode)
    {
      case MODE_PASSWORD:
      		if ((value = charbuf (ch)) == 0)
		  break;
		 else if (value == 1 && UWConfig.Password[0] &&
		 	  !strcmp (UWConfig.Password,buffer))
		  {
		    sendstr ("\r\nLogin succeeded.\r\n");
		    mode = MODE_COMMAND;
		    menu ();		// Send the login menu prompt.
		    clrbuf ();		// Clear the input buffer.
		  } /* if */
		 else
		  {
		    sendstr ("\r\nInvalid password - goobye!\r\n");
		    send ('X' & 0x1F);
		    UWMaster.remove ();
		  } /* if */
		break;
      case MODE_COMMAND:
      		if ((value = charbuf (ch)) == 0)
		  break;
		 else if (value == 1)
		  {
		    if (!execute ())	// Execute the command.
		      break;
		    menu ();
		    clrbuf ();
		  }
		 else
		  {
		    // Clear the current line when ESC pressed.
		    while (posn > 0)
		      sendstr ("\b \b");
		    clrbuf ();
		  }
		break;
      default:	break;
    } /* switch */
} // UWLoginTool::remote //
