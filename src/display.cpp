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
//    1.2    08/06/91  RW  Add support for cut and paste.
//    1.3    28/10/91  RW  Clean up some of the Win3 support.
//
//-------------------------------------------------------------------------

#pragma	inline			// This file contains some inline assembly.

#include "display.h"		// Declarations for this module.
#include "screen.h"		// Screen display handling routines.
#include "config.h"		// Status positions defined here.
#ifdef	UWPC_WINDOWS
#include "win3.h"		// IDM_* declarations are here.
#endif
#include <mem.h>		// Memory accessing routines.
#include <string.h>		// String handling routines.

#pragma	warn	-par

#ifdef	UWPC_DOS
#define	CURSOR_OFF()
#define	CURSOR_ON()
#else
#define	CURSOR_OFF()	cursoroff()
#define	CURSOR_ON()	cursoron()
extern	HANDLE	hInstance;
extern	int	UWFontHeight,UWFontWidth;
extern	int	UWFirstScan,UWCursHeight;
extern	HFONT	hFont;
#endif

void	UWDisplay::show (int X,int Y,unsigned pair)
{
  int offset;
  offset = X + (Y * width);
  screen[offset] = pair;
#ifdef	UWPC_DOS
  if (attop)
    HardwareScreen.draw (X,Y,pair);
#else
  // Check to see if the output is suspended first.
  if (suspended)
    {
      if (chbufferx < 0)
        {
	  // First character into the suspension buffer //
	  chbufferx = X;
	  chbuffery = Y;
	  chbufferlen = 1;
	}
       else if ((chbufferx + chbufferlen) == X && chbuffery == Y)
        {
	  // Add a new character to the suspension buffer //
	  chbufferlen++;
	}
       else
        {
	  // Flush the suspension buffer and add a new character //
	  drawline ();
	  chbufferx = X;
	  chbuffery = Y;
	  chbufferlen = 1;
	}
      return;		// Suspended character processed: exit.
    }

  // Draw the single character in the window.
  HDC hDC;
  unsigned char attr;
  extern COLORREF WindowsAttributes[];

  // Initialise the device context to perform drawing within //
  hDC = GetDC (hWnd);
  SetBkMode (hDC,OPAQUE);	// Fill in the background colour always.
  SelectObject (hDC,hFont);	// Get the font to use for characters.

  // Set the attribute colours to be used for the character draw //
  attr = pair >> 8;
#ifdef	NOT_CARET
  if (curson && y == Y && x == X) // Test for the cursor position.
    attr = (attr << 4) | ((attr >> 4) & 0x0F);
#endif
  SetBkColor (hDC,WindowsAttributes[(attr >> 4) & 7]);
  SetTextColor (hDC,WindowsAttributes[attr & 7]);

  // Draw the character that is to be shown //
  TextOut (hDC,X * UWFontWidth,Y * UWFontHeight,(LPSTR)(&pair),1);

  // Clean up the device context information //
  ReleaseDC (hWnd,hDC);
#endif
} // UWDisplay::show //

void	UWDisplay::scroll (int x1,int y1,int x2,int y2,int lines,int attribute)
{
  int ofs,saveofs,nextofs,x,y,linewid,diff;
  unsigned clearpair;
#ifdef	__TURBOC__
  unsigned far *scrn;
#endif

  // Abort if the region is "too small" //
  if (x2 < x1 || y2 < y1)
    return;

#ifdef	UWPC_WINDOWS
  if (suspended && chbufferx >= 0)
    {
      // Update the suspension buffer region before the scroll //
      drawline ();
      chbufferx = -1;
    }
#endif

  // Clear or scroll the memory for the screen image //
  clearpair = ' ' | (attribute << 8);
  if (lines == 0)
    {
      // Clear the entire area to be scrolled //
      ofs = x1 + (y1 * width);
#ifdef	__TURBOC__
      linewid = x2 - x1 + 1;
      scrn = screen;
#endif
      for (y = y1;y <= y2;++y)
        {
#ifndef	__TURBOC__
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
	    screen[ofs++] = clearpair;
	  ofs = saveofs + width;
#else
	  asm les di,scrn;		// Clear the line using assembly
	  asm mov ax,ofs;		// code to get the fastest clear
	  asm shl ax,1;			// possible to defeat slowdowns.
	  asm add di,ax;		// Only do in TC/BC so I can port
	  asm mov cx,linewid;		// this to Unix one of these days.
	  asm mov ax,clearpair;
	  asm cld;
	  asm rep stosw;
	  ofs += width;
#endif
	}
    }
   else if (lines > 0)
    {
#ifdef	DOOBERY
      // Scroll the area up a number of lines //
      ofs = x1 + (y1 * width);
      diff = width * lines;
      nextofs = ofs + diff;
      for (y = y1;y <= (y2 - lines);++y)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
	    screen[ofs++] = screen[nextofs++];
	  ofs = saveofs + width;
	  nextofs = ofs + diff;
	}
      while (y <= y2)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
            screen[ofs++] = clearpair;
	  ofs = saveofs + width;
	  ++y;
	}
    }
   else
    {
      // Scroll the area down a number of lines //
      ofs = x1 + (y2 * width);
      diff = width * lines;		// Note: lines < 0
      nextofs = ofs + diff;
      for (y = y2;y >= (y1 - lines);--y)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
	    screen[ofs++] = screen[nextofs++];
	  ofs = saveofs - width;
	  nextofs = ofs + diff;
	}
      while (y >= y1)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
            screen[ofs++] = clearpair;
	  ofs = saveofs - width;
	  --y;
	}
    }
#endif	DOOBERY
// #ifdef	DOOBERY		// Comment this code out - it doesn't work :-(
      // Scroll the area up a number of lines //
      ofs = x1 + (y1 * width);
      nextofs = ofs + width * lines;
      linewid = (x2 - x1 + 1) << 1;
      for (y = y1;y <= (y2 - lines);++y)
        {
	  memcpy (screen + ofs,screen + nextofs,linewid);
	  ofs += width;
	  nextofs += width;
	}
      while (y <= y2)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
            screen[ofs++] = clearpair;
	  ofs = saveofs + width;
	  ++y;
	}
    }
   else
    {
      // Scroll the area down a number of lines //
      ofs = x1 + (y2 * width);
      nextofs = ofs + width * lines;	// Note: lines < 0
      linewid = (x2 - x1 + 1) << 1;
      for (y = y2;y >= (y1 - lines);--y)
        {
	  memcpy (screen + ofs,screen + nextofs,linewid);
	  ofs -= width;
	  nextofs -= width;
	}
      while (y >= y1)
        {
	  saveofs = ofs;
          for (x = x1;x <= x2;++x)
            screen[ofs++] = clearpair;
	  ofs = saveofs - width;
	  --y;
	}
    }
// #endif	DOOBERY
#ifdef	UWPC_DOS
  if (attop)
    {
      if (HardwareScreen.dialogenabled)
        top (attop);		// Redraw window when there is a dialog box.
       else
        HardwareScreen.scroll (x1,y1,x2,y2,lines,attribute);
    }
#else
  // Perform a repaint on the area we just scrolled //
  if (!lines)
    repaint (x1,y1,x2,y2);	// Repaint (clear) the scrolling region.
   else
    {
      // Use the Windows 3.0 scrolling primitive (because it is faster) //
      RECT scrollrect;
      if (lines > 0)
        {
          scrollrect.left = x1 * UWFontWidth;
          scrollrect.top = (y1 + lines) * UWFontHeight;
          scrollrect.right = (x2 + 1) * UWFontWidth;
          scrollrect.bottom = (y2 + 1) * UWFontHeight;
        }
       else
        {
	  // Note: lines < 0.
          scrollrect.left = x1 * UWFontWidth;
          scrollrect.top = y1 * UWFontHeight;
          scrollrect.right = (x2 + 1) * UWFontWidth;
          scrollrect.bottom = (y2 + 1 + lines) * UWFontHeight;
	}
      ScrollWindow (hWnd,0,(-lines) * UWFontHeight,&scrollrect,NULL);
      UpdateWindow (hWnd);
    }
#endif
} // UWDisplay::scroll //

void	UWDisplay::setcurs (void)
{
  int dox = x;
  if (dox >= width)
    dox = width - 1;
#ifdef	UWPC_DOS
  if (attop && !HardwareScreen.dialogenabled)
    HardwareScreen.cursor (dox,y);
#else
#ifndef	NOT_CARET
  if (careton)
    SetCaretPos (dox * UWFontWidth,y * UWFontHeight + UWFirstScan);
#endif
#endif
} // UWDisplay::setcurs //

UWDisplay::UWDisplay (int number)
{
  if (number)		// Get a default window title first.
    {
      strcpy (title,"N - UW/PC");
      title[0] = number + '0';
    } /* then */
   else
    strcpy (title,"Unix Windows");
  wNumber = number;	// Record the window number for later.
#ifdef	UWPC_DOS
  width = HardwareScreen.width;
  height = HardwareScreen.height - 1;
#else
  RECT rect;
  int wid,ht,changed;
  width = 80;
  height = 24;
  suspended = 0;	// Don't suspend the output yet.
  hWnd = NULL;		// Mark the Windows 3.0 window as not present.
#endif
  // screen = new unsigned [width * height];
  screen = (unsigned far *)UWAlloc (width * height * sizeof (unsigned));
  attop = 0;
#ifdef	UWPC_DOS
  attr = HardwareScreen.attributes[ATTR_NORMAL];
#else
  attr = UWConfig.NewAttrs[ATTR_NORMAL];
#endif
  scrollattr = attr;
  x = 0;
  y = 0;
  deftabs ();
  wrap52 = 0;
  if (screen == 0)
    width = 0;
   else
    {
#ifdef	UWPC_WINDOWS
      // Create the Windows 3.0 window associated with this display object //
      curson = 0;
      mousebuttons = 0;
      hWnd = CreateWindow ((number == 0 ? "UWPCMainClass" : "UWPCSessClass"),
      						// Name of window class.
      			   title,		// Title on the window.
			   			// Window name.
			   WS_OVERLAPPEDWINDOW,	// Style of window to use.
			   CW_USEDEFAULT,	// Default initial position.
			   CW_USEDEFAULT,
			   CW_USEDEFAULT,	// Size of the window.
			   CW_USEDEFAULT,
			   NULL,		// Parent (none)
			   NULL,		// Menu handle - use default.
			   hInstance,		// Program instance.
			   (LPSTR)this);	// Extra information.
      if (!hWnd)
        {
	  width = 0;			// Creation failed.
	  return;
	}
#endif
      clear ();
#ifdef	UWPC_WINDOWS
      // Adjust the "Functions" menu to contain the function keys //
      int func;
      HMENU hMenu;
      char funckey[29];
      hMenu = GetMenu (hWnd);
      funckey[0] = 'F';		// Add skeleton "F&n \t" at start.
      funckey[1] = '&';
      funckey[3] = ' ';
      funckey[4] = '\t';
      funckey[25] = '.';	// Add "..." to end of string
      funckey[26] = '.';	// for long function key definitions.
      funckey[27] = '.';
      funckey[28] = '\0';
      for (func = IDM_F1;func <= IDM_F10;++func)
        {
	  if (func >= IDM_F10)
	    strcpy (funckey,"F1&0\t");
	   else
	    funckey[2] = '1' + func - IDM_F1;
	  if (UWConfig.FKeys[func - IDM_F1][0] == '\0')
	    strcpy (funckey + 5,"<nothing>");
	   else
	    strncpy (funckey + 5,UWConfig.FKeys[func - IDM_F1],20);
	  ModifyMenu (hMenu,func,MF_BYCOMMAND | MF_STRING,func,funckey);
	} /* for */

      // Resize the window to the required width and height.
      GetWindowRect (hWnd,&rect);
      wid = UWFontWidth * width + GetSystemMetrics (SM_CXFRAME) * 2;
      ht  = UWFontHeight * height + GetSystemMetrics (SM_CYFRAME) * 2 +
				    GetSystemMetrics (SM_CYCAPTION) +
				    GetSystemMetrics (SM_CYMENU) + 2;
      changed = 0;
      if ((rect.right - rect.left + 1) > wid)
        {
	  rect.right = wid + rect.left - 1;
	  changed = 1;
	}
      if ((rect.bottom - rect.top + 1) > ht)
        {
	  rect.bottom = ht + rect.top - 1;
	  changed = 1;
	}
      if (changed)
        MoveWindow (hWnd,rect.left,rect.top,rect.right - rect.left + 1,
		    rect.bottom - rect.top + 1,0);
      curson = 1;
      ShowWindow (hWnd,SW_SHOW);
      UpdateWindow (hWnd);
#endif
    }
  toprgn = 0;		// Set the default scrolling region.
  botmrgn = height - 1;
} // UWDisplay::UWDisplay //

UWDisplay::~UWDisplay (void)
{
  if (screen)
    UWFree (screen);
    // delete screen;
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
        HardwareScreen.shape ((CursorShapes)UWConfig.CursorSize);
#else
      attop = 1;
      if (IsIconic (hWnd))
        ShowWindow (hWnd,SW_RESTORE);	// De-iconify the window.
       else
        BringWindowToTop (hWnd);	// Bring to the top.
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
	  x = width;
	  // wrap52 = 1;
	  // --x;
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
  if (y > botmrgn)
    {
      // Scroll the display up one line //
      --y;
      scroll (0,toprgn,width - 1,botmrgn,1,scrollattr);
    }
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::lf //

// Perform a reverse line feed on the display.  When
// the cursor is on the first display line, the display
// will be scrolled one line down in the scrolling colour.
void	UWDisplay::revlf (void)
{
  CURSOR_OFF ();
  --y;
  if (y < toprgn)
    {
      // Scroll the display down one line //
      y = 0;
      scroll (0,toprgn,width - 1,botmrgn,-1,scrollattr);
    }
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::revlf //

// Set a scrolling region on the display.
void	UWDisplay::region (int top,int botm)
{
  if (top < 0)
    top = 0;
  if (botm >= height)
    botm = height - 1;
  if (botm < top)
    botm = top;
  toprgn = top;
  botmrgn = botm;
} // UWDisplay::region //

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
// If "nd" is non-zero then the tab is non-destructive.
void	UWDisplay::tab (int vt52wrap,int tabsize,int nd)
{
  do
    {
      if (!nd)
        send (' ',vt52wrap);
       else
        {
	  // Advance to the next screen position non-destructively //
	  if (wrap52)
	    {
	      cr ();
	      lf ();
	    }
	  CURSOR_OFF ();
	  ++x;
	  if (x >= width)
	    {
	      if (vt52wrap)
	        {
		  wrap52 = 1;
		  // --x;
		}
	       else
		{
		  cr ();
		  lf ();
		}
	    }
	  setcurs ();
	  CURSOR_ON ();
        } /* if */
    }
  while (x > 0 && x < width && !(tabs[x / 8] & (1 << (x % 8))));
  // while ((x % tabsize) != 0);
} // UWDisplay::tab //

// Ring the terminal bell - this directly calls
// the hardware routines to ring it.
void	UWDisplay::bell (void)
{
#ifdef	UWPC_DOS
  HardwareScreen.bell ();	// Call the BIOS for the DOS bell.
#else
  if (UWConfig.BeepEnable)
    MessageBeep (0);		// Use this as the Windows 3.0 bell for now.
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

// Move the cursor to a new position relative to
// its current position.
void	UWDisplay::moverel (int relx,int rely)
{
  CURSOR_OFF ();
  x += relx;
  y += rely;
  if (x < 0) x = 0;
  if (x >= width) x = width - 1;
  if (y < 0) y = 0;
  if (y >= height) y = height - 1;
  wrap52 = 0;
  setcurs ();
  CURSOR_ON ();
} // UWDisplay::moverel //

// Clear the display according to a particular clearing
// type.  This function does not move the cursor position.
void	UWDisplay::clear	(int clrtype)
{
  CURSOR_OFF ();
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
      		  scroll (0,y,x,y,0,scrollattr);
		break;
      default:	break;
    }
  wrap52 = 0;
  CURSOR_ON ();
} // UWDisplay::clear //

// Insert a new line on the display.
void	UWDisplay::insline	(void)
{
  CURSOR_OFF ();
  scroll (0,y,width - 1,height - 1,-1,scrollattr);
  wrap52 = 0;
  CURSOR_ON ();
} // UWDisplay::insline //

// Delete the current line from the display.
void	UWDisplay::delline	(void)
{
  CURSOR_OFF ();
  scroll (0,y,width - 1,height - 1,1,scrollattr);
  wrap52 = 0;
  CURSOR_ON ();
} // UWDisplay::delline //

// Insert a new character into the current line.
// If 'ch' is -1, then insert a blank and don't move cursor.
void	UWDisplay::inschar	(int ch)
{
  CURSOR_OFF ();
  int offset,xval;
#ifdef	UWPC_DOS
  int start;
#endif
  unsigned pair,temp;
  offset = x + (y * width);
#ifdef	UWPC_DOS
  start = offset;
#endif
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
      pair = temp;
      ++offset;
      ++xval;
    }
#ifdef	UWPC_WINDOWS
  if (x < (width - 1))
    repaint (x,y,width - 2,y);
#else
  if (attop)		// Redraw the screen line.
    HardwareScreen.line (x,y,screen + start,width - x + 1);
#endif
  if (ch != -1)
    send (ch);
  CURSOR_ON ();
} // UWDisplay::inschar //

// Delete the current character, and append the given
// character to the current line.
void	UWDisplay::delchar	(int ch)
{
  CURSOR_OFF ();
  int offset,xval;
#ifdef	UWPC_DOS
  int start;
#endif
  offset = x + (y * width);
#ifdef	UWPC_DOS
  start = offset;
#endif
  xval = x;
  while (xval < (width - 1))
    {
      // Move the characters back one on the line //
      screen[offset] = screen[offset + 1];
      ++offset;
      ++xval;
    }
  // Place the given character at the line's end //
  screen[offset] = (ch & 255) | (scrollattr << 8);
#ifdef	UWPC_DOS
  if (attop)		// Redraw the screen line after the deletion.
    HardwareScreen.line (x,y,screen + start,width - x + 1);
#else
  repaint (x,y,width - 1,y);
#endif
  wrap52 = 0;
  CURSOR_ON ();
} // UWDisplay::delchar //

// Scroll the screen up or down one line.
void	UWDisplay::scrollscreen (int up)
{
  CURSOR_OFF ();
  if (up)
    scroll (0,0,width - 1,height - 1,1,scrollattr);
   else
    scroll (0,0,width - 1,height - 1,-1,scrollattr);
  CURSOR_ON ();
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

// Mark a rectangle on the screen for cut-and-paste.
// Two calls to this routine will remove the mark.
void	UWDisplay::markcut (int x1,int y1,int x2,int y2)
{
  int temp,x,y,offset,nextline,length,start;
  unsigned pair;
  if (x1 > x2)
    {
      temp = x1;
      x1 = x2;
      x2 = temp;
    }
  if (y1 > y2)
    {
      temp = y1;
      y1 = y2;
      y2 = temp;
    }
  offset = x1 + y1 * width;
  start = offset;
  length = x2 - x1 + 1;
  nextline = width - length;
  for (y = y1;y <= y2;++y)
    {
      for (x = x1;x <= x2;++x)
        {
	  // Flip the attribute colours in the screen pair //
	  pair = screen[offset];
	  screen[offset++] = (pair & 0x88FF) | ((pair & 0x0700) << 4) |
	  	    		((pair & 0x7000) >> 4);
	}
#ifdef	UWPC_DOS
      HardwareScreen.line (x1,y,screen + start,length);
#else
      repaint (x1,y,x2,y);
#endif
      offset += nextline;
      start += width;
    }
} // UWDisplay::markcut //

// Copy screen data into a clipboard buffer.  The
// length of the written data is returned.
int	UWDisplay::copycut (int x1,int y1,int x2,int y2,char far *buffer)
{
  int temp,x,y,offset,nextline,length;
  if (x1 > x2)
    {
      temp = x1;
      x1 = x2;
      x2 = temp;
    }
  if (y1 > y2)
    {
      temp = y1;
      y1 = y2;
      y2 = temp;
    }
  offset = x1 + y1 * width;
  nextline = width - (x2 - x1 + 1);
  length = 0;
  for (y = y1;y <= y2;++y)
    {
      for (x = x1;x <= x2;++x)
        {
	  *buffer++ = screen[offset++] & 255;
	  ++length;
	}
      while (length > 0 && *(buffer - 1) == ' ')
        {
	  // Move back to eliminate trailing spaces on the line //
	  --length;
	  --buffer;
	}
      if (y != y2)
        {
	  *buffer++ = '\r';	// Paste ^M at end of each line except last.
	  ++length;
	}
      offset += nextline;
    }
  return (length);		// Return the length of the paste buffer.
} // UWDisplay::copycut //

// Set a tab stop at the current X position.
void	UWDisplay::settab (int posn)
{
  if (posn < 0)
    posn = x;
  if (posn < (TAB_SET_SIZE * 8))
    tabs[posn / 8] |= (1 << (posn % 8));
} // UWDisplay::settab //

// Reset a tab stop at the current Y position.
void	UWDisplay::restab (void)
{
  if (x < (TAB_SET_SIZE * 8))
    tabs[x / 8] &= ~(1 << (x % 8));
} // UWDisplay::restab //

// Clear all tab stops.
void	UWDisplay::clrtabs (void)
{
  int temp;
  for (temp = 0;temp < TAB_SET_SIZE;++temp)
    tabs[temp] = 0;
} // UWDisplay::clrtabs //

// Set the default tab stops every "tabsize" positions.
void	UWDisplay::deftabs (int tabsize)
{
  int temp;
  clrtabs ();
  for (temp = 0;temp < width;temp += tabsize)
    settab (temp);
} // UWDisplay::deftabs //

// Clear the current window title.
void	UWDisplay::clrtitle (void)
{
  if (wNumber)
    {
      // Keep the window number visible so the user doesn't get lost. :-)
      title[0] = wNumber + '0';
      title[1] = ' ';
      title[2] = '-';
      title[3] = ' ';
      title[4] = '\0';
    }
   else
    title[0] = '\0';
} // UWDisplay::clrtitle //

// Add another character to the end of the current window title.
void	UWDisplay::addtitle (int ch)
{
  int len = strlen (title);
  if (len < (WTITLE_LEN - 1))
    {
      title[len++] = ch;
      title[len] = '\0';
    }
} // UWDisplay::addtitle //

// Set the displayed title for the window.  This is mainly
// for the benefit of the Windows 3.0 version.
void	UWDisplay::showtitle (void)
{
#ifdef	UWPC_WINDOWS
  SetWindowText (hWnd,title);	// Set the Windows 3.0 title.
#endif
} // UWDisplay::showtitle //

// Fill the entire window with a character to perform an
// alignment test.
void	UWDisplay::aligntest (int ch)
{
#ifdef	UWPC_WINDOWS
  if (suspended && chbufferx >= 0)
    {
      // Update the suspension buffer region before the test //
      drawline ();
      chbufferx = -1;
    }
#endif
  unsigned pair = ch | (attr << 8);
  unsigned far *scrn = screen;
  int size = width * height;
  while (size-- > 0)
    *scrn++ = pair;
#ifdef	UWPC_DOS
  if (attop)
    top (attop);
#else
  repaint (0,0,width - 1,height - 1);
#endif
} // UWDisplay::aligntest //

#ifdef	UWPC_WINDOWS

// Suspend output in the window for a little while.
void	UWDisplay::suspend (int stop)
{
  if (stop)
    {
      // Suspend writing to the Windows 3.0 window for now //
      CURSOR_OFF ();
      suspended = 1;
      chbufferx = -1;
    }
   else
    {
      // Flush the buffer and start writing again //
      if (chbufferx >= 0)
        {
          drawline ();
	}
      suspended = 0;
      CURSOR_ON ();
    }
} // UWDisplay::suspend //

#endif
