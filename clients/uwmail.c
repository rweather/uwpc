/*-----------------------------------------------------------------------------

   UWMAIL.C - Server program for the UW/PC mail tool client.

    NOTE: This program relies on the 'MAIL' environment variable being
    	  set to allow it to find the default mailbox.  If the user
	  specifies a mailbox name in the UW.CFG file, then the 'MAIL'
	  variable will not be needed.

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
    -------  --------  --  ---------------------------------------------------
      1.0    25/07/91  RW  Original version of UWMAIL.C

-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/*
 * Define the global data for this module.
 */
#define	MAIL_BUF_LEN	1024
char	Buffer[MAIL_BUF_LEN];
char	MailBox[BUFSIZ];

int	main (argc,argv)
int	argc;
char	*argv[];
{
  /*### Need to put the terminal into non-echo mode here ###*/

  /* Output the "start-up" sequence to tell UW/PC to start a mail session */
  printf ("\033|M\n");

  /* Go into a command processing loop for this program */
  while (gets (Buffer))
    {
      if (Buffer[0] == 'Q' || Buffer[0] == 'q')	/* Check for quit command */
        break;
      switch (Buffer[0])		/* Check for the other commands */
        {
	  case 'R':	readmailbox (); break;
	  default:	break;
	} /* switch */
    } /* while */

  /*### Need to turn the echo mode back on here ###*/
} /* main */

/*
 * Read the contents of a mailbox and output header information
 * to the UW/PC client program.
 */
readmailbox ()
{
  FILE *file;
  extern char *sys_errlist[];
  extern int errno;
  int def=0;
  int keepgoing;
  if (Buffer[0] == '\0')
    {
      char *getenv ();
      char *mailbox;
      if ((mailbox = getenv ("MAIL")) == NULL)
        {
	  printf ("UNo default mailbox - 'MAIL' environment variable not set\n");
	  return;
	} /* if */
      strcpy (MailBox,mailbox);	/* Save the mailbox name */
      def = 1;
    } /* then */
   else
    strcpy (MailBox,Buffer + 1);
  if ((file = fopen (MailBox,"r")) == NULL)
    {
      /* Test for "no file found" - i.e. no mail in mailbox */
      if (errno == ENOENT && def)	/* Test only for default mailbox */
        {
	  printf ("h\n");		/* Simulate an empty mailbox */
	  return;
	} /* if */

      /* Could not open the file - report an error */
      printf ("U%s\n",sys_errlist[errno]);
      return;
    } /* if */
  if (fgets (Buffer,MAIL_BUF_LEN,file))
    keepgoing = 1;
   else
    keepgoing = 0;
  while (keepgoing)
    {
      /* Test for the start of a new mail header */
      if (!strncmp (Buffer,"From ",5))
	keepgoing = newheader (file);
       else
        {
	  /* Haven't found the next message yet - skip lines */
          if (fgets (Buffer,MAIL_BUF_LEN,file))
            keepgoing = 1;
           else
            keepgoing = 0;
	} /* if */
    } /* while */
  printf ("h\n");			/* Mark end of header list */
  fclose (file);
} /* readmailbox */

/*
 * Decode the "From:" line in a mail message.
 */
void	decodefromline (line,from,len)
char	*line,*from;
int	len;
{
  int posn=0;
  int start,end;
  start = 0;
  while (line[start] && isspace (line[start]))
    ++start;
  end = strlen (line);
  while (end > 0 && isspace (line[end - 1]))
    --end;
  if (start >= end)
    {
      /* Nothing on the line - shouldn't happen - BUT ... */
      strcpy (from,"<unknown>");
      return;
    } /* if */
  line[end] = '\0';		/* Truncate the line */
  if (line[end - 1] != '>')
    {
      /* Extract the name from comments within '(' and ')' */
      while (line[start] && line[start] != '(')
        ++start;
      if (line[start] == '(')
        {
	  int brackets=0;	/* Levels of bracketting */
	  ++start;
	  while (line[start] && isspace (line[start]))
	    ++start;
	  while (posn < len && line[start])
	    {
	      /* Skip anything after ')', ',' or '/' at top bracket level */
	      if ((line[start] == ')' || line[start] == ',' ||
	           line[start] == '/') && !brackets)
	        break;		/* End of comment field */
              if (line[start] == '(')
	        ++brackets;
	       else if (line[start] == ')')
	        --brackets;
	      if (line[start] != '|')
	        from[posn++] = line[start];
	      ++start;
	    } /* while */
	} /* if */
      while (posn > 0 && isspace (from[posn]))
        --posn;		/* Strip trailing white space */
      if (posn > 0)
        {
	  /* Found a name - terminate it and exit */
	  from[posn] = '\0';
	  return;
	} /* if */
    } /* then */
   else
    {
      /* Extract the name from the start of the line */
      int quotes;
      quotes = (line[start] == '"');	/* Determine if name is quoted */
      if (quotes)
        ++start;
      while (posn < len && line[start])
        {
	  /* End the name at ',', '/', '"' or '<' */
	  if ((quotes && line[start] == '"') ||
	      (!quotes && line[start] == '<') ||
	      line[start] == ',' ||
	      line[start] == '/')
	    break;
	  if (line[start] != '|')
	    from[posn++] = line[start];
	  ++start;
	} /* while */
      while (posn > 0 && isspace (from[posn]))
        --posn;		/* Strip trailing white space */
      if (posn > 0)
        {
	  /* Found a name - terminate it and exit */
	  from[posn] = '\0';
	  return;
	} /* if */
    } /* else */
  while (posn < len && line[start])	/* Copy the from line directly */
    {
      if (line[start] != '|')
        from[posn++] = line[start];
      ++start;
    } /* while */
  while (posn > 0 && isspace (from[posn]))
    --posn;		/* Strip trailing white space */
  from[posn] = '\0';
} /* decodefromline */

/*
 * A new header for a message has been detected - parse it.
 * Return zero on EOF or error.
 */
int	newheader (file)
FILE	*file;
{
  int lines=1;			/* For counting how many lines in message */
  char status='U';		/* Status of the message */
  char from[21]="";		/* Name of the sender */
  char subject[BUFSIZ]="";	/* Subject of the message */
  char mesgid[BUFSIZ]="";	/* Message identifier */
  int len;
  while (fgets (Buffer,MAIL_BUF_LEN,file) && Buffer[0] != '\n')
    {
      len = strlen (Buffer);
      if (len > 0 && Buffer[len - 1] == '\n')
        Buffer[len - 1] = '\0';
      if (!strncmp (Buffer,"From:",5))
        {
	  decodefromline (Buffer + 5,from,20);
	} /* then */
       else if (!strncmp (Buffer,"Subject:",8))
        {
	  int test=8;
	  while (Buffer[test] && isspace (Buffer[test]))
	    ++test;
	  if (Buffer[test])
	    strcpy (subject,Buffer + test); /* Copy the subject */
	   else
	    strcpy (subject,"<none>");	/* No subject line */
	} /* then */
       else if (!strncmp (Buffer,"Message-Id:",11))
        {
	  int start=11;
	  while (len > 0 && isspace (Buffer[len - 1]))
	    --len;		/* Strip trailing white space */
	  Buffer[len] = '\0';
	  while (Buffer[start] && isspace (Buffer[start]))
	    ++start;		/* Strip leading white space */
	  strcpy (mesgid,Buffer + start);
	} /* then */
      ++lines;
    } /* while */
  if (!ferror (file) && !feof (file))
    {
      /* Skip the rest of the message to get the line count */
      while (fgets (Buffer,MAIL_BUF_LEN,file) &&
      	     strncmp (Buffer,"From ",5))
	++lines;
    } /* if */
  printf ("H%c|%s|%s|%d|%s\n",status,mesgid,from,lines,subject);
  if (ferror (file) || feof (file))
    return (0);			/* Finished the file read */
  return (1);
} /* newheader */
