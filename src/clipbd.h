//-------------------------------------------------------------------------
//
// CLIPBD.H - Clipboard processing routines for UW/PC.
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
//    1.0    08/06/91  RW  Original Version of CLIPBD.H
//
//-------------------------------------------------------------------------

#ifndef __CLIPBD_H__
#define	__CLIPBD_H__

#include "client.h"		// Client processing declarations.

//
// Define a cut-to-clipboard client for UW/PC.  This client is
// called upon to mark a window region to be sent to the clipboard.
//
class	UWCutToClipboard : public UWClient {

private:

	int	x1,y1,x2,y2,corner;	// Area corners and current corner.
	int	usemouse;		// Non-zero for mouse cutting.

public:

	UWCutToClipboard (UWDisplay *wind,int mouse=0,int x=0,int y=0);
	~UWCutToClipboard (void);

	virtual	char far *name	()
		  { if (usemouse)
		      return ((char far *)"MCUT");
		     else
		      return ((char far *)" CUT");
		  };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);
	virtual	void	mouse	(int x,int y,int buttons);

};

//
// Define a paste-from-clipboard client for UW/PC.  This will
// take the data in the paste buffer and send it the remote for
// the current window.  A 10ms pause will be given between lines.
// If a paste is already active, then the new paste will abort.
//
class	UWPasteFromClipboard : public UWClient {

public:

	UWPasteFromClipboard (UWDisplay *wind);
	~UWPasteFromClipboard (void);

	virtual	char far *name	() { return ((char far *)"PASTE"); };
	virtual	void	key	(int keypress) { defkey (keypress); };
	virtual	void	remote	(int ch) { if (underneath)
					     underneath -> remote (ch); };
	virtual	void	tick	(void);

};

#endif	/* __CLIPBD_H__ */
