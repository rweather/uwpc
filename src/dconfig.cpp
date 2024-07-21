//-------------------------------------------------------------------------
//
// DCONFIG.CPP - Routines to dump the UW/PC configuration to UW.CFG.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1992  Rhys Weatherley
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
//    1.0    02/05/92  RW  Original Version of DCONFIG.CPP
//
//-------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include "extern.h"
#include "config.h"
#include "comms.h"
#include "screen.h"
#include "opcodes.h"

#ifndef	UWPC_WINDOWS
#define	ANSI_CHARSET		0
#define	SYMBOL_CHARSET		2
#define	OEM_CHARSET		255
#endif

#define	fpbool(f,s,v)	fprintf((f),s "=%s\n",((v) ? "yes" : "no"))

// Get the name of a terminal emulation.
static	char	*CfgGetTermName (unsigned char far *emul)
{
  int version;
  static char name[20];
  version = ((*(emul + 4)) & 255) | (((*(emul + 5)) & 255) << 8);
  if (version < 0x100 || version > UW_TERM_VERSION)
    return ("??");			// Illegal terminal description.
  if (version >= UW_TERM_TYPECODE_VERS)
    emul += TERM_HEADER_SIZE;		// New style header.
   else
    emul += TERM_OLD_HEADER_SIZE;	// Old style header.
  version = 0;
  while (*emul)
    name[version++] = (char)(*emul++);
  name[version] = '\0';
  strlwr (name);
  return (name);
} // CfgGetTermName //

// Dump the configuration settings out to UW.CFG.  The old
// configuration file is saved in UWCFG.OLD.
int	UWConfiguration::dumpconfig (char *argv0)
{
  static long baudrates[] = {110,150,300,600,1200,2400,4800,9600,
  			     19200,38400,57600,115200};
  int temp;
  FILE *file;
  if ((file = fopen ("UWCFG.NEW","w")) == NULL)
    return (0);

  // Dump the options for serial communications //
  fprintf (file,"#\n# Options for serial communications.\n#\n");
  if (comports[0] != 0x3F8)	// Output address changing sequences.
    fprintf (file,"port=1\naddress=0x%X\n",comports[0]);
  if (comports[1] != 0x2F8)
    fprintf (file,"port=2\naddress=0x%X\n",comports[1]);
  if (comports[2] != 0x3E8)
    fprintf (file,"port=3\naddress=0x%X\n",comports[2]);
  if (comports[3] != 0x2E8)
    fprintf (file,"port=4\naddress=0x%X\n",comports[3]);
  fprintf (file,"port=%d\n",ComPort);
  fprintf (file,"baud=%ld\n",baudrates[ComParams & BAUD_RATE]);
  fprintf (file,"baudest=%ld\n",baudrates[ComEstBaud]);
  if ((ComParams & PARITY_GET) == PARITY_EVEN)
    fprintf (file,"parity=even\n");
   else if ((ComParams & PARITY_GET) == PARITY_ODD)
    fprintf (file,"parity=odd\n");
   else
    fprintf (file,"parity=none\n");
  fprintf (file,"bits=%d\n",(ComParams & BITS_8 ? 8 : 7));
  fprintf (file,"stop=%d\n",(ComParams & STOP_2 ? 2 : 1));
  fpbool (file,"fossil",ComFossil);
  fpbool (file,"ctsrts",ComCtsRts);
  fpbool (file,"direct",ComDirect);
  fpbool (file,"strip",StripHighBit);

  // Output the modem control strings //
  fprintf (file,"#\n# Modem control strings.\n#\n");
  fpbool (file,"cinit",CarrierInit);
  fprintf (file,"dial=\"%s\"\n",DialString);
  fprintf (file,"init=\"%s\"\n",InitString);
  fprintf (file,"hangup=\"%s\"\n",HangupString);

  // Dump the dialing directory settings //
  fprintf (file,"#\n# Dialing directory.\n#\n");
  fprintf (file,"dialprefix=\"%s\"\ndialsuffix=\"%s\"\n",DialPrefix,DialSuffix);
  for (temp = 0;temp < NUM_DIAL_STRINGS;++temp)
    {
      // Dump out the entries in the dialing directory //
      if (DialNumber[temp][0])
        fprintf (file,"dialnum%d=\"%s\"\n",temp + 1,DialNumber[temp]);
      if (DialParams[temp] != -1)
        fprintf (file,"dialpar%d=\"%ld %c%d%d\"\n",temp + 1,
		 baudrates[DialParams[temp] & BAUD_RATE],
		 ((DialParams[temp] & PARITY_GET) == PARITY_EVEN ? 'E' :
		  (DialParams[temp] & PARITY_GET) == PARITY_ODD ? 'O' : 'N'),
		 (DialParams[temp] & BITS_8 ? 8 : 7),
		 (DialParams[temp] & STOP_2 ? 2 : 1));
    }

  // Output options specific to terminal emulation //
  fprintf (file,"#\n# Screen, mouse and keyboard handling.\n#\n");
  fpbool (file,"ansibright",AnsiBright);
  fpbool (file,"bigvideo",BigVideo);
  fpbool (file,"beep",BeepEnable);
  fprintf (file,"bellfreq=%d\n",BellFreq);
  fprintf (file,"belldur=%d\n",BellDur);
  if (NewAttrs[ATTR_HIGHLIGHT])
    fprintf (file,"colhighlight=%d\n",NewAttrs[ATTR_HIGHLIGHT]);
  if (NewAttrs[ATTR_HIGH_STATUS])
    fprintf (file,"colhighstatus=%d\n",NewAttrs[ATTR_HIGH_STATUS]);
  if (NewAttrs[ATTR_INVERSE])
    fprintf (file,"colinverse=%d\n",NewAttrs[ATTR_INVERSE]);
  if (NewAttrs[ATTR_NORMAL])
    fprintf (file,"colnormal=%d\n",NewAttrs[ATTR_NORMAL]);
  if (NewAttrs[ATTR_STATUS])
    fprintf (file,"colstatus=%d\n",NewAttrs[ATTR_STATUS]);
  if (CursorSize == CURS_HALF_HEIGHT)
    fprintf (file,"cursor=halfheight\n");
   else if (CursorSize == CURS_FULL_HEIGHT)
    fprintf (file,"cursor=fullheight\n");
   else
    fprintf (file,"cursor=underline\n");
  fpbool (file,"mouse",EnableMouse);
  fpbool (file,"swapbs",SwapBSKeys);

  // Dump commands to load the terminal emulations //
  fprintf (file,"#\n# Terminal emulations.\n#\n");
  for (temp = 0;temp < NumTermDescs;++temp)
    fprintf (file,"terminal=\"%s\"\n",TermFiles[temp]);
  fprintf (file,"emul=%s\n",CfgGetTermName (P1TermType));
  fprintf (file,"emul0=%s\n",CfgGetTermName (P0TermType));

  // Dump the UW protocol configuration options.
  fprintf (file,"#\n# UW protocol options.\n#\n");
  fpbool (file,"disable",DisableUW);
  fprintf (file,"fixterm=");
  switch (ShellKind)
    {
      case SHELL_NONE:	fprintf (file,"none\n"); break;
      case SHELL_BOURNE:fprintf (file,"sh\n"); break;
      case SHELL_CSHELL:fprintf (file,"csh\n"); break;
      case SHELL_STRING:fprintf (file,"\"%s\"\n",ShellString); break;
    }
  if (ObeyTerm == OBEY_ALWAYS)
    fprintf (file,"obeyterm=always\n");
   else if (ObeyTerm == OBEY_NOTFIRST)
    fprintf (file,"obeyterm=notfirst\n");
   else
    fprintf (file,"obeyterm=ignore\n");
  fpbool (file,"popup",PopUpNewWindow);
  fprintf (file,"protocol=%d\n",MaxProtocol);
  fprintf (file,"uw=\"%s\"\n",CommandString);
  fprintf (file,"xonxoff=%s\n",(XonXoffFlag ? "encoded" : "direct"));

  // Dump the Windows 3.0 font information.
  fprintf (file,"#\n# Font information for the Windows 3.0 version.\n#\n");
  fprintf (file,"font=\"%s\"\nfontsize=%d\nfontset=",FontFace,FontHeight);
  if (FontCharSet == ANSI_CHARSET)
    fprintf (file,"ansi\n");
   else if (FontCharSet == SYMBOL_CHARSET)
    fprintf (file,"symbol\n");
   else
    fprintf (file,"oem\n");

  // Dump the options for configuring the status line //
  fprintf (file,"#\n# Options for the DOS status line.\n#\n");
  fprintf (file,"sformat=\"%s\"\n",StatusFormat);
  fprintf (file,"sposn=");
  if (StatusPosn == STATUS_LEFT)
    fprintf (file,"left\n");
   else if (StatusPosn == STATUS_RIGHT)
    fprintf (file,"right\n");
   else if (StatusPosn == STATUS_CENTRE)
    fprintf (file,"center\n");
   else if (StatusPosn == STATUS_LEFT_SQUASH)
    fprintf (file,"leftsquash\n");
   else if (StatusPosn == STATUS_RIGHT_SQUASH)
    fprintf (file,"rightsquash\n");
   else
    fprintf (file,"centersquash\n");
  fpbool (file,"status",!DisableStatusLine);
  fprintf (file,"stlen=%d\n",MaxTitleLen);

  // Dump the options for file transfers //
  fprintf (file,"#\n# Options for file transfers.\n#\n");
  fprintf (file,"zmodem=\"%s\"\n",ZModemCommand);

  // Dump the function key settings //
  fprintf (file,"#\n# Function key settings.\n#\n");
  for (temp = 0;temp < 10;++temp)
    fprintf (file,"F%d=\"%s\"\n",temp + 1,FKeys[temp]);

  // Dump the foreign language currently in use //
  fprintf (file,"#\n# Foreign language settings.\n#\n");
  if (NewTransFile)
    fprintf (file,"langfile=\"%s\"\n",TransFile);
  if (Language[0])
    fprintf (file,"language=\"%s\"\n",Language);
   else
    fprintf (file,"language=none\n");

  fclose (file);
  return (1);
} // UWConfiguration::dumpconfig //
