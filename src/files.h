//-------------------------------------------------------------------------
//
// FILES.H - Declarations for creating UW clients for file transfers.
// 
// NOTE: <stdio.h> must be included before this file.
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
//    1.0    11/04/91  RW  Original Version of FILES.H
//    1.1    11/05/91  RW  Added X/YMODEM file transfers.
//
//-------------------------------------------------------------------------

#ifndef __FILES_H__
#define	__FILES_H__

#include "client.h"		// Client processing declarations.

//
// Define the structure of a UW file transfer client.
//
class	UWFileTransfer : public UWClient {

protected:

	FILE	*file;			// Current file being processed.

public:

	UWFileTransfer (UWDisplay *wind) : UWClient (wind) {};

	virtual	char far *name	() { if (underneath)
				       return (underneath -> name ());
				      else
				       return ((char far *)"FILES");
				   };

};

//
// Define the structure of a file transfer client for performing
// ASCII file transfers to the remote host.  Transfers are of
// three kinds: ASCII upload, ASCII download and screen capture.
// Screen capture exists to allow a normal terminal session to
// continue, yet also to place received characters into a file.
//
#define	ASCII_UPLOAD	0
#define	ASCII_DOWNLOAD	1
#define ASCII_CAPTURE	2
//
class	UWAsciiFileTransfer : public UWFileTransfer {

protected:

	int	kind;			// Kind of ASCII file transfer.

public:

	UWAsciiFileTransfer (UWDisplay *wind,int type,char *name);
	~UWAsciiFileTransfer (void);

	virtual	char far *name	() { if (kind == ASCII_CAPTURE)
				       return ((char far *)" CAP");
				      else if (kind == ASCII_UPLOAD)
				       return ((char far *)"ASEND");
				      else
				       return ((char far *)"ARECV");
				   };

	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch);
	virtual	void	tick	(void);
	virtual	char	*getstatus (void);
};

//
// Define the structure of a X/YMODEM file transfer protocol
// Because of the nature of clients these protocols must be
// written as state machines.
//
#define	XMOD_ORIGINAL	0	// Original XMODEM protocol.
#define	XMOD_CRC	1	// XMODEM with CRC-16.
#define	XMOD_1K		2	// XMODEM with 1K blocks.
#define	XMOD_1K_CRC	3	// XMODEM with 1K blocks and CRC-16.
#define	YMOD_NORMAL	4	// Normal YMODEM protocol.
#define	YMOD_BATCH	5	// YMODEM protocol with batch.
#define	YMOD_G		6	// YMODEM G protocol (fast/unreliable).
//
class	UWXYFileTransfer : public UWFileTransfer {

protected:

	int	state;		// Current transfer state.
	int	kind;		// Kind of transfer that is in effect.
	int	receive;	// Non-zero for a receive.
#ifdef	UWPC_DOS
	int	timer;		// For keeping track of timers.
#else	/* UWPC_DOS */
	DWORD	timer;		// Under Windows 3.0, we use GetCurrentTime.
#endif	/* UWPC_DOS */
	int	timeout;	// Length of timeout (-1 if none).
	int	bigblock;	// Non-zero for a 1K block receive.
	char	buffer[1024];	// Buffer for the data.
	int	posn;		// Current buffer position.
	int	crcblocks;	// Non-zero for CRC blocks.
	int	blocknum;	// Next/expected block number.
	int	thisblock;	// Number of currently received block.
	int	chksum;		// Checksum value.
	int	start;		// Start of transmission.
	int	endfile;	// The end of the file has been reached.

	// Do some processing for the current state.  If
	// "ch" is -1, then a timeout has occurred.
	void	process	(int ch);

	// Cancel a transmission - a CAN character was received.
	void	cancel	(void);

	// Read a block from the file to be sent.  Returns
	// non-zero if OK, or zero at the end of the file.
	int	readblock (void);

public:

	UWXYFileTransfer (UWDisplay *wind,int type,int recv,char *name);
	~UWXYFileTransfer (void);

	virtual	char far *name	();
	virtual	void	key	(int keypress);
	virtual	void	remote	(int ch) { process (ch); };
	virtual	void	tick	(void);
	virtual	void	timertick (void);
	virtual	char	*getstatus (void);
	virtual	int	getstatarg (int digit);

};

#endif	/* __FILES_H__ */
