//-------------------------------------------------------------------------
//
// MAIN.CPP - Main module for UW/PC Version 2.
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
//    1.0    23/03/91  RW  Original Version of MAIN.CPP
//    1.1    05/05/91  RW  Clean up and begin to add file transfers.
//
//-------------------------------------------------------------------------

#include <stdio.h>		// Standard I/O routines.
#include "screen.h"		// Screen accessing routines.
#include "client.h"		// Client control routines.
#include "uw.h"			// UW protocol routines.
#include "keys.h"		// Keyboard handling routines.
#include "config.h"		// Configuration routines.
#include "timer.h"		// System timer routines.
#include "dialog.h"		// Dialog box routines.
#include "files.h"		// File transfer routines.
#include "comms.h"		// Communications routines.
#include <dos.h>		// "delay" is defined here.
#include <string.h>		// String handling routines.

#pragma	warn	-par

//
// Define some useful macros for screen access.
//
#define	SCRN_WIDTH	(HardwareScreen.width)
#define SCRN_HEIGHT	(HardwareScreen.height)
#define	ATTR(x)		(HardwareScreen.attributes[(x)])

//
// Define the title to be displayed for this module.
//
char	*TitleString = 
	"UW/PC version 2.00, Copyright (C) 1990-1991 Rhys Weatherley\n"
	"UW/PC comes with ABSOLUTELY NO WARRANTY; see the file COPYING for details.\n"
	"This is free software, and you are welcome to redistribute it\n"
	"under certain conditions; see the file COPYING for details.\n\n"
        "Press ALT-Z for help on special keys.\n\n";

unsigned cdecl	_stklen=8192;

int	DebugMode;
FILE	*DebugFile;

int	main	(int argc,char *argv[])
{
  char *mesg;
  DebugMode = 0;
  if (argc > 1 && !strcmp (argv[1],"-debug"))
    {
      // Open a debugging file to help me with user problems //
      if ((DebugFile = fopen ("UWDEBUG.OUT","wb")) != NULL)
        DebugMode = 1;
    } /* if */
  UWConfig.doconfig (argv[0]);	// Configure the program as necessary.
  if (!HardwareScreen.init (0))
    {
      fprintf (stderr,"Cannot initialise screen - not enough memory\n");
      return (0);
    }
  HardwareScreen.scroll (0,0,SCRN_WIDTH - 1,SCRN_HEIGHT - 2,0,
  			 ATTR(ATTR_NORMAL));
  HardwareScreen.scroll (0,SCRN_HEIGHT - 1,SCRN_WIDTH - 1,SCRN_HEIGHT - 1,0,
  		(UWConfig.DisableStatusLine ? 0x07 : ATTR(ATTR_STATUS)));
  HardwareScreen.cursor (0,4);
  HardwareScreen.shape (CURS_UNDERLINE);
  InitKeyboard ();
  InitTimers ();
  mesg = UWMaster.start ();
  TermTimers ();
  TermKeyboard ();
  HardwareScreen.term ();
  if (mesg)
    fprintf (stderr,"%s\n",mesg);
  if (DebugMode)
    fclose (DebugFile);
  return (0);
} // main //

//
// Send a command-line for the DSZ program to do a ZMODEM file transfer.
//
static	void	SendZModemCommand (char *cmd,char *file)
{
  static char cmdline[256];
  sprintf (cmdline,"%s port %d portx %x,%d ha off %s %s",
  		UWConfig.ZModemCommand,
  		UWConfig.ComPort,
  		comports[UWConfig.ComPort - 1],
		4 - ((UWConfig.ComPort - 1) % 2),cmd,file);
  UWMaster.direct (1);		// Turn on direct communications.
  UWMaster.jumpdos (cmdline);	// Do the ZMODEM command line.
  UWMaster.direct (0);		// Turn off direct communications.
} // SendZModemCommand //

//
// Define the query dialog boxes used by this module.
//

class	QuitQueryBox : public UWQueryBox {

public:

	QuitQueryBox (UWDisplay *wind) :
		UWQueryBox (wind,"Exit UW/PC? Yes or No",21) {};

	virtual	void	process	(int index)
		  { if (index < 2) UWMaster.terminate = 1;  // Kill UW/PC.
		    terminate (); };

};

class	ExitQueryBox : public UWQueryBox {

public:

	ExitQueryBox (UWDisplay *wind) :
		UWQueryBox (wind,"Exit? Yes or No",15) {};

	virtual	void	process	(int index)
		  { if (index < 2) UWMaster.exitmulti = 1; // Exit protocol 1.
		    terminate (); };

};

class	HangupQueryBox : public UWQueryBox {

public:

	HangupQueryBox (UWDisplay *wind) :
		UWQueryBox (wind,"Hangup? Yes or No",17) {};

	virtual	void	process	(int index)
		  { if (index < 2) UWMaster.sendbreak (); // Send a BREAK.
		    terminate (); };

};

class	BreakQueryBox : UWQueryBox {

public:

	BreakQueryBox (UWDisplay *wind) :
		UWQueryBox (wind,"Send BREAK? Yes or No",21) {};

	virtual	void	process	(int index)
		  { if (index < 2) UWMaster.hangup ();	// Hangup the modem.
		    terminate (); };

};

static	char	*asciimsgs[] =
	 {"Enter name of file to upload, ^X to clear line, or ESC to abort.",
	  "Enter name of file to download, ^X to clear line, or ESC to abort.",
	  "Enter name of capture file, ^X to clear line, or ESC to abort."};

class	AsciiEditBox : public UWEditBox {

protected:

	int	kind;

public:

	AsciiEditBox (UWDisplay *wind,int Kind) :
		UWEditBox (wind,asciimsgs[Kind],MAX_EDIT_SIZE)
			{ kind = Kind; };

	virtual void	process (int esc)
		  { terminate ();
		    if (!esc && buffer[0] != '\0')
		      new UWAsciiFileTransfer (window,kind,buffer);
		  };
};

class	FileEditBox : public UWEditBox {

protected:

	int	upload;
	int	kind;

public:

	FileEditBox (UWDisplay *wind,int Upload,int Kind) :
		UWEditBox (wind,asciimsgs[!Upload],MAX_EDIT_SIZE)
			{ kind = Kind; upload = Upload; };

	virtual void	process (int esc)
		  { terminate ();
		    if (!esc && buffer[0] != '\0')
		      new UWXYFileTransfer (window,kind,!upload,buffer);
		  };
};

class	ZModemEditBox : public UWEditBox {

public:

	ZModemEditBox (UWDisplay *wind) :
		UWEditBox (wind,asciimsgs[0],MAX_EDIT_SIZE) {};

	virtual void	process (int esc)
		  { terminate ();
		    if (!esc && buffer[0] != '\0')
		      {
		        SendZModemCommand ("sz -b",buffer);
			send ('\r');	// Send ^M to get command line back.
		      }
		  };
};

class	FileQueryBox : public UWQueryBox {

protected:

	int	upload;

public:

	FileQueryBox (UWDisplay *wind,int Upload) :
		UWQueryBox (wind,UWMaster.protocol == 0 ?
			"[A]scii, [X]modem or [Z]modem?" :
			"[A]scii, [X]modem",
			UWMaster.protocol == 0 ? 30 : 20,
			UWMaster.protocol == 0 ? "\033aAxXzZ" :
				"\033aAxX")
			{ upload = Upload; };

	virtual	void	process	(int index)
		  { if (index < 1)
		      terminate ();
		     else if (index < 3)
		      {
		        terminate ();
			new AsciiEditBox (window,(upload ? ASCII_UPLOAD
							 : ASCII_DOWNLOAD));
		      }
		     else if (index < 5)
		      {
		        terminate ();
			new FileEditBox (window,upload,XMOD_ORIGINAL);
		      }
		     else if (index < 7)
		      {
		        terminate ();
			if (upload)
			  new ZModemEditBox (window);
			 else
			  {
			    SendZModemCommand ("rz -b","");
			    send ('\r'); // Send ^M to get command-line back.
			  }
		      }
		  };

};

// Process a default client keypress.  This should be called
// by the client for any keys it cannot process when
// "key" is called, and that are deemed to be control
// keys for the UW client program.
void	UWClient::defkey (int keypress)
{
  int wind;
  switch (keypress)
    {
      case QUIT_KEY:
      case QUIT_KEY2:	if (!(new QuitQueryBox (window)))
      			  UWMaster.terminate = 1; // Terminate if not
			break;			  // enough memory for box.
      case EXIT_KEY:	if (UWMaster.protocol == 0 ||
      			    !(new ExitQueryBox (window)))
      			  UWMaster.exitmulti = 1; // Exit protocol 1 anyway.
			break;
      case NEW_KEY:	wind = UWMaster.create ();
      			if (wind > 0)
			  UWMaster.top (wind);
			break;
      case KILL_KEY:	UWMaster.kill (); break;
      case JUMP_DOS_KEY:UWMaster.jumpdos (); break;
      case HANGUP_KEY:	if (UWMaster.protocol == 0 ||
      			    !(new HangupQueryBox (window)))
      			  UWMaster.hangup ();	// Hangup the modem anyway.
			break;
      case BREAK_KEY:	if (UWMaster.protocol == 0 ||
      			    !(new BreakQueryBox (window)))
      			  UWMaster.sendbreak (); // Send BREAK anyway.
			break;
      case INIT_KEY:	UWMaster.sendstring (UWConfig.InitString); break;
      case START_KEY:	if (UWMaster.protocol == 0)
      			  UWMaster.sendstring (UWConfig.CommandString);
			break;
      case DIAL_KEY:	UWMaster.sendstring (UWConfig.DialString); break;
      case HELP_KEY:	new UWHelpBox (window);
      			break;
      case UPLOAD_KEY:	new FileQueryBox (window,1);
      			break;
      case DOWNLOAD_KEY:new FileQueryBox (window,0);
      			break;
#ifdef	DOOBERY
      case UWFTP_KEY:	UWMaster.sendstring (UWConfig.FtpString); break;
#endif
      case CAPTURE_KEY:	new AsciiEditBox (window,ASCII_CAPTURE);
      			break;
      case PAUSE_KEY:	delay (500); break;
      default:		if (keypress >= ALT_WIND_NUM (1) &&
      			    keypress <= ALT_WIND_NUM (7))
			  UWMaster.top (ALT_GET_NUM (keypress));
			break;
    }
} // UWClient::defkey //
