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
#include <ctype.h>		// Character typing macros.

#pragma	warn	-par

//
// Define a client to get scroll-back buffer sizes.
//
class	UWScrollBackSize : public UWClient {

private:

	int	scrollsize;	// Current computed scroll-back size.

public:

	// Create a client that is attached to a particular
	// display window.
	UWScrollBackSize (UWDisplay *wind);

	// Retrieve the name of the client (terminal type, etc).
	virtual	char far *name	()
		  { if (underneath)
		      return (underneath -> name ());
		     else
		      return (UWClient::name ());
		  };

	// Process a user's keypress.  This will only be called
	// if this client is using the top-most displayed window.
	virtual	void	key	(int keypress)
		  { if (underneath)
		      underneath -> key (keypress);
		     else
		      UWClient::key (keypress);
		  };

	// Process a character from the remote server.  This may
	// be called at any time while the client is active.
	virtual	void	remote	(int ch);

};

// Create a client that is attached to a particular
// display window.
UWScrollBackSize::UWScrollBackSize (UWDisplay *wind)
	: UWClient (wind)
{
  UWMaster.install (this);
  scrollsize = -1;
} // UWScrollBackSize::UWScrollBackSize //

// Process a character for the scroll-back size retrieval.
void	UWScrollBackSize::remote (int ch)
{
  if (isdigit (ch))
    {
      // We have another digit - add it to the computed size //
      if (scrollsize < 0)
        scrollsize = 0;
      scrollsize = scrollsize * 10 + (ch - '0');
    }
   else if (ch == '\n')
    {
      // We now have the scroll-back size: set it and remove this client //
      UWMaster.remove ();
    }
   else if (!isspace (ch))
    UWMaster.remove ();
} // UWScrollBackSize::remote //

//
// Define a client to get a terminal emulation type.
//
#define	MAX_TERM_TYPE_NAME_SIZE		20
class	UWTermEmulType : public UWClient {

private:

	char	termname[MAX_TERM_TYPE_NAME_SIZE];
	int	namelen;

public:

	// Create a client that is attached to a particular
	// display window.
	UWTermEmulType (UWDisplay *wind);

	// Retrieve the name of the client (terminal type, etc).
	virtual	char far *name	()
		  { if (underneath)
		      return (underneath -> name ());
		     else
		      return (UWClient::name ());
		  };

	// Process a user's keypress.  This will only be called
	// if this client is using the top-most displayed window.
	virtual	void	key	(int keypress)
		  { if (underneath)
		      underneath -> key (keypress);
		     else
		      UWClient::key (keypress);
		  };

	// Process a character from the remote server.  This may
	// be called at any time while the client is active.
	virtual	void	remote	(int ch);

};

// Create a client that is attached to a particular
// display window.
UWTermEmulType::UWTermEmulType (UWDisplay *wind)
	: UWClient (wind)
{
  UWMaster.install (this);
  namelen = 0;
} // UWTermEmulType::UWTermEmulType //

// Process a character for the terminal emulation type.
void	UWTermEmulType::remote (int ch)
{
  if (ch == '\n')
    {
      // We now have the terminal type: set it and remove this client //
      termname[namelen] = '\0';
      UWMaster.remove ();
    }
   else if (!isspace (ch))
    {
      // Add a new character to the emulation name //
      if (namelen < (MAX_TERM_TYPE_NAME_SIZE - 1))
        termname[namelen++] = ch;
    }
} // UWTermEmulType::remote //

// Send a string to the remote host after converting to lower case.
void	sttysendlower (char far *str)
{
  static char newstr[20];
  int index=0;
  while (*str)
    {
      if (isupper (*str))
        newstr[index++] = tolower (*str);
       else
        newstr[index++] = *str;
      ++str;
    }
  newstr[index] = '\0';
  UWMaster.sendstring (newstr);
} // sttysendlower //

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
  if (ch == 'P')		// Pop-up the current window?
    {
      top (RoundWindow);
    }
   else if (ch == 'T')
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
   else if (ch == 't')
    {
      // Send back a terminal type command for Bourne shell-like shells //
      UWMaster.sendstring ("TERM=");
      sttysendlower (clients[RoundWindow] -> name ());
      UWMaster.sendstring (" export TERM\r");
    }
   else if (ch == 'u')
    {
      // Send back a terminal type command for C-Shell like shells //
      UWMaster.sendstring ("setenv TERM ");
      sttysendlower (clients[RoundWindow] -> name ());
      send ('\r');
    }
   else if (ch == 'S')
    {
      // Set the size of the window's scroll-back buffer //
      if (displays[RoundWindow])
        new UWScrollBackSize (displays[RoundWindow]);
    }
   else if (ch == 'E')
    {
      // Set the terminal emulation to the remote system's value //
      if (displays[RoundWindow])
        new UWTermEmulType (displays[RoundWindow]);
    }
} // UWProtocol::startclient //
