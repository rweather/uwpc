//-------------------------------------------------------------------------
//
// CLIENT.H - Declarations for creating UW clients within UW/PC.
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
//    1.0    20/03/91  RW  Original Version of CLIENT.H
//    1.1    08/06/91  RW  Add declarations for mouse handling.
//
//-------------------------------------------------------------------------

#ifndef __CLIENT_H__
#define	__CLIENT_H__

#pragma	warn	-par		// Turn off parameter checking.

//
// Define forward declarations of some classes.
//
class	UWDisplay;
class	UWTerminal;

//
// Define the structure of a UW window client.  When a client is
// created, it is assumed that its window is open and ready for
// interaction, and when it is destroyed, the window has already
// been killed and no more interactions are possible with it.  This
// client is not actually used by the UW program - only those
// classes that inherit from this one are used.
//
class	UWClient {

protected:

	UWDisplay	*window;	// Display window assoc with client.
	UWClient	*underneath;	// Client running underneath (or NULL)
	int		capture;	// Non-zero if client has capturing.

	// Send a character to the remote UW server for the
	// window this client is executing under.
	void	send	(int ch);

	// Process a default keypress.  This should be called
	// by the client for any keys it cannot process when
	// "key" is called, and that are deemed to be control
	// keys for the UW client program.
	void	defkey	(int keypress);

	// Determine if a client has capture on //
	int	hascapture (void) { if (capture || !underneath)
				      return (capture);
				     else
				      return (underneath -> hascapture ());
				  }

public:

	int	isaterminal;		// Non-zero if a UWTermDesc type.

	// Create a client that is attached to a particular
	// display window.
	UWClient (UWDisplay *wind)
		   { window = wind; underneath = 0;
		     isaterminal = 0; capture = 0; };

	// Get the display window for this client.
	UWDisplay *getwind (void) { return (window); };

	// Set the pointer to the client underneath.
	void	setunder (UWClient *under) { underneath = under; };

	// Retrieve the name of the client (terminal type, etc).
	virtual	char far *name	() { return ((char far *)"CLNT"); };

	// Retrieve the pointer to the client running under this one.
	UWClient *under	(void) { return (underneath); };

	// Process a user's keypress.  This will only be called
	// if this client is using the top-most displayed window.
	virtual	void	key	(int keypress) { defkey (keypress); };

	// Process a character from the remote server.  This may
	// be called at any time while the client is active.
	virtual	void	remote	(int ch) { /* do nothing here */ };

	// This function is called periodically to give the client
	// some "dead time" to do some work.
	virtual	void	tick	(void) { if (underneath)
					   underneath -> tick (); };

	// Get the status line for this client.  Returns NULL for
	// the ordinary configuration status line.
	virtual	char	*getstatus	(void) { return (0); };

	// Get an argument for the %0-%9 status line escape codes.
	virtual	int	getstatarg	(int digit) { return (0); };

	// Process an event from the mouse.  This will be called
	// whenever the mouse is detected to move or the button
	// status of the mouse changes.  Note that the co-ordinates
	// given are screen co-ordinates - not window co-ordinates.
	// Mouse events are sent to the top-most window always.
	virtual	void	mouse	(int x,int y,int buttons)
		  { if (underneath) underneath -> mouse (x,y,buttons); };

	// Process a timer event in the Windows 3.0 version.
	virtual	void	timertick (void) {};

};

//
// Define a terminal handling client class.  Objects of this
// class or its subclasses correspond to terminal emulations
// executing in windows.  This class is an extremely dumb
// terminal that does no translation of control sequences.
//
class	UWTerminal : public UWClient {

public:

	UWTerminal (UWDisplay *wind) : UWClient (wind) {};

	virtual	char far *name	() { return ((char far *)" TTY"); };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);
	virtual	void	mouse	(int x,int y,int buttons);

};

//
// Define a terminal emulation class that uses neutral
// terminal descriptions.
//
#define	STACK_SIZE	10
#define	TERM_ARGS	8
class	UWTermDesc : public UWTerminal {

protected:

	unsigned char far *description;	// Description buffer.
	int	PC;			// Current program counter.
	int	keys;			// Start of key mappings.
	int	acc,regx,regy;		// Description registers.
	int	flags;			// Terminal mode flags.
	int	compare;		// Comparison flag.
	int	stack[STACK_SIZE];	// Return stack for subroutines.
	int	SP;			// Stack pointer.
	int	savex,savey;		// Save positions for X and Y.
	int	saveattr;		// Saved attribute value.
	int	argarray[TERM_ARGS];	// Array of arguments for esc seqs.
	int	index;			// Index into argument array.
	int	base;			// Base of array for extraction.
	int	keytab;			// Secondary key table.

	// Jump to the currently stored location.
	void	jump		(void);

	// Interpret a terminal description until the next
	// character request, starting with the given character.
	void	interpret	(int ch);

public:

	UWTermDesc (UWDisplay *wind) : UWTerminal (wind)
		{ description = 0; isaterminal = 1; };

	virtual	char	far *name (void) { if (description)
					     return ((char far *)description);
					    else
					     return (UWTerminal::name ());
					 };
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);

	// Set the description to be used for the terminal
	// emulation of this terminal object, and start it up.
	void	setemul	(unsigned char far *desc);

};

extern "C" {

//
// Define the linked-in terminal emulation drivers.
//
extern	unsigned char far cdecl VT52_Driver;
extern	unsigned char far cdecl ADM31_Driver;
extern	unsigned char far cdecl ANSI_Driver;

}

#pragma	warn	+par		// Turn on parameter checking.

#endif	/* __CLIENT_H__ */
