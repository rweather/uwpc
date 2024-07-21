//-------------------------------------------------------------------------
//
// DISPLAY.H - Classes for handling the display of window information.
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
//    1.0    20/03/91  RW  Original Version of DISPLAY.H
//    1.1    05/05/91  RW  Start adding Windows 3.0 information.
//
//-------------------------------------------------------------------------

#ifndef __DISPLAY_H__
#define	__DISPLAY_H__

#include "extern.h"		// External DOS and Windows 3.0 declarations.

//
// Define the available clearing types for "UWDisplay::clear".
//
#define	CLR_ALL		0		// Clear the whole screen.
#define	CLR_END_LINE	1		// Clear to end of current line.
#define CLR_END_SCREEN	2		// Clear to end of screen.
#define CLR_ST_LINE	3		// Clear to start of current line.
#define CLR_ST_SCREEN	4		// Clear to start of screen.

//
// Define the general window display class that provides all of
// the facilities for processing a displayed UW window.  If the
// "width" attribute is zero after object creation, then not
// enough memory was available for the display.
//
class	UWDisplay {

protected:

	unsigned *screen;		// Screen RAM buffer.
	int	attop;			// Non-zero when top-most display.
	int	attr,scrollattr;	// Normal and scrolling attributes.
	int	wrap52;			// Non-zero for a VT52 wrap.

#ifdef	UWPC_WINDOWS
	int	charwid,charht;		// Size of characters to be drawn.
	int	curson;			// Non-zero if the cursor is on.

	// Invalidate an area in the window for repainting //
	// This will also do a window update as well.	   //
	void	invalidate (int x1,int y1,int x2,int y2);

	// Turn the cursor off while performing some window update //
	void	cursoroff (void);

	// Turn the cursor back on now //
	void	cursoron  (void);
#endif

	// Define some internal screen manipulation routines //
	void	show	(int X,int Y,unsigned pair);
	void	scroll	(int x1,int y1,int x2,int y2,int lines,int attribute);
	void	setcurs	(void);

public:

	int	width,height;		// Size of the display.
	int	x,y;			// Current cursor position.

#ifdef	UWPC_WINDOWS
	HWND	hWnd;			// Window's handle.
#endif

	UWDisplay (int number);
	~UWDisplay (void);

	// Bring the display to the top on the screen, or
	// disable it from being the top display.  This must
	// only be called by the UW protocol handling classes,
	// never by clients.
	void	top	(int bringup);

	// Send a character to the display directly with no
	// translation of control characters.  If 'vt52wrap'
	// is non-zero, the VT52 line wrapping scheme is used.
	void	send	(int ch,int vt52wrap=0);

	// Perform a carriage return operation on the display.
	void	cr	(void);

	// Perform a line feed operation on the display.  When
	// the cursor is on the last display line, the display
	// will be scrolled one line up in the scrolling colour.
	void	lf	(void);

	// Move back one position on the display.  If 'wrap'
	// is non-zero, wrap to previous lines as well.
	void	bs	(int wrap=1);

	// Tab across to the next tab stop of the supplied size.
	void	tab	(int vt52wrap=0,int tabsize=8);

	// Ring the terminal bell - this directly calls
	// the hardware routines to ring it.
	void	bell	(void);

	// Move the cursor to a new position on the display.
	// The origin is at (0,0).
	void	move	(int newx,int newy);

	// Clear the display according to a particular clearing
	// type.  This function does not move the cursor position.
	void	clear	(int clrtype=CLR_ALL);

	// Insert a new line on the display.
	void	insline	(void);

	// Delete the current line from the display.
	void	delline	(void);

	// Insert a new character into the current line.
	// If 'ch' is -1, then insert a blank and don't move cursor.
	void	inschar	(int ch);

	// Delete the current character, and append the given
	// character to the current line.
	void	delchar	(int ch=' ');

	// Set the normal printing attribute.
	void	setattr	(int attribute)
		  { attr = attribute; };

	// Get the normal printing attribute.
	int	getattr (void) { return (attr); };

	// Set the scrolling attribute for printing.
	void	setscroll (int attribute)
		  { scrollattr = attribute; };

	// Scroll the screen up or down one line.
	void	scrollscreen (int up);

	// If the screen has a status line, then set it to the
	// given string, otherwise ignore the request.  If "str"
	// is NULL, then clear the status line (i.e. don't display).
	void	status	(char *str,int length);

#ifdef	UWPC_WINDOWS
	// Process the messages for a display window.
	// This is a catch-all after the master routine
	// processes the messages.
	LONG	mesgs	(WORD message,WORD wParam,LONG lParam);
#endif
};

#endif	/* __DISPLAY_H__ */
