//-------------------------------------------------------------------------
//
// SCREEN.CPP - Direct screen accessing routines for textual displays.
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
//    1.0    23/03/91  RW  Original Version of SCREEN.CPP
//    1.1    26/05/91  RW  Add command-line to "jumpdos".
//    1.2    14/03/92  RW  Add support for 43/50 line modes.
//
//-------------------------------------------------------------------------

#include "screen.h"		// Declarations for this module.
#include "config.h"		// Configuration declarations.
#include "mouse.h"		// Mouse handling routines.
#include <dos.h>		// Turbo C++ DOS/BIOS/hardware routines.
#include <conio.h>		// Turbo C++ console routines.
#include <mem.h>		// Memory manipulation routines.
#include <stdlib.h>		// "system" is defined here.

#pragma	warn	-par		// Turn off annoying warnings.

//
// Define the primary hardware screen handling object.
//
ScreenClass	HardwareScreen;

//
// Define the bits in the "ScreenClass::flags" attribute.
//
enum   ScreenFlags {
	FLAG_MDA	= 1,		// MDA screen mode.
	FLAG_SNOW	= 2,		// Snow checking is enabled.
	FLAG_COLOR	= 4,		// Screen mode is colour.
	FLAG_LARGE	= 8		// Screen is in large 43/50 mode.
};

// Initialise the hardware screen.  Returns non-zero
// if OK, or zero if there is not enough memory.
int	ScreenClass::init (int color,int large)
{
  struct text_info ti;
  static unsigned char monoattrs[NUM_ATTRS] = {0x07,0x70,0x0F,0x70,0x07};
  static unsigned char colrattrs[NUM_ATTRS] = {0x1F,0x71,0x1E,0x4E,0x41};
  int temp;
  gettextinfo (&ti);
  origmode = ti.currmode;
  oldattr = ti.attribute;
  dialogenabled = 0;		// Disable the dialog box.
  if (ti.currmode == 7)
    {
      mode = 7;
      flags = FLAG_MDA;
      screenram = (unsigned far *)0xB0000000L;
    } /* then */
  else
    {
      if (!color && (ti.currmode == 0 || ti.currmode == 2))
        {
          mode = 2;
	  flags = FLAG_SNOW;
        } /* then */
       else
        {
          mode = 3;
	  flags = FLAG_SNOW | FLAG_COLOR;
        } /* else */
      screenram = (unsigned far *)0xB8000000L;
    } /* else */
  if (!iscolor ())
    memcpy (attributes,monoattrs,NUM_ATTRS);
   else
    memcpy (attributes,colrattrs,NUM_ATTRS);
  for (temp = 0;temp < NUM_ATTRS;++temp)
    {
      // Override the default attribute scheme //
      if (UWConfig.NewAttrs[temp])
        attributes[temp] = UWConfig.NewAttrs[temp];
    }
  textmode (mode);
  if (large && mode < 7)
    {
      textmode (C4350);
      flags |= FLAG_LARGE;
    }
  shape ((CursorShapes)UWConfig.CursorSize);
  gettextinfo (&ti);
  width = ti.screenwidth;
  height = ti.screenheight;
  delay (10);		// Initialise Turbo C++'s delay timer.
  return (1);
} // ScreenClass::init //

// Terminate the hardware screen.
void	ScreenClass::term (void)
{
  textmode (origmode);	// Return to the original text mode.
  shape (CURS_UNDERLINE);
  textattr (oldattr);
  clrscr ();
  gotoxy (1,1);
} // ScreenClass::term //

// Test to see if a colour mode is in effect.
int	ScreenClass::iscolor (void)
{
  return ((flags & FLAG_COLOR) != 0);
} // ScreenClass::iscolor //

// Set the position of the hardware screen cursor.
void	ScreenClass::cursor (int x,int y)
{
  union REGS regs;
  regs.h.ah = 2;
  regs.h.bh = 0;
  regs.h.dl = x;
  regs.h.dh = y;
  int86 (0x10,&regs,&regs);
} // ScreenClass::cursor //

// Set the shape of the hardware cursor.
void	ScreenClass::shape (CursorShapes curs)
{
  union REGS regs;
  static int monoshapes[] = {0x200C,0x0B0C,0x070C,0x000C};
  static int colrshapes[] = {0x2007,0x0607,0x0407,0x0007};
  regs.h.ah = 1;
  if (flags & FLAG_MDA)
    regs.x.cx = monoshapes[curs];
   else
    regs.x.cx = colrshapes[curs];
  int86 (0x10,&regs,&regs);
} // ScreenClass::shape //

// Ring the hardware terminal bell.
void	ScreenClass::bell (void)
{
  union REGS regs;
  if (UWConfig.BeepEnable)
    {
#ifdef	OLD_BELL
      regs.x.ax = 0x0E07;
      regs.x.bx = 0;
      int86 (0x10,&regs,&regs);	// Call video BIOS to ring the bell.
#else
      sound (UWConfig.BellFreq);// Use the sound functions for the sound.
      delay (UWConfig.BellDur);
      nosound ();
#endif
    }
} // ScreenClass::bell //

// Mark an area for the dialog box.
void	ScreenClass::mark (int x1,int y1,int x2,int y2)
{
  dialogenabled = 0;
  dlgx1 = x1;
  dlgy1 = y1;
  dlgx2 = x2;
  dlgy2 = y2;
} // ScreenClass::mark //

// Draw a single pair on the screen, waiting for snow //
static	void	drawsnow (unsigned far *addr,unsigned pair)
{
  _ES = FP_SEG(addr);
  _DI = FP_OFF(addr);
  _BX = pair;
  /* Assembly code here is:

		mov  dx,3dah
	 loop1: in   al,dx
		test al,1
		jnz  loop1
		cli
	 loop2: in   al,dx
		test al,1
		jz   loop2
		mov  es:[di],bx
		sti
  */
  __emit__ (0xBA,0xDA,0x03,0xEC,0xA8,0x01,
	    0x75,0xFB,0xFA,0xEC,0xA8,0x01,
	    0x74,0xFB,0x26,0x89,0x1D,0xFB);
} // drawsnow //

// Draw a character/attribute pair on the screen.
void	ScreenClass::draw (int x,int y,unsigned pair,int dialog)
{
  if (dialogenabled && !dialog)
    {
      if (y >= dlgy1 && y <= dlgy2 && x >= dlgx1 && x <= dlgx2)
        return;		// Ignore when in the area of the dialog box.
    }
  HideMouse ();		// Turn off mouse if necessary.
  if (flags & FLAG_SNOW)
    drawsnow (screenram + x + (y * width),pair);
   else
    screenram[x + (y * width)] = pair;
  ShowMouse ();		// Turn the mouse back on.
} // ScreenClass::draw //

// Draw a lines of character/attribute pairs on the screen.
void	ScreenClass::line (int x,int y,unsigned far *pairs,int numpairs,
		 	   int dialog)
{
  unsigned far *screen;
  screen = screenram + x + (y * width);
  if (dialogenabled && !dialog)
    {
      if (y >= dlgy1 && y <= dlgy2 && x <= dlgx2)
        {
	  // Send the line direct, but ignore where it crosses dialog box.
	  HideMouse ();
	  while (numpairs-- > 0)
	    {
	      if (x < dlgx1 || x > dlgx2)
	        {
                  if (flags & FLAG_SNOW)
                    drawsnow (screen++,*pairs);
                   else
                    *screen++ = *pairs;
		}
	      ++x;
	      ++pairs;
	    }
	  ShowMouse ();
	  return;		// Ignore the rest of this function.
	}
    }
  HideMouse ();
  if (flags & FLAG_SNOW)
    {
      while (numpairs-- > 0)
        drawsnow (screen++,*pairs++);
    }
   else
    {
      while (numpairs-- > 0)
        *screen++ = *pairs++;
    }
  ShowMouse ();
} // ScreenClass::line //

// Scroll an area of the screen a number of lines.
void	ScreenClass::scroll (int x1,int y1,int x2,int y2,int lines,
		 	     unsigned char attr)
{
  union REGS regs;
  if (lines >= 0)
    regs.x.ax = 0x600 + lines;
   else
    regs.x.ax = 0x700 - lines;
  regs.h.bh = attr;
  regs.h.cl = x1;
  regs.h.ch = y1;
  regs.h.dl = x2;
  regs.h.dh = y2;
  HideMouse ();
  int86 (0x10,&regs,&regs);
  ShowMouse ();
} // ScreenClass::scroll //

// Clear the screen and shell out to DOS.  Restore the screen
// mode on return (caller is responsible for screen redraw).
// Optionally execute a DOS command.
void	ScreenClass::jumpdos (char *cmdline)
{
  union REGS regs;
  regs.h.ah = 3;
  regs.h.bh = 0;
  int86 (0x10,&regs,&regs);
  savex = regs.h.dl;
  savey = regs.h.dh;
  saveshape = regs.x.cx;
  textattr (oldattr);
  HideMouse ();
  textmode (origmode);		// Go to original mode on a shell-out.
  clrscr ();
  gotoxy (1,1);
  shape (CURS_UNDERLINE);
  if (UWConfig.EnableMouse)
    TermMouse ();		// Terminate mouse during shell-out.
  if (cmdline)
    system (cmdline);		// Execute the DOS command.
   else
    {
      cprintf ("Type EXIT at the DOS prompt to return to UW/PC.\r\n");
      system ("");
    }
  textmode (mode);		// Restore the text mode.
  if (flags & FLAG_LARGE)
    textmode (C4350);		// Restore 43/50 line mode if required.
  if (UWConfig.EnableMouse)
    UWConfig.EnableMouse = InitMouse (); // Enable the mouse again.
  cursor (savex,savey);
  regs.h.ah = 1;
  regs.x.cx = saveshape;
  int86 (0x10,&regs,&regs);
} // ScreenClass::jumpdos //
