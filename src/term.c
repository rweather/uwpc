/*-------------------------------------------------------------------------

  TERM.C - Terminal handling routines for the Unix Windows protocol.

    This file is part of UW/PC - a multi-window comms package for the PC.
    Copyright (C) 1990-1991  Rhys Weatherley

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

  Revision History:
  ================

   Version  DD/MM/YY  By  Description
   -------  --------  --  --------------------------------------
     1.0    15/12/90  RW  Original Version of TERM.C
     1.1    01/01/91  RW  Clean up and remove __PROTO__
     1.2    17/03/91  RW  Fix up COM device after a DOS shell-out.

-------------------------------------------------------------------------*/

#include "term.h"		/* Declarations for this module */
#include "uw.h"			/* Unix Windows declarations */
#include "device.h"		/* Device handling routines */
#include "keys.h"		/* Keyboard handling routines */
#include <conio.h>		/* Turbo C screen handling routines */
#include <stdlib.h>		/* "system" is defined here */
#include <dos.h>		/* Low-level hardware/DOS routines */
#include <string.h>		/* String handling routines */
#include <mem.h>		/* Memory handling routines */
#include <stddef.h>		/* NULL is defined here */

/* Define the configurable terminal types to use for windows */
uwtype_t _Cdecl	DefTermType=UWT_ADM31;
uwtype_t _Cdecl	Def0TermType=UWT_ADM31;

/* The following variable is non-zero to allow the terminal beep sound */
int	_Cdecl	AllowBeep=1;

/* Define the structure of a screen image */
#define	SCREEN_SIZE	1920
typedef	struct	{
		  uwtype_t emul;	/* Window emulation type */
		  int x,y;		/* Current cursor position */
		  int escmode;		/* Escape mode for screen */
		  int xvalue,yvalue;	/* For cursor moves */
		  int savex,savey;	/* Saved X and Y values */
		  unsigned attr;	/* Attribute mask to use */
		  int insert;		/* Non-zero for insert mode */
		  unsigned image[SCREEN_SIZE];	/* Screen Image */
		} ScreenImage;

/* Define the data for screen information */
static	ScreenImage	_Cdecl	Screens[NUM_UW_WINDOWS];
static	int		_Cdecl	CurrScreen;
static	unsigned far   *_Cdecl	ScreenAddress;
static	unsigned	_Cdecl	ScreenSegment;
static	int		_Cdecl	snowchk,mdamode,scrnmode;
static	unsigned	_Cdecl	SavedScreen[SCREEN_SIZE + 80];

/* Define the default screen attributes */
static	int	_Cdecl	NormalAttr,StatusAttr,WindNumAttr,BoxAttr,InverseAttr;

/* Define the data for the help screen */
static	char	*_Cdecl	HelpData[] =
	 {"ALT-B - Send a BREAK pulse",
	  "ALT-D - Send the dialing string",
	  "ALT-E - Exit a UW session",
	  "ALT-H - Hangup the modem",
	  "ALT-I - Send modem init string",
	  "ALT-J - Jump to DOS",
	  "ALT-K - Kill current window",
	  "ALT-N - Create a new window",
	  "ALT-Q - Quit the program",
	  "ALT-R - Download (receive) a file",
	  "ALT-S - Upload (send) a file",
	  "ALT-U - Send \"uw^M\" to host",
	  "ALT-X - Exit the program",
	  "ALT-Z - This help information",
	  "ALT-n - Go to window \"n\" (1-7)",
	  " [Press any key to continue]"
	 };
#define HELP_LINES	16
#define HELP_COLUMNS	33

/* Define strings for the terminal types (padded to 5 chars) */
static	char	*_Cdecl	TermTypes[] = {"ADM31","VT52 ","ANSI ",
				       " TEK "," FTP ","PRINT"};

/* Display the status line on the screen */
static	void	_Cdecl	StatusLine (void)
{
  int savex,savey,wind;
  savex = wherex ();
  savey = wherey ();
  window (1,25,80,25);
  gotoxy (1,1);
  textattr (StatusAttr);
  cprintf (" %s \263 %s \263 %s \263 %s \263 Windows:",
  	   (UWProcList[UWCurrWindow].transfer == NULL ?
	       "ALT-Z for help" :
	       "ESC to abort  "),
	   TermTypes[Screens[CurrScreen].emul],
	   DeviceParameters,(UWProtocol > 0 ? "UW" : "  "));
  for (wind = 1;wind < NUM_UW_WINDOWS;++wind)
    {
      if (UWProcList[wind].used)
	{
          putch (' ');
          if (wind == CurrScreen)
	    textattr (WindNumAttr);	/* Change attributes for current */
	  putch (wind + '0');		/* Display the active windows */
	  textattr (StatusAttr);
	} /* then */
    } /* for */
  clreol ();
  window (1,1,80,24);			/* Restore the screen */
  gotoxy (savex,savey);
  textattr (NormalAttr);
} /* StatusLine */

/* Display a single character at an offset on the hardware screen */
static	void	_Cdecl	DisplayChar (int offset,unsigned ch)
{
  if (!snowchk)
    ScreenAddress[offset] = ch;
   else
    {
      _ES = ScreenSegment;
      _DI = offset;
      _BX = ch;
      /* Assembly code here is:

		shl  di,1
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
      __emit__ (0xD1,0xE7,0xBA,0xDA,0x03,0xEC,0xA8,0x01,
		0x75,0xFB,0xFA,0xEC,0xA8,0x01,
		0x74,0xFB,0x26,0x89,0x1D,0xFB);
    } /* else */
} /* DisplayChar */

/* Initialise a terminal window for use by UW */
/* If 'window' == 0, also setup this module.  */
void	_Cdecl	TermInit (int wind,uwtype_t emul)
{
  int index;
  unsigned value;
  if (!wind)
    {
      struct text_info ti;
      int mode;
      gettextinfo (&ti);
      if (ti.currmode == 7)
	{
	  mode = 7;
	  NormalAttr = 0x07;
	  StatusAttr = 0x70;
	  WindNumAttr = 0x07;
	  BoxAttr = 0x0F;
	  InverseAttr = 0x70;
	  snowchk = 0;
	  mdamode = 1;
	  ScreenAddress = (unsigned far *)0xB0000000L;
	  ScreenSegment = 0xB000;
	} /* then */
       else
	{
	  if (ti.currmode == 0 || ti.currmode == 2)
	    {
	      mode = 2;
	      NormalAttr = 0x07;
	      StatusAttr = 0x70;
	      WindNumAttr = 0x07;
	      BoxAttr = 0x0F;
	      InverseAttr = 0x70;
	    } /* then */
	   else
	    {
	      mode = 3;
	      NormalAttr = 0x1F;
	      StatusAttr = 0x4E;
	      WindNumAttr = 0x41;
	      BoxAttr = 0x1E;
	      InverseAttr = 0x71;
	    } /* else */
	  snowchk = 1;
	  mdamode = 0;
	  ScreenAddress = (unsigned far *)0xB8000000L;
	  ScreenSegment = 0xB800;
	} /* else */
      textmode (mode);
      scrnmode = mode;
      textattr (NormalAttr);
      clrscr ();
      window (1,25,80,25);
      textattr (StatusAttr);
      clrscr ();
      window (1,1,24,80);
      CurrScreen = 0;
    }
  /* Clear and setup the window for use by this module */
  value = (NormalAttr << 8) + 0x20;
  for (index = 0;index < SCREEN_SIZE;++index)
    Screens[wind].image[index] = value;
  Screens[wind].emul = emul;
  Screens[wind].x = 1;
  Screens[wind].y = 1;
  Screens[wind].escmode = 0;
  Screens[wind].savex = 1;
  Screens[wind].savey = 1;
  Screens[wind].attr = NormalAttr << 8;
  Screens[wind].insert = 0;
  if (wind == CurrScreen)
    {
      /* Clear the current screen */
      clrscr ();
      gotoxy (1,1);
    }
  StatusLine ();
} /* TermInit */

/* Restore the cursor position on a window if necessary */
static	void	_Cdecl	RestoreCursor (int window)
{
  if (window == CurrScreen)
    {
      int x;
      x = Screens[window].x;	/* Allow for the VT52's weird wrap strategy */
      if (x > 80)
	x = 80;
      gotoxy (Screens[window].x,Screens[window].y);
    } /* if */
} /* RestoreCursor */

/* Scroll a window down one line */
static	void	_Cdecl	ScrollDown (int window)
{
  unsigned *line,value;
  int count;
  if (window == CurrScreen)
    {
      union REGS regs;
      regs.x.ax = 0x701;	/* Prepare to scroll the screen down */
      regs.h.bh = NormalAttr;	/* Clear the line to the normal attribute */
      regs.x.cx = 0;
      regs.x.dx = 0x174F;	/* Only scroll the first 24 lines */
      int86 (0x10,&regs,&regs);	/* Call the BIOS to scroll the screen */
    } /* if */
  line = Screens[window].image;	/* Move the saved window image */
  memmove (line + 80,line,(SCREEN_SIZE - 80) * 2);
  value = (NormalAttr << 8) | 0x20;
  for (count = 0;count < 80;++count)
    *line++ = value;		/* Clear the first line in the window */
} /* ScrollDown */

/* Insert a new line into a window */
static	void	_Cdecl	InsertLine (int window)
{
  unsigned *line,value;
  int count,y;
  if (window == CurrScreen)
    {
      union REGS regs;
      regs.x.ax = 0x701;	/* Prepare to scroll the screen down */
      regs.h.bh = NormalAttr;	/* Clear the line to the normal attribute */
      regs.x.cx = (Screens[window].y - 1) << 8;
      regs.x.dx = 0x174F;	/* Only scroll to the 24th line */
      int86 (0x10,&regs,&regs);	/* Call the BIOS to scroll the screen */
    } /* if */
  y = Screens[window].y;
  line = Screens[window].image + (y - 1) * 80;
  if (y < 24)			/* Move the rest of the screen down */
    memmove (line + 80,line,(24 - y) * 160);
  value = (NormalAttr << 8) | 0x20;
  for (count = 0;count < 80;++count)
    *line++ = value;		/* Clear the first line in the window */
} /* InsertLine */

/* Scroll a window up one line */
static	void	_Cdecl	ScrollUp (int window)
{
  unsigned *line,value;
  int count;
  if (window == CurrScreen)
    {
      union REGS regs;
      regs.x.ax = 0x601;	/* Prepare to scroll the screen up */
      regs.h.bh = NormalAttr;	/* Clear the line to the normal attribute */
      regs.x.cx = 0;
      regs.x.dx = 0x174F;	/* Only scroll the first 24 lines */
      int86 (0x10,&regs,&regs);	/* Call the BIOS to scroll the screen */
    } /* if */
  line = Screens[window].image;	/* Move the saved window image */
  memmove (line,line + 80,(SCREEN_SIZE - 80) * 2);
  line += SCREEN_SIZE - 80;
  value = (NormalAttr << 8) | 0x20;
  for (count = 0;count < 80;++count)
    *line++ = value;		/* Clear the last line in the window */
} /* ScrollUp */

/* Delete the current line from the screen */
static	void	_Cdecl	DeleteLine (int window)
{
  unsigned *line,value;
  int count,y;
  if (window == CurrScreen)
    {
      union REGS regs;
      regs.x.ax = 0x601;	/* Prepare to scroll the screen up */
      regs.h.bh = NormalAttr;	/* Clear the line to the normal attribute */
      regs.x.cx = (Screens[window].y - 1) << 8;
      regs.x.dx = 0x174F;	/* Only scroll to the 24th line */
      int86 (0x10,&regs,&regs);	/* Call the BIOS to scroll the screen */
    } /* if */
  y = Screens[window].y;
  line = Screens[window].image + (y - 1) * 80;
  if (y < 24)			/* Move the rest of the screen up */
    memmove (line,line + 80,(24 - y) * 160);
  value = (NormalAttr << 8) | 0x20;
  line = Screens[window].image + SCREEN_SIZE - 80;
  for (count = 0;count < 80;++count)
    *line++ = value;		/* Clear the last line in the window */
} /* DeleteLine */

/* Delete the current character from the screen */
static	void	_Cdecl	DeleteChar (int window)
{
  unsigned *line;
  int size,offset,x;
  x = Screens[window].x;
  size = 80 - x;		/* Get the size of the line to be moved */
  if (size < 0)			/* Off the screen side - ignore request */
    return;
  offset = (Screens[window].y - 1) * 80 + x - 1;
  line = Screens[window].image + offset;
  if (size > 0)			/* Move the line back one character */
    memmove (line,line + 1,size * 2);
  *(line + size) = (NormalAttr << 8) | 0x20; /* Show a space at the end */
  if (window == CurrScreen)
    {
      /* Now re-display the line on the visible screen */
      ++size;
      while (size--)
	DisplayChar (offset++,*line++);
    } /* if */
} /* DeleteChar */

/* Insert a character pair at the current cursor position */
static	void	_Cdecl	InsertChar (int window,unsigned pair)
{
  unsigned *line;
  int size,offset,x;
  x = Screens[window].x;
  size = 80 - x;		/* Get the size of the line to be moved */
  if (size < 0)			/* Off the screen side - ignore request */
    return;
  offset = (Screens[window].y - 1) * 80 + x - 1;
  line = Screens[window].image + offset;
  if (size > 0)			/* Move the line forward one character */
    memmove (line + 1,line,size * 2);
  *line = pair;			/* Display the inserted pair */
  if (window == CurrScreen)
    {
      /* Now re-display the line on the visible screen */
      ++size;
      while (size--)
	DisplayChar (offset++,*line++);
    } /* if */
} /* InsertChar */

/* Erase to the end of the current line in a window */
static	void	_Cdecl	EraseEndLine (int window)
{
  int offset,x,size;
  unsigned value,*line;
  x = Screens[window].x;
  size = 81 - x;			/* Get the line size to clear */
  if (size < 1)
    return;				/* Ignore request if off-screen */
  value = (NormalAttr << 8) | 0x20;	/* Clear with a normal space */
  offset = (Screens[window].y - 1) * 80 + x - 1;
  line = Screens[window].image + offset;
  if (window == CurrScreen)
    {
      /* Clear both the memory image and the screen */
      while (size--)
	{
	  *line++ = value;
	  DisplayChar (offset++,value);
	} /* while */
    } /* then */
   else
    {
      /* Just clear the memory image */
      while (size--)
	*line++ = value;
    } /* else */
} /* EraseEndLine */

/* Erase to the end of the screen in a window */
static	void	_Cdecl	EraseEndScreen (int window)
{
  unsigned *line,value;
  int yval,count,y;
  EraseEndLine (window);	/* Erase to the line's end first */
  y = Screens[window].y;
  if (y >= 24)
    return;			/* Abort if on last screen line */
  if (window == CurrScreen)
    {
      union REGS regs;
      regs.x.ax = 0x600;	/* Prepare to clear the screen */
      regs.h.bh = NormalAttr;	/* Clear the screen to the normal attr */
      regs.x.cx = y << 8;
      regs.x.dx = 0x174F;	/* Only clear to the 24th line */
      int86 (0x10,&regs,&regs);	/* Call the BIOS to clear the screen */
    } /* if */
  yval = y * 80;
  line = Screens[window].image + yval;
  value = (NormalAttr << 8) | 0x20;
  for (count = yval;count < SCREEN_SIZE;++count)
    *line++ = value;		/* Clear the window image in memory */
} /* EraseEndScreen */

/* Output a character to a VT52 terminal window */
static	void	_Cdecl	VT52TermOutput (int window,int ch)
{
  if (Screens[window].escmode)
    {
      if (Screens[window].escmode == 2)
	{
	  /* Set the value of Y in an "ESC Y" request */
	  Screens[window].yvalue = ch - ' ' + 1;
	  Screens[window].escmode++;
	} /* then */
       else if (Screens[window].escmode == 3)
	{
	  /* Set the value of X in an "ESC Y" request and move the cursor */
	  Screens[window].xvalue = ch - ' ' + 1;
	  Screens[window].x = Screens[window].xvalue;
	  Screens[window].y = Screens[window].yvalue;
	  if (window == CurrScreen)	/* Move the cursor as necessary */
	    gotoxy (Screens[window].xvalue,Screens[window].yvalue);
	  Screens[window].escmode = 0;	/* Back to normal character mode */
	} /* then */
       else
	{
	  /* Determine the function to perform in an escape sequence */
	  Screens[window].escmode = 0;	/* Reset the escape mode */
	  switch (ch)
	    {
	      case '7': /* Save the current screen position */
			Screens[window].savex = Screens[window].x;
			Screens[window].savey = Screens[window].y;
			break;
	      case '8': /* Restore the screen position */
			Screens[window].x = Screens[window].savex;
			Screens[window].y = Screens[window].savey;
			RestoreCursor (window);
			break;
	      case 'A':	/* Move the cursor up */
			if (Screens[window].y > 1)
			  {
			    Screens[window].y--;
			    RestoreCursor (window);
			  } /* if */
			break;
	      case 'B':	/* Move the cursor down */
			if (Screens[window].y < 24)
			  {
			    Screens[window].y++;
			    RestoreCursor (window);
			  } /* then */
			 else
			  ScrollUp (window);
			break;
	      case 'C': /* Move the cursor right */
			if (Screens[window].x < 80)
			  {
			    Screens[window].x++;
			    RestoreCursor (window);
			  } /* if */
			break;
	      case 'D': /* Move the cursor left */
			if (Screens[window].x > 1)
			  {
			    Screens[window].x--;
			    RestoreCursor (window);
			  } /* if */
			break;
	      case 'H': /* Home the cursor */
			Screens[window].x = 1;
			Screens[window].y = 1;
			RestoreCursor (window);
			break;
	      case 'I': /* Reverse line feed */
			ScrollDown (window);
			break;
	      case 'J': /* Erase to end of screen */
			EraseEndScreen (window);
			break;
	      case 'K': /* Erase to end of line */
			EraseEndLine (window);
			break;
	      case 'Y': /* Set the cursor position */
			Screens[window].escmode = 2;
			break;
	      default:	break;
	    } /* switch */
	} /* else */
    } /* then */
   else if (ch == 27)
    Screens[window].escmode = 1;
   else if (ch == 13)
    {
      Screens[window].x = 1;
      RestoreCursor (window);
    } /* then */
   else if (ch == 10)
    {
      if (Screens[window].y < 24)
	{
	  Screens[window].y++;
	  RestoreCursor (window);
	} /* then */
       else
	ScrollUp (window);
    } /* then */
   else if (ch == 7)
    {
      if (AllowBeep)
	putch (7);
    } /* then */
   else if (ch == 8)
    {
      if (Screens[window].x > 1)
	{
	  Screens[window].x--;
	  RestoreCursor (window);
	} /* if */
    } /* then */
   else if (ch == 9)
    {
      do
	{
	  /* Output spaces until the next TAB is reached */
	  VT52TermOutput (window,' ');
	}
      while ((Screens[window].x & 7) != 0);
    } /* then */
   else if (ch != 0)	/* Ignore NUL characters */
    {
      int offset,x;
      unsigned value;
      /* Wrap around to the next line if required */
      x = Screens[window].x;
      if (x > 80)
	{
	  /* Set x to 1 and output LF to move to next row */
	  Screens[window].x = 1;
	  VT52TermOutput (window,10);
	  x = 1;
	} /* if */
      offset = (Screens[window].y - 1) * 80 + x - 1;
      value = ch | (NormalAttr << 8);
      Screens[window].image[offset] = value;
      if (window == CurrScreen)
	DisplayChar (offset,value);
      Screens[window].x++;
      RestoreCursor (window);
    } /* if */
} /* VT52TermOutput */

/* Output a character to an ADM31 terminal window */
static	void	_Cdecl	ADM31TermOutput (int window,int ch)
{
  int offset,x;
  unsigned value;
  if (Screens[window].escmode)
    {
      if (Screens[window].escmode == 2)
	{
	  Screens[window].yvalue = ch - ' ' + 1;
	  Screens[window].escmode++;
	} /* then */
       else if (Screens[window].escmode == 3)
	{
	  Screens[window].xvalue = ch - ' ' + 1;
	  Screens[window].x = Screens[window].xvalue;
	  Screens[window].y = Screens[window].yvalue;
	  if (window == CurrScreen)	/* Move the cursor as necessary */
	    gotoxy (Screens[window].xvalue,Screens[window].yvalue);
	  Screens[window].escmode = 0;	/* Back to normal character mode */
	} /* then */
       else if (Screens[window].escmode == 4)
	{
	  /* Process argument to ESC G request */
	  Screens[window].escmode = 0;	/* Reset the escape mode */
	  if (ch == '0')
	    Screens[window].attr = NormalAttr << 8;
	   else if (ch == '1')
	    Screens[window].attr = InverseAttr << 8;
	} /* then */
       else
	{
	  Screens[window].escmode = 0;	/* Reset the escape mode */
	  switch (ch)
	    {
	      case '*': ADM31TermOutput (window,26); break;
	      case '=': /* Set the cursor position */
			Screens[window].escmode = 2;
			break;
	      case 'E': InsertLine (window); break;
	      case 'G': Screens[window].escmode = 4; break;
	      case 'R': DeleteLine (window); break;
	      case 'T': EraseEndLine (window); break;
	      case 'W': DeleteChar (window); break;
	      case 'Y': EraseEndScreen (window); break;
	      case 'q': Screens[window].insert = 1; break;
	      case 'r': Screens[window].insert = 0; break;
	      default:	break;
	    } /* switch */
	} /* else */
    } /* then */
   else
    switch (ch)
      {
	case 27:	Screens[window].escmode = 1; break;
	case 13:	Screens[window].x = 1;
			RestoreCursor (window);
			break;
	case 11:	if (Screens[window].y > 1)
			  {
			    Screens[window].y--;
			    RestoreCursor (window);
			  } /* if */
			break;
	case 10:	if (Screens[window].y < 24)
			  {
			    Screens[window].y++;
			    RestoreCursor (window);
			  } /* then */
			 else
			  ScrollUp (window);
			break;
	case 7:		if (AllowBeep)
			  putch (7);
			break;
	case 8:		if (Screens[window].x > 1)
			  {
			    Screens[window].x--;
			    RestoreCursor (window);
			  } /* if */
			break;
	case 9:		do
			  {
			    /* Output spaces until next TAB is reached */
			    ADM31TermOutput (window,' ');
			  }
			while ((Screens[window].x & 7) != 0);
			break;
	case ('^' & 0x1F):
	case 26:	/* Home the cursor (and maybe clear screen) */
			Screens[window].x = 1;
			Screens[window].y = 1;
			RestoreCursor (window);
			if (ch == 26)	/* Clear the screen if ^Z */
			  EraseEndScreen (window);
			break;
	case 0:		break;		/* Ignore NUl characters */
	case 12:	Screens[window].x++;
			if (Screens[window].x > 80)
			  {
			    /* Set x to 1 and output LF to move to next row */
			    Screens[window].x = 1;
			    VT52TermOutput (window,10);
			  } /* if */
			RestoreCursor (window);
			break;
	default:        x = Screens[window].x;
			value = ch | Screens[window].attr;
			if (Screens[window].insert)
			  InsertChar (window,value);
			 else
			  {
			    offset = (Screens[window].y - 1) * 80 + x - 1;
			    Screens[window].image[offset] = value;
			    if (window == CurrScreen)
			      DisplayChar (offset,value);
			  } /* else */
			x++;
			if (x > 80)
			  {
			    Screens[window].x = 1;
			    ADM31TermOutput (window,10);
			  } /* then */
			 else
			  Screens[window].x = x;
			RestoreCursor (window);
			break;
      } /* switch */
} /* ADM31TermOutput */

/* Output a character to a terminal window */
void	_Cdecl	TermOutput (int window,int ch)
{
  if (UWProcList[window].transfer != NULL)
    {
      /* Send the character to the file transfer that is in progress */
      (*(UWProcList[window].transfer -> output)) (window,ch);
      if (!UWProcList[window].terminal)
        return;		/* Exit if terminal echo not wanted */
    } /* if */
  switch (Screens[window].emul)
    {
      case UWT_ADM31:	ADM31TermOutput (window,ch); break;
      case UWT_VT52:	VT52TermOutput (window,ch); break;
      case UWT_ANSI:	break;
      default:		break;
    } /* switch */
} /* TermOutput */

/* Kill a terminal window - no longer required */
/* If 'window' == 0, clean everything up.      */
void	_Cdecl	TermKill (int wind)
{
  /* If a file transfer is in progress, then abort it */
  if (UWProcList[wind].transfer != NULL)
    (*(UWProcList[wind].transfer -> kill)) (wind);

  /* Now clean up the screen processing of the window */
  if (wind)
    StatusLine ();	/* Redraw the status line - one less window */
   else
    {
      window (1,1,80,25);	/* Clean up the screen handling */
      textattr (0x07);
      clrscr ();
      gotoxy (1,1);
    }
} /* TermKill */

/* Bring a terminal window to the top on the screen */
void	_Cdecl	TermTop (int window)
{
  int offset;
  unsigned *line;
  if (window == CurrScreen)
    {
      StatusLine ();	/* Just redraw the status line */
      return;		/* Don't need to change anything */
    } /* if */
  line = Screens[window].image;
  for (offset = 0;offset < SCREEN_SIZE;++offset)
    DisplayChar (offset,*line++);
  gotoxy (Screens[window].x,Screens[window].y);
  CurrScreen = window;
  StatusLine ();
} /* TermTop */

/* Send a sequence of characters for a special keystroke */
static	void	_Cdecl	SendKeys (int window,char *str)
{
  while (*str)
    UWProcWindow (window,*str++);
} /* SendKeys */

/* Translate keypresses according to the terminal type */
/* Calls UWProcWindow to send keycodes as necessary.   */
void	_Cdecl	TermKey (int window,int key)
{
  if (UWProcList[window].transfer != NULL)
    {
      /* Send the key to the file transfer in progress to allow abort, etc */
      (*(UWProcList[window].transfer -> key)) (window,key);
      return;
    } /* if */
  switch (Screens[window].emul)
    {
      case UWT_ADM31:
	  switch (key)
	    {
	      case CURSOR_UP:	UWProcWindow (window,'K' & 0x1F); break;
	      case CURSOR_DOWN:	UWProcWindow (window,10); break;
	      case CURSOR_LEFT: UWProcWindow (window,8); break;
	      case CURSOR_RIGHT:UWProcWindow (window,12); break;
	      case CURSOR_HOME:	UWProcWindow (window,'^' & 0x1F); break;
	      case 0x4400:	SendKeys (window,"\0010\r"); break; /* F0 */
	      default:		if (key >= 0x3B00 && key <= 0x4300)
				  {
				    /* Process function keys F1 - F9 */
				    char str[4] = "\001 \r";
				    str[2] = (key >> 8) - 0x3B + '1';
				    SendKeys (window,str);
				  } /* if */
				break;
	    } /* switch */
	  break;
      case UWT_VT52:
	  switch (key)
	    {
	      case CURSOR_UP:	SendKeys (window,"\033A"); break;
	      case CURSOR_DOWN: SendKeys (window,"\033B"); break;
	      case CURSOR_LEFT: SendKeys (window,"\033D"); break;
	      case CURSOR_RIGHT:SendKeys (window,"\033C"); break;
	      default:		if (key >= 0x3B00 && key <= 0x3E00)
				  {
				    /* Process function keys F1 - F4 */
				    char str[3] = "\033 ";
				    str[2] = (key >> 8) - 0x3B + 'P';
				    SendKeys (window,str);
				  } /* if */
				break;
	    } /* switch */
	  break;
      case UWT_ANSI:	break;
      default:		break;
    } /* switch */
} /* TermKey */

/* Turn off the screen cursor */
static	void	_Cdecl	CursorOff (void)
{
  union REGS regs;
  regs.h.ah = 1;
  regs.x.cx = 0x2007;
  int86 (0x10,&regs,&regs);
} /* CursorOff */

/* Turn the screen cursor back on */
static	void	_Cdecl	CursorOn (void)
{
  union REGS regs;
  regs.h.ah = 1;
  if (mdamode)
    regs.x.cx = 0x0B0C;
   else
    regs.x.cx = 0x0607;
  int86 (0x10,&regs,&regs);
} /* CursorOn */

/* Save the screen details and jump to a DOS shell */
void	_Cdecl	JumpToDOS (void)
{
  int x,y;
  struct text_info ti;
  x = wherex ();
  y = wherey ();
  gettextinfo (&ti);
  gettext (1,1,80,25,SavedScreen);
  textattr (0x07);
  window (1,1,80,25);
  clrscr ();
  cprintf ("Type EXIT at the DOS prompt to return to UW/PC.\r\n");
  system ("");
  FixComDevice ();
  textmode (scrnmode);
  CursorOff ();
  puttext (1,1,80,25,SavedScreen);
  textattr (ti.attribute);
  window (1,1,80,24);
  gotoxy (x,y);
  CursorOn ();
} /* JumpToDOS */

/* Draw an information box on the screen */
static	void	_Cdecl	DrawBox (int width,int height)
{
  int x1,y1,x2,y2,temp;
  x1 = 39 - (width / 2);
  x2 = x1 + width + 1;
  y1 = 12 - (height / 2);
  y2 = y1 + height + 1;
  gettext (x1,y1,x2,y2,SavedScreen);
  window (x1,y1,x2,y2);
  textattr (NormalAttr);
  clrscr ();
  window (1,1,80,25);
  textattr (BoxAttr);
  gotoxy (x1,y1);
  putch (213);
  for (temp = x1 + 1;temp < x2;++temp)
    putch (205);
  putch (184);
  for (temp = y1 + 1;temp < y2;++temp)
    {
      gotoxy (x1,temp);
      putch (179);
      gotoxy (x2,temp);
      putch (179);
    } /* for */
  gotoxy (x1,y2);
  putch (212);
  for (temp = x1 + 1;temp < x2;++temp)
    putch (205);
  putch (190);
  window (x1 + 1,y1 + 1,x2 - 1,y2 - 1);
} /* DrawBox */

/* Erase an information box from the screen */
static	void	_Cdecl	EraseBox (int width,int height)
{
  int x1,y1,x2,y2;
  x1 = 39 - (width / 2);
  x2 = x1 + width + 1;
  y1 = 12 - (height / 2);
  y2 = y1 + height + 1;
  puttext (x1,y1,x2,y2,SavedScreen);
} /* EraseBox */

/* Pop-up the help screen, and wait for the user to press a key */
void	_Cdecl	HelpScreen (void)
{
  int x,y,line;
  struct text_info ti;
  CursorOff ();
  x = wherex ();
  y = wherey ();
  gettextinfo (&ti);
  DrawBox (HELP_COLUMNS + 2,HELP_LINES);
  textattr (NormalAttr);
  for (line = 0;line < HELP_LINES;++line)
    {
      gotoxy (2,line + 1);
      cprintf ("%s",HelpData[line]);
    } /* for */
  while (GetKeyPress () == -1)
    ; /* Wait until a key has been pressed */
  EraseBox (HELP_COLUMNS + 2,HELP_LINES);
  window (1,1,80,24);
  textattr (ti.attribute);
  gotoxy (x,y);
  CursorOn ();
} /* HelpScreen */

/* Pop-up a box and wait for one of a set of keystrokes */
/* Returns the key that was pressed. 			*/
int	_Cdecl	PopupBox (char *message,char *keys)
{
  int x,y,len,key;
  struct text_info ti;
  CursorOff ();
  x = wherex ();
  y = wherey ();
  gettextinfo (&ti);
  len = strlen (message);
  DrawBox (len + 2,3);
  textattr (NormalAttr);
  gotoxy (2,2);
  cprintf ("%s",message);
  while (1)
    {
      key = GetKeyPress ();
      if (key > 0 && key <= 255 && strchr (keys,key))
	break;
    } /* while */
  EraseBox (len + 2,3);
  window (1,1,80,24);
  textattr (ti.attribute);
  gotoxy (x,y);
  CursorOn ();
  return (key);
} /* PopupBox */

/* Prompt the user for a string on the screen.  Returns */
/* non-zero if OK, or zero if ESC was pressed.  The     */
/* previous contents of the buffer may be edited.  The	*/
/* buffer must be 'len' + 1 bytes in length at least.	*/
int	_Cdecl	PromptUser (char *prompt,char *buf,int len)
{
  int retval,x,y,key,posn,length,insert,temp,temp2;
  struct text_info ti;
  x = wherex ();
  y = wherey ();
  gettextinfo (&ti);
  DrawBox (len + 4,4);
  textattr (NormalAttr);
  gotoxy (2,2);
  cprintf ("%s",prompt);		/* Print prompt for user */
  gotoxy (2,3);
  cprintf ("\020 %s",buf);		/* Print current buffer contents */
  length = strlen (buf);		/* Save the length of the buffer */
  posn = 0;
  retval = -1;
  insert = 1;				/* Start in insert mode */
  while (retval < 0)			/* Loop until CR or ESC pressed */
    {
      gotoxy (4 + posn,3);
      while ((key = GetKeyPress ()) < 0)
        ;				/* Loop until a key received */
      switch (key)
        {
	  case CURSOR_INS:	insert = !insert; break;
	  case CURSOR_LEFT:	if (posn > 0)		/* Move cursor left */
	  			  --posn;
				break;
	  case CURSOR_RIGHT:	if (posn < length)	/* Move cursor right */
	  			  ++posn;
				break;
	  case '\b':		if (posn <= 0)		/* Delete last char */
	  			  break;		/* Don't move back */
				--posn;
				/* Fall through to delete char code */
	  case CURSOR_DEL:	temp = posn;		/* Move chars back 1 */
	  			if (!buf[temp])
				  break;		/* Can't delete here */
	  			while (buf[temp])
				  {
				    buf[temp] = buf[temp + 1];
				    ++temp;
				  } /* while */
				gotoxy (4 + posn,3);
				cprintf ("%s ",buf + posn);
				--length;		/* Decrease length */
				break;
	  case CURSOR_HOME:	posn = 0; break;
	  case CURSOR_END:	posn = length; break;
	  case '\r':		retval = 1; break;	/* Exit with line */
	  case '\033':		retval = 0; break;	/* Exit - no line */
	  case ('X' & 0x1F):	posn = 0;		/* Clear whole line */
	  			length = 0;
				buf[0] = '\0';
				gotoxy (4,3);
				clreol ();
				break;
	  default:		if (key >= 32 && key <= 255)
	  			  {
				    if (insert)
				      {
				        /* Insert the character at this posn */
				        if (length >= len)
				          break;	/* Too many chars */
					temp = posn;
					while (temp < length)
					  {
					    /* Swap insert and this char */
					    temp2 = buf[temp];
					    buf[temp++] = key;
					    key = temp2;
					  } /* while */
					buf[temp++] = key;
					buf[temp] = '\0';
					++length;
					cprintf ("%s",buf + posn++);
				      } /* then */
				     else
				      {
				        if (posn >= len)
					  break;	/* No more space */
					buf[posn++] = key;
					putch (key);
					if (posn > length)
					  buf[++length] = '\0';
				      } /* else */
				  } /* if */
				break;
	} /* switch */
    } /* while */
  EraseBox (len + 4,4);
  window (1,1,80,24);
  textattr (ti.attribute);
  gotoxy (x,y);
  return (retval);
} /* PromptUser */
