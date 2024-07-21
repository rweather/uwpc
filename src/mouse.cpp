//-------------------------------------------------------------------------
//
// MOUSE.CPP - Declarations for handling the mouse events.
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
//    1.0    08/06/91  RW  Original Version of MOUSE.CPP
//    1.1    14/03/92  RW  Add support for large screen modes.
//
//-------------------------------------------------------------------------

#pragma	inline			// Module contains inline assembly.

#include "mouse.h"		// Declarations for this module.
#include "client.h"		// Client declarations.
#include "screen.h"		// Screen accessing declarations.
#include <dos.h>		// Interrupt services, etc.

//
// The following variable will be non-zero when the mouse status
// has been detectably changed.
//
int	MouseChange=0;

//
// The following variable will be non-zero when the mouse is active.
//
int	MouseActive=0;

//
// Global data for this module.
//
static	void	far *SaveIntSubr;
static	int	SaveIntMask;
static	int	MouseVisible;

//
// Define the mouse interrupt subroutine to be called
// whenever the mouse status changes.
///
static	void	far MouseIntSubr (void)
{
  asm push ds;
  asm mov ax,DGROUP;		// Restore the proper data segment address.
  asm mov ds,ax;
  MouseChange = 1;		// Record a change in the mouse status.
  asm pop ds;			// Restore DS and exit to mouse driver.
} // MouseIntSubr //

//
// Initialise the mouse to be used by UW/PC.  This routine should
// be called after the screen has been initialised.  It returns
// a non-zero value if the mouse could be initialised, zero otherwise.
//
int	InitMouse (void)
{
  REGS regs;
  SREGS sregs;
  MouseChange = 0;
  MouseVisible = 0;
  regs.x.ax = 0x0000;		// Reset the mouse driver.
  int86 (0x33,&regs,&regs);
  if (regs.x.ax != 0xFFFF)
    return (0);			// Could not initialise the driver.
  regs.x.ax = 0x0014;		// Install mouse interrupt subroutine.
  regs.x.cx = 0x001F;		// Call for mouse movements and buttons.
  regs.x.dx = FP_OFF (MouseIntSubr);
  sregs.es = FP_SEG (MouseIntSubr);
  int86x (0x33,&regs,&regs,&sregs);
  SaveIntSubr = MK_FP (sregs.es,regs.x.dx);
  SaveIntMask = regs.x.cx;
  MouseActive = 1;
  regs.x.ax = 0x0007;		// Set the horizontal mouse extent.
  regs.x.cx = 0;
  regs.x.dx = HardwareScreen.width * 8 - 1;
  int86 (0x33,&regs,&regs);
  regs.x.ax = 0x0008;		// Set the vertical mouse extent.
  regs.x.cx = 0;
  regs.x.dx = HardwareScreen.height * 8 - 1;
  int86 (0x33,&regs,&regs);
  regs.x.ax = 0x0004;		// Set the initial mouse cursor position.
  regs.x.cx = (HardwareScreen.width - 1) * 8;
  regs.x.dx = (HardwareScreen.height - 1) * 8;
  int86 (0x33,&regs,&regs);
  ShowMouse ();
  return (1);
} // InitMouse //

//
// Terminate the handling of the mouse.
//
void	TermMouse (void)
{
  REGS regs;
  SREGS sregs;
  HideMouse ();
  regs.x.ax = 0x0014;		// Restore mouse interrupt subroutine.
  regs.x.cx = SaveIntMask;
  regs.x.dx = FP_OFF (SaveIntSubr);
  sregs.es = FP_SEG (SaveIntSubr);
  int86x (0x33,&regs,&regs,&sregs);
  MouseChange = 0;
  MouseActive = 0;
} // TermMouse //

//
// Send the current mouse status to the given client.  This may
// be called any time after initialisation, but usually after
// "MouseChange" has been set.
//
void	SendMouseEvent (UWClient *client)
{
  REGS regs;
  regs.x.ax = 0x0003;		// Read the mouse status.
  int86 (0x33,&regs,&regs);
  client -> mouse (regs.x.cx / 8,regs.x.dx / 8,(regs.x.bx & 3));
  MouseChange = 0;
} // SendMouseEvent //

//
// Display the mouse if it is not already visible.
//
void	ShowMouse (void)
{
  REGS regs;
  if (MouseActive && !MouseVisible)
    {
      regs.x.ax = 0x0001;
      int86 (0x33,&regs,&regs);
      MouseVisible = 1;
    }
} // ShowMouse //

//
// Remove the mouse from the screen.
//
void	HideMouse (void)
{
  REGS regs;
  if (MouseActive && MouseVisible)
    {
      regs.x.ax = 0x0002;
      int86 (0x33,&regs,&regs);
      MouseVisible = 0;
    }
} // HideMouse //
