//-------------------------------------------------------------------------
//
// CONFIG.CPP - Configuration objects for UW/PC.
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
//    1.0    23/03/91  RW  Original Version of CONFIG.CPP
//    1.1    30/10/91  RW  Modify to make it work with Windows 3.0
//    1.2    08/12/91  RW  Add international language support.
//    1.3    15/03/92  RW  More options and support for VT100 driver.
//
//-------------------------------------------------------------------------

#define	NOWINMESSAGES	1	// Don't include some windows.h for extra space
#define NOWINSTYLES	1	// otherwise it won't compile under BCC -W.
#define	NOGDI		1

#include "config.h"		// Declarations for this module.
#include "comms.h"		// Communications routines.
#include "client.h"		// Client manipulation classes.
#include "opcodes.h"		// Opcodes for terminal emulations.
#include "screen.h"		// Screen attributes defined here.
#include "uwproto.h"		// UW protocol constants and terminal types.
#include <stdio.h>		// Standard I/O routines.
#include <string.h>		// String processing routines.
#include <ctype.h>		// Character type macros.
#include <stdlib.h>		// "exit" was defined here.
#include <alloc.h>		// Memory allocation routines.
#include <errno.h>		// Error message declarations.

//
// Declare the Windows 3.0 character types since
// we need them in DOS and we aren't including
// the GDI stuff because of space limitations.
// We shouldn't do this, but it's the best we can do.
//
#define	ANSI_CHARSET		0
#define	SYMBOL_CHARSET		2
#define	OEM_CHARSET		255

//
// Define the main UW/PC configuration object.
//
UWConfiguration	UWConfig;

//
// Global information for this module.
//
#ifdef	UWPC_WINDOWS
static	CATCHBUF	CatchBuf;
#define	SPRINTF		wsprintf
#define	STRING		LPSTR
#else
#define	SPRINTF		sprintf
#define	STRING		char *
#endif
static	char		ErrorBuffer[256];

//
// Report an error during configuration.  If "perr" is TRUE then
// use the message for the current "perror" error message.
//
static	void	ConfigError (char *msg,int perr=0)
{
#ifdef	UWPC_DOS
  // Report DOS configuration errors on the standard output //
  if (!perr)
    fprintf (stderr,"%s\n",msg);
   else
    perror (msg);
  exit (1);		// Abort the program completely.
#else
  // Report Windows 3.0 configuration errors in message boxes //
  if (!perr)
    MessageBox (NULL,msg,NULL,MB_OK | MB_ICONEXCLAMATION);
   else
    MessageBox (NULL,sys_errlist[errno],msg,MB_OK | MB_ICONEXCLAMATION);
  Throw (CatchBuf,1);	// Throw back to the "doconfig" function.
#endif
} /* ConfigError */

// Setup the default configuration.
UWConfiguration::UWConfiguration (void)
{
  int key;
  ComPort = 1;
  ComParams = BAUD_2400 | BITS_8 | PARITY_NONE | STOP_1;
  ComCtsRts = 1;
  ComFossil = 0;
  StripHighBit = 0;
  DisableStatusLine = 0;
  P0TermType = 0;
  P1TermType = &ADM31_Driver;
  strcpy (InitString,"AT^M~ATZ^M~~~AT S7=45 S0=0 V1 X1^M~");
  strcpy (HangupString,"~~~+++~~~ATH0^M");
  CarrierInit = 1;
  XonXoffFlag = 1;
  BeepEnable = 1;
  SwapBSKeys = 0;
  strcpy (DialString,"ATDT");
  strcpy (CommandString,"uw^M");
  strcpy (FtpString,"uwftp^M");
  strcpy (MailString,"uwmail^M");
  for (key = 0;key < 10;++key)
    FKeys[key][0] = '\0';
  strcpy (StatusFormat," ALT-Z for Help %v %e %v %p %v %u %v Windows: %a");
  StatusPosn = STATUS_LEFT;
  NumTermDescs = 0;
  for (key = 0;key < 5;++key)
    NewAttrs[key] = 0;
  PopUpNewWindow = 0;
  DisableUW = 0;
  strcpy (ZModemCommand,"DSZ");
  EnableMouse = 1;
  MailBoxName[0] = '\0';		// Use the default system mailbox.
  Password[0] = '\0';			// No password to start with.
  CursorSize = CURS_UNDERLINE;
  strcpy (FontFace,"Terminal");		// Font information for Windows 3.0.
  FontHeight = 12;
  FontCharSet = OEM_CHARSET;
  for (key = 0;key < MAX_CHARS;++key)
    {
      KeyTransTable[key] = key;		// Do straight-through translation.
      PrintTransTable[key] = key;
    } /* for */
  TransFile[0] = '\0';			// No translation table file yet.
  NewTransFile = 0;
  Language[0] = '\0';
  BigVideo = 0;
  BellFreq = 1200;
  BellDur = 200;
  AnsiBright = 0;
  strcpy (DialPrefix,"AT^M~ATDT");
  strcpy (DialSuffix,"^M");
  for (key = 0;key < NUM_DIAL_STRINGS;++key)
    {
      DialNumber[key][0] = '\0';	// No dialing string here in directory.
      DialParams[key] = -1;		// Don't override default settings.
    } /* for */
  ObeyTerm = OBEY_IGNORE;
  MaxProtocol = 2;
  MaxTitleLen = 30;
  ShellKind = SHELL_NONE;
  ShellString[0] = '\0';
  ComEstBaud = -1;			// Default to "baud"'s value.
  ComDirect = 0;
} // UWConfigration::UWConfiguration //

// Abort the configuration with an error.
void	UWConfiguration::error (char *msg)
{
  SPRINTF (ErrorBuffer,"Configuration line %d: %s",linenum,(STRING)msg);
  ConfigError (ErrorBuffer);
} // UWConfiguration::error //

// Abort the configuration with an "illegal" config message.
void	UWConfiguration::illegal (char *msg)
{
  SPRINTF (ErrorBuffer,"Configuration line %d: Illegal %s",linenum,(STRING)msg);
  ConfigError (ErrorBuffer);
} // UWConfiguration::illegal //

// Load a particular language into memory.
void	UWConfiguration::loadlang (char *language)
{
  FILE *file;
  static char langbuf[BUFSIZ];
  int posn,flag,orig,newval,ch,len;

  // Clear the translation tables to the default assignments //
  for (posn = 0;posn < MAX_CHARS;++posn)
    {
      KeyTransTable[posn] = posn;
      PrintTransTable[posn] = posn;
    } /* for */
  if (language[0] == '\0')
    return;		// No language specified: stay with defaults.

  // Open the file and set the language specified //
  if ((file = fopen (TransFile,"r")) == NULL)
    error ("Could not open translation table file");
  setvbuf (file,NULL,_IOFBF,BUFSIZ);
  flag = 0;
  len = strlen (language);
  while (fgets (langbuf,BUFSIZ,file))
    {
      // Process a line from the translation table file //
      posn = 0;
      while (langbuf[posn] && isspace (langbuf[posn]))
        ++posn;
      if (!langbuf[posn] || langbuf[posn] == '#')
        continue;		// Skip comment lines.

      // Distinguish between language names and translation specifications //
      if (posn && flag)
        {
	  // Parse the original character code //
	  if (isdigit (langbuf[posn]))
	    {
	      orig = 0;
	      while (langbuf[posn] && isdigit (langbuf[posn]))
	        orig = orig * 10 + (langbuf[posn++] - '0');
	    } /* then */
	   else if (langbuf[posn] == '\'' && langbuf[posn + 1] &&
	   	    langbuf[posn + 2] == '\'')
	    {
	      orig = langbuf[posn + 1];
	      posn += 3;
	    } /* then */
	   else if (langbuf[posn] == '"' && langbuf[posn + 1] &&
	   	    langbuf[posn + 2] == '"')
	    {
	      orig = langbuf[posn + 1];
	      posn += 3;
	    } /* then */
	   else
	    continue;		// Ignore the error line.

	  // Skip spaces and get the translation direction character //
	  while (langbuf[posn] && isspace (langbuf[posn]))
	    ++posn;
	  if (!(ch = langbuf[posn]) ||
	      (ch != '=' && ch != '>' && ch != '<'))
	    continue;
	  ++posn;
	  while (langbuf[posn] && isspace (langbuf[posn]))
	    ++posn;

	  // Parse the translation character code //
	  if (isdigit (langbuf[posn]))
	    {
	      newval = 0;
	      while (langbuf[posn] && isdigit (langbuf[posn]))
	        newval = newval * 10 + (langbuf[posn++] - '0');
	    } /* then */
	   else if (langbuf[posn] == '\'' && langbuf[posn + 1] &&
	   	    langbuf[posn + 2] == '\'')
	    {
	      newval = langbuf[posn + 1];
	      posn += 3;
	    } /* then */
	   else if (langbuf[posn] == '"' && langbuf[posn + 1] &&
	   	    langbuf[posn + 2] == '"')
	    {
	      newval = langbuf[posn + 1];
	      posn += 3;
	    } /* then */
	   else
	    continue;		// Ignore the error line.

	  // Set the required translation //
	  orig &= 255;
	  newval &= 255;
	  switch (ch)
	    {
	      case '=':	KeyTransTable[orig] = newval; // fall through //
	      case '<': PrintTransTable[newval] = orig; break;
	      case '>': KeyTransTable[orig] = newval; break;
	    } /* switch */
	} /* then */
       else if (!posn)
        {
	  // Test if we have found a relevant language table //
	  flag = 0;
	  while (langbuf[posn])
	    {
	      if (!strncmpi (langbuf + posn,language,len) &&
	          (isspace (langbuf[posn + len]) ||
		   !langbuf[posn + len] ||
		   langbuf[posn + len] == ','))
		{
		  flag = 1;		// Found the language.
		  break;
		} /* if */
	      while (langbuf[posn] && !isspace (langbuf[posn]) &&
	      	     langbuf[posn] != ',')
		++posn;
	      while (langbuf[posn] && (isspace (langbuf[posn]) ||
	      	     langbuf[posn] == ','))
		++posn;
	    } /* while */
	} /* then */
    } /* while */
  fclose (file);
} // UWConfiguration::loadlang //

// Load a new terminal type from the specified filename.
void	*UWConfiguration::LoadTerminal (char *filename)
{
  FILE *file;
  long filesize;
  void *desc;

  // Open the terminal description file for reading //
  if ((file = fopen (filename,"rb")) == NULL)
    ConfigError (filename,1);	// Report a "perror".

  // Get the length of the file and return to the start //
  if (fseek (file,0L,SEEK_END) ||
      (filesize = ftell (file)) == -1L ||
      fseek (file,0L,SEEK_SET))
    {
      int err;
      err = errno;
      fclose (file);
      errno = err;
      ConfigError (filename);
    }

  // Allocate some memory for the terminal description //
  if (filesize > 17000L ||
      (desc = (void *)malloc ((size_t)filesize)) == NULL)
    {
      SPRINTF (ErrorBuffer,"%s : Not enough memory",(STRING)filename);
      ConfigError (ErrorBuffer);
    }

  // Read the contents of the terminal description //
  if (fread (desc,1,(size_t)filesize,file) != ((size_t)filesize))
    {
      int err;
      err = errno;
      fclose (file);
      errno = err;
      ConfigError (filename);
    }

  // Close the file and return the loaded terminal description //
  fclose (file);
  return (desc);
} // UWConfiguration::LoadTerminal //

// Find a terminal description in the loaded descriptions.
// Returns NULL if the terminal type could not be found.
unsigned char far *UWConfiguration::FindTerminal (char *type)
{
  int index=0;

  // Search for a type in the loaded emulations //
  while (index < NumTermDescs)
    {
      // Check the name of the wanted emulation against loaded emulation //
      if (!stricmp (((char *)(TermDescs[index])) + TERM_HEADER_SIZE,type))
        return ((unsigned char far *)(TermDescs[index]));
      ++index;
    }

  // Test for the standard terminal types //
  if (!stricmp (type,"adm31"))
    return ((unsigned char far *)&ADM31_Driver);
   else if (!stricmp (type,"vt52"))
    return ((unsigned char far *)&VT52_Driver);
   else if (!stricmp (type,"ansi"))
    return ((unsigned char far *)&ANSI_Driver);
   else if (!stricmp (type,"vt100"))
    return ((unsigned char far *)&VT100_Driver);
   else
    return (NULL);
} // UWConfiguration::FindTerminal //

// Parse a dialing directory comms parameter spec.  Returns
// -1 if the string was illegal (it also generates an error msg).
int	UWConfiguration::parsecom (char *str)
{
  unsigned long baud=0;
  int params=0;
  int errors=0;
  int temp;
  static unsigned long baudrates[] = {110,150,300,600,1200,2400,
      				      4800,9600,19200,38400,57600,115200};

  // Extract and search for the baud rate //
  while (*str && isspace (*str))
    ++str;
  while (*str && isdigit (*str))
    baud = baud * 10 + (*str++ - '0');
  temp = 0;
  while (temp <= BAUD_115200 && baud != baudrates[temp])
    ++temp;
  if (temp <= BAUD_115200)
    params |= temp;
   else
    errors = 1;

  // Parse the rest of the parameters //
  while (*str && isspace (*str))
    ++str;
  if (*str == 'n' || *str == 'N')
    params |= PARITY_NONE;
   else if (*str == 'e' || *str == 'E')
    params |= PARITY_EVEN;
   else if (*str == 'o' || *str == 'O')
    params |= PARITY_ODD;
   else
    errors = 1;
  if (*str)
    ++str;
  if (*str == '8')
    params |= BITS_8;
   else if (*str == '7')
    params |= BITS_7;
   else
    errors = 1;
  if (*str)
    ++str;
  if (*str == '1')
    params |= STOP_1;
   else if (*str == '2')
    params |= STOP_2;
   else
    errors = 1;
  if (*str)
    ++str;
  while (*str && isspace (*str))
    ++str;
  if (*str || errors)
    {
      illegal ("comms parameter specification");
      return (-1);
    } /* then */
   else
    return (params);
} // UWConfiguration::parsecom //

#define	CONFIG_NAME_LEN		20
#define	TYPE_NUMBER		0
#define TYPE_IDENT		1
#define TYPE_STRING		2
#define	TYPE_BOOL		3

// Process a configuration line.
void	UWConfiguration::processline (char *line)
{
  int index,type;
  unsigned long number;
  static char ConfigName[CONFIG_NAME_LEN + 1];
  static char ConfigParam[STR_LEN];
  char *errmsg="Illegal configuration command format";

  while (*line && isspace (*line))	// Skip initial white space.
    ++line;
  if (!(*line) || *line == '#')
    return;				// Comment line - ignore.
  index = 0;				// Get configuration option name.
  while (index < CONFIG_NAME_LEN && *line &&
         (isalpha (*line) || isdigit (*line)))
    ConfigName[index++] = *line++;
  ConfigName[index] = '\0';
  while (*line && isspace (*line))	// Skip white space before "=".
    ++line;
  if (index <= 0 || *line != '=')
    error (errmsg);
  ++line;				// Skip "=" in config option.
  while (*line && isspace (*line))	// Skip white space after "=".
    ++line;
  if (*line == '0' && *(line + 1) == 'x')
    {
      // Recognise a hexadecimal constant.
      line += 2;
      number = 0;
      while (isxdigit (*line))
        {
	  number = number * 16;
	  if (isdigit (*line))
	    number += *line - '0';
	   else
	    number += ((*line & 0x5F) - 'A' + 10);
	  ++line;
	}
      type = TYPE_NUMBER;
    }
   else if (*line == '0')
    {
      // Recognise an octal constant.
      number = 0;
      while (isdigit (*line) && *line < '8')
        number = (number * 8) + (*line++ - '0');
      type = TYPE_NUMBER;
    }
   else if (isdigit (*line))
    {
      // Recognise a decimal constant.
      number = 0;
      while (isdigit (*line))
        number = (number * 10) + (*line++ - '0');
      type = TYPE_NUMBER;
    }
   else if (isalpha (*line))
    {
      // Recognise an identifier.
      index = 0;
      while (isalpha (*line) || isdigit (*line))
        ConfigParam[index++] = *line++;
      ConfigParam[index] = '\0';
      type = TYPE_IDENT;
    }
   else if (*line == '"')
    {
      // Recognise a string.
      ++line;
      index = 0;
      while (index < (STR_LEN - 1) && *line && *line != '"')
        ConfigParam[index++] = *line++;
      ConfigParam[index] = '\0';
      if (*line != '"')
        illegal ("string constant");
      ++line;
      type = TYPE_STRING;
    }
   else
    error (errmsg);
  while (*line && isspace (*line))	// Skip white space after "=".
    ++line;
  if (*line != '\0' && *line != '#')	// Abort if not at line end.
    error (errmsg);

  // Convert yes/no and on/off to the boolean type code //
  if (type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"yes") || !stricmp (ConfigParam,"on"))
        {
	  type = TYPE_BOOL;
	  number = 1;
	}
       else if (!stricmp (ConfigParam,"no") || !stricmp (ConfigParam,"off"))
        {
	  type = TYPE_BOOL;
	  number = 0;
	}
    }

  // Determine the configuration option to the changed //
  if (!stricmp (ConfigName,"port") && type == TYPE_NUMBER)
    {
      if (number >= 1 && number <= 4)
        ComPort = number;
       else
        illegal ("port number");
    }
   else if (!stricmp (ConfigName,"address") && type == TYPE_NUMBER)
    {
      comports[ComPort - 1] = number;	// Set the COM port address.
    }
   else if ((!stricmp (ConfigName,"baud")||!stricmp (ConfigName,"baudest")) &&
   	    type == TYPE_NUMBER)
    {
      static unsigned long baudrates[] = {110,150,300,600,1200,2400,
      				     4800,9600,19200,38400,57600,115200};
      int baud=0;
      while (baud <= BAUD_115200 && number != baudrates[baud])
        ++baud;
      if (baud <= BAUD_115200)
        {
	  if (!stricmp (ConfigName,"baud"))
	    ComParams = (ComParams & ~BAUD_RATE) | baud;
	   else
	    ComEstBaud = baud;
	}
       else
        illegal ("baud rate");
    }
   else if (!stricmp (ConfigName,"parity") && type == TYPE_IDENT)
    {
      int parity;
      if (!stricmp (ConfigParam,"none"))
        parity = PARITY_NONE;
       else if (!stricmp (ConfigParam,"even"))
        parity = PARITY_EVEN;
       else if (!stricmp (ConfigParam,"odd"))
        parity = PARITY_ODD;
       else
        illegal ("parity value");
      ComParams = (ComParams & ~PARITY_GET) | parity;
    }
   else if (!stricmp (ConfigName,"stop") && type == TYPE_NUMBER)
    {
      if (number == 1)
        ComParams &= ~STOP_2;
       else if (number == 2)
        ComParams |= STOP_2;
       else
        illegal ("number of stop bits");
    }
   else if (!stricmp (ConfigName,"bits") && type == TYPE_NUMBER)
    {
      if (number == 7)
        ComParams &= ~BITS_8;
       else if (number == 8)
        ComParams |= BITS_8;
       else
        illegal ("number of data bits");
    }
   else if (!stricmp (ConfigName,"bellfreq") && type == TYPE_NUMBER)
    {
      BellFreq = number;
    }
   else if (!stricmp (ConfigName,"belldur") && type == TYPE_NUMBER)
    {
      BellDur = number;
    }
   else if (!stricmp (ConfigName,"font") && type == TYPE_STRING)
    {
      if (strlen (ConfigParam) >= FONT_STR_LEN)
        illegal ("font facename - too long");
      strcpy (FontFace,ConfigParam);
    }
   else if (!stricmp (ConfigName,"fontsize") && type == TYPE_NUMBER)
    {
      if (number < 1 || number > 50)
        illegal ("font height - must be between 1 and 50");
      FontHeight = number;
    }
   else if (!stricmp (ConfigName,"fontset") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"ansi"))
        FontCharSet = ANSI_CHARSET;
       else if (!stricmp (ConfigParam,"oem"))
        FontCharSet = OEM_CHARSET;
       else if (!stricmp (ConfigParam,"symbol"))
        FontCharSet = SYMBOL_CHARSET;
       else
        illegal ("font character set");
    }
   else if (!stricmp (ConfigName,"protocol") && type == TYPE_NUMBER)
    {
      if (number < 1 || number > 2)
        illegal ("protocol number - must be 1 or 2");
       else
        MaxProtocol = number;
    }
   else if (!stricmp (ConfigName,"stlen") && type == TYPE_NUMBER)
    {
      if (((long)number) < 0 || number > 63)
        illegal ("title length - must be between 0 and 63");
       else
        MaxTitleLen = number;
    }
   else if (!stricmp (ConfigName,"langfile") && type == TYPE_STRING)
    {
      NewTransFile = 1;
      strcpy (TransFile,ConfigParam);
    }
   else if (!stricmp (ConfigName,"language") &&
   	    (type == TYPE_STRING || type == TYPE_IDENT))
    {
      if (stricmp (ConfigParam,"none"))
        {
          strcpy (Language,ConfigParam);
          loadlang (ConfigParam);
        }
    }
   else if (!stricmp (ConfigName,"init") && type == TYPE_STRING)
    {
      strcpy (InitString,ConfigParam);
    }
   else if (!stricmp (ConfigName,"cinit") && type == TYPE_BOOL)
    {
      CarrierInit = number;
    }
   else if (!stricmp (ConfigName,"direct") && type == TYPE_BOOL)
    {
      ComDirect = number;
    }
   else if (!stricmp (ConfigName,"ctsrts") && type == TYPE_BOOL)
    {
      ComCtsRts = number;
    }
   else if (!stricmp (ConfigName,"ansibright") && type == TYPE_BOOL)
    {
      AnsiBright = number;
    }
   else if (!stricmp (ConfigName,"bigvideo") && type == TYPE_BOOL)
    {
      BigVideo = number;
    }
   else if (!stricmp (ConfigName,"fossil") && type == TYPE_BOOL)
    {
      ComFossil = number;
    }
   else if (!stricmp (ConfigName,"disable") && type == TYPE_BOOL)
    {
      DisableUW = number;
    }
   else if (!stricmp (ConfigName,"cursor") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"underline"))
        CursorSize = CURS_UNDERLINE;
       else if (!stricmp (ConfigParam,"halfheight"))
        CursorSize = CURS_HALF_HEIGHT;
       else if (!stricmp (ConfigParam,"fullheight"))
        CursorSize = CURS_FULL_HEIGHT;
       else
        illegal ("value: must be 'underline', 'halfheight' or 'fullheight'");
    }
   else if (!stricmp (ConfigName,"fixterm") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"none"))
        ShellKind = SHELL_NONE;
       else if (!stricmp (ConfigParam,"sh"))
        ShellKind = SHELL_BOURNE;
       else if (!stricmp (ConfigParam,"csh"))
        ShellKind = SHELL_CSHELL;
       else
        illegal ("value: must be 'none', 'sh', 'csh' or a string");
    }
   else if (!stricmp (ConfigName,"fixterm") && type == TYPE_STRING)
    {
      ShellKind = SHELL_STRING;
      strcpy (ShellString,ConfigParam);
    }
   else if (!stricmp (ConfigName,"hangup") && type == TYPE_STRING)
    {
      strcpy (HangupString,ConfigParam);
    }
   else if (!stricmp (ConfigName,"terminal") && type == TYPE_STRING)
    {
      if (NumTermDescs >= MAX_DESCS)
        error ("Too many extra loaded terminal descriptions");
       else
        {
          strcpy (TermFiles[NumTermDescs],ConfigParam);
          TermDescs[NumTermDescs++] = LoadTerminal (ConfigParam);
        }
    }
   else if (!stricmp (ConfigName,"emul") &&
   	    (type == TYPE_IDENT || type == TYPE_STRING))
    {
      void far *desc;
      if ((desc = FindTerminal (ConfigParam)) == NULL)
        illegal ("terminal type");
      P1TermType = (unsigned char far *)desc;
    }
   else if (!stricmp (ConfigName,"emul0") &&
   	    (type == TYPE_IDENT || type == TYPE_STRING))
    {
      void far *desc;
      if ((desc = FindTerminal (ConfigParam)) == NULL)
        illegal ("terminal type");
      P0TermType = (unsigned char far *)desc;
    }
   else if (!stricmp (ConfigName,"xonxoff") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"encoded"))
        XonXoffFlag = 1;
       else if (!stricmp (ConfigParam,"direct"))
        XonXoffFlag = 0;
       else
        illegal ("value: must be 'encoded' or 'direct'");
    }
   else if (!stricmp (ConfigName,"beep") && type == TYPE_BOOL)
    {
      BeepEnable = number;
    }
   else if (!stricmp (ConfigName,"popup") && type == TYPE_BOOL)
    {
      PopUpNewWindow = number;
    }
   else if (!stricmp (ConfigName,"strip") && type == TYPE_BOOL)
    {
      StripHighBit = number;
    }
   else if (!stricmp (ConfigName,"mouse") && type == TYPE_BOOL)
    {
      EnableMouse = number;
    }
   else if (!stricmp (ConfigName,"swapbs") && type == TYPE_BOOL)
    {
      SwapBSKeys = number;
    }
   else if (!stricmp (ConfigName,"status") && type == TYPE_BOOL)
    {
      DisableStatusLine = !number;
    }
   else if (!stricmp (ConfigName,"sformat") && type == TYPE_STRING)
    strcpy (StatusFormat,ConfigParam);
   else if (!stricmp (ConfigName,"mailbox") && type == TYPE_STRING)
    strcpy (MailBoxName,ConfigParam);
   else if (!stricmp (ConfigName,"password") && type == TYPE_STRING)
    strcpy (Password,ConfigParam);
   else if (!stricmp (ConfigName,"sposn") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"left"))
        StatusPosn = STATUS_LEFT;
       else if (!stricmp (ConfigParam,"right"))
        StatusPosn = STATUS_RIGHT;
       else if (!stricmp (ConfigParam,"center") ||
       	        !stricmp (ConfigParam,"centre"))
	StatusPosn = STATUS_CENTRE;
       else if (!stricmp (ConfigParam,"leftsquash"))
        StatusPosn = STATUS_LEFT_SQUASH;
       else if (!stricmp (ConfigParam,"rightsquash"))
        StatusPosn = STATUS_RIGHT_SQUASH;
       else if (!stricmp (ConfigParam,"centersquash") ||
       		!stricmp (ConfigParam,"centresquash"))
	StatusPosn = STATUS_CENTRE_SQUASH;
       else
        illegal ("status position");
    }
   else if (!stricmp (ConfigName,"obeyterm") && type == TYPE_IDENT)
    {
      if (!stricmp (ConfigParam,"ignore"))
        ObeyTerm = OBEY_IGNORE;
       else if (!stricmp (ConfigParam,"always"))
        ObeyTerm = OBEY_ALWAYS;
       else if (!stricmp (ConfigParam,"notfirst"))
        ObeyTerm = OBEY_NOTFIRST;
       else
        illegal ("terminal type override mode");
    }
   else if (!stricmp (ConfigName,"dial") && type == TYPE_STRING)
    strcpy (DialString,ConfigParam);
   else if (!stricmp (ConfigName,"dialprefix") && type == TYPE_STRING)
    strcpy (DialPrefix,ConfigParam);
   else if (!stricmp (ConfigName,"dialsuffix") && type == TYPE_STRING)
    strcpy (DialSuffix,ConfigParam);
   else if (!strncmpi (ConfigName,"dialnum",7) && type == TYPE_STRING)
    {
      int dnum = atoi (ConfigName + 7);
      if (dnum < 1 || dnum > NUM_DIAL_STRINGS)
        illegal ("dialing directory entry number");
       else
        strcpy (DialNumber[dnum - 1],ConfigParam);
    }
   else if (!strncmpi (ConfigName,"dialpar",7) && type == TYPE_STRING)
    {
      int dnum = atoi (ConfigName + 7);
      if (dnum < 1 || dnum > NUM_DIAL_STRINGS)
        illegal ("dialing directory entry number");
       else
        DialParams[dnum - 1] = parsecom (ConfigParam);
    }
   else if (!stricmp (ConfigName,"uw") && type == TYPE_STRING)
    strcpy (CommandString,ConfigParam);
   else if (!stricmp (ConfigName,"uwftp") && type == TYPE_STRING)
    strcpy (FtpString,ConfigParam);
   else if (!stricmp (ConfigName,"uwmail") && type == TYPE_STRING)
    strcpy (MailString,ConfigParam);
   else if (!stricmp (ConfigName,"zmodem") && type == TYPE_STRING)
    strcpy (ZModemCommand,ConfigParam);
   else if (!stricmp (ConfigName,"f1") && type == TYPE_STRING)
    strcpy (FKeys[0],ConfigParam);
   else if (!stricmp (ConfigName,"f2") && type == TYPE_STRING)
    strcpy (FKeys[1],ConfigParam);
   else if (!stricmp (ConfigName,"f3") && type == TYPE_STRING)
    strcpy (FKeys[2],ConfigParam);
   else if (!stricmp (ConfigName,"f4") && type == TYPE_STRING)
    strcpy (FKeys[3],ConfigParam);
   else if (!stricmp (ConfigName,"f5") && type == TYPE_STRING)
    strcpy (FKeys[4],ConfigParam);
   else if (!stricmp (ConfigName,"f6") && type == TYPE_STRING)
    strcpy (FKeys[5],ConfigParam);
   else if (!stricmp (ConfigName,"f7") && type == TYPE_STRING)
    strcpy (FKeys[6],ConfigParam);
   else if (!stricmp (ConfigName,"f8") && type == TYPE_STRING)
    strcpy (FKeys[7],ConfigParam);
   else if (!stricmp (ConfigName,"f9") && type == TYPE_STRING)
    strcpy (FKeys[8],ConfigParam);
   else if (!stricmp (ConfigName,"f10") && type == TYPE_STRING)
    strcpy (FKeys[9],ConfigParam);
   else if (!stricmp (ConfigName,"colnormal") && type == TYPE_NUMBER)
    NewAttrs[ATTR_NORMAL] = number;
   else if (!stricmp (ConfigName,"colinverse") && type == TYPE_NUMBER)
    NewAttrs[ATTR_INVERSE] = number;
   else if (!stricmp (ConfigName,"colhighlight") && type == TYPE_NUMBER)
    NewAttrs[ATTR_HIGHLIGHT] = number;
   else if (!stricmp (ConfigName,"colstatus") && type == TYPE_NUMBER)
    NewAttrs[ATTR_STATUS] = number;
   else if (!stricmp (ConfigName,"colhighstatus") && type == TYPE_NUMBER)
    NewAttrs[ATTR_HIGH_STATUS] = number;
   else
    error (errmsg);
} // UWConfiguration::processline //

// Do the configuration for UW/PC.
int	UWConfiguration::doconfig (char *argv0)
{
  static char ConfigBuffer[BUFSIZ];
  char *FileName = "UW.CFG";
  FILE *file=NULL;
  int len;

#ifdef	UWPC_WINDOWS
  // Catch any errors that occur during Windows 3.0 configuration.
  if (Catch (CatchBuf))
    {
      if (file != NULL)
        fclose (file);
      return (0);		// Configuration failed.
    } /* if */
#endif

  // Set the default translation filename.
  strcpy (TransFile,argv0);
  len = strlen (TransFile);
  while (len > 0 && TransFile[len - 1] != '\\' &&
         TransFile[len - 1] != '/')
    --len;
  strcpy (TransFile + len,"UW.LNG");

  // Find the configuration file (if present).
  strcpy (ConfigBuffer,FileName);
  if ((file = fopen (ConfigBuffer,"r")) == NULL)
    {
      strcpy (ConfigBuffer,argv0);
      len = strlen (ConfigBuffer);
      while (len > 0 && ConfigBuffer[len - 1] != '\\' &&
             ConfigBuffer[len - 1] != '/')
	--len;
      strcpy (ConfigBuffer + len,FileName);
      file = fopen (ConfigBuffer,"r");
    }
  if (file != NULL)
    {
      linenum = 1;
      while (fgets (ConfigBuffer,BUFSIZ,file) != NULL)
        {
	  processline (ConfigBuffer);
	  ++linenum;
	}
      fclose (file);
    }

  // Set high bit stripping always for 7-bit transmission.
  if (!(ComParams & BITS_8))
    StripHighBit = 1;

  // Setup the string version of the COM parameters //
  makeparams ();

  // Set the default Protocol 0 terminal type if not set already //
  if (P0TermType == 0)
    P0TermType = P1TermType;

  // Set the default baud rate estimate value //
  if (ComEstBaud < 0)
    ComEstBaud = ComParams & BAUD_RATE;

#ifdef	UWPC_WINDOWS
  // Set the screen attributes for use by the Windows 3.0 version //
  static unsigned char colrattrs[NUM_ATTRS] = {0x70,0x07,0x07,0x07,0x07};
  int attr;
  for (attr = 0;attr < 5;++attr)
    {
      // If the attribute isn't set yet, then set it //
      if (!NewAttrs[attr])
        NewAttrs[attr] = colrattrs[attr];
    } /* for */
#endif	/* UWPC_WINDOWS */

  return (1);			// Configuration succeeded.
} // UWConfiguration::doconfig //

// Process ComParams and create the DeviceParameters string.
void	UWConfiguration::makeparams (void)
{
  static char *BaudRates[] = {"110","150","300","600","1200","2400",
			      "4800","9600","19200","38400","57600",
			      "115200"};
  static char Parities[] = "NOE?";

  SPRINTF (DeviceParameters,"COM%d %s %c-%c-%c",
		ComPort,
		(STRING)(BaudRates[ComParams & BAUD_RATE]),
		Parities[(ComParams & PARITY_GET) >> 6],
		((ComParams & BITS_8) ? '8' : '7'),
		((ComParams & STOP_2) ? '2' : '1'));
} // UWConfiguration::makeparams //

// Test a terminal emulation to see if it is a particular terminal type.
static	int	IsRightTermType (unsigned char far *emul,int termtype)
{
  int version = ((*(emul + 4)) & 255) + (((*(emul + 5)) & 255) << 8);
  if (version >= UW_TERM_TYPECODE_VERS)
    return (termtype == *(emul + 6));
   else
    return (0);
} // IsRightTermType //

// Get a terminal emulation for a particular terminal type.
unsigned char far *UWConfiguration::getterminal (int termtype)
{
  int index=0;

  // Search for a type in the loaded emulations //
  while (index < NumTermDescs)
    {
      if (IsRightTermType ((unsigned char far *)(TermDescs[index]),termtype))
        return ((unsigned char far *)(TermDescs[index]));
      ++index;
    }

  // Test the type against the linked-in terminal emulations //
  if (termtype == UWT_ADM31)
    return ((unsigned char far *)&ADM31_Driver);
   else if (termtype == UWT_VT52)
    return ((unsigned char far *)&VT52_Driver);
   else if (termtype == UWT_ANSI)
    return ((unsigned char far *)&ANSI_Driver);
   else
    return (NULL);
} // UWConfiguration::getterminal //
