//-------------------------------------------------------------------------
//
// FILES.CPP - Declarations for creating UW clients for file transfers.
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
//    1.0    06/05/91  RW  Original Version of FILES.CPP
//
//-------------------------------------------------------------------------

#include <stdio.h>		// Standard I/O routines.
#include "files.h"		// Declarations for this module.
#include "keys.h"		// Keyboard handling declarations.
#include "uw.h"			// Declarations for the protocol master.
#include <dos.h>		// "delay" is defined here.

UWAsciiFileTransfer::UWAsciiFileTransfer (UWDisplay *wind,int type,char *name)
		: UWFileTransfer (wind)
{
  kind = type;
  capture = (kind == ASCII_CAPTURE);
  if (type == ASCII_UPLOAD)
    file = fopen (name,"rt");	// Open the Ascii file for text mode reading.
   else if (type == ASCII_DOWNLOAD)
    file = fopen (name,"wb");	// Open the Ascii file for binary mode writing.
   else
    file = fopen (name,"ab");	// Open the Ascii file for binary mode append.
  UWMaster.install (this);	// Install the file transfer object.
  if (file == NULL)
    UWMaster.remove ();		// Remove object if initialisation failed.
   else
    setvbuf (file,NULL,_IOFBF,BUFSIZ); // Set a file buffer if possible.
} // UWAsciiFileTransfer::UWAsciiFileTransfer //

// The destructor closes the transfer file if necessary so that
// ASCII file transfers are correctly cleaned up if windows are
// destroyed when protocol 1 exits.
UWAsciiFileTransfer::~UWAsciiFileTransfer (void)
{
  if (file != NULL)
    fclose (file);
} // UWAsciiFileTransfer::~UWAsciiFileTransfer //

void	UWAsciiFileTransfer::key (int keypress)
{
  if ((kind == ASCII_CAPTURE && keypress == CAPTURE_KEY) ||
      (kind != ASCII_CAPTURE && keypress == 033))
    {
      // termination key has been pressed - close off the transfer file //
      fclose (file);
      file = NULL;		// Indicator for the destructor.
      UWMaster.remove ();	// Remove this client from the client stack.
    }
   else if (underneath)
    underneath -> key (keypress); // Do the normal terminal key processing.
} // UWAsciiFileTransfer //

void	UWAsciiFileTransfer::remote (int ch)
{
  if (kind != ASCII_UPLOAD)
    fputc (ch,file);		// Save the character in the transfer file.
  if (underneath)
    underneath -> remote (ch);	// Do the normal terminal remote processing.
} // UWAsciiFileTransfer //

void	UWAsciiFileTransfer::tick (void)
{
  if (kind == ASCII_UPLOAD)
    {
      int ch;
      ch = fgetc (file);	// Get the next character to be transfered.
      if (ch == '\n')		// Check for the end of the line.
        {
	  send ('\r');		// Send a CR character at the end of lines.
	  delay (10);		// Delay 10ms between lines to allow catch-up.
	}
       else if (ch != EOF)
        send (ch);		// Send the transfer character directly.
       else
        {
	  // Upload is finished - clean up the transfer information //
          fclose (file);
          file = NULL;		// Indicator for the destructor.
          UWMaster.remove ();	// Remove this client from the client stack.
	}
    }
  if (underneath)
    underneath -> tick ();	// Pass the tick on to the client underneath.
} // UWAsciiFileTransfer::tick //

char	*UWAsciiFileTransfer::getstatus (void)
{
  static char *names[] =
  	  {"ASCII upload in progress - ESC to abort",
	   "ASCII download in progress - ESC to abort/end",
	   "ASCII capture in progress - ALT-L to abort/end"};
  return (names[kind]);
} // UWAsciiFileTransfer::getstatus //
