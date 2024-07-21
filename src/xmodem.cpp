//-------------------------------------------------------------------------
//
// XMODEM.CPP - Declarations for handling X/YMODEM file transfers.
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
//    1.1    11/05/91  RW  Original Version of XMODEM.CPP
//
//-------------------------------------------------------------------------

#include <stdio.h>		// Standard I/O routines.
#include "files.h"		// Declarations for this module.
#include "uw.h"			// UW protocol master declarations.
#include "timer.h"		// Timer handling routines.
#include "display.h"		// Display handling routines.

#pragma	warn	-par

//
// Uncomment the following line to turn on debugging writes.
//
// #define	XMOD_DEBUG	1

//
// Define the allowable states the file transfer can be in.
//
#define	ST_XREC		0	// Start XMODEM receive (original).
#define	ST_XREC_CRC	1	// Start XMODEM receive (CRC-16).
#define	ST_XSEND	2	// Start XMODEM send (original).
#define ST_XSEND_CRC	3	// Start XMODEM send (CRC-16).
#define	ST_YREC		4	// Start YMODEM receive.
#define	ST_YREC_G	5	// Start YMODEM/G receive.
#define	ST_YSEND	6	// Start YMODEM send.
#define	ST_XREC_CRC_2	7	// To switch from CRC-16 to checksum receive.
#define	ST_HEAD_START	8	// Start of a block header.
#define	ST_PURGE	9	// Purge the line.
#define	ST_BLOCK_NUM	10	// Get the block number.
#define	ST_COMP_BLOCK	11	// Get the complemented block number.
#define	ST_READ_DATA	12	// Read data from the block.
#define	ST_GET_CHKSUM	13	// Get the checksum for current block.
#define	ST_SEND_DATA	14	// Send data to the remote host.
#define	ST_SEND_STOP	15	// Send EOT to remote host.

//
// Define the special X/YMODEM file transfer constants.
//
#define	SOH		0x01
#define	STX		0x02
#define	EOT		0x04
#define	ACK		0x06
#define	NAK		0x15
#define	CAN		0x18
#define	EOF_CHAR	0x1A

#define	TICKS_PER_SEC	19	// Set at 19 to give "at least" one second.

//
// Define the starting states for each of the transfer types.
//
static	int	SendStates[] =
	  {ST_XSEND,ST_XSEND_CRC,ST_XSEND,ST_XSEND_CRC,
	   ST_YSEND,ST_YSEND,ST_YSEND};
static	int	RecvStates[] =
	  {ST_XREC,ST_XREC_CRC,ST_XREC,ST_XREC_CRC,
	   ST_YREC,ST_YREC,ST_YREC_G};

UWXYFileTransfer::UWXYFileTransfer (UWDisplay *wind,int type,int recv,
				    char *name) :
		UWFileTransfer (wind)
{
  kind = type;
  receive = recv;
  if (!recv)
    {
      file = fopen (name,"rb");	// Open the file for binary mode reading.
      state = SendStates[kind];	// Get the initial state to be processed.
    }
   else
    {
      file = fopen (name,"wb");	// Open the file for binary mode writing.
      state = RecvStates[kind];	// Get the initial state to be processed.
    }
  blocknum = 1;
  UWMaster.install (this);	// Install the file transfer object.
  if (file == NULL)
    UWMaster.remove ();		// Remove object if initialisation failed.
   else
    {
      setvbuf (file,NULL,_IOFBF,BUFSIZ); // Set a file buffer if possible.
      UWMaster.direct (1);	// Turn on direct Protocol 0 handling.
    }
  if (recv)
    {
      timer = TimerCount - 1;	// Simulate an immediate timeout.
      timeout = 0;
    }
   else
    timeout = -1;
  crcblocks = -1;
  bigblock = 0;
  start = 1;
  endfile = 0;
  if (!recv)
    readblock ();		// Read the first block from the file.
} // UWXYFileTransfer::UWXYFileTransfer //

UWXYFileTransfer::~UWXYFileTransfer (void)
{
  if (file != NULL)
    fclose (file);
  UWMaster.direct (0);
} // UWXYFileTransfer::~UWXYFileTransfer //

//
// Define the names of the various transfer types.
//
static	char	*TransferNames[] =
	  {"XMDM","XMDMC","XMDM1","XMD1C","YMDM","YMDMB","YMDMG"};

char far *UWXYFileTransfer::name ()
{
  return ((char far *)TransferNames[kind]);
} // UWXYFileTransfer::name //

// Cancel a transmission - a CAN character was received.
void	UWXYFileTransfer::cancel (void)
{
  fclose (file);
  file = NULL;		// Indicator for the destructor.
  UWMaster.remove ();	// Remove us from the client stack.
  UWMaster.direct (0);
} // UWXYFileTransfer::cancel //

// Setup the current state to perform a line purge.
// This uses a 2-second instead of the standard
// XMODEM 1-second timeout to allow for load on the remote host.
#define	purge()		(state = ST_PURGE, timer = TimerCount, \
			 timeout = 2 * TICKS_PER_SEC)

// Setup a 1-second timeout for characters in a block.
#define	one_sec_timeout() (timer = TimerCount, timeout = TICKS_PER_SEC)

// Send the cancel sequence to the remote host.
#define	cancel_seq()	{ int x; for (x = 0;x < 10;++x) send (CAN); \
				 for (x = 0;x < 10;++x) send (8); }

// Do some processing for the current state.  If
// "ch" is -1, then a timeout has occurred.
void	UWXYFileTransfer::process (int ch)
{
#ifdef	XMOD_DEBUG
  /* if (ch != -1)
    window -> send (ch); */
#endif
  switch (state)
    {
      case ST_XREC_CRC_2:
      case ST_XREC:
      		if (ch == CAN)
		  {
		    cancel_seq ();
		    cancel ();
		    break;
		  }
      		if (ch != SOH && ch != STX && ch != EOT)
		  {
		    send (NAK);
		    timer = TimerCount;
		    timeout = 5 * TICKS_PER_SEC;
		    state = ST_XREC;
		    break;
		  }
		if (crcblocks < 0)
		  {
		    if (state != ST_XREC)
		      crcblocks = 1;
		     else
		      crcblocks = 0;
		  }
		// Fall through to receiving a block start //
      case ST_HEAD_START:
#ifdef	XMOD_DEBUG
      		window -> send ('H');
#endif
      		if (ch == CAN || ch == EOT)
		  {
		    if (ch == EOT)
		      send (ACK);	// Acknowledge end of file and cancel.
		    cancel_seq ();
		    cancel ();
		    break;
		  }
		if (ch != SOH && ch != STX)
		  {
		    purge ();
		    break;
		  }
		if (ch == SOH)
		  bigblock = 0;
		 else
		  bigblock = 1;
		one_sec_timeout ();
		state = ST_BLOCK_NUM;
		break;
      case ST_XREC_CRC:
      		if (ch == -1)
		  {
		    send ('C');
		    timer = TimerCount;
		    timeout = 3 * TICKS_PER_SEC;
		    state = ST_XREC_CRC_2;	// Go back to normal transfer.
		    break;
		  }
		state = ST_HEAD_START;
		crcblocks = 1;
		process (ch);
		break;
      case ST_XSEND:
#ifdef	XMOD_DEBUG
		window -> send ('H');
		if (ch != -1)
		  window -> send (ch);
#endif
      		if (ch == CAN)		// If a cancel character - abort.
		  {
		    cancel_seq ();
		    cancel ();
		    break;
		  }
      		 else if (ch == ACK)	// Block acknowledged - get next
		  {
		    if (!start && !readblock ())
		      {
		        // We are at the end of the file - send EOT's //
		        state = ST_SEND_STOP;
			send (EOT);
			break;
		      }
		  }
		 else if (ch != NAK)	// Waiting for a NAK.
		  break;
		if (bigblock)
		  send (STX);
		 else
		  send (SOH);
		send (blocknum);
		send (blocknum ^ 255);
		state = ST_SEND_DATA;	// We want to send data to remote.
		chksum = 0;
		posn = 0;
		start = 0;
		break;
      case ST_XSEND_CRC:
      case ST_YREC:
      case ST_YREC_G:
      case ST_YSEND:
      		break;
      case ST_PURGE:
#ifdef	XMOD_DEBUG
      		window -> send ('P');
#endif
      		if (ch == -1)
		  {
		    // Timeout occurred - line has been purged //
		    send (NAK);
		    timer = TimerCount;
		    timeout = 5 * TICKS_PER_SEC;
		    state = ST_XREC;		// Wait for a block again.
		  }
		 else
		  {
		    // Ignore the purged character and timeout again //
		    purge ();
		  }
		break;
      case ST_BLOCK_NUM:
#ifdef	XMOD_DEBUG
      		window -> send ('B');
#endif
		if (ch != -1)
		  {
		    // We can continue for the expected or last block //
		    thisblock = ch;
		    state = ST_COMP_BLOCK;
		    one_sec_timeout ();
		  }
		 else
		  purge ();
		break;
      case ST_COMP_BLOCK:
#ifdef	XMOD_DEBUG
      		window -> send ('b');
#endif
      		if (ch != (thisblock ^ 255))
		  purge ();
		 else if (thisblock != blocknum &&
		          thisblock != (blocknum - 1))
		  {
		    cancel_seq ();	// Gross loss of sync - abort.
		    cancel ();
		  }
		 else
		  {
		    posn = 0;		// Start reading data in block.
		    chksum = 0;
		    state = ST_READ_DATA;
		    one_sec_timeout ();
		  }
		break;
      case ST_READ_DATA:
#ifdef	XMOD_DEBUG
      		if (posn == 0)
		  window -> send ('D');
#endif
      		if (ch == -1)
		  {
		    purge ();
		    break;
		  }
		chksum += ch;		// Update the checksum.
		buffer[posn++] = ch;
		if ((bigblock && posn >= 1024) || (!bigblock && posn >= 128))
		  {
		    // Got all of the data bytes - wait for checksum //
		    state = ST_GET_CHKSUM;
		    one_sec_timeout ();
		  }
		 else
		  one_sec_timeout ();	// Wait for next character.
		break;
      case ST_GET_CHKSUM:
#ifdef	XMOD_DEBUG
      		window -> send ('C');
#endif
      		if (ch == -1)
		  {
		    purge ();
		    break;
		  }
		if ((chksum & 255) != ch)
		  purge ();		// Illegal checksum.
		 else
		  {
		    int temp;
		    if (thisblock == blocknum)
		      {
		        // Write the block to the output file //
		        for (temp = 0;temp < posn;++temp)
			  fputc (buffer[temp],file);
			++blocknum;	// Move onto the next block number.
			blocknum &= 255;
			UWMaster.status (); // Update the status line.
		      }
		    send (ACK);		// Acknowledge the block.
		    state = ST_XREC;	// Back around for next block.
		  }
		break;
      case ST_SEND_DATA:
#ifdef	XMOD_DEBUG
		if (posn == 0)
		  window -> send ('D');
#endif
      		// Note: incoming characters are ignored during block send //
		ch = buffer[posn++];
#ifdef	XMOD_DEBUG
		window -> send (ch);
#endif
		chksum += (ch & 255);
		send (ch);			// Send next data character.
		if ((bigblock && posn >= 1024) || (!bigblock && posn >= 128))
		  {
#ifdef	XMOD_DEBUG
		    window -> send ('C');
#endif
		    send (chksum & 255);	// Send the block's checksum.
		    state = ST_XSEND;		// Back to sending blocks.
		  }
		break;
      case ST_SEND_STOP:
#ifdef	XMOD_DEBUG
		window -> send ('E');
#endif
      		if (ch != ACK && ch != CAN)
		  {
		    send (EOT);		// Re-send end of file marker.
		    break;
		  }
		cancel_seq ();		// Cancel the transfer.
		cancel ();
		break;
    } /* switch */
} // UWXYFileTransfer::process //

// Read a block from the file to be sent.  Returns
// non-zero if OK, or zero at the end of the file.
int	UWXYFileTransfer::readblock (void)
{
  int posn,size,ch;
  if (endfile)
    return (0);			// The whole file has been sent.
  posn = 0;
  if (bigblock)
    size = 1024;
   else
    size = 128;
  while (size && (ch = fgetc (file)) != EOF)
    {
      // Fill the transmission buffer with the next block //
      buffer[posn++] = ch;
      --size;
    } /* while */
  while (size--)
    buffer[posn++] = EOF_CHAR;	// Fill the rest of the block with ^Z's
  if (ch == EOF)
    endfile = 1;		// Mark the end of the file for next call.
  if (!start)
    {
      blocknum = (blocknum + 1) & 255; // Increment the block count.
      UWMaster.status ();	// Update the status line.
    }
  return (1);			// Ready the block to be sent.
} // UWXYFileTransfer::readblock //

void	UWXYFileTransfer::key (int keypress)
{
  if (keypress == 033)		// Abort transfer if ESC pressed.
    {
      cancel_seq ();
      cancel ();
    }
   else
    defkey (keypress);		// Do the default key operation.
} // UWXYFileTransfer::key //

void	UWXYFileTransfer::tick (void)
{
  if (timeout != -1 && (TimerCount - timer) > timeout)
    {
      timeout = -1;		// Disallow another timeout.
      process (-1);		// Tell the state machine a timeout occurred.
    }
  if (state == ST_SEND_DATA)
    process (-1);		// Need to send some data to remote.
} // UWXYFileTransfer::tick //

char	*UWXYFileTransfer::getstatus (void)
{
  static char *sendnames[] =
  	  {"XMODEM upload - ESC to abort - %0 blocks transferred"};
  static char *recvnames[] =
          {"XMODEM download - ESC to abort - %0 blocks transferred"};
  if (receive)
    return (recvnames[kind]);
   else
    return (sendnames[kind]);
} // UWXYFileTransfer::getstatus //

int	UWXYFileTransfer::getstatarg (int digit)
{
  return (blocknum - 1);
} // UWXYFileTransfer::getstatarg //
