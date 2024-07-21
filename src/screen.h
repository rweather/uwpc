//-------------------------------------------------------------------------
//
// SCREEN.H - Direct screen accessing routines for textual displays.
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
//    1.0    23/03/91  RW  Original Version of SCREEN.H
//    1.1    26/05/91  RW  Add command-line to "jumpdos".
//
//-------------------------------------------------------------------------

#ifndef __SCREEN_H__
#define	__SCREEN_H__

//
// Define the available hardware cursor shapes.
//
enum   CursorShapes {
	CURS_INVISIBLE		= 0,	// Invisible cursor shape.
	CURS_UNDERLINE		= 1,	// Underlining cursor shape.
	CURS_HALF_HEIGHT	= 2,	// A half-height cursor shape.
	CURS_FULL_HEIGHT	= 3	// A full-height cursor shape.
};

//
// Define the attribute information.
//
enum   ScreenAttrs {
	ATTR_NORMAL		= 0,
	ATTR_INVERSE		= 1,
	ATTR_HIGHLIGHT		= 2,
	ATTR_STATUS		= 3,
	ATTR_HIGH_STATUS	= 4,
	NUM_ATTRS		= 5
};

//
// Define the hardware screen object.  An instance of this
// object is created statically as "HardwareScreen".  All
// (x,y) co-ordinates have their origin at (0,0).
//
class	ScreenClass {

private:

	unsigned far	*screenram;	// Position of the hardware screen RAM.
	int	 flags;			// Internal hardware screen flags.
	unsigned char	oldattr;	// Old screen attribute to be restored.
	int	 mode;			// Screen mode being used.
	int	 saveshape;		// Saved cursor shape
	int	 savex,savey;		// Saved cursor position.
	int	 dlgx1,dlgy1,dlgx2,dlgy2; // Position of the dialog box.

public:

	int	width,height;		// Size of the hardware screen.
	unsigned char	attributes[NUM_ATTRS]; // Attribute indexes.
	int	 dialogenabled;		// Non-zero if a dialog box is enabled.

	// Initialise the hardware screen.  Returns non-zero
	// if OK, or zero if there is not enough memory.
	int	init	(int color=1,int large=1);

	// Terminate the hardware screen.
	void	term	(void);

	// Test to see if a colour mode is in effect.
	int	iscolor	(void);

	// Set the position of the hardware screen cursor.
	void	cursor	(int x,int y);

	// Set the shape of the hardware cursor.
	void	shape	(CursorShapes curs);

	// Ring the hardware terminal bell.
	void	bell	(void);

	// Mark an area for the dialog box.
	void	mark	(int x1,int y1,int x2,int y2);

	// Clear the marked dialog box area.
	void	clearmark (void) { dialogenabled = 0; };

	// Draw a character/attribute pair on the screen.
	void	draw	(int x,int y,unsigned pair,int dialog=0);

	// Draw a lines of character/attribute pairs on the screen.
	void	line	(int x,int y,unsigned *pairs,int numpairs,
			 int dialog=0);

	// Scroll an area of the screen a number of lines.
	void	scroll	(int x1,int y1,int x2,int y2,int lines,
			 unsigned char attr);

	// Clear the screen and shell out to DOS.  Restore the screen
	// mode on return (caller is responsible for screen redraw).
	// Optionally execute a DOS command.
	void	jumpdos	(char *cmdline);
};

//
// Define the primary hardware screen handling object.
//
extern	ScreenClass	HardwareScreen;

#endif	/* __SCREEN_H__ */
