//-------------------------------------------------------------------------
//
// UW.H - Declarations for the UW protocol within UW/PC.
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
//    1.0    23/03/91  RW  Original Version of UW.H
//    1.1    05/05/91  RW  Cleanup phase.
//    1.2    26/05/91  RW  Add command-line to "jumpdos".
//
//-------------------------------------------------------------------------

#ifndef __UW_H__
#define	__UW_H__

//
// Forward declarations of some classes needed by the UWProtocol class.
//
class	UWDisplay;
class	UWClient;

//
// Define the top-level interface to the UW protocol routines.
// The static object "UWMaster" is the only object of this
// type and once the program is initialised, it is in control
// of all communication.
//
#define	NUM_UW_WINDOWS	8

class	UWProtocol {

private:

	int	CurrWindow;	// Current window that is in use.
	int	LastInput;	// Last input window.
	int	RoundWindow;	// Round-robin service window.
	int	OutputWindow;	// Current output window.
	int	gotmeta;	// Non-zero for meta escape.
	int	gotiac;		// Non-zero for IAC escape.
	int	getpcl;		// For protocol negotiation.
	int	dirproc;	// Non-zero for direct processing.

	UWClient  *clients[NUM_UW_WINDOWS];
	UWDisplay *displays[NUM_UW_WINDOWS];
	int	numwinds;	// Number of windows in service - 1.
	UWClient  *freelist;	// List of clients to be freed.

	friend	class	UWClient;

	// Send a UW command to the remote host.
	void	command (int cmd);

	// Send a character to the remote host in the
	// round-robin service window.  This is called by
	// the function UWClient::send.
	void	send	(int ch);

	// Send a character to the "remote" method of
	// the client attached to the output window.
	void	remote	(int ch);

	// Process a character incoming from the host
	void	fromhost (int ch);

public:

	int	terminate;	// Set this to non-zero to terminate program.
	int	exitmulti;	// Set this to non-zero to exit UW session.
	int	protocol;	// Number of the protocol that is in use.

	// Start the processing of the UW protocol.  When
	// this method exits, the program has been terminated.
	// On startup, an initial dumb terminal is created.
	// Returns NULL or an error message.
	char	*start	(void);

	// Force the exit from Protocol 1 or higher, and a
	// return to Protocol 0 (ignored in Protocol 0).
	void	exit	(void);

	// Create a new window (ignored in Protocol 0).  Returns
	// the identifier, or 0 if no window could be created.
	// If number != 0, then the number has been supplied
	// explicitly, usually by the remote host.
	int	create	(int number=0);

	// Install a new client on top of the one in the current
	// round-robin window.
	void	install	(UWClient *newclient);

	// Remove the top-most client from the current round-
	// robin window, and return to the client underneath.
	void	remove	(void);

	// Display a new status line on the screen bottom
	void	status	(void);

	// Turn direct character processing in protocol 0 on or off.
	void	direct	(int on);

	// Kill a particular window.  Once all Protocol 1 or 2
	// windows have been destroyed, "exit" is automatically
	// called to exit the protocol service.  If number == 0,
	// then the current window is killed.
	void	kill	(int number=0);

	// Bring a particular window to the top (i.e. make it
	// the current window).
	void	top	(int number);

	// Jump out to a DOS shell, and fix everything on return.
	// Optionally execute a command in DOS.
	void	jumpdos	(char *cmdline=0);

	// Hangup the modem and return to Protocol 0.
	void	hangup	(void);

	// Send a modem control string through the current window.
	// Modem control strings include initialisation, hangup, etc.
	void	sendstring (char *str);

	// Exit protocol 1 and send a modem line break.
	void	sendbreak (void);

};

//
// Define the master object for handling the UW protocol.
//
extern	UWProtocol	UWMaster;

#endif	/* __UW_H__ */
