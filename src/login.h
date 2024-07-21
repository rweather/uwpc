//-------------------------------------------------------------------------
//
// LOGIN.H - Declarations for the DOS login facility to allow a user to
//	     login to the PC remotely.
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
//    1.0    26/07/91  RW  Original Version of LOGIN.H
//
//-------------------------------------------------------------------------

#ifndef __LOGIN_H__
#define	__LOGIN_H__

#include "client.h"		// Client handling declarations.

//
// Define the structure of a DOS login client.  The only special
// control character sent is CTRL-Z to clear the screen.
//
class	UWLoginTool : public UWClient {

private:

	int	mode;		// Operation mode.
	char	buffer[80];	// Buffer for string entry.
	int	posn;		// Current buffer position.

	// Send a string of characters to the remote host.
	void	sendstr	(char *str);

	// Clear the buffer.
	void	clrbuf	(void) { posn = 0; };

	// Process a character meant for the buffer.  Returns
	// zero if OK, 1 if '\r' pressed, -1 if ESC pressed.
	int	charbuf	(int ch);

	// Output the main login menu.
	void	menu	(void);

	// Execute the current command.
	int	execute	(void);

public:

	UWLoginTool (UWDisplay *wind);

	virtual	char far *name	() { return ((char far *)"LOGIN"); };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);
	virtual	char	*getstatus (void)
		  { return ("Login in progress - ESC aborts"); };

};

#endif	/* __LOGIN_H__ */
