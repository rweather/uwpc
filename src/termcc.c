/*-------------------------------------------------------------------------

  TERMCC.C - Main module for the Termcap Compiler.
 
    This file is part of the Termcap Compiler source code.
    Copyright (C) 1990-1992  Rhys Weatherley

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
     1.0    23/03/91  RW  Original Version of TERMCC.C
     1.1    13/03/92  RW  Add support for the TCAP2CAP module.
     1.2    15/03/92  RW  Add -t option for conditional compilation.

-------------------------------------------------------------------------*/

#include <string.h>		/* String processing routines */
#include <stdio.h>		/* Standard I/O routines */

/*
 * Define some buffers to hold the names of the input and output files.
 */
char	InFileName[BUFSIZ];
char	OutFileName[BUFSIZ];

extern	int	numerrors;
extern	int	ConvertTermcap (char *type,char *tcapfile,char *capfile);
extern	char	ReqTermName[];

int	main	(argc,argv)
int	argc;
char	*argv[];
{
  FILE *infile;
  int posn;

  /* Print the program banner and copyright message */
  printf ("TERMCC version 1.03, Copyright (C) 1991-1992 Rhys Weatherley\n");
  printf ("TERMCC comes with ABSOLUTELY NO WARRANTY; see the file COPYING for details.\n");
  printf ("This is free software, and you are welcome to redistribute it\n");
  printf ("under certain conditions; see the file COPYING for details.\n");

  /* Determine the input and output files */
  if (argc <= 1)
    usage ();
   else if (!stricmp (argv[1],"-c"))
    {
      /* Convert a termcap entry into a .CAP file */
      if (argc < 4 || argc > 5)
        usage ();
      if (argc == 5)
        {
	  /* Use the supplied filename and add .CAP if necessary */
	  strcpy (OutFileName,argv[4]);
          posn = strlen (OutFileName);	/* Find ".", "/" or "\" */
          while (posn > 0 && OutFileName[posn - 1] != '.' &&
      	         OutFileName[posn - 1] != '\\' &&
	         OutFileName[posn - 1] != '.')
	    --posn;
          if (posn == 0 || OutFileName[posn - 1] != '.')
            strcat (OutFileName,".cap");
	} /* then */
       else
        {
	  /* Construct a filename as "termtype.CAP" */
	  strcpy (OutFileName,argv[2]);
	  strcat (OutFileName,".cap");
	} /* else */
      if (ConvertTermcap (argv[2],argv[3],OutFileName))
        exit (0);
       else
        exit (1);
    } /* then */
   else
    {
      /* Check for the conditional compilation option */
      if (!stricmp (argv[1],"-t"))
        {
	  argv++;
	  argc--;
	  if (argc <= 1)
	    usage ();
	  strcpy (ReqTermName,argv[1]);
	  argv++;
	  argc--;
	  if (argc <= 1)
	    usage ();
	} /* if */

      /* Build the default input and output files */
      strcpy (InFileName,argv[1]);
      posn = strlen (InFileName);	/* Find ".", "/" or "\" */
      while (posn > 0 && InFileName[posn - 1] != '.' &&
      	     InFileName[posn - 1] != '\\' &&
	     InFileName[posn - 1] != '.')
	--posn;
      if (posn == 0 || InFileName[posn - 1] != '.')
        strcat (InFileName,".cap");

      /* Get the default output filename */
      strcpy (OutFileName,InFileName);
      posn = strlen (OutFileName);	/* Find last "." */
      while (posn > 0 && OutFileName[posn - 1] != '.')
        --posn;
      if (posn > 0)
        strcpy (OutFileName + posn,"trm");	/* Replace extension */
    } /* else */
  if (argc == 3)
    strcpy (OutFileName,argv[2]);
   else if (argc > 3)
    usage ();

  /* Parse the input file to create the resultant file */
  if ((infile = fopen (InFileName,"rt")) == NULL)
    {
      fprintf (stderr,"\n");
      perror (InFileName);
      exit (1);
    } /* if */
  initlex (infile);	/* Initialise the lexical analyser */
  yyparse ();		/* Parse the input file */
  fixups ();		/* Do all label fixups */
  dumpcode ();		/* Dump the code to the output file */
  fclose (infile);	/* Close the input file at the end */

  if (numerrors == 0)
    exit (0);
   else
    exit (1);
} /* main */

/*
 * Print a usage message for the program and exit.
 */
usage ()
{
  fprintf (stderr,"\nUsage: TERMCC infile[.CAP] [outfile[.TRM]]");
  fprintf (stderr,"\n   or: TERMCC -c termtype tcapfile [capfile[.CAP]]\n");
  exit (1);
} /* usage */
