//-------------------------------------------------------------------------
//
// CLIPBD.CPP - Clipboard processing routines for UW/PC.
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
//    1.0    08/06/91  RW  Original Version of CLIPBD.CPP
//
//-------------------------------------------------------------------------

#include "clipbd.h"		// Declarations for this module.
#include "client.h"		// Client processing declarations.
#include "display.h"		// Display processing routines.
#include "keys.h"		// Keyboard declarations.
#include "uw.h"			// Master protocol routines.
#include "mouse.h"		// Mouse handling routines.
#include <dos.h>		// "delay" is defined here.

//
// Define the global data for the clipboard.
//
#define	CLIPBOARD_SIZE	(81 * 24)
static	char	Clipboard[CLIPBOARD_SIZE];
static	int	ClipLength=0;	// Length of the clipboard.
static	int	ClipActive=0;	// Non-zero for an active clipboard.
static	int	ClipPosn;	// Current paste position.

UWCutToClipboard::UWCutToClipboard (UWDisplay *wind,int mouse,int x,int y) :
	UWClient (wind)
{
  UWMaster.install (this);	// Install this client in the client stack.
  usemouse = mouse;		// Save the mouse usage flag for later.
  if (usemouse)
    {
      x1 = x;				// Start at current mouse position.
      y1 = y;
      corner = 1;
    } /* then */
   else
    {
      x1 = (window -> width) / 2;	// Start at screen middle for keyboard.
      y1 = (window -> height) / 2;
      corner = 0;
    } /* else */
  x2 = x1;
  y2 = y1;
  window -> markcut (x1,y1,x2,y2); // Mark the original cut position.
} // UWCutToClipboard::UWCutToClipboard //

UWCutToClipboard::~UWCutToClipboard (void)
{
  if (corner != -1)
    window -> markcut (x1,y1,x2,y2); // Remove the mark from the screen.
} // ~UWCutToClipboard::UWCutToClipboard //

void	UWCutToClipboard::key (int keypress)
{
  int changex=0;
  int changey=0;
  if (usemouse)			// Ignore keyboard if using the mouse.
    return;
  switch (keypress)
    {
      case 033: window -> markcut (x1,y1,x2,y2);
      		corner = -1;
      		UWMaster.remove (); break;
      case 13:	if (corner == 0)
      		  ++corner;		// Move to the other corner.
		 else
		  {
		    // Copy the screen data to the clipboard and exit //
		    ClipLength = window -> copycut (x1,y1,x2,y2,Clipboard);
		    window -> markcut (x1,y1,x2,y2);
		    corner = -1;
		    UWMaster.remove (); break;
		  }
		break;
      case CURSOR_UP:
      		if (y2 > 0)
		  changey = -1;
		break;
      case CURSOR_DOWN:
      		if (y2 < (window -> height - 1))
		  changey = 1;
		break;
      case CURSOR_LEFT:
      		if (x2 > 0)
		  changex = -1;
		break;
      case CURSOR_RIGHT:
      		if (x2 < (window -> width - 1))
		  changex = 1;
		break;
      default:	defkey (keypress);
      		break;
    }
  if (changex || changey)
    {
      if (corner == 0)
        {
          window -> markcut (x1,y1,x2,y2);	// Remove the current mark.
          x2 += changex;			// Move the amount required.
          y2 += changey;
	  x1 = x2;
          y1 = y2;
          window -> markcut (x1,y1,x2,y2);	// Set the new mark.
	}
       else
        {
	  // We just want to mark/unmark the sides that change //
	  if (changex > 0 && x2 >= x1)
	    window -> markcut (x2 + 1,y1,x2 + changex,y2);
	   else if (changex > 0 && x2 < x1)
	    window -> markcut (x2,y1,x2 + changex - 1,y2);
	   else if (changex < 0 && x2 > x1)
	    window -> markcut (x2,y1,x2 + changex + 1,y2);
	   else if (changex < 0 && x2 <= x1)
	    window -> markcut (x2 + changex,y1,x2 - 1,y2);
	  x2 += changex;
	  if (changey > 0 && y2 >= y1)
	    window -> markcut (x1,y2 + 1,x2,y2 + changey);
	   else if (changey > 0 && y2 < y1)
	    window -> markcut (x1,y2,x2,y2 + changey - 1);
	   else if (changey < 0 && y2 > y1)
	    window -> markcut (x1,y2,x2,y2 + changey + 1);
	   else if (changey < 0 && y2 <= y1)
	    window -> markcut (x1,y2 + changey,x2,y2 - 1);
	  y2 += changey;
	}
    }
} // UWCutToClipboard::key //

void	UWCutToClipboard::remote (int ch)
{
  window -> markcut (x1,y1,x2,y2);	// Remove the current mark.
  if (underneath)
    underneath -> remote (ch);
  window -> markcut (x1,y1,x2,y2);	// Put the mark back.
} // UWCutToClipboard::remote //

void	UWCutToClipboard::mouse (int x,int y,int buttons)
{
  if (!usemouse)
    return;				// Not using the mouse - ignore.
  if (buttons & MOUSE_LEFT)
    {
      // Move the bottom right corner to the mouse position */
      if (y >= window -> height)
	y = (window -> height) - 1;
      window -> markcut (x1,y1,x2,y2);
      x2 = x;
      y2 = y;
      window -> markcut (x1,y1,x2,y2);
    } /* then */
   else
    {
      // Copy the screen data to the clipboard and exit //
      ClipLength = window -> copycut (x1,y1,x2,y2,Clipboard);
      window -> markcut (x1,y1,x2,y2);
      corner = -1;
      UWMaster.remove ();
    } /* else */
} // UWCutToClipboard::mouse //

UWPasteFromClipboard::UWPasteFromClipboard (UWDisplay *wind)
	: UWClient (wind)
{
  UWMaster.install (this);
  if (ClipActive)
    UWMaster.remove ();
   else
    {
      ClipPosn = 0;
      ClipActive = 1;
    }
} // UWPasteFromClipboard::UWPasteFromClipboard //

UWPasteFromClipboard::~UWPasteFromClipboard (void)
{
  if (ClipActive)
    ClipActive = 0;
} // ~UWPasteFromClipboard::UWPasteFromClipboard //

void	UWPasteFromClipboard::tick (void)
{
  int ch;
  if (!ClipActive)
    return;			// Ignore the paste - not active.
  if (ClipPosn < ClipLength)
    {
      ch = Clipboard[ClipPosn++];
      send (ch);		// Send pasted character to remote.
      if (ch == '\r')
        DELAY_FUNC (10);	// Give a delay between lines.
    }
   else
    {
      UWMaster.remove ();	// Remove the client - end of paste.
      ClipActive = 0;
    }
} // UWPasteFromClipboard::tick //
