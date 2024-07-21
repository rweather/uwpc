/*-------------------------------------------------------------------------

  MAIN.C - Main module for the UW program.

    This file is part of UW/PC - a multi-window comms package for the PC.
    Copyright (C) 1990-1991  Rhys Weatherley

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Revision History:
  ================

   Version  DD/MM/YY  By  Description
   -------  --------  --  --------------------------------------
     1.0    14/12/90  RW  Original Version of MAIN.C
     1.1    01/01/91  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#include "device.h"		/* Device handling declarations */
#include "keys.h"		/* Keyboard handling declarations */
#include "uw.h"			/* The Unix Windows protocol */
#include "term.h"		/* Terminal handling routines */
#include "config.h"		/* Configure the program */
#include "ascii.h"		/* ASCII file transfers */
#include <stdio.h>		/* Standard I/O routines */

#ifdef	__TURBOC__
#pragma	warn -par		/* Turn off annoying warnings */
#endif

/* Forward declarations for this module */
void	_Cdecl	AbortProgram    (char *msg);
void	_Cdecl	HandleTerminal	(void);
void	_Cdecl	OutputString	(char *str);

/* Define the base name of the configuration file.  The current */
/* directory is tried first, and then the program directory.	*/
char	_Cdecl	*ConfigFile="UW.CFG";

/* Define the default upload and download file names */
#define	FILE_NAME_SIZE	64
static	char	_Cdecl	UploadFile[FILE_NAME_SIZE + 1];
static	char	_Cdecl	DownloadFile[FILE_NAME_SIZE + 1];

/* Main program entry point */
int	_Cdecl	main (int argc,char *argv[])
{
  Configure (argv[0]);		/* Configure the program */
  UploadFile[0] = '\0';
  DownloadFile[0] = '\0';
  if (!InitComDevice ())
    AbortProgram ("Cannot initialise COM port");
  InitUWProtocol (Def0TermType);
  InitKeyboard ();
  OutputString ("UW/PC version 1.04, Copyright (C) 1990-1991 Rhys Weatherley\r\n");
  OutputString ("UW/PC comes with ABSOLUTELY NO WARRANTY; see the file COPYING for details.\r\n");
  OutputString ("This is free software, and you are welcome to redistribute it\r\n");
  OutputString ("under certain conditions; see the file COPYING for details.\r\n\r\n");
  OutputString ("Press ALT-Z for help on special keys.\r\n\r\n");
  if (ModemCInit || !TestConnection ())
    ModemInitString ();		/* Send if flag set, or no carrier present */
  HandleTerminal ();
  TermKeyboard ();
  TermUWProtocol ();
  TermComDevice (0);
  return (0);
} /* main */

/* Abort the program with a message on stderr */
static	void	_Cdecl	AbortProgram (char *msg)
{
  fprintf (stderr,"Program aborted: %s\n",msg);
  exit (1);
} /* AbortProgram */

/* Handle the terminal, processing keystrokes and the UW protocols */
static	void	_Cdecl	HandleTerminal (void)
{
  int key,window;
  while (1)
    {
      UWTick ();		/* Do some Unix Windows processing */
      key = GetKeyPress ();	/* Get the user's keypress and decode */
      if (SwapBackSpaces)
        {
	  /* Swap the BS and DEL key usage values if required */
	  if (key == 8)
	    key = 0x7F;
	   else if (key == 0x7F)
	    key = 8;
	} /* if */
      if (XonXoffDirect && (key == 021 || key == 023))
	WriteComDevice (key);	/* Send XON and XOFF direct if nec */
       else if (key >= 0 && key <= 255 && key != '\033')
	UWProcessChar (key);	/* Process the normal ASCII character */
       else
	{
	  switch (key)
	    {
	      case QUIT_KEY2:
	      case QUIT_KEY: key=PopupBox("Exit UW/PC? Yes or No","yYnN\033");
			     if (key != 'Y' && key != 'y')
			       break;
			     UWExit ();	/* Exit the protocol */
			     return;	/* Terminate the program */
	      case HELP_KEY: HelpScreen ();
			     break;
	      case EXIT_KEY: if (UWProtocol > 0)
			       {
				 key=PopupBox ("Exit? Yes or No","yYnN\033");
				 if (key != 'Y' && key != 'y')
				   break;
			       } /* if */
			     UWExit ();	/* Exit the protocol */
			     break;
	      case HANGUP_KEY:if (UWProtocol > 0)  /* Confirm in protocol 1 */
	                       {
			         key=PopupBox("Hangup? Yes or No","yYnN\033");
				 if (key != 'Y' && key != 'y')
				   break;
			       } /* if */
	      		     UWExit ();
			     ModemHangup ();
			     break;
	      case BREAK_KEY:if (UWProtocol > 0)  /* Confirm in protocol 1 */
	                       {
			         key =
				  PopupBox("Send BREAK? Yes or No","yYnN\033");
				 if (key != 'Y' && key != 'y')
				   break;
			       } /* if */
	      		     UWExit ();/* Exit before break, for safety */
			     DeviceBreak ();
			     break;
	      case NEW_KEY:  if ((window = UWTerminal (DefTermType)) > 0)
			       UWTopWindow (window);
			     break;
	      case KILL_KEY: UWKillWindow (UWCurrWindow);
			     break;
	      case JUMP_DOS_KEY:
			     JumpToDOS ();
			     break;
	      case INIT_KEY: ModemInitString ();
			     break;
	      case START_KEY:if (UWProtocol < 1)
			       {
				 /* Send "uw^M" to the host to begin   */
				 /* a Unix Windows session on the host */
				 CommandString ();
			       } /* if */
			     break;
	      case DIAL_KEY: DialModem (); break;
	      case UPLOAD_KEY:		/* Perform a file upload */
	      case CURSOR_PGUP:
/*	      		     key = PopupBox ("Upload: [X]modem or [A]scii ?",
			     		     "XxAa\033");
			     if (key != 'a' && key != 'A')
			       break;				*/
			     if (!PromptUser ("Enter file to upload, ^X to"
			     		      " clear line, or ESC to abort.",
					      UploadFile,FILE_NAME_SIZE))
			       break;		/* User pressed ESC */
			     AsciiSend (UWCurrWindow,UploadFile);
	      		     break;
	      case DOWNLOAD_KEY:	/* Perform a file download */
	      case CURSOR_PGDN:
/*	      		     key = PopupBox ("Download: [X]modem or [A]scii ?",
			     		     "XxAa\033");
			     if (key != 'a' && key != 'A')
			       break;				*/
			     if (!PromptUser ("Enter file to download, ^X to"
			     		      " clear line, or ESC to abort.",
					      DownloadFile,FILE_NAME_SIZE))
			       break;		/* User pressed ESC */
			     AsciiReceive (UWCurrWindow,DownloadFile,0);
	      		     break;
	      default:	     if (key >= ALT_WIND_NUM(1) &&
				 key <= ALT_WIND_NUM(7))
			       UWTopWindow (ALT_GET_NUM(key));
			      else if (key >= 0x3B00 && key <= 0x4400)
			       {
			         /* Process a function key from F1 to F10 */
			         int func;
				 func = (key >> 8) - 0x3B;
				 if (FunctionKeys[func][0] != '\0')
				   SendFunction (func); /* Send string */
				  else
				   UWSendKey (key); /* Send through terminal */
			       } /* then */
			      else if (key != '\033' ||
			      	 UWProcList[UWCurrWindow].transfer != NULL)
			       UWSendKey (key); /* Send a special key */
			      else if (key == '\033')
			       UWProcessChar (key); /* Process ESC normally */
			     break;
	    } /* switch */
	} /* if */
    } /* while */
} /* HandleTerminal */

/* Output a string to window 0 */
static	void	_Cdecl	OutputString (char *str)
{
  while (*str)
    TermOutput (0,*str++);
} /* OutputString */
