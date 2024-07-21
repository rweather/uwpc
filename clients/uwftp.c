/*-------------------------------------------------------------------------

  UWFTP.C - FTP server for use with the UW/PC communications program.

  This server is quite "dumb".  Most of the brains is in the client in
  the UW/PC program.  This protocol is not conformant with the standard
  Internet FTP protocol - it is for transmission across a serial link only.
 
  To compile this server, execute the following:

                cc -o uwftp uwftp.c

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
     1.0    13/04/91  RW  Original Version of UWFTP.C
     1.1    07/05/91  RW  Being serious writing of the server.

-------------------------------------------------------------------------*/

#include <stdio.h>              /* Standard I/O routines */
#include <sys/types.h>          /* System type declarations */
#include <sys/time.h>           /* Time value declarations */
#include <sgtty.h>              /* Terminal control declarations */
#include <signal.h>             /* Signal handling routines */

/*
 * Global declarations for this module.
 */
#define HOST_LEN        256		/* Length of host name buffers */
char    hostname[HOST_LEN];		/* Name of host from "gethostname" */
struct  sgttyb  terminal;		/* Saved terminal statistics */
#define CANCEL_CHAR     ('X' & 0x1F)	/* Character to cancel transfer */
#define	NO_CHAR		(-2)		/* From "waitfortimeout" function */
#define STARTUP_SEQ	"\001\076F"	/* Startup message for client */
#define	MAX_PLEN	94		/* Maximum packet length */
struct	packet	{
		  int	number;		/* Packet number */
		  char	type;		/* Type of packet */
		  int	length;		/* Packet length */
		  int	checksum;	/* Checksum value */
		  char	data[MAX_PLEN];	/* Data in the packet */
		};
struct	packet	RecvPacket;		/* Received packet */
struct	packet	SendPacket;		/* Transmitted packet */
char	tempbuf[BUSIZ];			/* Temporary reception buffer */
#define	TOCHAR(x)	((x) + ' ')	/* Convert numeric value to char */
#define	FROMCHAR(x)	((x) - ' ')	/* Convert character to numeric val */
FILE	*tfile=NULL;			/* Current transfer file */

int
main    (argc,argv)
int     argc;
char    *argv[];
{
  time_t currenttime;
  struct sgttyb newterm;
  int ch;

  /* Check to make sure both stdin and stdout are terminals */
  if (!isatty (0) || !isatty (1))
    {
      fprintf (stderr,
        "Standard input and output must be terminals for UWFTP.\n");
      exit (1);
    } /* if */

  /* Initialise the terminal input into cbreak and non-echo mode */
  gtty (0,&terminal);
  newterm = terminal;
  newterm.sg_flags |= CBREAK;
  newterm.sg_flags &= ~ECHO;
  stty (0,&newterm);

  /* Turn off signals that may cause problems during processing */
  signal (SIGINT,SIG_IGN);      /* Interrupt signal */
#ifdef  SIGTSTP                 /* Conditional just in case :-) */
  signal (SIGTSTP,SIG_IGN);     /* Stop signal from keyboard */
#endif

  /* Send the startup sequence and wait until a valid response is received */
  ch = ' ';
  while (ch != CANCEL_CHAR && ch != 'A')
    ch = sendstring (STARTUP_SEQ,1);
  if (ch == CANCEL_CHAR)
    abortftp ();                /* ^X received - abort the transfer */

  /* We are now connected to the client - output the welcome message */
  if (gethostname (hostname,HOST_LEN))
    strcpy (hostname,"process");
  time (&currenttime);
  printf ("HHHHH  UWFTP server %s started on %s\n\n",hostname,
          ctime (&currenttime));
  fflush (stdout);

  /* Go into the client service mode, waiting for file/command requests */
  serveclient ();
} /* main */

/*
 * Wait for a particular timeout for a character to arrive
 * on the server's standard input.  Returns the character or
 * NO_CHAR if no character was received within the timeout period.
 */
int	waitfortimeout (timeout)
int	timeout;
{
  fd_set readfds;
  struct timeval timeoutst;
  timeoutst.tv_sec = timeout;
  timeoutst.tv_usec = 0;
  FD_ZERO (&readfds);
  FD_SET (0,&readfds);
  if (select (1,&readfds,NULL,NULL,&timeoutst) > 0)
    return (getchar ());
   else
    return (NO_CHAR);
} /* waitfortimeout */

/*
 * Send (and resend) a string to the client machine until a
 * character is received from the client program which is
 * returned from this function.  CR and LF characters are
 * ignored in the input.
 */
int	sendstring (packet,timeout)
char    *packet;
int     timeout;
{
  int ch;
  while (1)
    {
      printf ("%s",packet);		/* Send the packet to the client */
      fflush (stdout);
      ch = '\r';
      while (ch == '\r' || ch == '\n')	/* Wait for timeout/non-CRLF char */
        ch = waitfortimeout (timeout);
      if (ch != NO_CHAR)		/* If a character received, exit */
        return (ch);
    } /* while */
} /* sendstring */

/*
 * Receive a packet from the client program.  Returns
 * non-zero for a valid packet or zero if invalid/timeout.
 */
int	receivepacket (timeout)
int	timeout;
{
  int ch=NO_CHAR;
  int posn=0;
  int save;

  /* Wait until the next packet starts */
  while (timeout > 0 && (ch == NO_CHAR || ch == '\r' || ch == '\n'))
    {
      ch = waitfortimeout (1);	/* Wait for the next character */
      if (ch == CANCEL_CHAR)
        abortftp ();
      if (ch == NO_CHAR)
        --timeout;		/* Decrease timeout if none yet */
    } /* while */

  /* Read characters into the temporary reception buffer */
  while (posn < (BUFSIZ - 1) && ch != '\r' && ch != '\n')
    {
      tempbuf[posn++] = ch;
      ch = waitfortimeout (3);	/* Wait for the next packet character */
      if (ch == CANCEL_CHAR)
        abortftp ();
    } /* while */
  if (posn >= (BUFSIZ - 1))
    return (0);			/* Packet too long - abort */
  tempbuf[posn] = '\0';
  posn = 0;

  /* Decode the received packet header into its components */
  if (!tempbuf[posn])
    return (0);
  RecvPacket.number = FROMCHAR(tempbuf[posn++] & 255);
  if (RecvPacket.number < 0 || RecvPacket.number >= MAX_PLEN)
    return (0);
  if (!tempbuf[posn])
    return (0);
  RecvPacket.type = tempbuf[posn++] & 255;
  if (!tempbuf[posn])
    return (0);
  RecvPacket.length = FROMCHAR(tempbuf[posn++] & 255);
  if (RecvPacket.length < 0 || RecvPacket.length >= MAX_PLEN)
    return (0);
  if (!tempbuf[posn])
    return (0);
  RecvPacket.checksum = FROMCHAR(tempbuf[posn++] & 255);
  if (RecvPacket.checksum < 0 || RecvPacket.checksum >= MAX_PLEN)
    return (0);
  if (!tempbuf[posn])
    return (0);
  ch = FROMCHAR(tempbuf[posn++] & 255);
  if (ch < 0 || ch >= MAX_PLEN)
    return (0);
  RecvPacket.checksum += ch * MAX_PLEN;

  /* Decode the data portion of the packet */
  save = 0;
  while (tempbuf[posn] && save < RecvPacket.length)
    {
      ch = tempbuf[posn++];
      if (ch < ' ' || ch >= 0177)
        return (0);		/* Illegal character in packet */
      if (ch != '#')
        RecvPacket.data[save++] = ch;
       else
        {
	  /* Process an escaped character */
	  ch = tempbuf[posn++];
	  if (ch == '#' || ch == '&')
	    RecvPacket.data[save++] = ch;
	   else if (ch >= '@' && ch <= ('@' + 0x1F))
	    RecvPacket.data[save++] = (ch & 0x1F);
	   else
	    return (0);		/* Illegal escape sequence in packet */
	} /* if */
    } /* while */
  if (tempbuf[posn])
    return (0);			/* Extra characters in packet */

  /* Recompute the packet checksum and check it */
  save = 0;
  while (posn = 0;posn < RecvPacket.length;++posn)
    save += RecvPacket.data[posn];
  save %= (MAX_PLEN * MAX_PLEN); /* Truncate to 94 squared values */
  return (save == RecvPacket.checksum);
} /* receivepacket */

/*
 * Transmit the characters for the send packet.
 */
transmitpacket ()
{
  int posn,ch;
  putchar ('\n');		   	/* For possible error correction */
  putchar (TOCHAR(SendPacket.number));	/* Packet number for reference */
  putchar (SendPacket.type);	   	/* Type of packet */
  putchar (TOCHAR(SendPacket.length));	/* Length of the packet following */
  putchar (TOCHAR(SendPacket.checksum % MAX_PLEN)); /* Send packet checksum */
  putchar (TOCHAR(SendPacket.checksum / MAX_PLEN));
  for (posn = 0;posn < SendPacket.length;++posn)
    {
      ch = SendPacket.data[posn] & 255;
      if (ch & 0x80)
        {
	  putchar ('&');		/* Send bit 8 escape character */
	  ch &= 0x7F;
	} /* if */
      if (ch < ' ')
        {
	  putchar ('#');		/* Send control character escape */
	  putchar (ch + '@');		/* Send ASCII version of CTRL char */
	} /* then */
       else if (ch == '#' | ch == '&')
        {
	  putchar ('#');		/* Escape the special character */
	  putchar (ch);			/* and then send it */
	} /* then */
       else
        putchar (ch);			/* Send the character direct */
    } /* for */
  putchar ('\n');			/* Terminate the packet */
  fflush (stdout);			/* Flush the output to the client */
} /* transmitpacket */

/*
 * Send (and resend) the send packet with a specified timeout
 * for a response from the client machine.
 */
sendpacket (timeout)
int	timeout;
{
  int posn;

  /* Compute the checksum for the packet to be sent */
  SendPacket.checksum = 0;
  for (posn = 0;posn < SendPacket.length;++posn)
    SendPacket.checksum += SendPacket.data[posn];
  SendPacket.checksum %= (MAX_PLEN * MAX_PLEN);

  /* Continually send the packet until a response is received */
  do
    {
      transmitpacket ();		/* Transmit the send packet */
    }
  while (!receivepacket (timeout));	/* Keep sending until acknowledged */
} /* sendpacket */

/*
 * Abort an FTP transfer - a ^X character was received.
 */
abortftp ()
{
  if (tfile)
    fclose (tfile);			/* Close off the transfer file */
  printf ("\n\nX\nX\nX\nX\nX\n\n");     /* Send the cancel sequence */
  fflush (stdout);
  stty (0,&terminal);                   /* Reset the terminal stats */
  exit (0);                             /* Exit the program */
} /* abortftp */

/* Serve the client UWFTP program */
serveclient ()
{
  abortftp ();				/* Abort the file transfer */
} /* serveclient */
