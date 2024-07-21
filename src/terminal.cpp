//-------------------------------------------------------------------------
//
// TERMINAL.CPP - Declarations for terminal clients based on neutral
//		  terminal descriptions.
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
//    1.0    04/04/91  RW  Original Version of TERMINAL.CPP
//    1.1    25/07/91  RW  Add handling of UW/PC clients.
//    1.2    08/12/91  RW  Add international language support.
//    1.3    10/12/91  RW  Add support for secondary key tables
//			   and extra stuff for VT100.
//    1.4    15/03/92  RW  Enhance TERMCC with lots of new instructions.
//
//-------------------------------------------------------------------------

#include "client.h"		// Declarations for this module
#include "display.h"		// Window display declarations
#include "opcodes.h"		// Opcodes for the terminal description
#include "screen.h"		// Screen accessing classes
#include "config.h"		// Configuration handling routines
#include "uw.h"			// UW protocol declarations
#include <ctype.h>		// Character typing macros
#include <mem.h>		// Memory handling functions

// Define a mapping from ANSI colour codes to IBM-PC colour codes.
static	unsigned char forecols[] = {0x00,0x04,0x02,0x06,0x01,0x05,0x03,0x07};
static	unsigned char britecols[]= {0x00,0x0C,0x0A,0x0E,0x09,0x0D,0x0B,0x0F};
static	unsigned char backcols[] = {0x00,0x40,0x20,0x60,0x10,0x50,0x30,0x70};

// Jump to the currently stored location.
void	UWTermDesc::jump (void)
{
  PC = (description[PC] & 255) | ((description[PC + 1] & 255) << 8);
} // UWTermDesc::jump //

// Interpret a terminal description until the next
// character request, starting with the given character.
void	UWTermDesc::interpret (int ch)
{
  unsigned char opcode;
  int value;
  acc = ch;			// Set accumulator from received character.

  // Loop around fetching opcodes until next character get //
  while ((opcode = description[PC++]) != OP_GETCH)
    switch (opcode)
      {
	case OP_SEND:		window -> send (mapchars[acc],0); break;
	case OP_SEND52:		window -> send (mapchars[acc],1); break;
	case OP_SENDG:		if (acc >= 32)
				  {
				    window -> send (mapchars[acc],0);
				    PC -= 2;	// Go back to "getch".
				  } /* if */
				break;
	case OP_SENDG52:	if (acc >= 32)
				  {
				    window -> send (mapchars[acc],1);
				    PC -= 2;	// Go back to "getch".
				  } /* if */
				break;
	case OP_SENDGI:		if (acc >= 32)
				  {
				    if (flags & (1 << description[PC]))
				      window -> inschar (mapchars[acc]);
				     else
				      window -> send (mapchars[acc],0);
				    PC -= 2;	// Go back to "getch".
				  } /* then */
				 else
				  PC++;		// Skip flag bit spec.
				break;
	case OP_SENDGI52:	if (acc >= 32)
				  {
				    if (flags & (1 << description[PC]))
				      window -> inschar (mapchars[acc]);
				     else
				      window -> send (mapchars[acc],1);
				    PC -= 2;	// Go back to "getch".
				  } /* then */
				 else
				  PC++;		// Skip flag bit spec.
				break;
	case OP_SENDGS:		if (acc >= 32)
				  {
				    if (flags & (1 << description[PC]))
				      window -> send (mapchars[acc],1);
				     else
				      window -> send (mapchars[acc],0);
				    PC -= 2;	// Go back to "getch".
				  } /* then */
				 else
				  PC++;		// Skip flag bit spec.
				break;
	case OP_SENDGSI:	if (acc >= 32)
				  {
				    if (flags & (1 << description[PC + 1]))
				      window -> inschar (mapchars[acc]);
				     else if (flags & (1 << description[PC]))
				      window -> send (mapchars[acc],1);
				     else
				      window -> send (mapchars[acc],0);
				    PC -= 2;	// Go back to "getch".
				  } /* then */
				 else
				  PC += 2;	// Skip flag bit specs.
				break;
	case OP_CR:		window -> cr (); break;
	case OP_LF:		window -> lf (); break;
	case OP_BS:		window -> bs (0); break;
	case OP_BSWRAP:		window -> bs (1); break;
	case OP_MOVE:		window -> move (regx,regy); break;
	case OP_MOVED:		window -> moverel (0,acc); break;
	case OP_MOVEH:		window -> move (0,0); break;
	case OP_MOVEL:		window -> moverel (-acc,0); break;
	case OP_MOVER:		window -> moverel (acc,0); break;
	case OP_MOVEREL:	window -> moverel (regx,regy); break;
	case OP_MOVEU:		window -> moverel (0,-acc); break;
	case OP_CLEAR:		window -> clear (); break;
	case OP_CLREOL:		window -> clear (CLR_END_LINE); break;
	case OP_CLREOS:		window -> clear (CLR_END_SCREEN); break;
	case OP_CLRSOL:		window -> clear (CLR_ST_LINE); break;
	case OP_CLRSOS:		window -> clear (CLR_ST_SCREEN); break;
	case OP_INSLINE:	window -> insline (); break;
	case OP_DELLINE:	window -> delline (); break;
	case OP_INSCHAR:	window -> inschar (mapchars[acc]); break;
	case OP_DELCHAR:	window -> delchar (); break;
#ifdef	UWPC_DOS
	case OP_SETATTR:	window -> setattr
		  (HardwareScreen.attributes[description[PC++]]); break;
	case OP_SETATTR_ACC:	window -> setattr
		  (HardwareScreen.attributes[acc]); break;
	case OP_SETSCRL:	window -> setscroll
		  (HardwareScreen.attributes[description[PC++]]); break;
	case OP_SETSCRL_ACC:	window -> setscroll
		  (HardwareScreen.attributes[acc]); break;
#else	/* UWPC_DOS */
	case OP_SETATTR:	window -> setattr
		  	(UWConfig.NewAttrs[description[PC++]]); break;
	case OP_SETATTR_ACC:	window -> setattr
		  	(UWConfig.NewAttrs[acc]); break;
	case OP_SETSCRL:	window -> setscroll
		  	(UWConfig.NewAttrs[description[PC++]]); break;
	case OP_SETSCRL_ACC:	window -> setscroll
		  	(UWConfig.NewAttrs[acc]); break;
#endif	/* UWPC_DOS */
	case OP_SETX:		regx = acc; break;
	case OP_SETY:		regy = acc; break;
	case OP_LOAD:		acc = description[PC++] & 255; break;
	case OP_LOAD_WORD:	acc = description[PC++] & 255;
				acc |= (description[PC++] & 255) << 8;
				break;
	case OP_LOAD_WIDTH:	acc = window -> width; break;
	case OP_LOAD_HEIGHT:	acc = window -> height; break;
	case OP_ADD:		acc += description[PC++] & 255; break;
	case OP_ADD_WORD:	acc += description[PC++] & 255;
				acc += (description[PC++] & 255) << 8;
				break;
	case OP_ADD_WIDTH:	acc += window -> width; break;
	case OP_ADD_HEIGHT:	acc += window -> height; break;
	case OP_SUB:		acc -= description[PC++] & 255; break;
	case OP_SUB_WORD:	value = description[PC++] & 255;
				value |= (description[PC++] & 255) << 8;
				acc -= value;
				break;
	case OP_SUB_WIDTH:	acc -= window -> width; break;
	case OP_SUB_HEIGHT:	acc -= window -> height; break;
	case OP_CMP:		compare = acc - (description[PC++] & 255);
				break;
	case OP_CMP_WORD:	value = description[PC++] & 255;
				value |= (description[PC++] & 255) << 8;
				compare = acc - value;
				break;
	case OP_CMP_WIDTH:	compare = acc - (window -> width); break;
	case OP_CMP_HEIGHT:	compare = acc - (window -> height); break;
	case OP_SWITCH:		if (acc == (description[PC++] & 255))
				  jump ();
				 else
				  PC += 2;	// Skip jump address.
				break;
	case OP_SWITCH_WORD:	value = description[PC++] & 255;
				value |= (description[PC++] & 255) << 8;
				if (acc == value)
				  jump ();
				 else
				  PC += 2;	// Skip jump address.
				break;
	case OP_SWITCH_DIGIT:	value = description[PC++] & 255;
				value |= (description[PC++] & 255) << 8;
				if (acc >= '0' && acc <= '9')
				  jump ();
				 else
				  PC += 2;	// Skip jump address.
				break;
	case OP_JE:		if (compare == 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JNE:		if (compare != 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JA:		if (compare > 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JAE:		if (compare >= 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JB:		if (compare < 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JBE:		if (compare <= 0)
				  jump ();
				 else
				  PC += 2;
				break;
	case OP_JSR:		stack[SP++] = PC + 2;
	case OP_JMP:		jump (); break;
	case OP_RET:		PC = stack[--SP]; break;
	case OP_REMOTE:		send (acc); break;
	case OP_ESCAPE:		++PC; break;	/* Ignore for now */
	case OP_ESCAPE_WORD:	PC += 2; break;	/* Ignore for now */
	case OP_TAB:		window -> tab (); break;
	case OP_BELL:		window -> bell (); break;
	case OP_SAVEXY:		savex = window -> x;
				savey = window -> y;
				break;
	case OP_RESTXY:		window -> move (savex,savey); break;
	case OP_GETXY:		regx = window -> x;
				regy = window -> y;
				break;
	case OP_GETX:		acc = regx; break;
	case OP_GETY:		acc = regy; break;
	case OP_SET:		flags |= 1 << (description[PC++]); break;
	case OP_RESET:		flags &= ~(1 << (description[PC++])); break;
	case OP_TEST:		compare = flags & (1 << (description[PC++]));
				break;
	case OP_SCRLUP:		window -> scrollscreen (1); break;
	case OP_SCRLDN:		window -> scrollscreen (0); break;
	case OP_RESARR:		for (value = 0;value < TERM_ARGS;++value)
				  argarray[value] = -1;	// Set defaults.
				index = 0;
				base = -1;
				break;
	case OP_GETARG:		if (isdigit (acc))
				  {
				    if (argarray[index] == -1)
				      argarray[index] = 0;
				    argarray[index] = (argarray[index] * 10)
				    	+ (acc - '0');
				    PC -= 2;	// Backup and do instr again
				  }
				 else
				  ++index;	// Arg recognised - move on
				break;
	case OP_GETA:		acc = argarray[(description[PC] & 255) + base];
				if (acc == -1)
				  acc = description[PC + 1] & 255;
				PC += 2;
				break;
	case OP_GETA_WIDTH:	acc = argarray[(description[PC++] & 255) +
					base];
				if (acc == -1)
				  acc = window -> width;
				break;
	case OP_GETA_HEIGHT:	acc = argarray[(description[PC++] & 255) +
					base];
				if (acc == -1)
				  acc = window -> height;
				break;
	case OP_DEC:		--index; compare = index; break;
	case OP_SHIFT:		++base; break;
	case OP_SETC:		index = acc; break;
	case OP_SAVEATTR:	saveattr = window -> getattr (); break;
	case OP_RESTATTR:	window -> setattr (saveattr); break;
	case OP_INSBLANK:	window -> inschar (-1); break;
	case OP_CLIENT:		UWMaster.startclient (acc);
				break;
	case OP_KEYTAB:		keytab = (description[PC] & 255) |
					 ((description[PC + 1] & 255) << 8);
				PC += 2;
				break;
	case OP_KEYTAB_NONE:	keytab = -1; break;
	case OP_TABND:		window -> tab (0,8,1); break;
	case OP_REVLF:		window -> revlf (); break;
	case OP_CLRMAP:		clrmap (); break;
	case OP_MAP:		changemap (description[PC] & 255,
					   description[PC + 1] & 255);
				PC += 2;
				break;
	case OP_CLRRGN:		window -> clrrgn (); break;
	case OP_REGION:		window -> region (regx,regy); break;
	case OP_REMSTR:		while ((value = description[PC++])
					!= 0)
				  send (value);
				break;
	case OP_REMNUM:		value = acc;
				if (value >= 100)
				  send (value / 100 + '0');
				value %= 100;
				if (value >= 10)
				  send (value / 10 + '0');
				send (value % 10 + '0');
				break;
	case OP_SETTAB:		window -> settab (); break;
	case OP_RESTAB:		window -> restab (); break;
	case OP_CLRTABS:	window -> clrtabs (); break;
	case OP_DEFTABS:	window -> deftabs (); break;
	case OP_SETFORE:	if (!UWConfig.AnsiBright)
				  window -> setattr (
				  	(window -> getattr () & 0xF0)
					| forecols[acc & 7]);
				 else
				  window -> setattr (
				  	(window -> getattr () & 0xF0)
					| britecols[acc & 7]);
				break;
	case OP_SETBACK:	window -> setattr (window -> getattr () & 0x0F
					| backcols[acc & 7]);
				break;
	case OP_COPYATTR:	window -> setscroll (window -> getattr ());
				break;
	case OP_SETBOLD:	window -> setattr (window -> getattr () | 8);
				break;
	case OP_SETBOLDOFF:	window -> setattr (window -> getattr () & ~8);
				break;
	case OP_SETBLINK:	window -> setattr (window -> getattr () | 128);
				break;
	case OP_SETBLINKOFF:	window -> setattr (window -> getattr () & 127);
				break;
	case OP_ADDTITLE:	window -> addtitle (acc); break;
	case OP_CLRTITLE:	window -> clrtitle (); break;
	case OP_SHOWTITLE:	window -> showtitle ();
				UWMaster.status ();
				break;
	case OP_DIRECT:		UWMaster.direct (1); break;
	case OP_NODIRECT:	UWMaster.direct (0); break;
	case OP_ALIGN:		window -> aligntest (acc); break;
	default:		break;
      } // switch //
} // UWTermDesc::interpret //

// Reset the character mapping table to the default.
void	UWTermDesc::clrmap (void)
{
  // Copy the original language translation table //
  memcpy (mapchars,UWConfig.PrintTransTable,256);
} // UWTermDesc::clrmap //

// Process any key mappings for this terminal type.
void	UWTermDesc::key (int keypress)
{
  int posn,match;
  if (keypress < 0x3B00)		// ASCII and normal ALT keys.
    {
      UWTerminal::key (keypress);	// Process keypress normally.
      return;
    }
  if (keytab == -1)
    posn = keys;			// Want to scan key mapping table.
   else
    posn = keytab;			// Use the other key table.
  while (1)
    {
      match = (description[posn] & 255) | ((description[posn + 1] & 255) << 8);
      posn += 2;
      if (!match || keypress == match)
        break;				// Found the key or no match at all.
      if (match == -1)			// Should we jump to a new table?
        posn = (description[posn] & 255)|((description[posn + 1] & 255) << 8);
       else
        posn += description[posn] & 255;// Skip past the string to the next one
    }
#ifdef	DOOBERY
  if (!match && keytab != -1)
    {
      // Try the secondary key table if it is present //
      posn = keytab;			// Want to scan key mapping table.
      while (1)
        {
          match = (description[posn] & 255) |
	  		((description[posn + 1] & 255) << 8);
          posn += 2;
          if (!match || keypress == match)
            break;			   // Found the key or no match at all.
          posn += description[posn] & 255; // Skip past the current string.
        }
    }
#endif
  if (match)
    {
      // Output the characters associated with the key mapping //
      int ch;
      posn++;				// Skip string length value.
      while ((ch = (description[posn++] & 255)) != 0)
	send (ch);			// Send mapping to remote machine.
    }
   else
    UWTerminal::key (keypress);		// Process the key normally.
} // UWTermDesc //

// Process a character from the remote server.  This may
// be called at any time while the client is active.
void	UWTermDesc::remote (int ch)
{
  if (description == 0)
    {
      UWTerminal::remote (ch);		// Emulate a "really dumb" terminal.
      return;
    }
   else
    interpret (ch);			// Interpret the emulation.
} // UWTermDesc::remote //

// Set the description to be used for the terminal
// emulation of this terminal object, and start it up.
void	UWTermDesc::setemul (unsigned char far *desc)
{
  int version;
  if (desc == 0)
    {
      description = desc;		// Just disable the emulation.
      return;
    }
  // Get the starting address from the description and initialise.
  PC = ((*desc) & 255) | (((*(desc + 1)) & 255) << 8);
  keys = ((*(desc + 2)) & 255) | (((*(desc + 3)) & 255) << 8);
  version = ((*(desc + 4)) & 255) | (((*(desc + 5)) & 255) << 8);
  if (version < 0x100 || version > UW_TERM_VERSION)
    return;				// Illegal terminal description.
  if (version >= UW_TERM_TYPECODE_VERS)
    {
      termtype = *(desc + 6);		// Extract the terminal type.
      if (termtype == 127)		// Default to ADM31 if unknown for now.
        termtype = 0;
      description = desc + TERM_HEADER_SIZE; // Skip past starting index value.
    } /* then */
   else
    {
      termtype = 0;			// Default to ADM31 :-( .
      description = desc + TERM_OLD_HEADER_SIZE;
    } /* else */
  regx = 0;				// Start all registers at 0.
  regy = 0;
  flags = 0;
  compare = 0;
  SP = 0;
  savex = 0;
  savey = 0;
  keytab = -1;
  saveattr = window -> getattr ();
  clrmap ();
  interpret (0);			// Execute till first character get.
} // UWTermDesc::setemul //
