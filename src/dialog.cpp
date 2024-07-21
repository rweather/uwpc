//-------------------------------------------------------------------------
//
// DIALOG.CPP - Declarations for creating dialog boxes.
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
//    1.0    14/04/91  RW  Original Version of DIALOG.CPP
//
//-------------------------------------------------------------------------

#include "dialog.h"		// Declarations for this module.
#include "client.h"		// Client processing routines.
#include "screen.h"		// Screen processing routines.
#include "uw.h"			// UW Protocol accessing routines.
#include "display.h"		// Display handling routines.
#include "keys.h"		// Keypress declarations.
#include <string.h>		// String handling routines.

#pragma	warn	-par

#define	ATTR(x)		(HardwareScreen.attributes[(x)])
#define	DRAW(x,y,pair)	HardwareScreen.draw (x,y,pair,1)
#define	LINES		(HardwareScreen.height - 1)
#define	COLS		(HardwareScreen.width)

#define	BOX_TL		213
#define	BOX_TR		184
#define	BOX_HORZ	205
#define	BOX_BL		212
#define	BOX_BR		190
#define	BOX_VERT	179

UWDialogBox::UWDialogBox (UWDisplay *wind,int x1,int y1,int x2,int y2) :
	UWClient (wind)
{
  int temp;
  unsigned attr;
  UWMaster.install (this);		// Install a new client.
  dx1 = x1;
  dy1 = y1;
  dx2 = x2;
  dy2 = y2;
  HardwareScreen.shape (CURS_INVISIBLE);
  HardwareScreen.mark (x1,y1,x2,y2);
  HardwareScreen.scroll (x1,y1,x2,y2,0,ATTR(ATTR_NORMAL));
  attr = ATTR(ATTR_HIGHLIGHT) << 8;	// Draw a box around dialog area.
  DRAW (x1,y1,BOX_TL | attr);
  DRAW (x2,y1,BOX_TR | attr);
  DRAW (x1,y2,BOX_BL | attr);
  DRAW (x2,y2,BOX_BR | attr);
  for (temp = x1 + 1;temp < x2;++temp)
    {
      DRAW (temp,y1,BOX_HORZ | attr);
      DRAW (temp,y2,BOX_HORZ | attr);
    }
  for (temp = y1 + 1;temp < y2;++temp)
    {
      DRAW (x1,temp,BOX_VERT | attr);
      DRAW (x2,temp,BOX_VERT | attr);
    }
  cleared = 0;
} // UWDialogBox::UWDialogBox //

UWDialogBox::~UWDialogBox (void)
{
  if (!cleared)
    HardwareScreen.clearmark ();
} // UWDialogBox::~UWDialogBox //

// Terminate this dialog box and return to the previously
// active client in this window.
void	UWDialogBox::terminate (void)
{
  HardwareScreen.clearmark ();
  cleared = 1;
  window -> top (1);		// Restore top-most window (this one).
  UWMaster.remove ();		// Remove this client from client stack.
} // UWDialogBox::terminate //

// Show a string on the screen in the dialog box.
void	UWDialogBox::showstring (int x,int y,char *str)
{
  unsigned attr;
  attr = ATTR(ATTR_NORMAL) << 8;
  while (*str)
    HardwareScreen.draw (x++,y,((*str++) & 255) | attr,1);
} // UWDialogBox::showstring //

void	UWDialogBox::key (int keypress)
{
  terminate ();			// Dummy for now.
} // UWDialogBox::key //

//
// Declare the text of the help box.
//
static	char *HelpBox[] =
	 {"ALT-B - Send a BREAK pulse",
	  "ALT-D - Send the dialing string",
	  "ALT-E - Exit a UW session",
#ifdef	DOOBERY
	  "ALT-F - Send \"uwftp^M\" to host",
#endif
	  "ALT-H - Hangup the modem",
	  "ALT-I - Send modem init string",
	  "ALT-J - Jump to DOS",
	  "ALT-K - Kill current window",
	  "ALT-L - Capture ON/OFF",
	  "ALT-N - Create a new window",
	  "ALT-Q - Quit the program",
	  "ALT-R - Download (receive) a file",
	  "ALT-S - Upload (send) a file",
	  "ALT-U - Send \"uw^M\" to host",
	  "ALT-X - Exit the program",
	  "ALT-Z - This help information",
	  "ALT-n - Go to window \"n\" (1-7)",
	  " [Press any key to continue]"
	 };
#define	HELP_WIDTH	33
#define	HELP_HEIGHT	17

UWHelpBox::UWHelpBox (UWDisplay *wind) :
	UWDialogBox (wind,(COLS - HELP_WIDTH - 4) / 2,
			  (LINES - HELP_HEIGHT - 2) / 2,
			  ((COLS - HELP_WIDTH - 4) / 2) + HELP_WIDTH + 3,
			  ((LINES - HELP_HEIGHT - 2) / 2) + HELP_HEIGHT + 1)
{
  int line;
  for (line = 0;line < HELP_HEIGHT;++line)
    showstring (dx1 + 2,dy1 + 1 + line,HelpBox[line]);
}

// Create a query dialog box.  If "answers" is NULL,
// then the default answer string "yYnN\033" is used.
UWQueryBox::UWQueryBox (UWDisplay *wind,char *query,int qlen,char *answers) :
	UWDialogBox (wind,(COLS - qlen - 4) / 2,
			  LINES / 2 - 2,
			  (COLS - qlen - 4) / 2 + qlen + 3,
			  LINES / 2 + 2)
{
  showstring (dx1 + 2,dy1 + 2,query);
  if (!answers)
    answers = "yYnN\033";
  anschars = answers;
} // UWQueryBox::UWQueryBox //

void	UWQueryBox::key	(int keypress)
{
  int index=0;
  while (anschars[index] && anschars[index] != keypress)
    ++index;
  if (anschars[index])
    process (index);		// Process the received key.
} // UWQueryBox::key //

// Process the character provided by the user if it
// is in the answer string.  The string index is provided.
void	UWQueryBox::process (int index)
{
  terminate ();			// Just terminate box in this class.
} // UWQueryBox::process //

// Create a editing dialog box to get a string //
UWEditBox::UWEditBox (UWDisplay *wind,char *prompt,int editsize) :
	UWDialogBox (wind,(COLS - (editsize + 2) - 4) / 2,
			  LINES / 2 - 3,
			  (COLS - (editsize + 2) - 4) / 2 + editsize + 5,
			  LINES / 2 + 2)
{
  showstring (dx1 + 2,dy1 + 2,prompt);
  showstring (dx1 + 2,dy1 + 3,"\020");
  size = editsize;
  posn = 0;
  HardwareScreen.cursor (dx1 + 4 + posn,dy1 + 3);
  HardwareScreen.shape (CURS_UNDERLINE);
  buffer[0] = '\0';
  length = 0;
} // UWEditBox::UWEditBox //

void	UWEditBox::key (int keypress)
{
  int temp,ch,save;
  switch (keypress)
    {
      case 033:	// Fall through to CR handling code //
      case '\r':process (keypress == 033);
		break;
      case 8:	if (posn <= 0)
      		  break;
		--posn;
		// Fall through to character delete code //
      case CURSOR_DEL:
      		if (!buffer[posn])	// Abort if at end of string.
		  break;
      		temp = posn;
      		while (temp < (length - 1))
		  {
		    buffer[temp] = buffer[temp + 1];
		    ++temp;
		  }
		buffer[temp] = '\0';
		--length;
		showstring (dx1 + 4 + posn,dy1 + 3,buffer + posn);
		showstring (dx1 + 4 + length,dy1 + 3," ");
		break;
      case CURSOR_LEFT:
      		if (posn > 0)
		  --posn;
		break;
      case CURSOR_RIGHT:
      		if (buffer[posn])
		  ++posn;
		break;
      case CURSOR_HOME:
      		posn = 0;
		break;
      case CURSOR_END:
      		posn = length;
		break;
      case 030:	for (temp = 0;temp < length;++temp)
      		  showstring (dx1 + 4 + temp,dy1 + 3," ");
		length = 0;
		posn = 0;
		buffer[0] = '\0';
		break;
      default:	if (keypress >= ' ' && keypress <= 255)
      		  {
		    if (length >= size)
		      break;		// String is at the maximum size.
		    ch = keypress;
		    temp = posn;
		    while (buffer[temp])
		      {
		        // Swap characters until at the end of the line //
		        save = buffer[temp];
			buffer[temp++] = ch;
			ch = save;
		      }
		    buffer[temp] = ch;
		    buffer[temp + 1] = '\0';
		    showstring (dx1 + 4 + posn,dy1 + 3,buffer + posn);
		    ++length;
		    ++posn;
		  }
		break;
    }
  if (!cleared)		// Reset the cursor position if necessary.
    HardwareScreen.cursor (dx1 + 4 + posn,dy1 + 3);
} // UWEditBox::key //

// Process a fully entered line.  If 'esc' is non-zero
// then the ESC key was pressed to abort the editing.
void	UWEditBox::process (int esc)
{
  terminate ();		// Just terminate the box in this class.
} // UWEditBox::process //
