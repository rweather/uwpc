//-------------------------------------------------------------------------
//
// CLIENT.CPP - Declarations for creating UW clients within UW/PC.
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
//    1.0    20/03/91  RW  Original Version of CLIENT.CPP
//
//-------------------------------------------------------------------------

#include "client.h"		// Declarations for this module
#include "display.h"		// Window display declarations
#include "config.h"		// Configuration routines
#include "uw.h"			// Main UW protocol declarations
#include "comms.h"		// Direct serial comms routines

// Process a user's keypress.  This will only be called
// if this client is using the top-most displayed window.
void	UWTerminal::key (int keypress)
{
  if ((keypress == 021 || keypress == 023) && !UWConfig.XonXoffFlag)
    comsend (UWConfig.ComPort,keypress); // Send XON/XOFF direct.
   else if (keypress == 8 && UWConfig.SwapBSKeys)
    send (127);			// Swap BS and DEL.
   else if (keypress == 127 && UWConfig.SwapBSKeys)
    send (8);
   else if (keypress >= 0 && keypress <= 255)
    send (keypress);		// Send the ASCII key over the serial line.
   else
    defkey (keypress);		// Process the keypress in the default way.
} // UWTerminal::key //

// Process a character from the remote server.  This may
// be called at any time while the client is active.
void	UWTerminal::remote (int ch)
{
  switch (ch)
    {
      case '\0': break;				// Ignore NULs
      case '\r': window -> cr (); break;
      case '\n': window -> lf (); break;
      case '\b': window -> bs (); break;
      case '\t': window -> tab (); break;
      case  007: window -> bell (); break;
      default:	 window -> send (ch,0); break;	// Send character directly
    } /* switch */
} // UWTerminal::remote //
