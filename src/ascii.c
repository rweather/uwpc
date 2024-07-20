/*-------------------------------------------------------------------------

  ASCII.C - ASCII file transfer routines for UW/PC.

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
     1.0    26/01/91  RW  Original Version of ASCII.C

-------------------------------------------------------------------------*/

#include "ascii.h"		/* Declarations for this module */
#include "term.h"		/* Terminal handling routines */
#include <stdio.h>		/* Standard I/O routines */
#include <dos.h>		/* "delay" is defined here */
#include <alloc.h>		/* Memory allocation routines */

#ifdef	__TURBOC__
#pragma	warn	-par
#endif

/* Define some forward declarations for this module */
static	void	AscInit		(int window,uwtype_t emul);
static	void	AscOutput	(int window,int ch);
static	void	AscTick		(int window);
static	void	AscKill		(int window);
static	void	AscTop		(int window);
static	void	AscKey		(int window,int key);

/* Define the window process descriptor to use for file transfers */
static	UWProcess	TransferProcess =
	  {1,0,NULL,AscInit,AscOutput,AscTick,AscKill,AscTop,AscKey};

/* Define an array of ASCII file transfer descriptors for each window */
#define FILE_BUFSIZ	4096
typedef	struct	{
		  FILE *file;	/* File descriptor in use */
		  int	send;	/* Non-zero for a file send */
		  char *buffer;	/* File buffer for the transfer */
		} TransferRec;
static	TransferRec	FileTransfers[NUM_UW_WINDOWS];

/* Start an ASCII send of a file in the given window */
void	_Cdecl	AsciiSend (int window,char *filename)
{
  /* Setup the file transfer record */
  if ((FileTransfers[window].file = fopen (filename,"rt")) == NULL)
    return;		/* Could not open file - abort */
  FileTransfers[window].send = 1;
  if ((FileTransfers[window].buffer = (char *)malloc (FILE_BUFSIZ)) != NULL)
    {
      /* Set a transfer buffer on the file to smooth things out */
      setvbuf (FileTransfers[window].file,FileTransfers[window].buffer,
    	       _IOFBF,FILE_BUFSIZ);
    } /* if */

  /* Install the file transfer process descriptor */
  UWProcList[window].transfer = &TransferProcess;
  if (window == UWCurrWindow)
    TermTop (window);		/* Change status line if necessary */
} /* AsciiSend */

/* Start an ASCII receive of a file in the given window */
void	_Cdecl	AsciiReceive (int window,char *filename,int append)
{
  /* Setup the file transfer record */
  if ((FileTransfers[window].file = fopen (filename,
  	(append ? "ab" : "wb"))) == NULL)
    return;		/* Could not open file - abort */
  FileTransfers[window].send = 0;
  if ((FileTransfers[window].buffer = (char *)malloc (FILE_BUFSIZ)) != NULL)
    {
      /* Set a transfer buffer on the file to smooth things out */
      setvbuf (FileTransfers[window].file,FileTransfers[window].buffer,
    	       _IOFBF,FILE_BUFSIZ);
    } /* if */

  /* Install the file transfer process descriptor */
  UWProcList[window].transfer = &TransferProcess;
  if (window == UWCurrWindow)
    TermTop (window);		/* Change status line if necessary */
} /* AsciiReceive */

/* Initialise an ASCII transfer - dummy routine */
static	void	AscInit (int window,uwtype_t emul)
{
  /*#### Nothing to be done here ####*/
} /* AscInit */

/* Output a character to the ASCII transfer.  i.e. this */
/* is a character from the remote host machine.		*/
static	void	AscOutput (int window,int ch)
{
  if (!FileTransfers[window].send)	/* Save character for receive */
    {
      if (fputc (ch,FileTransfers[window].file) == EOF)
        AscKill (window);		/* Kill transfer on file error */
    } /* if */
} /* AscOutput */

/* Process a time slice tick for an ASCII transfer */
static	void	AscTick (int window)
{
  if (FileTransfers[window].send)
    {
      int ch;
      if ((ch = fgetc (FileTransfers[window].file)) == EOF)
        AscKill (window);		/* Kill window on EOF/error */
       else if (ch == '\n')		/* Special processing at line ends */
        {
	  UWProcWindow (window,'\r');	/* Send CR and wait for 100 ms */
	  delay (100);			/* to give host time to catch up */
	} /* then */
       else
	UWProcWindow (window,ch);	/* Send character to the window */
    } /* if */
} /* AscTick */

/* Kill an ASCII transfer that is currently in progress */
static	void	AscKill (int window)
{
  /* Clean up the file transfer in use */
  fclose (FileTransfers[window].file);		/* Close the file */
  if (FileTransfers[window].buffer != NULL)
    free (FileTransfers[window].buffer);	/* Free the transfer buffer */

  /* Disable the file transfer on the nominated window */
  UWProcList[window].transfer = NULL;
  if (window == UWCurrWindow)
    TermTop (window);		/* Change status line if necessary */
} /* AscKill */

/* Bring an ASCII window to the top - dummy routine */
static	void	AscTop (int window)
{
  /*#### Nothing to be done here ####*/
} /* AscTop */

/* Process a special key for the window */
static	void	AscKey (int window,int key)
{
  if (key == '\033')		/* ESC aborts a transfer */
    AscKill (window);		/* so kill the transfer window */
} /* AscKey */
