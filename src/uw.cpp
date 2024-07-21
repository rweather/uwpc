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
//  Copyright (C) 1990-1992  Rhys Weatherley
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
//    1.5    15/03/92  RW  Add lots of Protocol 2 support.
//    1.6    27/04/92  RW  Fine-tune the scheduling of requests.
//    1.7    10/05/92  RW  Don't translate VK_BACK into WM_CHAR.
//
//-------------------------------------------------------------------------

#include "uw.h"			// Declarations for this module.
#include "comms.h"		// Communications routines.
#include "client.h"		// Client declarations.
#include "config.h"		// Configuration declarations.
#include "uwproto.h"		// UW protocol constants.
#include "display.h"		// UW client display routines.
#include "keys.h"		// Keyboard handling routines.
#include "screen.h"		// Screen handling routines.
#include "opcodes.h"		// Opcodes for the terminal descriptions.
#ifdef	UWPC_DOS
#include "timer.h"		// System timer handlers.
#include "mouse.h"		// Mouse accessing routines.
#else	/* UWPC_DOS */
#include "win3.h"		// Windows 3.0 resource definitions.
#endif	/* UWPC_DOS */
#include <stdio.h>		// "sprintf" is defined here.
#include <string.h>		// String handling routines.
#include <dos.h>		// "delay" is defined here.

#pragma	warn	-par

extern	int	DebugMode;
extern	FILE	*DebugFile;

//
// Define the master object for handling the UW protocol.
// Also define all of its static members.
//
UWProtocol	UWMaster;
#ifdef	DOOBERY
int	UWProtocol::CurrWindow;
int	UWProtocol::LastInput;
int	UWProtocol::RoundWindow;
int	UWProtocol::OutputWindow;
int	UWProtocol::gotmeta;
int	UWProtocol::gotiac;
int	UWProtocol::getpcl;
int	UWProtocol::newwind;
int	UWProtocol::dirproc;
UWClient  *UWProtocol::clients[NUM_UW_WINDOWS];
UWDisplay *UWProtocol::displays[NUM_UW_WINDOWS];
int	UWProtocol::numwinds;
UWClient  *UWProtocol::freelist;
int	UWProtocol::terminate;
int	UWProtocol::exitmulti;
int	UWProtocol::protocol;
#endif

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

//
// Define the protocol flags for UWMaster.protoflags.
//
#define	PF_IAC		1	// We just received an IAC character.
#define	PF_META		2	// A meta sequence was just received.
#define	PF_PNEG		4	// Protocol negotiation is in effect.
#define	PF_STRIP	8	// Strip high bits from the characters.
#define	PF_UWOFF	16	// Don't use the UW protocol at all.
#define	PF_DIRECT	32	// Pass characters through directly.
#define	PF_PROTO1	64	// Protocol 1 and higher is in effect.
#define	PF_PROTO2	128	// Protocol 2 and higher is in effect.
#define	PF_WTYPE	256	// Get window type in protocol 2.
#define	PF_NEGSET	512	// Negotiate a protocol set.
#define	PF_NEGCAN	1024	// Negotiate a protocol cancel.
#define	PF_WOPT		2048	// Reading window options from remote.

//
// Define the option state values for UWMaster.optflag.
//
#define	OPT_CMD		0	// Get the first command character.
#define	OPT_LCMD	1	// Get the second command character.
#define	OPT_INQUIRE	2	// Request for current
#define	OPT_SET		3	// Processing on option setting command.
#define	OPT_SKIP_1	4	// Skip one byte from the remote host.
#define	OPT_SKIP_2	5	// Skip two bytes from the remote host.
#define	OPT_SKIP_3	6	// Skip three bytes from the remote host.
#define	OPT_SKIP_4	7	// Skip four bytes from the remote host.
#define	OPT_SKIP_STR	8	// Skip the bytes of a string from the host.

// Get a terminal type from a terminal emulation.
static	int	GetTermType (unsigned char far *emul)
{
  // Determine the terminal type to create Protocol 2 windows with by //
  // inspecting the terminal type in the default Protocol 1 emulation //
  int version = ((*(emul + 4)) & 255) + (((*(emul + 5)) & 255) << 8);
  if (version >= UW_TERM_TYPECODE_VERS)
    return (*(emul + 6));	// Read the compiled-in terminal type.
   else
    return (UWT_ADM31);		// Default to ADM31 if unknown.
} // GetTermType //

// Transmit a character to the remote host.
inline	void	transmit (int ch)
{
  comsend (UWConfig.ComPort,ch);
#ifdef	TRANS_DEBUG
  if (DebugMode)
    {
      fputc ('<',DebugFile);
      fputc (ch,DebugFile);
      fputc ('>',DebugFile);
    }
#endif
} // transmit //

// Send a UW command to the remote host.
void	UWProtocol::command (int cmd)
{
  transmit (P1_IAC);
  transmit (cmd | P1_DIR_CTOH);
} // UWProtocol::command //

// Output a Protocol 2 option command to the remote host.
void	UWProtocol::option (int window,int action,int optnum)
{
  command (P2_FN_WOPT | window);
  if (!WONUM_USELONG(optnum))
    transmit (action | WONUM_SENCODE(optnum));
   else
    {
      transmit (action | WONUM_LPREFIX);
      transmit (WONUM_LENCODE(optnum));
    } /* if */
} // UWProtocol::option //

// Output a numeric option argument to the remote host.
// If "longval" is non-zero, the argument is 12-bit, rather
// than 6-bit.
void	UWProtocol::optarg (int arg,int longval)
{
  transmit (0100 | (arg & 077));
  if (longval)
    transmit (0100 | ((arg >> 6) & 077));
} // UWProtocol::optarg //

#define	WMASK(n)	((n) & (NUM_UW_WINDOWS - 1))

// Process a received option character for window "newwind".
void	UWProtocol::procoptions (int ch)
{
  int optnum,windnum;
  static int previac;
  if (DebugMode)
    fputc ('0' + optflag,DebugFile);
  switch (optflag)
    {
      case OPT_CMD:
		newwind = WMASK (newwind) | (WONUM_COMMAND(ch) << 6);
		if ((ch & WONUM_MASK) == WONUM_LPREFIX)
		  {
		    // Need to get the long form of an option //
		    optflag = OPT_LCMD;
		    break;
		  } /* if */
      		optnum = WONUM_SDECODE(ch);
		// Fall through to long option processing //
      case OPT_LCMD:
      		if (optflag == OPT_LCMD)
		  optnum = WONUM_LDECODE(ch);
		optflag = OPT_CMD;
		if (optnum == WOG_END)
		  {
		    // The option processing has finished //
		    protoflags &= ~PF_WOPT;
		    newwind = 0; // Must be set to 0 or create window mucks up.
		    break;
		  } /* if */
		switch (newwind >> 6)
		  {
		    case WOC_SET:	optflag = OPT_SET; break;
		    case WOC_INQUIRE:	optflag = OPT_INQUIRE; break;
		    case WOC_DO:	option(WMASK(newwind),WOC_WILL,optnum);
		    			transmit (0);
		    			optflag = OPT_INQUIRE;
		    			break;
		    case WOC_DONT:	option(WMASK(newwind),WOC_WONT,optnum);
		    			transmit (0);
		    			break;
		    default:		break;
		  } /* switch */
		break;
      case OPT_SKIP_1:
      		optflag = OPT_CMD;
		break;
      case OPT_SKIP_2:
      		optflag = OPT_SKIP_1;
		break;
      case OPT_SKIP_3:
      		optflag = OPT_SKIP_2;
		break;
      case OPT_SKIP_4:
      		optflag = OPT_SKIP_3;
		break;
      case OPT_SKIP_STR:
      		windnum = WMASK(newwind);
      		if (ch == 0)
		  {
		    // Got the end of the string - show the title now //
		    optflag = OPT_CMD;
		    if (displays[windnum])
		      displays[windnum] -> showtitle ();
		    status ();
		  }
		 else
		  {
		    // Another character in the string to add to the title //
		    if (previac)
		      {
		        if ((ch & P1_FN) != P1_FN_META &&
			    (ch & P1_FN) != P1_FN_CTLCH)
			  {
			    // Abort processing on non META or CTLCH command //
		    	    protoflags &= ~PF_WOPT;
		    	    newwind = 0;
		    	    if (displays[windnum])
		    	      displays[windnum] -> showtitle ();
		    	    status ();
			  }
		      }
		     else if (ch >= ' ' && ch < 0x7F && displays[windnum])
		      displays[windnum] -> addtitle (ch);
		    if (ch == IAC)	// Determine if a UW command follows.
		      previac = 1;
		     else
		      previac = 0;
		  }
		break;
    } /* switch */
  windnum = WMASK(newwind);
  if (optflag == OPT_INQUIRE)
    {
      // Process option inquiries: most of these are dummies to simulate //
      // the behaviour of the Macintosh client program.			 //
      optflag = OPT_CMD;
      if ((newwind >> 6) == WOC_DO && optnum != WOG_SIZE &&
          optnum != WOTTY_SIZE && optnum != WOG_TYPE)
	return;	// Ignore the "do" inquiry unless a size/termtype request.
      switch (optnum)
        {
	  case WOG_VIS:
	  		// Pretend the visibility is on always //
	  		option (windnum,WOC_SET,WOG_VIS);
			optarg (1,0);
			transmit (0);
			break;
	  case WOG_TYPE:
	  		// Send the window type to the remote host //
			option (windnum,WOC_INQUIRE,WOG_TYPE);
			transmit (0);
			option (windnum,WOC_SET,WOG_TYPE);
			if (clients[windnum])
			  optarg (clients[windnum] -> termtype,0);
			 else
			  optarg (UWT_ADM31,0);
			transmit (0);
			break;
	  case WOG_POS:
	  		// Pretend the window position is (0,0) //
			option (windnum,WOC_SET,WOG_POS);
			optarg (0,1);
			optarg (0,1);
			transmit (0);
			break;
	  case WOG_TITLE:
	  		// Send the window title to the remote host //
			option (windnum,WOC_SET,WOG_TITLE);
			char *title = displays[windnum] -> wtitle ();
			while (*title)
			  transmit (*title++);
			transmit (0);	// Terminate string.
			transmit (0);	// Terminate options.
			break;
	  case WOG_SIZE:
	  		// Get the size (in pixels) of the window //
			if (!displays[windnum])
			  break;
			option (windnum,WOC_SET,WOG_SIZE);
#ifdef	UWPC_DOS
			// Under DOS - make a dummy size in pixels //
	  		optarg (displays[windnum] -> height * 8,1);
	  		optarg (displays[windnum] -> width * 8,1);
#else	/* UWPC_DOS */
			// Under Windows - get the real window size //
			RECT rect;
			GetClientRect (displays[windnum] -> hWnd,&rect);
			optarg (rect.bottom - rect.top,1);
			optarg (rect.right - rect.left,1);
#endif	/* UWPC_DOS */
			transmit (0);
			break;
	  case WOTTY_SIZE:
	  		// Get the size (in characters) of the window //
			if (!displays[windnum])
			  break;
			option (windnum,WOC_SET,WOTTY_SIZE);
	  		optarg (displays[windnum] -> height,1);
	  		optarg (displays[windnum] -> width,1);
			transmit (0);
			break;
	  case WOTTY_FONTSZ:
	  		// Get the size of the font in the window //
			option (windnum,WOC_SET,WOTTY_FONTSZ);
			optarg (0,0);	// Specify a dummy "small" font.
			transmit (0);
			break;
	  case WOTTY_MOUSE:
	  		// Get the mouse event encoding flag //
			option (windnum,WOC_SET,WOTTY_MOUSE);
			optarg (0,0);	// Mouse events aren't being encoded.
			transmit (0);
			break;
	  case WOTTY_BELL:
	  		// Get the bell method flag //
			option (windnum,WOC_SET,WOTTY_BELL);
			optarg (2,0);	// Only audible bells supported.
			transmit (0);
			break;
	  case WOTTY_CURSOR:
	  		// Get the cursor type //
			option (windnum,WOC_SET,WOTTY_CURSOR);
			if (UWConfig.CursorSize != CURS_FULL_HEIGHT)
			  optarg (1,0);	// Non-block cursor.
			 else
			  optarg (0,0);	// Block cursor.
			transmit (0);
			break;
	  default:	break;
	} /* switch */
    } /* then */
   else if (optflag == OPT_SET)
    {
      // Set things up to skip the characters in an option setting command //
      optflag = OPT_CMD;
      switch (optnum)
        {
	  case WOG_VIS: case WOG_TYPE: case WOTTY_FONTSZ: case WOTTY_MOUSE:
	  case WOTTY_BELL: case WOTTY_CURSOR:
	  	optflag = OPT_SKIP_1; break;
	  case WOG_TITLE:
	  	if (displays[windnum])
		  displays[windnum] -> clrtitle ();
	  	previac = 0;
	  	optflag = OPT_SKIP_STR;
		break;
	  case WOG_POS: case WOG_SIZE: case WOTTY_SIZE:
	  	optflag = OPT_SKIP_4; break;
	} /* switch */
    } /* then */
} // UWProtocol::procoptions //

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
      if (OutputWindow != CurrWindow &&
          !(clients[OutputWindow] -> recvchars))
        {
	  // Update the status line to indicate characters in the window //
	  clients[OutputWindow] -> recvchars = 1;
	  status ();
	} /* if */
      RoundWindow = OutputWindow;
      clients[OutputWindow] -> remote (ch);
      if (clients[OutputWindow] -> firstch)
        {
	  // Send the terminal type once the first character received //
	  clients[OutputWindow] -> firstch = 0;
          switch (UWConfig.ShellKind)
            {
	      case SHELL_NONE:	 break;
	      case SHELL_BOURNE: startclient ('t'); break;
	      case SHELL_CSHELL: startclient ('u'); break;
	      case SHELL_STRING: sendstring (UWConfig.ShellString); break;
	      default:		 break;
	    } /* switch */
	} /* if */
    }
} // UWProtocol::remote //

// Process a character incoming from the host.  Note: protoflags
// is copied into the register variable flags to help increase the
// speed of checking the heaps of flags in this function.  Previously
// the routine was spending lots of time doing unnecessary object
// dereferences to get simple flags.
void	UWProtocol::fromhost (int ch)
{
  register int flags;
  int arg,temp,windtype;
  flags = protoflags;		// Get the current flag settings.
  if (DebugMode)
    fputc (ch,DebugFile);	// Save character in the debugging file.
  // if (protocol > 0 || (UWConfig.StripHighBit && !dirproc))
  if (protocol > 0 || ((flags & PF_STRIP) && !(flags & PF_DIRECT)))
    ch &= 0x7F;		// Strip unwanted parity bit.
   else
    ch &= 0xFF;		// Make a little more 8-bit clean.
  // if (getpcl)
  if (flags & PF_PNEG)
    {
      // Process the character for a protocol negotiation //
      int proto;
      proto = ch - ' ' + 1;		// Get protocol number from remote
      if (flags & PF_NEGSET)		// Set the protocol if forced to
        protocol = proto;		// by the server.
       else if (proto > UWConfig.MaxProtocol) // Need a supported protocol.
        {
	  // Cancel the server's suggested protocol and suggest another //
	  command (P1_FN_MAINT | P1_MF_CANPCL);
	  transmit (UWConfig.MaxProtocol - 1 + ' ');
	}
       else if (flags & PF_NEGCAN)	// If a supported protocol set it.
        {
	  // Protocol is acceptable - tell the server to set it //
	  command (P1_FN_MAINT | P1_MF_SETPCL);
	  transmit (proto - 1 + ' ');
	  protocol = proto;		// Set the new protocol to be used.
	}
      // getpcl = 0;
      protoflags &= ~(PF_PNEG | PF_NEGCAN | PF_NEGSET);
      return;				// Exit - character processed
    }
  // if (gotiac)
  if (flags & (PF_IAC | PF_WTYPE | PF_WOPT))
    {
      int windtype;
      // if (newwind)
      if (flags & PF_WTYPE)
        {
	  // Get the window type character for a
	  // window creation in Protocol 2.			
	  windtype = ch - ' ';
	  ch = P1_DIR_HTOC | P1_FN_NEWW | newwind;
	} /* then */
       else if (flags & PF_WOPT)
        {
	  // Process an option character for a window //
	  procoptions (ch);
	  return;
	} /* then */
      // gotiac = 0;
      protoflags &= ~(PF_IAC | PF_WTYPE);
      if ((ch & P1_DIR) != P1_DIR_HTOC)
        return;				// Skip command - wrong direction.
      arg = ch & P1_FN_ARG;		// Get function argument.
      switch (ch & P1_FN)
        {
	  case P1_FN_NEWW:
	  	if (protocol < 1 || arg < 1)
		  break;
		if (protocol > 1)
		  {
		    /* Process the window type char in Protocol 2 */
		    if (!newwind && arg)
		      {
		        newwind = arg;
			protoflags |= PF_WTYPE;
			if (DebugMode)
			  fputc ('?',DebugFile);
			// gotiac = 2;
			break;
		      } /* then */
		     else
		      {
		        if (DebugMode)
			  fputc ('/',DebugFile);
			arg = newwind;
			newwind = 0;
			if (UWConfig.ObeyTerm == OBEY_IGNORE)
			  windtype = UWT_UNKNOWN;
			 else if (!numwinds &&
			 	  UWConfig.ObeyTerm == OBEY_NOTFIRST)
			  windtype = UWT_UNKNOWN;
		      } /* else */
		  } /* then */
		 else
		  windtype = UWT_UNKNOWN;
	  	if (!(temp = create (arg,windtype))) // Attempt to create.
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
		newwind = arg;		// Save the window number.
		protoflags |= PF_WOPT;	// Process options on next character.
		optflag = OPT_CMD;	// Start in the right option state.
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
	  	  protoflags |= PF_META;	// Enable the next meta bit.
	  	  // gotmeta = 1;		// Enable the next meta bit.
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
		    // if (gotmeta)
		      // {
		        // gotmeta = 0;
			// ch |= 0x80;
		      // }
		    if (flags & PF_META)
		      {
		        protoflags &= ~PF_META;
			remote (ch | 0x80);
		      }
		     else
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
		    		protoflags |= PF_PNEG | PF_NEGCAN;
				break;
		    case P1_MF_SETPCL:
		    		protoflags |= PF_PNEG | PF_NEGSET;
		    		// getpcl = arg;
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
   // else if (ch == P1_IAC && !dirproc && !UWConfig.DisableUW)
   else if (ch == P1_IAC && !(flags & (PF_DIRECT | PF_UWOFF)))
    // gotiac = 1;
    protoflags |= PF_IAC;
   else
    {
      // if (gotmeta)
        // {
	  // ch |= 0x80;		// Enable the received meta bit.
	  // gotmeta = 0;
	// }
      if (flags & PF_META)
        {
	  protoflags &= ~PF_META;
	  remote (ch | 0x80);
	}
       else
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
	      		int wflag=0;
  			for (window = 1;window < NUM_UW_WINDOWS;++window)
  			  {
  			    if (displays[window] != 0)
        		      {
			        wflag = 1;
	  			if (CurrWindow == window)
	    			  {
	      			    Buffer[posn++] = '\001';
	      			    Buffer[posn++] = window + '0';
	      			    Buffer[posn++] = '\002';
				    attrbytes += 2;
	    			  }
	   			 else
	    			  Buffer[posn++] = window + '0';
				if (clients[window] -> recvchars)
				  Buffer[posn++] = '*';
				 else
	  			  Buffer[posn++] = ' ';
			      }
    			  }
			if (!wflag)
			  {
			    // Fake out the while loop into believing that
			    // at least one window is present.
			    Buffer[posn++] = '\001';
			    Buffer[posn++] = '\002';
			  }
			while ((posn - startposn) < 16) // 14 + 2 attr chars
			  Buffer[posn++] = ' ';
			break;
	      case 'c': char *title;
	      		if (displays[CurrWindow])
			  title = displays[CurrWindow] -> wtitle ();
			 else
			  title = "";
	      		startposn = posn;
			while (*title && posn < (STR_LEN - 1) &&
			       (posn - startposn) < UWConfig.MaxTitleLen)
			  Buffer[posn++] = *title++;
			while (posn < (STR_LEN - 1) &&
			       (posn - startposn) < UWConfig.MaxTitleLen)
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
	      case 'r': Buffer[posn++] = protocol + '0';
	      		break;
	      case 's': startposn = CurrSystemTime ();	// Get system time.
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
	      		    startposn = OnlineTime;	// Get online time.
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

// Change the title bar on a client's window to reflect
// the current mode.  This doesn't do anything under DOS.
void	UWProtocol::titlebar (UWClient *client)
{
#ifdef	UWPC_WINDOWS
#define	WIND_TEXT_LEN	64
  HWND hWnd;
  char title[WIND_TEXT_LEN],far *name;
  int posn;
  hWnd = (client -> getwind ()) -> hWnd;
  GetWindowText (hWnd,title,WIND_TEXT_LEN);
  posn = 0;
  while (title[posn] && title[posn] != '[')
    ++posn;
  while (posn > 0 && title[posn - 1] == ' ')
    --posn;
  if (client -> isaterminal)
    title[posn] = '\0';
   else
    {
      // Get the name of the client, skip spaces, and display it //
      name = client -> name ();
      while (*name == ' ')
        ++name;
      wsprintf (title + posn," [%s]",(LPSTR)name);
    } /* if */
  SetWindowText (hWnd,title);
#endif	/* UWPC_WINDOWS */
} // UWProtocol::titlebar //

#define	SLICE_SIZE	8	// Must be a power of 2.
#define	RECEIVE_SLICE	15	// This can be any value.

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
  int wincount=0;	// Counter for scheduling Yield requests.
#define	WIN_MAXCOUNT	8
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
  protoflags = (UWConfig.StripHighBit ? PF_STRIP : 0) |
  	       (UWConfig.DisableUW ? PF_UWOFF : 0);
  gotmeta = 0;
  gotiac = 0;
  getpcl = 0;
  newwind = 0;
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
  clients[0] -> termtype = GetTermType (UWConfig.P0TermType);
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

#ifdef	UWPC_WINDOWS
  // Enable the communications port
  comenable (UWConfig.ComPort,(UWConfig.ComFossil ? COMEN_FOSSIL : 0) |
  			      (UWConfig.ComCtsRts ? COMEN_CTSRTS : 0));
  comparams (UWConfig.ComPort,UWConfig.ComParams);
#endif	/* UWPC_WINDOWS */

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

      // Process characters received from the remote host.  //
      // The looping until the time-slice is up is designed //
      // to give better performance when lots of characters //
      // are being received at once.			    //
      if ((ch = comreceive (UWConfig.ComPort)) >= 0)
        {
	  fromhost (ch);
	  if ((ch = comreceive (UWConfig.ComPort)) >= 0)
	    {
#ifdef	UWPC_WINDOWS
	      // Suspend the character redrawing to get some speed //
	      int wind = CurrWindow;
	      if (displays[wind])
	        displays[wind] -> suspend (1);
#endif
              int recslice = 0;
	      do
	        {
		  fromhost (ch);
		}
              while (++recslice < RECEIVE_SLICE &&
      	             (ch = comreceive (UWConfig.ComPort)) >= 0);
#ifdef	UWPC_WINDOWS
	      // Turn the character redrawing back on //
	      if (displays[wind])
	        displays[wind] -> suspend (0);
#endif
	    } /* if */
	} /* if */

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

      if (!(slice = ((slice + 1) & (SLICE_SIZE - 1))))
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
      if (++wincount >= WIN_MAXCOUNT)
        {
	  wincount = 0;

          // Let the other applications do some work //
          Yield ();

          // Process the Windows 3.0 messages that are waiting for processing //
          if (!PeekMessage (&msg,NULL,NULL,NULL,PM_NOYIELD | PM_REMOVE))
	    continue;
          if (msg.message == WM_QUIT)
            break;		// The application has terminated.
          if (!TranslateAccelerator (hMainWnd,hAccTable,&msg))
            {
	      // Don't translate the BS key, because we don't want it
	      // to be processed twice in the handling for WM_KEYDOWN
	      // and WM_CHAR.
	      if (msg.message != WM_KEYDOWN || msg.wParam != VK_BACK)
	        TranslateMessage (&msg);
	      DispatchMessage (&msg);
	    }
	} /* if */
#endif	/* UWPC_WINDOWS */
    }

  // Destroy all active windows and exit UW protocol handling //
  exit ();
#ifdef	UWPC_WINDOWS
  delete displays[0];		// Remove the protocol 0 window.
#endif	/* UWPC_WINDOWS */

#ifdef	UWPC_WINDOWS
  comdisable (UWConfig.ComPort,1);
#endif	/* UWPC_WINDOWS */
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

// Process a timer pulse in the Windows 3.0 version of
// the program.  The timer pulse is passed onto all
// currently active clients.
void	UWProtocol::timer (void)
{
  int window;
  for (window = 0;window < NUM_UW_WINDOWS;++window)
    {
      RoundWindow = window;
      if (clients[window])
        clients[window] -> timertick ();
    } /* for */
} // UWProtocol::timer //

// Send a mouse message to a particular window.  This
// is only called in the Windows 3.0 version.
void	UWProtocol::mouse (int wind,int x,int y,int buttons)
{
  int oldwind = RoundWindow;
  RoundWindow = wind;
  clients[wind] -> mouse (x,y,buttons);
  RoundWindow = oldwind;
} // UWProtocol::mouse //

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
  hMenu = GetMenu (displays[0] -> hWnd);
  EnableMenuItem (hMenu,IDM_EXIT,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_NEW,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_KILL,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_NEXTWIN,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_MINALL,MF_GRAYED);
  EnableMenuItem (hMenu,IDM_START,MF_ENABLED);
  EnableMenuItem (hMenu,IDM_HANGUP,MF_ENABLED);
  EnableMenuItem (hMenu,IDM_BREAK,MF_ENABLED);
#endif	/* UWPC_WINDOWS */
} // UWProtocol::exit //

// Create a new window (ignored in Protocol 0).  Returns
// the identifier, or 0 if no window could be created.
// If number != 0, then the number has been supplied
// explicitly, usually by the remote host.
int	UWProtocol::create (int number,int windtype)
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
  unsigned char far *emul;
  if (protocol < 2 || windtype == UWT_UNKNOWN || windtype == UWT_NOTUW)
    {
      windtype = GetTermType (UWConfig.P1TermType);
      emul = UWConfig.P1TermType;
    }
   else if ((emul = UWConfig.getterminal (windtype)) == NULL)
    emul = UWConfig.P1TermType;
  if (windtype == UWT_UNKNOWN || windtype == UWT_NOTUW)
    windtype = UWT_ADM31;
  clients[number] -> termtype = windtype;
  if (clients[number] -> isaterminal)
    ((UWTermDesc *)clients[number]) -> setemul (emul);
  if (numwinds == 0)		// If this is the first window in Protocol 1
    {				// then display it at the top.
      top (number);
#ifdef	UWPC_WINDOWS
      // Minimize the main window when entering Protocol 1 to get it //
      // out of the way on the Windows 3.0 desktop.		     //
      ShowWindow (displays[0] -> hWnd,SW_MINIMIZE);

      // Enable the Protocol 1 specific menu items //
      HMENU hMenu;
      hMenu = GetMenu (displays[0] -> hWnd);
      EnableMenuItem (hMenu,IDM_EXIT,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_NEW,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_KILL,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_NEXTWIN,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_MINALL,MF_ENABLED);
      EnableMenuItem (hMenu,IDM_START,MF_GRAYED);
      EnableMenuItem (hMenu,IDM_HANGUP,MF_GRAYED);
      EnableMenuItem (hMenu,IDM_BREAK,MF_GRAYED);
#endif	/* UWPC_WINDOWS */
    } /* if */
  ++numwinds;
  status ();			// Redraw the status line.
  if (docreate)
    {
      command (P1_FN_NEWW | number); // Send a create command to remote host.
      if (protocol > 1)
        {
	  transmit (windtype + ' '); // Send the window type in Protocol 2.
	  option (number,WOC_SET,WOTTY_SIZE); // Set the window size.
	  optarg (displays[number] -> height,1);
	  optarg (displays[number] -> width,1);
	  transmit (0);
	} /* if */
      if (UWConfig.ShellKind != SHELL_NONE)
        clients[number] -> firstch = 1;	// Request terminal type to be sent.
    } /* if */
  return (number);		// Window successfully created.
} // UWProtocol::create //

// Install a new client on top of the one in the current
// round-robin window.
void	UWProtocol::install (UWClient *newclient)
{
  newclient -> setunder (clients[RoundWindow]);
  clients[RoundWindow] = newclient;
  status ();
  titlebar (newclient);
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
  titlebar (clients[RoundWindow]);
} // UWProtocol::remove //

// Turn direct character processing in protocol 0 on or off.
void	UWProtocol::direct (int on)
{
  if (protocol == 0)
    {
      dirproc = on;
      if (on)
        protoflags |= PF_DIRECT;
       else
        protoflags &= ~PF_DIRECT;
    } /* if */
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
  if (number != CurrWindow)	// Only bring a new one to the top if we
    {				// have killed the current one.
      LastInput = 0;
      status ();
      return;
    }
  number = 1;			// Search for a new top window on a normal kill
  while (number < NUM_UW_WINDOWS && clients[number] == 0)
    ++number;
  if (number < NUM_UW_WINDOWS)
    {
      top (number);		// Set a new top window.
      CurrWindow = number;
      LastInput = 0;		// Force an input window change.
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
  clients[CurrWindow] -> recvchars = 0;	// Indicate no received characters.
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
    exitmulti = 1;
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
  void	sttysendlower (char far *str);
#ifdef	UWPC_WINDOWS
  extern HCURSOR hHourGlass;
  HCURSOR hCursor;
  hCursor = SetCursor (hHourGlass);
#endif
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
	  if (*(str + 1) == '?')	// Check for terminal type request.
	    sttysendlower (clients[RoundWindow] -> name ());
	   else if (*(str + 1))
	    send (*(++str));		// Send following character direct.
	}
       else
        send (*str);			// Send the character direct
      ++str;
    }
#ifdef	UWPC_WINDOWS
  SetCursor (hCursor);
#endif
} // UWProtocol::sendstring //

// Exit protocol 1 and send a modem line break.
void	UWProtocol::sendbreak (void)
{
  if (protocol > 0)			// Exit protocol 1 if necessary.
    exitmulti = 1;
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
