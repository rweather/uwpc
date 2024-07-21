//-------------------------------------------------------------------------
//
// DISPLAY.CPP - Classes for handling the display of window information.
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
//    1.0    21/03/91  RW  Original Version of DISPLAY.CPP
//    1.1    05/05/91  RW  Clean up and start adding Win3 support.
//
//-------------------------------------------------------------------------

#include "display.h"		// Declarations for this module.
#include "screen.h"		// Screen display handling routines.
#include "config.h"		// Status positions defined here.
#include <mem.h>		// Memory accessing routines.

#pragma	warn	-par

#ifdef	UWPC_DOS
#define	CURSOR_OFF()
#define	CURSOR_ON()
#else
#define	CURSOR_OFF()	cursoroff()
#define	CURSOR_ON()	cursoron()
#endif

#ifdef	UWPC_WINDOWS

//
// Define the window style to use for creating display windows.
//
//	WS_CHILD	- child window to parent UW/PC window.
//	WS_CLIPSIBLINGS	- clip to neighbours boundaries.
//	WS_CAPTION	- has a title bar.
//	WS_BORDER	- window has a normal (non-resizable) border.
//
#define	WINDOW_STYLE	(WS_CHILD | WS_CLIPSIBLINGS | WS_CAPTION | WS_BORDER)

// Invalidate an area in the window for repainting //
// This will also do a window update as well.	   //
void	UWDisplay::invalidate (int x1,int y1,int x2,int y2)
{
  RECT rect;
  rect.left = x1 * charwid;	// Get the region to be invalidated.
  rect.top = y1 * charht;
  rect.right = (x2 + 1) * charwid - 1;
  rect.bottom = (y2 + 1) * charht - 1;
  InvalidateRect (hWnd,&rect,0);// Invalidate the rectangle.
  UpdateWindow (hWnd);		// Now update the invalidated area.
} // UWDisplay::invalidate //

// Turn the cursor off while performing some window update //
void	UWDisplay::cursoroff (void)
{
  if (curson)
    {
      curson = 0;
      invalidate (x,y,x,y);	// Redraw character at cursor position.
    }
} // UWDisplay::cursoroff //

// Turn the cursor back on now //
void	UWDisplay::cursoron (void)
{
  if (!curson)
    {
      curson = 1;
      invalidate (x,y,x,y);	// Redraw character at cursor position.
    }
} // UWDisplay::cursoron //

#endif	/* UWPC_WINDOWS */

void	UWDisplay::show (int X,int Y,unsigned pair)
{
  int offset;
  offset = X + (Y * width);
  screen[offset] = pair;
#ifdef	UWPC_DOS
  if (attop)
    HardwareScreen.draw (X,Y,pair);
#else
  invalidate (X,Y,X,Y);
#endif
} // UWDisplay::show //

void	UWDisplay::scroll (int x1,int y1,int x2,int y2,int lines,int attribute)
{
  int offset,saveofs,nextofs,x,y,linewid;
  unsigned clearpair;

  // Clear or scroll the memory for the screen image //
  clearpair = ' ' | (attribute << 8);
  if (lines == 0)
    {
      // Clear the entire area to be scrolled //
      offset = x1 + (y1 * width);
      for (y = y1;y <= y2;++y)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
	    screen[offset++] = clearpair;
	  offset = saveofs + width;
	}
    }
   else if (lines > 0)
    {
// #ifdef	DOOBERY
      // Scroll the area up a number of lines //
      offset = x1 + (y1 * width);
      nextofs = offset + width;
      for (y = y1;y <= (y2 - lines);++y)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
	    screen[offset++] = screen[nextofs++];
	  offset = saveofs + width;
	  nextofs = offset + width;
	}
      while (y <= y2)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
            screen[offset++] = clearpair;
	  offset = saveofs + width;
	  ++y;
	}
    }
   else
    {
      // Scroll the area down a number of lines //
      offset = x1 + (y2 * width);
      nextofs = offset - width;
      for (y = y2;y >= (y1 - lines);--y)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
	    screen[offset++] = screen[nextofs++];
	  offset = saveofs - width;
	  nextofs = offset - width;
	}
      while (y >= y1)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
            screen[offset++] = clearpair;
	  offset = saveofs - width;
	  --y;
	}
    }
// #endif	DOOBERY
#ifdef	DOOBERY		// Comment this code out - it doesn't work :-(
      // Scroll the area up a number of lines //
      offset = x1 + (y1 * width);
      nextofs = offset + width;
      linewid = (x2 - x1 + 1) << 1;
      for (y = y1;y <= (y2 - lines);++y)
        {
	  memcpy (screen + offset,screen + nextofs,linewid);
	  offset += width;
	  nextofs += width;
	}
      while (y <= y2)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
            screen[offset++] = clearpair;
	  offset = saveofs + width;
	  ++y;
	}
    }
   else
    {
      // Scroll the area down a number of lines //
      offset = x1 + (y2 * width);
      nextofs = offset - width;
      linewid = (x2 - x1 + 1) << 1;
      for (y = y2;y >= (y1 - lines);--y)
        {
	  memcpy (screen + offset,screen + nextofs,linewid);
	  offset += width;
	  nextofs += width;
	}
      while (y >= y1)
        {
	  saveofs = offset;
          for (x = x1;x <= x2;++x)
            screen[offset++] = clearpair;
	  offset = saveofs - width;
	  --y;
	}
    }
#endif	DOOBERY
#ifdef	UWPC_DOS
  if (attop)
    {
      if (HardwareScreen.dialogenabled)
        top (attop);		// Redraw window when there is a dialog box.
       else
        HardwareScreen.scroll (x1,y1,x2,y2,lines,attribute);
    }
#else
  invalidate (x1,y1,x2,y2);	// Invalidate the scrolling area.
#endif
} // UWDisplay::scroll //

void	UWDisplay::setcurs (void)
{
#ifdef	UWPC_DOS
  if (attop && !HardwareScreen.dialogenabled)
    HardwareScreen.cursor (x,y);
#endif
} // UWDisplay::setcurs //

UWDisplay::UWDisplay (int number)
{
#ifdef	UWPC_DOS
  width = HardwareScreen.width;
  height = HardwareScreen.height - 1;
#else
  char title[2] = "N";
  width = 80;
  height = 24;
  hWnd = NULL;		// Mark the Windows 3.0 window as not present.
#endif
  screen = new unsigned [width * height];
  attop = 0;
#ifdef	UWPC_DOS
  attr = HardwareScreen.attributes[ATTR_NORMAL];
#else
  attr = 0x70;
#endif
  scrollattr = attr;
  x = 0;
  y = 0;
  wrap52 = 0;
  if (screen == 0)
    width = 0;
   else
    {
#ifdef	UWPC_WINDOWS
      // Create the Windows 3.0 window associated with this display object //
      title[0] = number + '0';
      hWnd = CreateWindow ("UWSESSION",		// Name of window class.
      			   title,		// Window name.
			   WINDOW_STYLE,	// Style of window to use.
			   CW_USEDEFAULT,	// Default initial position.
			   CW_USEDEFAULT,
			   CW_USEDEFAULT,	// Size of the window.
			   CW_USEDEFAULT,
			   NULL,		// Parent (*****)
			   NULL,		// Menu handle - use default.
			   NULL,		// Program instance (*****)
			   NULL);		// Extra information.
#endif
      clear ();
    }
} // UWDisplay::UWDisplay //

UWDisplay::~UWDisplay (void)
{
  if (screen)
    delete screen;
#ifdef	UWPC_WINDOWS
  if (hWnd)
    DestroyWindow (hWnd);
#endif
} // UWDisplay::~UWDisplay //

// Bring the display to the top on the screen, or
// disable it from being the top display.  This must
// only be called by the UW protocol handling classes,
// never explicitly by clients.
void	UWDisplay::top	(int bringup)
{
  if (bringup)
    {
#ifdef	UWPC_DOS
      // Mark the display as being at top and then redraw it //
      int offset,endofs,y;
      attop = 1;
      endofs = width * height;
      y = 0;
      for (offset = 0;offset < endofs;offset += width)
        HardwareScreen.line (0,y++,screen + offset,width);
      setcurs ();
      if (!HardwareScreen.dialogenabled)
        HardwareScreen.shape (CURS_UNDERLINE);
#else
      attop = 1;
      BringWindowToTop (hWnd);	// Just bring to the top in Windows 3.0.
#endif
    }
   else
    attop = 0;
} // UWDisplay::top //

// Send a character to the display directly with no
// translation of control characters.  If 'vt52wrap'
// is non-zero, the VT52 line wrapping scheme is used.
void	UWDisplay::send	(int ch,int vt52wrap)
{
  if (wrap52)
    {
      cr ();
      lf ();
    }
  CURSOR_OFF ();
  show (x,y,((unsigned)(ch & 255)) | ((unsigned)(attr << 8)));
  ++x;
  if (x >= width)
    {
      if (vt52wrap)
        {
	  wrap52 = 1;
	  --x;
	}
       else
	{
	  cr ();
	  lf ();
	}
    }
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::send //

// Perform a carriage return operation on the display.
void	UWDisplay::cr	(void)
{
  CURSOR_OFF ();
  x = 0;
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::cr //

// Perform a line feed operation on the display.  When
// the cursor is on the last display line, the display
// will be scrolled one line up in the scrolling colour.
void	UWDisplay::lf	(void)
{
  CURSOR_OFF ();
  ++y;
  if (y >= height)
    {
      // Scroll the display up one line //
      --y;
      scroll (0,0,width - 1,height - 1,1,scrollattr);
    }
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::lf //

// Move back one position on the display.  If 'wrap'
// is non-zero, wrap to previous lines as well.
void	UWDisplay::bs	(int wrap)
{
  CURSOR_OFF ();
  --x;
  if (x < 0)
    {
      if (wrap && y > 0)
        {
	  --y;
	  x = width - 1;
	}
       else
        x = 0;
    }
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::bs //

// Tab across to the next tab stop of the supplied size.
void	UWDisplay::tab (int vt52wrap,int tabsize)
{
  do
    {
      send (' ',vt52wrap);
    }
  while ((x % tabsize) != 0);
} // UWDisplay::tab //

// Ring the terminal bell - this directly calls
// the hardware routines to ring it.
void	UWDisplay::bell (void)
{
#ifdef	UWPC_DOS
  HardwareScreen.bell ();
#endif
} // UWDisplay::bell //

// Move the cursor to a new position on the display.
// The origin is at (0,0).
void	UWDisplay::move	(int newx,int newy)
{
  CURSOR_OFF ();
  if (newx >= 0 && newx < width && newy >= 0 && newy < height)
    {
      x = newx;
      y = newy;
    }
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::move //

// Clear the display according to a particular clearing
// type.  This function does not move the cursor position.
void	UWDisplay::clear	(int clrtype)
{
  switch (clrtype)
    {
      case CLR_ALL:
      		scroll (0,0,width - 1,height - 1,0,scrollattr);
		break;
      case CLR_END_SCREEN:
      		if (y < (height - 1))
		  scroll (0,y + 1,width - 1,height - 1,0,scrollattr);
		/* Fall through to end of line clear */
      case CLR_END_LINE:
      		scroll (x,y,width - 1,y,0,scrollattr);
		break;
      case CLR_ST_SCREEN:
      		if (y > 0)
		  scroll (0,0,width - 1,y - 1,0,scrollattr);
		/* Fall through to start of line clear */
      case CLR_ST_LINE:
      		if (x > 0)
      		  scroll (0,y,x - 1,y,0,scrollattr);
		break;
      default:	break;
    }
  wrap52 = 0;
} // UWDisplay::clear //

// Insert a new line on the display.
void	UWDisplay::insline	(void)
{
  scroll (0,y,width - 1,height - 1,-1,scrollattr);
  wrap52 = 0;
} // UWDisplay::insline //

// Delete the current line from the display.
void	UWDisplay::delline	(void)
{
  scroll (0,y,width - 1,height - 1,1,scrollattr);
  wrap52 = 0;
} // UWDisplay::delline //

// Insert a new character into the current line.
// If 'ch' is -1, then insert a blank and don't move cursor.
void	UWDisplay::inschar	(int ch)
{
  int offset,xval;
  unsigned pair,temp;
  offset = x + (y * width);
  xval = x;
  if (ch != -1)
    pair = (ch & 255) | (attr << 8);
   else
    pair = ' ' | (attr << 8);
  while (xval < width)
    {
      // Swap the insertion character with the screen character //
      temp = screen[offset];
      screen[offset] = pair;
#ifdef	UWPC_DOS
      if (attop)
        HardwareScreen.draw (xval,y,pair);
#endif
      pair = temp;
      ++offset;
      ++xval;
    }
#ifdef	UWPC_WINDOWS
  if (x < (width - 1))
    invalidate (x,y,width - 2,y);
#endif
  if (ch != -1)
    send (ch);
} // UWDisplay::inschar //

// Delete the current character, and append the given
// character to the current line.
void	UWDisplay::delchar	(int ch)
{
  int offset,xval;
  offset = x + (y * width);
  xval = x;
  while (xval < (width - 1))
    {
      // Move the characters back one on the line //
      screen[offset] = screen[offset + 1];
#ifdef	UWPC_DOS
      if (attop)
        HardwareScreen.draw (xval,y,screen[offset]);
#endif
      ++offset;
      ++xval;
    }
  // Place the given character at the line's end //
  screen[offset] = (ch & 255) | (scrollattr << 8);
#ifdef	UWPC_DOS
  if (attop)
    HardwareScreen.draw (width - 1,y,screen[offset]);
#else
  invalidate (x,y,width - 1,y);
#endif
  wrap52 = 0;
} // UWDisplay::delchar //

// Scroll the screen up or down one line.
void	UWDisplay::scrollscreen (int up)
{
  if (up)
    scroll (0,0,width - 1,height - 1,1,scrollattr);
   else
    scroll (0,0,width - 1,height - 1,-1,scrollattr);
} // UWDisplay::scrollscreen //

// If the screen has a status line, then set it to the
// given string, otherwise ignore the request.  If "str"
// is NULL, then clear the status line (i.e. don't display).
void	UWDisplay::status	(char *str,int length)
{
#ifdef	UWPC_DOS		// Only display status line in DOS version.
  int x=0;
  unsigned attrmask;
  if (str != 0)
    switch (UWConfig.StatusPosn)
      {
        case STATUS_LEFT:
        case STATUS_LEFT_SQUASH:
      		x = 0;
		if (UWConfig.StatusPosn == STATUS_LEFT)
		  attrmask = HardwareScreen.attributes[ATTR_STATUS];
		 else
		  attrmask = 0x07;
		if (length < width)
		  HardwareScreen.scroll (length,
		  			 HardwareScreen.height - 1,
		  		         HardwareScreen.width - 1,
					 HardwareScreen.height - 1,
					 0,attrmask);
      		break;
        case STATUS_RIGHT:
        case STATUS_RIGHT_SQUASH:
      		if (length <= width)
		  x = width - length;
		 else
      		  x = 0;
		if (UWConfig.StatusPosn == STATUS_RIGHT)
		  attrmask = HardwareScreen.attributes[ATTR_STATUS];
		 else
		  attrmask = 0x07;
		if (length < width)
		  HardwareScreen.scroll (0,HardwareScreen.height - 1,
		  		         width - length - 1,
					 HardwareScreen.height - 1,
					 0,attrmask);
      		break;
        case STATUS_CENTRE:
        case STATUS_CENTRE_SQUASH:
      		if (length <= width)
		  x = (width - length) / 2;
		 else
      		  x = 0;
		if (UWConfig.StatusPosn == STATUS_CENTRE)
		  attrmask = HardwareScreen.attributes[ATTR_STATUS];
		 else
		  attrmask = 0x07;
		HardwareScreen.scroll (0,HardwareScreen.height - 1,
		  		       HardwareScreen.width - 1,
				       HardwareScreen.height - 1,
				       0,attrmask);
      		break;
      } // switch //
  if (x < 0)
    x = 0;
  if (str != 0)
    {
      attrmask = HardwareScreen.attributes[ATTR_STATUS] << 8;
      while (x < width && *str)
        {
          if (*str == '\001')
            attrmask = HardwareScreen.attributes[ATTR_HIGH_STATUS] << 8;
           else if (*str == '\002')
            attrmask = HardwareScreen.attributes[ATTR_STATUS] << 8;
           else
            HardwareScreen.draw (x++,height,attrmask | (*str & 255));
          ++str;
        }
    }
   else
    {
      attrmask = 0x0700;	// Clear status line (so it's not displayed)
      x = 0;
    }
  if (str == 0 || UWConfig.StatusPosn < STATUS_LEFT_SQUASH)
    {
      attrmask |= ' ';
      while (x < width)
        HardwareScreen.draw (x++,height,attrmask);
    }
#endif
} // UWDisplay::status //

#ifdef	UWPC_WINDOWS

// Process the messages for a display window.
// This is a catch-all after the master routine
// processes the messages.
LONG	UWDisplay::mesgs (WORD message,WORD wParam,LONG lParam)
{
  switch (message)
    {
      case WM_PAINT:
      		break;
      case WM_CREATE:
      		break;
      case WM_DESTROY:
      		break;
      default:	return (DefWindowProc (hWnd,message,wParam,lParam));
    }
  return (NULL);
} // UWDisplay::mesgs //

#endif
