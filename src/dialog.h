//-------------------------------------------------------------------------
//
// DIALOG.H - Declarations for creating dialog boxes.  Only one dialog
//	      box can be active at any time, corresponding to the current
//	      window.  The code ensures that keys for changing windows
//	      will never be passed through to the default handlers.
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
//    1.0    11/04/91  RW  Original Version of DIALOG.H
//
//-------------------------------------------------------------------------

#ifndef __DIALOG_H__
#define	__DIALOG_H__

#include "client.h"		// Client processing routines.

//
// Define the structure of a client dialog box handler.  Note
// that a dialog box must always have a specified underlying client.
// This default client will terminate whenever a key is pressed.
//
class	UWDialogBox : public UWClient {

protected:

	int	dx1,dy1,dx2,dy2;	// Corners of dialog box.
	int	cleared;		// Non-zero when mark is cleared.

	// Terminate this dialog box and return to the previously
	// active client in this window.
	void	terminate (void);

	// Show a string on the screen in the dialog box.
	void	showstring (int x,int y,char *str);

public:

	UWDialogBox (UWDisplay *wind,int x1,int y1,int x2,int y2);
	~UWDialogBox (void);

	virtual	char far *name	() { if (underneath)
				       return (underneath -> name ());
				      else
				       return ((char far *)" DLG"); };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch) { if (underneath)
					     underneath -> remote (ch); };

};

//
// Declare the structure of the dialog box to display UW/PC's help.
//
class	UWHelpBox : public UWDialogBox {

public:

	UWHelpBox (UWDisplay *wind);

};

//
// Declare the structure of a query box (single character answers).
//
class	UWQueryBox : public UWDialogBox {

protected:

	char	*anschars;		// Answer characters.

public:

	// Create a query dialog box.  If "answers" is NULL,
	// then the default answer string "yYnN\033" is used.
	UWQueryBox (UWDisplay *wind,char *query,int qlen,char *answers=0);

	virtual	void	key	(int keypress);

	// Process the character provided by the user if it
	// is in the answer string.  The string index is provided.
	virtual	void	process	(int index);

};

//
// Declare the structure of a pop-up line editing box.
//
#define	MAX_EDIT_SIZE	70
class	UWEditBox : public UWDialogBox {

protected:

	char	buffer[MAX_EDIT_SIZE + 1];	// Buffer for the answer.
	int	size;				// Size of edit area.
	int	posn;				// Current editing position.
	int	length;				// Current buffer length.

public:

	// Create a editing dialog box to get a string //
	UWEditBox (UWDisplay *wind,char *prompt,int editsize);

	virtual	void	key	(int keypress);

	// Process a fully entered line.  If 'esc' is non-zero
	// then the ESC key was pressed to abort the editing.
	virtual	void	process	(int esc);

};

#endif	/* __DIALOG_H__ */
