//-------------------------------------------------------------------------
//
// CLIENTS.CPP - Control routines for the special UW/PC clients.
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
//    1.0    25/07/91  RW  Original Version of CLIENTS.CPP
//
//-------------------------------------------------------------------------

#include "uw.h"			// UW protocol declarations.
#include "display.h"		// Window display declarations.
#include "mail.h"		// Mail client declarations.
#include "login.h"		// DOS login declarations.

#pragma	warn	-par

// Start a client service that was requested by a
// "^[|" escape sequence from the remote host.
void	UWProtocol::startclient (int ch)
{
#ifdef	DOOBERY
  if (ch == 'M')		// Start a mail tool client?
    {
      if (!(new UWMailTool (displays[RoundWindow])))
        {
	  send ('Q');		// Send Q to quit the mail server.
	  send ('\r');
	}
    }
  if (ch == 'L')		// Start a DOS login?
    {
      if (!(new UWLoginTool (displays[RoundWindow])))
	send ('X' & 0x1F);	// Send CTRL-X to cancel the login server.
    }
#endif
  if (ch == 'T')
    {
      // Send the string "stty rows N columns M" //
      UWDisplay *window;
      window = displays[RoundWindow];
      UWMaster.sendstring ("stty rows ");
      if (window -> height >= 100)
        send ('0' + (window -> height / 100));
      if (window -> height >= 10)
        send ('0' + ((window -> height / 10) % 10));
      send ('0' + (window -> height % 10));
      UWMaster.sendstring (" columns ");
      if (window -> width >= 100)
        send ('0' + (window -> width / 100));
      if (window -> width >= 10)
        send ('0' + ((window -> width / 10) % 10));
      send ('0' + (window -> width % 10));
      send ('\r');
    }
} // UWProtocol::startclient //
