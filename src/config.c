/*-------------------------------------------------------------------------

  CONFIG.C - Configuration declarations for the UW client.

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
     1.0    28/12/90  RW  Original Version of CONFIG.C
     1.1    01/01/90  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#include "config.h"		/* Declarations for this module */
#include "device.h"		/* Communication device handling */
#include "comms.h"		/* Low-level serial routines */
#include "term.h"		/* Terminal handling routines */
#include "keys.h"		/* Keyboard handling declarations */
#include <stdio.h>		/* Standard I/O routines */
#include <errno.h>		/* Error number declarations */
#include <ctype.h>		/* Character type macros */
#include <stdarg.h>		/* Variable argument list parsing */
#include <stdlib.h>		/* "atoi" is defined here */

/* Define the status of the XON/XOFF keys.  If this variable is */
/* non-zero the XON and XOFF characters are sent direct.	*/
int	_Cdecl	XonXoffDirect=0;

/* Global data for this module */
static	char	_Cdecl	Buffer[BUFSIZ];
static	int	_Cdecl	LineNum;
static	FILE	*_Cdecl	Config;
static	int	_Cdecl	PrevChar;
static	int	_Cdecl	Emul0Set;	/* Non-zero if "emul0" was set */

/* Define a mapping between baud values for internal */
/* and external representations.		     */
static	struct	{ int baud,value; } _Cdecl	BaudMappings[] =
	  {{110,BAUD_110},
	   {150,BAUD_150},
	   {300,BAUD_300},
	   {600,BAUD_600},
	   {1200,BAUD_1200},
	   {2400,BAUD_2400},
	   {4800,BAUD_4800},
	   {9600,BAUD_9600},
	   {19200,BAUD_19200},
	   {0,0}};

/* Abort the reading of the configuration file */
static	void	_Cdecl	AbortConfig (void)
{
  perror (ConfigFile);		/* Report an error for the file */
  if (Config != NULL)
    fclose (Config);
  exit (1);			/* Exit the program */
} /* AbortConfig */

/* Abort the program with a configuration error */
static	void	_Cdecl	ConfigError (char *msg)
{
  fclose (Config);
  fprintf (stderr,"Line %d: %s\n",LineNum,msg);
  exit (1);
} /* ConfigError */

/* Read the next character from the configuration file */
static	int	_Cdecl	ReadConfig (void)
{
  if (PrevChar == EOF)		/* Don't allow a read past EOF */
    return (EOF);
  if (PrevChar == '\n')		/* Advance the line count if required */
    LineNum++;
  PrevChar = fgetc (Config);	/* Read the next character from the file */
  return (PrevChar);
} /* ReadConfig */

/* Validate the number in Buffer against a list of numbers */
/* Return the number once validated, or report an error.   */
static	int	ValidNumber (int first,...)
{
  int current,number;
  va_list args;
  number = atoi (Buffer);	/* Convert the buffer to decimal */
  current = first;
  va_start (args,first);
  while (current)
    {
      /* Check to see if the number is valid */
      if (number == current)
	{
	  do			/* Eat up rest of args (for safety) */
	    {
	      current = va_arg (args,int);
	    }
	  while (current);
	  va_end (args);
	  return (number);
	} /* if */
      current = va_arg (args,int);
    } /* while */
  va_end (args);
  ConfigError ("Illegal numeric argument");
  return (0);			/* Never done - to shut up warnings */
} /* ValidNumber */

/* Read and process a configuration command */
static	int	_Cdecl	ConfigCommand (int ch)
{
  static char command[11];
  int index=0;
  int type;

  /* Read the command name from the file */
  while (index < 10 && ch != EOF && (isalpha (ch) || isdigit (ch)))
    {
      command[index++] = ch;
      ch = ReadConfig ();
    } /* while */
  if (ch != EOF && ch != '=' && !isspace (ch))
    ConfigError ("Illegal command name");
  command[index] = '\0';

  /* Skip any white space before "=" */
  while (ch != EOF && isspace (ch))
    ch = ReadConfig ();
  if (ch != '=')
    ConfigError ("\"=\" expected in command");

  /* Skip any white space after "=" */
  do
    {
      ch = ReadConfig ();
    }
  while (ch != EOF && isspace (ch));

  /* Read the argument from the file */
  index = 0;
  if (ch == '"')
    {
      /* Read a quote-delimited string argument */
      ch = ReadConfig ();
      while (index < (BUFSIZ - 1) && ch != EOF && ch != '"')
	{
	  Buffer[index++] = ch;
	  ch = ReadConfig ();
	} /* while */
      if (ch != '"')
	ConfigError ("Unterminated string constant");
      ch = ReadConfig ();
      Buffer[index] = '\0';
      type = 0;		/* Indicate a string argument */
    } /* then */
   else if (ch != EOF && isdigit (ch))
    {
      /* Read a numeric argument */
      while (index < (BUFSIZ - 1) && ch != EOF && isdigit (ch))
	{
	  Buffer[index++] = ch;
	  ch = ReadConfig ();
	} /* while */
      if (ch != EOF && ch != '#' && !isspace (ch))
	ConfigError ("Illegal numeric argument");
      Buffer[index] = '\0';
      type = 1;		/* Indicate a numeric argument */
    } /* then */
   else if (ch != EOF && isalpha (ch))
    {
      /* Read an alphanumeric argument */
      while (index < (BUFSIZ - 1) && ch != EOF &&
      	     (isalpha (ch) || isdigit (ch)))
	{
	  Buffer[index++] = ch;
	  ch = ReadConfig ();
	} /* while */
      if (ch != EOF && ch != '#' && !isspace (ch))
	ConfigError ("Illegal alphanumeric argument");
      Buffer[index] = '\0';
      type = 2;		/* Indicate an alphanumeric argument */
    } /* then */
   else
    ConfigError ("Argument expected");

  /* Process the command to determine what actions need */
  /* to be performed to modify UW/PC's behaviour.       */
  if (!stricmp (command,"port") && type == 1)
    ComPort = ValidNumber (1,2,0);
   else if (!stricmp (command,"baud") && type == 1)
    {
      int baud,index;
      baud = ValidNumber (110,150,300,600,1200,2400,4800,9600,19200,0);
      index = 0;
      while (BaudMappings[index].baud && BaudMappings[index].baud != baud)
	++index;
      if (BaudMappings[index].baud)
	ComParams = (ComParams & ~BAUD_RATE) | BaudMappings[index].value;
       else
	ConfigError ("Internal error - contact author");
    } /* then */
   else if (!stricmp (command,"parity") && type == 2)
    {
      if (!stricmp (Buffer,"none"))
	ComParams = (ComParams & ~PARITY_GET) | PARITY_NONE;
       else if (!stricmp (Buffer,"even"))
	ComParams = (ComParams & ~PARITY_GET) | PARITY_EVEN;
       else if (!stricmp (Buffer,"odd"))
	ComParams = (ComParams & ~PARITY_GET) | PARITY_ODD;
       else
	ConfigError ("Illegal parity: must be \"none\", \"even\" or \"odd\"");
    } /* then */
   else if (!stricmp (command,"bits") && type == 1)
    {
      if (ValidNumber (7,8,0) == 8)
	ComParams |= BITS_8;
       else
	ComParams &= ~BITS_8;
    } /* then */
   else if (!stricmp (command,"stop") && type == 1)
    {
      if (ValidNumber (1,2,0) == 2)
	ComParams |= STOP_2;
       else
	ComParams &= ~STOP_2;
    } /* then */
   else if (!stricmp (command,"init") && type == 0)
    {
      if (strlen (Buffer) > MODEM_STR_SIZE)
	ConfigError ("Initialisation string is too long");
      strcpy (ModemInit,Buffer);
    } /* then */
   else if (!stricmp (command,"cinit") && type == 2)
    {
      if (!stricmp (Buffer,"yes"))
	ModemCInit = 1;
       else if (!stricmp (Buffer,"no"))
	ModemCInit = 0;
       else
	ConfigError ("Illegal value: must be \"yes\" or \"no\"");
    } /* if */
   else if (!stricmp (command,"hangup") && type == 0)
    {
      if (strlen (Buffer) > MODEM_STR_SIZE)
	ConfigError ("Hangup string is too long");
      strcpy (ModemHang,Buffer);
    } /* then */
   else if (!stricmp (command,"emul") && type == 2)
    {
      if (!stricmp (Buffer,"adm31"))
	DefTermType = UWT_ADM31;
       else if (!stricmp (Buffer,"vt52"))
	DefTermType = UWT_VT52;
       else
	ConfigError ("Illegal value: must be \"adm31\" or \"vt52\"");
    } /* then */
   else if (!stricmp (command,"emul0") && type == 2)
    {
      if (!stricmp (Buffer,"adm31"))
	Def0TermType = UWT_ADM31;
       else if (!stricmp (Buffer,"vt52"))
	Def0TermType = UWT_VT52;
       else
	ConfigError ("Illegal value: must be \"adm31\" or \"vt52\"");
      Emul0Set = 1;
    } /* then */
   else if (!stricmp (command,"xonxoff") && type == 2)
    {
      if (!stricmp (Buffer,"direct"))
	XonXoffDirect = 1;
       else if (!stricmp (Buffer,"encoded"))
	XonXoffDirect = 0;
       else
	ConfigError ("Illegal value: must be \"direct\" or \"encoded\"");
    } /* then */
   else if (!stricmp (command,"beep") && type == 2)
    {
      if (!stricmp (Buffer,"on"))
	AllowBeep = 1;
       else if (!stricmp (Buffer,"off"))
	AllowBeep = 0;
       else
	ConfigError ("Illegal value: must be \"on\" or \"off\"");
    } /* then */
   else if (!stricmp (command,"dial") && type == 0)
    {
      if (strlen (Buffer) > MODEM_STR_SIZE)
	ConfigError ("Dialing string is too long");
      strcpy (ModemDial,Buffer);
    } /* then */
   else if (!stricmp (command,"uw") && type == 0)
    {
      if (strlen (Buffer) > MODEM_STR_SIZE)
        ConfigError ("Command name string is too long");
      strcpy (UWCommandString,Buffer);
    } /* then */
   else if ((command[0] == 'f' || command[0] == 'F') && type == 0)
    {
      int func;
      if ((func = atoi (command + 1)) < 1 || func > 10)
        ConfigError ("Illegal function key number");
      if (strlen (Buffer) > FUNC_STR_SIZE)
	ConfigError ("Function key definition string is too long");
      strcpy (FunctionKeys[func - 1],Buffer);
    } /* then */
   else
    ConfigError ("Illegal command format");
  return (ch);		/* Return the next character */
} /* ConfigCommand */

/* Read the configuration file and set the configurable parameters  */
/* Will abort the program if a configuration error is detected.	The */
/* program name is supplied so the configuration file can be found. */
void	_Cdecl	Configure (char *progname)
{
  int ch;

  /* Attempt to open the configuration file */
  if ((Config = fopen (ConfigFile,"r")) == NULL)
    {
      int len;
      /* Build the name in the program's directory */
      strcpy (Buffer,progname);
      len = strlen (Buffer);
      while (len > 0 && Buffer[len - 1] != '/' && Buffer[len - 1] != '\\')
	--len;
      if (len <= 0)
	return;
      strcpy (Buffer + len,ConfigFile);
      if ((Config = fopen (Buffer,"r")) == NULL)
	{
	  if (errno == ENOENT)
	    return;		/* Couldn't find - use defaults */
	  AbortConfig ();	/* Otherwise abort the program */
	} /* if */
    } /* if */

  /* Process the file to obtain the configuration information */
  LineNum = 1;
  PrevChar = 0;
  Emul0Set = 0;			/* "emul0" hasn't been set yet */
  ch = ReadConfig ();
  while (ch != EOF)
    {
      if (ch == '#')
	{
	  /* Skip to the end of the current line for a comment */
	  while (ch != EOF && ch != '\n')
	    ch = ReadConfig ();
	} /* then */
       else if (isspace (ch))
	ch = ReadConfig ();	/* Skip white space characters */
       else if (isalpha (ch))
	ch = ConfigCommand (ch);/* Process a configuration command */
       else
	ConfigError ("Illegal character in configuration file");
    } /* while */
  if (!feof (Config))		/* Test to see if an error occurred */
    AbortConfig ();

  /* Set "emul0" to the same value as "emul" if it hasn't been set already */
  if (!Emul0Set)
    Def0TermType = DefTermType;

  /* Close the configuration file and exit */
  fclose (Config);
} /* Configure */
