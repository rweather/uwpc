//-------------------------------------------------------------------------
//
// UW.CPP - Declarations for the UW protocol within UW/PC.
// 
// NOTE: If "UWMaster::protocol" is -1, then an intermediate state for the
//       protocol exit on the request from the remote host is in effect
//       and "UWProtocol::exit" will reset it to 0 after clean-up.  If
//	 the value is -2, then quick exits required by UW/PC are needed
//	 where the server no longer exists.  If the value is -3, a
//	 user-requested exit is in progress.
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
//    1.0    23/03/91  RW  Original Version of UW.CPP
//    1.1    05/05/91  RW  Many bug fixes and cleanups.
//    1.2    26/05/91  RW  Add command-line to "jumpdos".
//    1.3    08/06/91  RW  Add mouse support to the clients.
//    1.4    31/10/91  RW  Port this module to Windows 3.0.
//
//-------------------------------------------------------------------------

#include "uw.h"			// Declarations for this module.
#include "comms.h"		// Communications routines.
#include "client.h"		// Client declarations.
#include "config.h"		// Configuration declarations.
#include "uwproto.h"		// UW protocol constants.
#include "display.h"		// UW client display routines.
#include "keys.h"		// Keyboard handling routines.
#ifdef	UWPC_DOS
#include "screen.h"		// Screen handling routines.
#include "timer.h"		// System timer handlers.
#include "mouse.h"		// Mouse accessing routines.
#else	/* UWPC_DOS */
#include "win3.h"		// Windows 3.0 resource definitions.
#endif	/* UWPC_DOS */
#include <stdio.h>		// "sprintf" is defined here.
#include <string.h>		// String handling routines.
#include <dos.h>		// "delay" is defined here.

#pragma	warn	-par

//
// Define the master object for handling the UW protocol.
//
UWProtocol	UWMaster;

//
// Define the special control codes to be translated.
//
#define	IAC	001
#define XON	021
#define	XOFF	023

#ifdef	UWPC_DOS

//
// Define the title string to be displayed on startup.
//
extern	char	*TitleString;

#endif	/* UWPC_DOS */

//
// Define the class to use when creating new terminals.
//
#define	UW_TERM_CLASS	UWTermDesc

// Transmit a character to the remote host.
inline	void	transmit (int ch)
{
  comsend (UWConfig.ComPort,ch);
} // transmit //

// Send a UW command to the remote host.
void	UWProtocol::command (int cmd)
{
  transmit (P1_IAC);
  transmit (cmd | P1_DIR_CTOH);
} // UWProtocol::command //

// Send a character to the remote host in the
// round-robin service window.  This is called by
// the function UWClient::send.
void	UWProtocol::send (int ch)
{
  if (protocol < 1)
    transmit (ch);
   else
    {
      if (LastInput != RoundWindow)
        {
	  // Send a new input window command to host //
	  command (P1_FN_ISELW | RoundWindow);
	  LastInput = RoundWindow;
	}
      if (ch & 0x80)			// Escape the top-most bit.
        command (P1_FN_META);
      ch &= 0x7F;			// Strip down to 7 bits.
      switch (ch)
        {
	  case IAC:  command (P1_FN_CTLCH | P1_CC_IAC);  break;
	  case XON:  command (P1_FN_CTLCH | P1_CC_XON);  break;
	  case XOFF: command (P1_FN_CTLCH | P1_CC_XOFF); break;
	  default:   transmit (ch);	 		 break;
	}
    }
} // UWProtocol::send //

// Send a character to the "remote" method of
// the client attached to the output window.
void	UWProtocol::remote (int ch)
{
  if (clients[OutputWindow])
    {
      RoundWindow = OutputWindow;
      clients[OutputWindow] -> remote (ch);
    }
} // UWProtocol::remote //

extern	int	DebugMode;
extern	FILE	*DebugFile;

// Process a character incoming from the host
void	UWProtocol::fromhost (int ch)
{
  int arg,temp;
  if (DebugMode)
    fputc (ch,DebugFile);	// Save character in the debugging file.
  if (protocol > 0 || (UWConfig.StripHighBit && !dirproc))
    ch &= 0x7F;		// Strip unwanted parity bit.
   else
    ch &= 0xFF;		// Make a little more 8-bit clean.
  if (getpcl)
    {
      // Process the character for a protocol negotiation //
      int proto;
      proto = ch - ' ' + 1;		// Get protocol number from remote
      if (proto != 1)			// Check for a Protocol 1 suggestion
        {
	  // Cancel the server's suggested protocol and suggest Protocol 1 //
	  command (P1_FN_MAINT | P1_MF_CANPCL);
	  transmit (' ');
	}
       else if (getpcl == P1_MF_CANPCL)	// If a Protocol 1 suggestion, set it
        {
	  // Protocol is acceptable - tell server to set it //
	  command (P1_FN_MAINT | P1_MF_SETPCL);
	  transmit (' ');
	}
      getpcl = 0;
      return;				// Exit - character processed
    }
  if (gotiac)
    {
      gotiac = 0;
      if ((ch & P1_DIR) != P1_DIR_HTOC)
        return;				// Skip command - wrong direction.
      arg = ch & P1_FN_ARG;		// Get function argument.
      switch (ch & P1_FN)
        {
	  case P1_FN_NEWW:
	  	if (protocol < 1 || arg < 1)
		  break;
	  	if (!(temp = create (arg))) // Attempt to create a window.
		  {
		    // Send a kill command back to the remote host if
		    // the window could not be created on this client.
		    command (P1_FN_KILLW | arg);
		  }
		 else if (UWConfig.PopUpNewWindow)
		  top (temp);		// Bring new window to top.
	  	break;
	  case P1_FN_KILLW:
	  	if (protocol < 1 || arg < 1)
		  break;
	  	kill (arg);		// Kill the requested window.
	  	break;
	  case P1_FN_OSELW:
	  	if (protocol < 1)	// Ignore command in Protocol 0.
		  break;
		if (clients[arg] != 0 && arg > 0)
		  OutputWindow = arg;
	  	break;
          case P2_FN_WOPT:		// Process Protocol 2 window options.
	  	if (protocol < 2)	// Ignore options in Protocol 0/1.
		  break;
	  	break;
	  case P1_FN_META:
	  	if (protocol < 1)	// Ignore command in Protocol 0.
		  break;
		if (protocol > 1 && arg)
		  {
		    // Extract the meta encoding in Protocol 2 //
		    switch (arg)
		      {
		        case P1_CC_IAC:  remote (IAC  | 0x80); break;
		        case P1_CC_XON:  remote (XON  | 0x80); break;
		        case P1_CC_XOFF: remote (XOFF | 0x80); break;
		        default:	 break;
		      } /* switch */
		  } /* then */
		 else
	  	  gotmeta = 1;		// Enable the next meta bit.
	  	break;
	  case P1_FN_CTLCH:
	  	if (protocol < 1)	// Ignore command in Protocol 0.
		  break;
		switch (arg)
		  {
		    case P1_CC_IAC:  ch = IAC;  break;
		    case P1_CC_XON:  ch = XON;  break;
		    case P1_CC_XOFF: ch = XOFF; break;
		    default:	     ch = 0;
		  }
		if (ch)
		  {
		    if (gotmeta)
		      {
		        gotmeta = 0;
			ch |= 0x80;
		      }
		    remote (ch);
		  }
	  	break;
	  case P1_FN_MAINT:
	  	switch (arg)
		  {
		    case P1_MF_ENTRY:			// Enter protocol 1
		    		if (protocol < 1)
				  {
				    protocol = 1;
				    // Start protocol negotiation.
				    command (P1_FN_MAINT | P1_MF_ASKPCL);
				  }
				break;
		    case P1_MF_CANPCL:
		    case P1_MF_SETPCL:
		    		getpcl = arg;
				break;
		    case P1_MF_EXIT:			// Exit protocol 1
		    		protocol = -1;		// Special exit mode
		    		exit ();
				break;
		    default:	break;
		  }
	  	break;
	  default: break;
	}
    }
   else if (ch == P1_IAC && !dirproc && !UWConfig.DisableUW)
    gotiac = 1;
   else
    {
      if (gotmeta)
        {
	  ch |= 0x80;		// Enable the received meta bit.
	  gotmeta = 0;
	}
      remote (ch);
    }
} // UWProtocol::fromhost //

// Display a new status line on the screen bottom
void	UWProtocol::status (void)
{
#ifdef	UWPC_DOS		// Windows 3.0 doesn't have status lines.
  static char Buffer[STR_LEN + 20];
  static char TermName[6];
  int len,window,posn,startposn,attrbytes,sposn;
  char far *name;
  char *sformat;
  attrbytes = 0;
  sformat = clients[CurrWindow] -> getstatus (); // Get the status format.
  sposn = UWConfig.StatusPosn;		// Save status line position.
  if (UWConfig.DisableStatusLine && !sformat)
    {
      displays[0] -> status (0,0);	// Clear the status line.
      return;				// Skip rest of this function.
    }
  if (!sformat)
    sformat = UWConfig.StatusFormat;	// Get the configured status line.
   else
    UWConfig.StatusPosn = STATUS_CENTRE; // Centre alternate status lines.
  len = 0;
  name = clients[CurrWindow] -> name ();// Get the name of the client.
  while (len < 5 && *name)
    TermName[len++] = *name++;
  TermName[len] = '\0';
  posn = 0;
  while (posn < (STR_LEN - 1) && *sformat)
    {
      if (*sformat == '%')
        {
	  switch (*(++sformat))
	    {
	      case '\0':--sformat; break;
	      case '%':	Buffer[posn++] = '%'; break;
	      case 'a': startposn = posn;
  			for (window = 1;window < NUM_UW_WINDOWS;++window)
  			  {
  			    if (displays[window] != 0)
        		      {
	  			if (CurrWindow == window)
	    			  {
	      			    Buffer[posn++] = '\001';
	      			    Buffer[posn++] = window + '0';
	      			    Buffer[posn++] = '\002';
				    attrbytes += 2;
	    			  }
	   			 else
	    			  Buffer[posn++] = window + '0';
	  			Buffer[posn++] = ' ';
			      }
    			  }
			while ((posn - startposn) < 16) // 14 + 2 attr chars
			  Buffer[posn++] = ' ';
			break;
	      case 'e': strcpy (Buffer + posn,TermName);
	      		posn += len;
			if (len < 5)
			  {
			    int pad;
			    pad = len;
			    while (pad++ < 5)
			      Buffer[posn++] = ' ';
			  }
			break;
	      case 'h': Buffer[posn++] = '\001'; // Turn on highlight.
	      		attrbytes++; break;
	      case 'i': Buffer[posn++] = '\002'; // Turn off highlight.
	      		attrbytes++; break;
	      case 'n':	window = NUM_UW_WINDOWS - 1;
	      		while (window > 0 && !clients[window])
			  --window;
			Buffer[posn++] = '0' + window;
			break;
	      case 'o': if (comcarrier (UWConfig.ComPort))
			  strcpy (Buffer + posn,"Online ");
			 else
			  strcpy (Buffer + posn,"Offline");
			posn += 7;
			break;
	      case 'p': strcpy (Buffer + posn,UWConfig.DeviceParameters);
	      		posn += strlen (UWConfig.DeviceParameters);
			break;
	      case 's': startposn = CurrentTime;	// Get current time.
	      		Buffer[posn++] = (startposn / 600) + '0';
			startposn %= 600;
			Buffer[posn++] = (startposn / 60) + '0';
			startposn %= 60;
			Buffer[posn++] = ':';
			Buffer[posn++] = (startposn / 10) + '0';
			Buffer[posn++] = (startposn % 10) + '0';
			break;
	      case 't': if (comcarrier (UWConfig.ComPort))
	      		  {
	      		    startposn = OnlineTime;	// Get current time.
	      		    Buffer[posn++] = (startposn / 600) + '0';
			    startposn %= 600;
			    Buffer[posn++] = (startposn / 60) + '0';
			    startposn %= 60;
			    Buffer[posn++] = ':';
			    Buffer[posn++] = (startposn / 10) + '0';
			    Buffer[posn++] = (startposn % 10) + '0';
			  }
			 else
			  {
			    strcpy (Buffer + posn,"     ");
			    posn += 5;
			  }
			break;
	      case 'u': if (protocol > 0)
	      		  strcpy (Buffer + posn,"UW");
			 else
			  strcpy (Buffer + posn,"  ");
			posn += 2;
			break;
	      case 'v': Buffer[posn++] = '\263'; break;
	      case 'w': Buffer[posn++] = '0' + CurrWindow; break;
	      default:	if (*sformat >= '0' && *sformat <= '9')
	      		  {
			    int arg;
			    arg = clients[CurrWindow] -> getstatarg
			    		(*sformat - '0');
			    sprintf (Buffer + posn,"%d",arg);
			    posn = strlen (Buffer);
			  }
			break;
	    }
	}
       else
        Buffer[posn++] = *sformat;
      ++sformat;
    }
  Buffer[posn] = '\0';
  displays[0] -> status (Buffer,strlen (Buffer) - attrbytes);
  UWConfig.StatusPosn = sposn;		// Restore the status position.
#endif	/* UWPC_DOS */
} // UWProtocol::status //

#define	SLICE_SIZE	5

// Start the processing of the UW protocol.  When
// this method exits, the program has been terminated.
// On startup, an initial dumb terminal is created.
// Returns NULL or an error message.
char	*UWProtocol::start (void)
{
  int ch,window,carrier,newcarrier,slice;
  char *str;
#ifdef	UWPC_WINDOWS
  MSG msg;
  extern HWND hMainWnd;
  extern HANDLE hAccTable;
#else
  int lasttime,lastonline;
#endif	/* UWPC_WINDOWS */
  terminate = 0;
  exitmulti = 0;
  protocol = 0;
  CurrWindow = 0;
  LastInput = 0;
  RoundWindow = 0;
  OutputWindow = 0;
  gotmeta = 0;
  gotiac = 0;
  getpcl = 0;
  numwinds = 0;
  freelist = 0;
  dirproc = 0;
  for (window = 0;window < NUM_UW_WINDOWS;++window)
    {
      clients[window] = 0;
      displays[window] = 0;
    }
  if ((displays[0] = new UWDisplay (0)) == 0 || displays[0] -> width == 0 ||
      (clients[0] = new UW_TERM_CLASS (displays[0])) == 0)
    return ("Not enough memory for primary terminal");
  if (clients[0] -> isaterminal)
    ((UWTermDesc *)clients[0]) -> setemul (UWConfig.P0TermType);
  displays[0] -> top (1);
#ifdef	UWPC_DOS
  lasttime = CurrentTime;
  lastonline = OnlineTime;
  status ();		// Draw the status line
#else	/* UWPC_DOS */
  hMainWnd = displays[0] -> hWnd;	// Record the main window's handle.
#endif	/* UWPC_DOS */

#ifdef	UWPC_DOS
  // Display the title string in the window just created.
  HideMouse ();
  str = TitleString;
  while (*str)
    {
      if (*str == '\n')
        {
	  displays[0] -> cr ();
	  displays[0] -> lf ();
	  ++str;
	}
       else
        displays[0] -> send (*str++);
    }
  ShowMouse ();
#endif	/* UWPC_DOS */

  // Enable the communications port
  comenable (UWConfig.ComPort);
  comparams (UWConfig.ComPort,UWConfig.ComParams);

  // Send the initialisation string to the modem.
#ifdef	UWPC_DOS
  // Do some checks under DOS, but always send under Windows 3.0 //
  if (UWConfig.CarrierInit || !comcarrier (UWConfig.ComPort))
#else	/* UWPC_DOS */
  // Display a "I am doing something" message //
  if (UWConfig.InitString[0] != '\0')
    {
      str = "Initialising the modem...";
      while (*str)
        displays[0] -> send (*str++);
      displays[0] -> cr ();
      displays[0] -> lf ();
    } /* if */
#endif	/* UWPC_DOS */
    sendstring (UWConfig.InitString);

  // Go into a processing loop to process incoming events //
  carrier = comcarrier (UWConfig.ComPort);
  slice = 0;
  while (!terminate)
    {
      // Determine if the carrier status has changed //
      if ((newcarrier = comcarrier (UWConfig.ComPort)) != carrier)
        {
	  carrier = newcarrier;
#ifdef	UWPC_DOS
	  if (carrier)
	    ResetOnline ();		// Reset the online time.
	  status ();			// Redraw the status line.
#endif	/* UWPC_DOS */
	  if (!newcarrier && protocol > 0)
	    {
	      // Carrier has dropped - leave Protocol 1 //
	      protocol = -2;
	      exit ();
	    }
	}

#ifdef	UWPC_DOS
      // Determine if the current system time has changed //
      if (lasttime != CurrentTime)
        {
	  lasttime = CurrentTime;
	  status ();			// Redraw the status line.
	}

      // Determine if the current online time has changed //
      if (carrier && lastonline != OnlineTime)
        {
	  lastonline = OnlineTime;
	  status ();			// Redraw the status line.
	}
#endif	/* UWPC_DOS */

      // Process characters received from the remote host //
      if ((ch = comreceive (UWConfig.ComPort)) >= 0)
	fromhost (ch);

#ifdef	UWPC_DOS
      // Under Windows 3.0, the keyboard and mouse events come   //
      // through the ordinary Windows 3.0 event processing loops //
      // but under DOS we need to do it ourselves.		 //

      // Process keypresses from the user //
      if ((ch = GetKeyPress ()) != -1)
        {
	  RoundWindow = CurrWindow;
	  clients[CurrWindow] -> key (ch);
	  if (exitmulti)		// Check for user-requested exit.
	    {
	      // This check is required because otherwise objects //
	      // will be destroyed while they are being called on //
	      exit ();			// Do the protocol exit.
	      exitmulti = 0;
	    }
	}

      // Process the mouse events if necessary //
      if (MouseChange)
        {
	  RoundWindow = CurrWindow;
	  SendMouseEvent (clients[CurrWindow]);
	}
#endif	/* UWPC_DOS */

      if (!(slice = (slice + 1) % SLICE_SIZE))
        {
          // Send a "tick" to every currently active window //
          for (window = 0;window < NUM_UW_WINDOWS;++window)
            {
	      RoundWindow = window;
	      if (clients[window])
	        clients[window] -> tick ();
	    }
	  // Clean up unwanted clients from the free list //
	  if (freelist)
	    {
	      UWClient *temp;
	      temp = freelist -> under ();
	      delete freelist;
	      freelist = temp;
	    }
	}

#ifdef	UWPC_WINDOWS
      // Let the other applications do some work //
      Yield ();

      // Process the Windows 3.0 messages that are waiting for processing //
      if (!PeekMessage (&msg,NULL,NULL,NULL,PM_NOYIELD | PM_REMOVE))
	continue;
      if (msg.message == WM_QUIT)
        break;		// The application has terminated.
      if (!TranslateAccelerator (hMainWnd,hAccTable,&msg))
        {
	  TranslateMessage (&msg);
	  DispatchMessage (&msg);
	}
#endif	/* UWPC_WINDOWS */
    }

  // Destroy all active windows and exit UW protocol handling //
  exit ();
#ifdef	UWPC_WINDOWS
  delete displays[0];		// Remove the protocol 0 window.
#endif	/* UWPC_WINDOWS */

  comdisable (UWConfig.ComPort,1);
  return (0);
} // UWProtocol::start //

// Pass a keypress from outside the UW protocol master.
// This is only called in the Windows 3.0 version.
void	UWProtocol::sendkey (int wind,int key)
{
  // NOTE: This should mirror the GetKeyPress handling in ::start
  // for the DOS version of the program.
  RoundWindow = wind;
  clients[wind] -> key (key);
  if (exitmulti)		// Check for user-requested exit.
    {
      // This check is required because otherwise objects //
      // will be destroyed while they are being called on //
      exit ();			// Do the protocol exit.
      exitmulti = 0;
    }
} // UWProtocol::sendkey //

// Force the exit from Protocol 1 or higher, and a
// return to Protocol 0 (ignored in Protocol 0).
void	UWProtocol::exit (void)
{
  int window;
  if (protocol == 0)
    return;			// Ignore request in protocol 0
   else if (protocol > 0)
    protocol = -3;		// User-requested exit from protocol 1.
  for (window = 1;window < NUM_UW_WINDOWS;++window)
    kill (window);		// Kill all currently present windows.
  if (protocol == -3)		// Only send exit command on user-request.
    command (P1_FN_MAINT | P1_MF_EXIT); // Send the exit command to remote host
  top (0);			// Return window 0 to the top.
  protocol = 0;			// Now return to protocol 0
  OutputWindow = 0;		// Reset input and output windows.
  LastInput = 0;
  numwinds = 0;			// Resync number of windows (just in case).
  status ();			// Reset the status line.
#ifdef	UWPC_WINDOWS
  // Disable the Protocol 1 specific menu items //
  HMENU hMenu;
  extern HWND hMainWnd;
  hMenu = GetMenu (hMainWnd);
  EnableMenuItem (hMenu,IDM_EXIT,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_NEW,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_KILL,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_NEXTWIN,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_MINALL,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_START,MF_ENABLED);
#endif	/* UWPC_WINDOWS */
} // UWProtocol::exit //

// Create a new window (ignored in Protocol 0).  Returns
// the identifier, or 0 if no window could be created.
// If number != 0, then the number has been supplied
// explicitly, usually by the remote host.
int	UWProtocol::create (int number)
{
  int docreate=0;
  if (protocol < 1)
    return (0);			// Abort request in protocol 0.
  if (number > 0)
    {
      if (clients[number] != 0)
        return (number);	// Window already exists - ignore request.
    }
   else
    {
      docreate = 1;
      number = 1;
      while (number < NUM_UW_WINDOWS && clients[number] != 0)
        ++number;
      if (number >= NUM_UW_WINDOWS)
        return (0);		// Already at window limit.
    }
  if ((displays[number] = new UWDisplay (number)) == 0)
    return (0);			// Not enough memory for the display.
  if (displays[number] -> width == 0 ||
      (clients[number] = new UW_TERM_CLASS (displays[number])) == 0)
    {
      delete displays[number];	// Remove display that was created.
      displays[number] = 0;
      return (0);
    }
  if (clients[number] -> isaterminal)
    ((UWTermDesc *)clients[number]) -> setemul (UWConfig.P1TermType);
  if (numwinds == 0)		// If this is the first window in Protocol 1
    {				// then display it at the top.
      top (number);
#ifdef	UWPC_WINDOWS
      // Minimize the main window when entering Protocol 1 to get it //
      // out of the way on the Windows 3.0 desktop.		     //
      ShowWindow (displays[0] -> hWnd,SW_MINIMIZE);

      // Enable the Protocol 1 specific menu items //
      HMENU hMenu;
      extern HWND hMainWnd;
      hMenu = GetMenu (hMainWnd);
      EnableMenuItem (hMenu,IDM_EXIT,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_NEW,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_KILL,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_NEXTWIN,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_MINALL,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_START,MF_GRAYED);
#endif	/* UWPC_WINDOWS */
    } /* if */
  ++numwinds;
  status ();			// Redraw the status line.
  if (docreate)
    command (P1_FN_NEWW | number); // Send a create command to remote host.
  return (number);		// Window successfully created.
} // UWProtocol::create //

// Install a new client on top of the one in the current
// round-robin window.
void	UWProtocol::install (UWClient *newclient)
{
  newclient -> setunder (clients[RoundWindow]);
  clients[RoundWindow] = newclient;
  status ();
} // UWProtocol::install //

// Remove the top-most client from the current round-
// robin window, and return to the client underneath.
void	UWProtocol::remove (void)
{
  UWClient *temp;
  temp = clients[RoundWindow];
  clients[RoundWindow] = temp -> under ();
  temp -> setunder (freelist);	// Save client on the free list.
  freelist = temp;
  status ();
} // UWProtocol::remove //

// Turn direct character processing in protocol 0 on or off.
void	UWProtocol::direct (int on)
{
  if (protocol == 0)
    dirproc = on;
} // UWProtocol::direct //

// Kill a particular window.  Once all Protocol 1 or 2
// windows have been destroyed, "exit" is automatically
// called to exit the protocol service.  If number == 0,
// then the current window is killed.
void	UWProtocol::kill (int number)
{
  UWClient *client,*temp;
  if (protocol == 0)
    return;			// Ignore request in protocol 0.
  if (number == 0)
    number = CurrWindow;	// Set the default window to kill.
  if (clients[number] == 0)
    return;			// Ignore if window number is illegal.
  if (protocol != -2 && protocol != -1) // Only worry about user-req. kills.
    command (P1_FN_KILLW | number); // Send kill command to remote host.
  delete displays[number];	// Delete the display window.
  client = clients[number];	// Delete the stacked clients in the window.
  while (client)
    {
      temp = client -> under (); // Get the client under this one.
      delete client;		 // Delete the current client.
      client = temp;		 // Move onto the next client.
    }
  clients[number] = 0;		// Mark the window as unused.
  displays[number] = 0;
  --numwinds;
  if (protocol < 0)		// Ignore rest if quick exit wanted.
    return;
  number = 1;			// Search for a new top window on a normal kill
  while (number < NUM_UW_WINDOWS && clients[number] == 0)
    ++number;
  if (number < NUM_UW_WINDOWS)
    {
      top (number);		// Set a new top window.
      CurrWindow = number;
    }
   else
    exit ();			// Last destroyed - exit protocol.
} // UWProtocol::kill //

// Bring a particular window to the top (i.e. make it
// the current window).
void	UWProtocol::top (int number)
{
  if (protocol == 0 || clients[number] == 0)
    return;
  if (displays[CurrWindow])
    displays[CurrWindow] -> top (0);	// Remove current window from top.
  CurrWindow = number;
  displays[CurrWindow] -> top (1);	// Make the new current window the top.
  status ();				// Redraw the status line.
} // UWProtocol::top //

// Cycle around to the next window in Protocol 1/2.
void	UWProtocol::nextwindow (void)
{
  int newwin;
  if (protocol == 0)
    return;				// Cannot cycle windows in Protocol 0.
  newwin = CurrWindow + 1;
  if (newwin >= NUM_UW_WINDOWS)
    newwin = 1;
  while (newwin != CurrWindow && clients[newwin] == 0)
    {
      ++newwin;
      if (newwin >= NUM_UW_WINDOWS)
        newwin = 1;
    } /* while */
  top (newwin);				// Bring the new window to the top.
} // UWProtocol::nextwindow //

// Jump out to a DOS shell, and fix everything on return.
// Optionally execute a command in DOS.
void	UWProtocol::jumpdos (char *cmdline)
{
#ifdef	UWPC_DOS
  HardwareScreen.jumpdos (cmdline);	// Clear screen and jump out to DOS.
  comfix (UWConfig.ComPort);		// Fix the communications port.
  displays[CurrWindow] -> top (1);	// Redraw the top-most window.
  status ();				// Redraw the status line.
#endif	/* UWPC_DOS */
} // UWProtocol::jumpdos //

// Hangup the modem and return to Protocol 0.
void	UWProtocol::hangup (void)
{
  if (protocol > 0)			// Exit protocol 1 if necessary.
    protocol = -2;
  exit ();
  comdropdtr (UWConfig.ComPort);	// Drop the COM port's DTR signal
  DELAY_FUNC (100);			// Wait for 100 ms before raising
  comraisedtr (UWConfig.ComPort);	// Raise the DTR signal again
  if (comcarrier (UWConfig.ComPort))	// If the carrier is still present
    sendstring (UWConfig.HangupString);	// then send the hangup string
} // UWProtocol::hangup //

// Send a modem control string through the current window.
// Modem control strings include initialisation, hangup, etc.
void	UWProtocol::sendstring (char *str)
{
  while (*str)
    {
      if (*str == '~')
        DELAY_FUNC (500);		// Wait 1/2th a second for '~'
       else if (*str == '^')
        {
	  if (*(str + 1))
	    send (*(++str) & 0x1F);	// Send an escaped control code
	}
       else if (*str == '#')
        {
	  if (*(str + 1))
	    send (*(++str));		// Send following character direct.
	}
       else
        send (*str);			// Send the character direct
      ++str;
    }
} // UWProtocol::sendstring //

// Exit protocol 1 and send a modem line break.
void	UWProtocol::sendbreak (void)
{
  if (protocol > 0)			// Exit protocol 1 if necessary.
    protocol = -2;
  exit ();
  combreak (UWConfig.ComPort,1);	// Turn on the BREAK signal.
  DELAY_FUNC (500);			// Wait for 500ms for the pulse.
  combreak (UWConfig.ComPort,0);	// Turn off the BREAK signal.
} // UWProtocol::sendbreak //

// Minimize all windows except the given window.  This only
// has an effect in the Windows 3.0 version.
void	UWProtocol::minall (int wind)
{
#ifdef	UWPC_WINDOWS
  int window;
  for (window = 0;window < NUM_UW_WINDOWS;++window)
    {
      if (window != wind && displays[window])
        ShowWindow(displays[window] -> hWnd,SW_MINIMIZE);
    } /* if */
#endif	/* UWPC_WINDOWS */
} // UWProtocol::minall //

// Send a character from a client to the remote host.
void	UWClient::send (int ch)
{
  UWMaster.send (ch);
} // UWClient::send //
