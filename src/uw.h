//-------------------------------------------------------------------------
//
// UW.H - Declarations for the UW protocol within UW/PC.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1992  Rhys Weatherley
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
//    1.3    16/03/92  RW  Make things static to improve performance.
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

// Note: these were made static because the object lookup was
// totally murdering performance in the main loops.  Since there
// is only one instance of this class in UW/PC, this doesn't matter.

	int	CurrWindow;	// Current window that is in use.
	int	LastInput;	// Last input window.
	int	RoundWindow;	// Round-robin service window.
	int	OutputWindow;	// Current output window.
	int	protoflags;	// Flags for the protocol decoding.
	int	optflag;	// State variable for procoptions.
	int	gotmeta;	// Non-zero for meta escape.
	int	gotiac;		// Non-zero for IAC escape.
	int	getpcl;		// For protocol negotiation.
	int	newwind;	// New window number for protocol 2.
	int	dirproc;	// Non-zero for direct processing.

	UWClient  *clients[NUM_UW_WINDOWS];
	UWDisplay *displays[NUM_UW_WINDOWS];
	int	numwinds;	// Number of windows in service - 1.
	UWClient  *freelist;	// List of clients to be freed.

	friend	class	UWClient;

	// Send a UW command to the remote host.
	void	command (int cmd);

	// Output a Protocol 2 option command to the remote host.
	void	option	(int window,int action,int optnum);

	// Output a numeric option argument to the remote host.
	// If "longval" is non-zero, the argument is 12-bit, rather
	// than 6-bit.
	void	optarg	(int arg,int longval);

	// Process a received option character for window "newwind".
	void	procoptions (int ch);

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

	// Pass a keypress from outside the UW protocol master.
	// This is only called in the Windows 3.0 version.
	void	sendkey	(int wind,int key);

	// Process a timer pulse in the Windows 3.0 version of
	// the program.  The timer pulse is passed onto all
	// currently active clients.
	void	timer	(void);

	// Send a mouse message to a particular window.  This
	// is only called in the Windows 3.0 version.
	void	mouse	(int wind,int x,int y,int buttons);

	// Force the exit from Protocol 1 or higher, and a
	// return to Protocol 0 (ignored in Protocol 0).
	void	exit	(void);

	// Create a new window (ignored in Protocol 0).  Returns
	// the identifier, or 0 if no window could be created.
	// If number != 0, then the number has been supplied
	// explicitly, usually by the remote host.
	int	create	(int number=0,int windtype=-1);

	// Install a new client on top of the one in the current
	// round-robin window.
	void	install	(UWClient *newclient);

	// Remove the top-most client from the current round-
	// robin window, and return to the client underneath.
	void	remove	(void);

	// Display a new status line on the screen bottom
	void	status	(void);

	// Change the title bar on a client's window to reflect
	// the current mode.  This doesn't do anything under DOS.
	void	titlebar (UWClient *client);

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

	// Change the number of the current window.  This is called
	// by the Windows 3.0 version when a window becomes active.
	void	setcurrent (int number)
		  { if (clients[number]) CurrWindow = number; };

	// Cycle around to the next window in Protocol 1/2.
	void	nextwindow (void);

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

	// Start a client service that was requested by a
	// "^[|" escape sequence from the remote host.
	void	startclient (int ch);

	// Minimize all windows except the given window.  This only
	// has an effect in the Windows 3.0 version.
	void	minall	(int wind);
};

//
// Define the master object for handling the UW protocol.
//
extern	UWProtocol	UWMaster;

#endif	/* __UW_H__ */
