//-------------------------------------------------------------------------
//
// MAIL.CPP - Declarations for the mail handling tool of UW/PC.
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
//    1.0    25/07/91  RW  Original Version of MAIL.CPP
//
//-------------------------------------------------------------------------

#include "mail.h"		// Declarations for this module.
#include "client.h"		// Client handling declarations.
#include "display.h"		// Display handling declarations.
#include "uw.h"			// UW protocol declarations.
#include "config.h"		// Configuration declarations.
#include "screen.h"		// Screen handling routines.
#include <string.h>		// String handling routines.

//
// Define the mail tool modes.
//
#define	MODE_NORMAL	0	// Normal operation.
#define	MODE_EDITING	1	// Editing a file on the remote host.
#define	MODE_READING	2	// Reading headers from a mailbox.
#define	MODE_BROWSING	3	// Browsing a mail message.

UWMailTool::UWMailTool (UWDisplay *wind)
	: UWClient (wind)
{
  mode = MODE_READING;		// Enter the "reading headers" mode.
  UWMaster.install (this);	// Install this client.
  window -> clear ();		// Clear the screen.
  window -> move (0,0);		// Home the cursor.
  resposn = 0;			// Clear the response buffer.
  numheaders = 0;		// No headers in mailbox at the moment.
  write ("Reading mailbox...");	// Inform the user that something is happening.
  send ('R');			// Begin a "read mailbox command".
  command (UWConfig.MailBoxName);
} // UWMailTool::UWMailTool //

// Send a mail server command to the remote host.
void	UWMailTool::command (char *str)
{
  while (*str)
    send (*str++);		// Send the text of the command.
  send ('\r');			// Terminate the command line.
} // UWMailTool::command //

// Write a string to the screen.
void	UWMailTool::write (char *str)
{
  while (*str)
    window -> send (*str++);
} // UWMailTool::write //

// Print a string field up to a maximum number of characters //
static	int	printstring (UWDisplay *wind,char *header,int index,int width)
{
  while (width > 0 && header[index] != '|' && header[index] != '\0')
    {
      wind -> send (header[index++]);
      --width;
    }
  while (header[index] != '|' && header[index] != '\0')
    ++index;
  if (header[index] == '\0')
    return (index);			// Quick hack for end of subject.
  ++index;				// Skip past next '|'.
  while (width > 0)
    {
      wind -> send (' ');		// Pad to the field width.
      --width;
    }
  return (index);
} // printstring //

// Display mail header information in a window.
static	void	displayheader (UWDisplay *wind,char *header,int inverse)
{
  int index;

  // Set the printing attribute to use for the header //
  if (inverse)
    wind -> setattr (HardwareScreen.attributes[ATTR_INVERSE]);
   else
    wind -> setattr (HardwareScreen.attributes[ATTR_NORMAL]);

  // Print the header status information //
  wind -> send (' ');
  wind -> send (header[0]);
  wind -> send (' ');

  // Print the sender, lines and subject information //
  index = 2;
  while (header[index] != '|')
    ++index;			/* Skip past the Message-Id */
  ++index;			/* Skip the last '|' character */
  index = printstring (wind,header,index,20);
  wind -> send (' ');
  index = printstring (wind,header,index,5);
  wind -> send (' ');
  printstring (wind,header,index,(wind -> width) - 31);

  // Clear the rest of the line with the header information //
  wind -> clear (CLR_END_LINE);

  // Return the printing attribute to normal //
  if (inverse)
    wind -> setattr (HardwareScreen.attributes[ATTR_NORMAL]);
} // displayheader //

// Show the header information on the screen.
void	UWMailTool::showheaders (void)
{
  int y=0;
  while (y < window -> height && (topheader + y) < numheaders)
    {
      window -> move (0,y);
      displayheader (window,headers[topheader + y],
      		     currheader == (topheader + y));
    }
} // UWMailTool::showheaders //

// Handle the current buffer of information from the remote host.
void	UWMailTool::handle (void)
{
  switch (respbuf[0])		// Determine the response type.
    {
      case 'H': if (numheaders >= MAIL_MAX_HDRS ||
      		    !(headers[numheaders] = new char [resposn]))
      		  break;	// Not enough memory - ignore.
		strncpy (headers[numheaders],respbuf + 1,resposn - 1);
		headers[numheaders++][resposn - 1] = '\0';
		break;
      case 'h':	mode = MODE_NORMAL;
      		topheader = 0;		// Setup for the first page display.
		currheader = 0;
		UWMaster.status ();	// Update the status line.
		window -> clear ();
      		showheaders ();
      		break;
      default:	break;
    } /* switch */
} // UWMailTool::handle //

void	UWMailTool::key (int keypress)
{
  if (mode == MODE_EDITING)
    {
      // If we are editing - pass characters straight through //
      if (underneath)
        underneath -> key (keypress);
      return;
    }
  switch (keypress)
    {
      case 033:	command ("Q");		// Quit the mail tool handling.
      		window -> clear ();	// Clear screen for the return
		UWMaster.remove ();	// to terminal emulation mode.
		disposeheaders ();	// Clean up the memory.
      		break;
      default:	defkey (keypress);	// Do the default key operation.
      		break;
    } /* switch */
} // UWMailTool::key //

void	UWMailTool::remote (int ch)
{
  if (mode == MODE_EDITING)
    {
      // If an edit is in progress, then pass characters straight through //
      if (ch == ('X' & 0x1F))	// Abort editing mode if ^X received.
        {
	  mode = MODE_NORMAL;
	  UWMaster.status ();
	}
       else if (underneath)
        underneath -> remote (ch);
      return;			// Finished with this character.
    }
  if (ch == '\r' || ch == '\n')
    {
      if (resposn > 0)
        handle ();		// Handle the incoming buffer.
      resposn = 0;		// Clear buffer for next line.
    }
   else if (resposn < MAIL_BUF_LEN)
    respbuf[resposn++] = ch;	// Store character in the response buffer.
} // UWMailTool::remote //

// Dispose the current mail headers.
void	UWMailTool::disposeheaders (void)
{
  int temp;
  for (temp = 0;temp < numheaders;++temp)
    delete headers[temp];
  numheaders = 0;
} // UWMailTool::disposeheaders //

char	*UWMailTool::getstatus (void)
{
  if (mode == MODE_EDITING)
    return ("Edit mail message");
   else if (mode == MODE_READING)
    return ("Reading mailbox - please wait ...");
   else if (mode == MODE_BROWSING)
    return ("%h\033\030\031\032 PgUp PgDn Home End Space Enter Esc%i "
            "%hH%ieader");
   else
    return ("%h\030 \031 PgUp PgDn Enter%i Mail%hb%iox %hD%ielete %hM%iail "
  	    "%hN%iew %hR%ieply %hS%iave %hU%indelete %hW%irite "
	    "%hQ%iuit E%hx%iit");
} // UWMailTool::getstatus //
