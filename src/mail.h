//-------------------------------------------------------------------------
//
// MAIL.H - Declarations for the mail handling tool of UW/PC.
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
//    1.0    25/07/91  RW  Original Version of MAIL.H
//
//-------------------------------------------------------------------------

#ifndef __MAIL_H__
#define	__MAIL_H__

#include "client.h"		// Client handling declarations.

//
// Define the structure of a mail handling tool client.
//
#define	MAIL_BUF_LEN	1024
#define	MAIL_MAX_HDRS	200		// Maximum of 200 headers in mailbox.
class	UWMailTool : public UWClient {

private:

	char	respbuf[MAIL_BUF_LEN];	// Response buffer from host.
	int	resposn;		// Current position.
	int	mode;			// Current mail tool mode.
	char	*headers[MAIL_MAX_HDRS];// Header information for mailbox.
	int	numheaders;		// Number of active headers.
	int	topheader;		// Top-most header on the screen.
	int	currheader;		// Current header.

	// Send a mail server command to the remote host.
	void	command	(char *str);

	// Write a string to the screen.
	void	write	(char *str);

	// Show the header information on the screen.
	void	showheaders	(void);

	// Handle the current buffer of information from the remote host.
	void	handle	(void);

	// Dispose the current mail headers.
	void	disposeheaders	(void);

public:

	UWMailTool (UWDisplay *wind);
	~UWMailTool (void) { disposeheaders (); };

	virtual	char far *name	() { return ((char far *)"MAIL"); };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);
	virtual	char	*getstatus (void);

};

#endif	/* __MAIL_H__ */
