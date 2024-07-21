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
//    1.2    08/06/91  RW  Add support for cut and paste.
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

#define	WTITLE_LEN	64		// Maximum title buffer length.
#define	TAB_SET_SIZE	30

//
// Define the general window display class that provides all of
// the facilities for processing a displayed UW window.  If the
// "width" attribute is zero after object creation, then not
// enough memory was available for the display.
//
class	UWDisplay {

protected:

	unsigned far *screen;		// Screen RAM buffer.
	int	attop;			// Non-zero when top-most display.
	int	attr,scrollattr;	// Normal and scrolling attributes.
	int	wrap52;			// Non-zero for a VT52 wrap.
	int	toprgn,botmrgn;		// Top and bottom of scrolling region.
	unsigned char tabs[TAB_SET_SIZE]; // Positions of the tabs.

#ifdef	UWPC_WINDOWS
	int	curson;			// Non-zero if the cursor is on.
	BOOL	careton;		// TRUE if caret is on.
	int	mousebuttons;		// Status of the mouse buttons.
	int	chbufferx,chbuffery;	// Position of suspension buffer.
	int	chbufferlen;		// Number of characters in buffer.
	int	suspended;		// Non-zero if output is suspended.

	// Draw a line of characters from the suspension buffer.
	void	drawline (HDC hDC=NULL);

	// Repaint and area of the current window.  If hDC    //
	// is NULL, then GetDC will be used to get a context. //
	void	repaint (int x1,int y1,int x2,int y2,HDC hDC=NULL);

	// Turn the cursor off while performing some window update //
	void	cursoroff (void);

	// Turn the cursor back on now //
	void	cursoron  (void);
#endif

	char	title[WTITLE_LEN];	// Title to display in the status bar.

	// Define some internal screen manipulation routines //
	void	show	(int X,int Y,unsigned pair);
	void	scroll	(int x1,int y1,int x2,int y2,int lines,int attribute);
	void	setcurs	(void);

public:

	int	width,height;		// Size of the display.
	int	x,y;			// Current cursor position.
	int	wNumber;		// Window number.

#ifdef	UWPC_WINDOWS
	HWND	hWnd;			// Window's handle.
#endif

	UWDisplay (int number);
	~UWDisplay (void);

	// Get the current window title.  This is only called
	// in the DOS version of UW/PC.
	char	*wtitle (void) { return (title); };

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

	// Perform a reverse line feed on the display.  When
	// the cursor is on the first display line, the display
	// will be scrolled one line down in the scrolling colour.
	void	revlf	(void);

	// Clear any current scrolling region on the display.
	void	clrrgn	(void) { toprgn = 0; botmrgn = height - 1; };

	// Set a scrolling region on the display.
	void	region	(int top,int botm);

	// Move back one position on the display.  If 'wrap'
	// is non-zero, wrap to previous lines as well.
	void	bs	(int wrap=1);

	// Tab across to the next tab stop of the supplied size.
	// If "nd" is non-zero then the tab is non-destructive.
	void	tab	(int vt52wrap=0,int tabsize=8,int nd=0);

	// Ring the terminal bell - this directly calls
	// the hardware routines to ring it.
	void	bell	(void);

	// Move the cursor to a new position on the display.
	// The origin is at (0,0).
	void	move	(int newx,int newy);

	// Move the cursor to a new position relative to
	// its current position.
	void	moverel	(int relx,int rely);

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

	// Mark a rectangle on the screen for cut-and-paste.
	// Two calls to this routine will remove the mark.
	void	markcut	(int x1,int y1,int x2,int y2);

	// Copy screen data into a clipboard buffer.  The
	// length of the written data is returned.
	int	copycut	(int x1,int y1,int x2,int y2,char far *buffer);

	// Set a tab stop at the current X position, or "posn"
	// if it is not negative.
	void	settab (int posn=-1);

	// Reset a tab stop at the current Y position.
	void	restab (void);

	// Clear all tab stops.
	void	clrtabs (void);

	// Set the default tab stops every "tabsize" positions.
	void	deftabs (int tabsize=8);

	// Clear the current window title.
	void	clrtitle (void);

	// Add another character to the end of the current window title.
	void	addtitle (int ch);

	// Set the displayed title for the window.  This is mainly
	// for the benefit of the Windows 3.0 version.
	void	showtitle (void);

	// Fill the entire window with a character to perform an
	// alignment test.
	void	aligntest (int ch);

#ifdef	UWPC_WINDOWS
	// Suspend output in the window for a little while.
	void	suspend (int stop);

	// Process a Windows 3.0 mouse message and create a
	// UW/PC mouse message for the client attached to
	// this particular window.
	void	sendmouse (int x,int y);

	// Process the messages for a display window.
	// This is a catch-all after the master routine
	// processes the messages.
	LONG	mesgs	(WORD message,WORD wParam,LONG lParam);
#endif
};

#endif	/* __DISPLAY_H__ */
