/*-------------------------------------------------------------------------

  TCAP2CAP.C - Code to convert /etc/termcap entries into .CAP files.
 
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
     1.0    13/03/92  RW  Original Version of TCAP2CAP.C

-------------------------------------------------------------------------*/

#include <string.h>		/* String processing routines */
#include <stdio.h>		/* Standard I/O routines */
#include <alloc.h>		/* Memory allocation routines */
#include <ctype.h>		/* Character typing macros */

static	int	BuildStateMachine (void);
static	void	GenerateStateCode (FILE *outfile);

/* Global data for this module */
#define	TCAP_BUFSIZ	8192	/* Make it big for lots of work room */
static	char	*TermcapEntry;	/* Buffer to hold the temporary entry */
static	int	TermcapLen;	/* Length of buffer currently */
static	char	ReadBuffer[BUFSIZ]; /* General purpose reading buffer */

/*
 * Test to see if the terminal type "type" matches any
 * of the terminal names in "buf".  Case is ignored during
 * compares.
 */
static	int	MatchingTermcapName (char *type,char *buf)
{
  int index=0;
  int len = strlen (type);
  while (buf[index] && buf[index] != '\\' &&
  	 buf[index] != ':' && buf[index] != '\n')
    {
      if (!strncmpi (type,buf + index,len) &&
          (buf[index + len] == '|' || buf[index + len] == ':'))
	return (1);		/* Found a match on this line */
      while (buf[index] && buf[index] != '|' && buf[index] != ':' &&
     	     buf[index] != '\\' && buf[index] != '\n')
        ++index;		/* Skip to the next name */
      if (buf[index] == '|')	/* Skip the separator between names */
        ++index;
    } /* while */
  return (0);
} /* MatchingTermcapName */

/*
 * Test for a trailing backslash on a string.
 */
static	int	TrailingSlosh (char *str)
{
  int index = strlen (str);
  while (index > 0 && isspace (str[index - 1]))
    --index;	/* Strip any trailing space first */
  return (index > 0 && str[index - 1] == '\\');
} /* TrailingSlosh */

/*
 * Add a string to the stored termcap entry.  Returns the
 * index of the character after the string.  If the string
 * is a "tc" capability, then "*newtermtype" is set to
 * point to the stored string.  Returns -1 if some error
 * occurred in the string parse.
 */
static	int	AddTermcapString (char *buf,int index,char **newtermtype)
{
  int overflow=0;
  while (!overflow && buf[index] && buf[index] != ':' && buf[index] != '\n')
    {
      /* Copy the contents of the capability string directly across */
      if (TermcapLen >= TCAP_BUFSIZ)
        overflow = 1;
       else
        TermcapEntry[TermcapLen++] = buf[index++];
    } /* while */
  if (TermcapLen < TCAP_BUFSIZ)
    TermcapEntry[TermcapLen++] = '\0';	/* Terminate with a NUL byte */
   else
    overflow = 1;
  if (overflow)
    return (-1);
   else
    return (index);
} /* AddTermcapString */

/*
 * Read a termcap entry from an /etc/termcap file.  Also
 * recursively read in any entries specified by "tc=".
 * Returns zero if something went wrong.
 */
static	int	ReadTermcapEntry (char *type,char *tcapfile)
{
  FILE *infile;
  int found=0;
  int index,first;
  char *temp,*newtermtype;

  /* Open the input file ready for processing */
  if ((infile = fopen (tcapfile,"r")) == NULL)
    {
      perror (tcapfile);
      return (0);
    } /* if */
  setvbuf (infile,NULL,_IOFBF,BUFSIZ);

  /* Read in lines until the required entry has been found */
  newtermtype = NULL;
  while (fgets (ReadBuffer,BUFSIZ,infile))
    {
      if (ReadBuffer[0] == '#' || !ReadBuffer[0] || isspace (ReadBuffer[0]))
        continue;		/* Skip this line: not a terminal id line */
      if (MatchingTermcapName (type,ReadBuffer))
        {
	  found = 1;		/* We have found the required terminal type */
	  first = 1;
	  do
	    {
	      fputs (ReadBuffer,stdout);

	      /* Check to see if this is a legal termcap entry line */
	      if (ReadBuffer[0] == '#')
	        {
		  first = 0;
		  continue;	/* Skip comments within a termcap entry */
		} /* if */

	      /* Scan the line and save away the strings */
	      index = 0;
	      if (first)
	        {
		  /* Skip past the terminal names */
		  while (ReadBuffer[index] && ReadBuffer[index] != ':')
		    ++index;
		} /* if */
	      while (ReadBuffer[index])
	        {
		  /* Skip white space, colons and backslashes */
		  if (isspace (ReadBuffer[index]) ||
		      ReadBuffer[index] == ':' ||
		      ReadBuffer[index] == '\\')
		    {
		      ++index;
		      first = 0;
		      continue;
		    } /* if */
		  index = AddTermcapString (ReadBuffer,index,&newtermtype);
		  if (index < 0)
		    {
		      /* An error occurred while storing the string */
		      fclose (infile);
		      return (0);
		    } /* if */
		} /* while */
	      first = 0;
	    }
	  while (TrailingSlosh (ReadBuffer) &&
	  	 fgets (ReadBuffer,BUFSIZ,infile));
	  break;		/* Exit: we've finished the terminal type */
	} /* if */
    } /* while */

  /* Close the termcap file and recursively scan for any included types */
  fclose (infile);
  if (newtermtype)
    ReadTermcapEntry (newtermtype,tcapfile);

  /* Check to see if we found anything and return the relevant error status */
  if (!found)
    fprintf (stderr,"Could not find the terminal type %s\n",type);
  return (found);
} /* ReadTermcapEntry */

/*
 * Dump the termcap buffer to stdout.
 */
static	void	DumpTermcap (void)
{
  int index=0;
  printf ("%d\n",TermcapLen);
  while (index < TermcapLen)
    {
      printf ("%s\n",TermcapEntry + index);
      index += strlen (TermcapEntry + index) + 1;
    } /* while */
} /* DumpTermcap */

/*
 * Convert the terminal type "type", found in the /etc/termcap
 * file "tcapfile" and write the output to the file "capfile".
 * Returns zero if something went wrong during processing.
 */
int	ConvertTermcap (char *type,char *tcapfile,char *capfile)
{
  FILE *outfile;

  /* Allocate some memory for the termcap buffer */
  if ((TermcapEntry = (char *)malloc (TCAP_BUFSIZ)) == NULL)
    {
      fprintf (stderr,"Not enough memory for the termcap conversion.\n");
      return (0);
    } /* if */
  TermcapLen = 0;

  /* Read the terminal type into memory.  Any included types are */
  /* also read into memory for later processing.		 */
  if (!ReadTermcapEntry (type,tcapfile))
    return (0);

  DumpTermcap ();

  /* Build the state machine in memory */
  if (!BuildStateMachine ())
    {
      fprintf (stderr,"Not enough memory to process the termcap entry.\n");
      return (0);
    } /* if */

  /* Open the output file */
  if ((outfile = fopen (capfile,"w")) == NULL)
    {
      perror (capfile);
      return (0);
    } /* if */
  setvbuf (outfile,NULL,_IOFBF,BUFSIZ);

  /* Write some comment header information to the .CAP file */
  fprintf (outfile,"// %s - Automatically generated CAP file for %s\n\n",
  		capfile,type);
  fprintf (outfile,"\t\tname\t\"%s\"\n\n",type);

  /* Output the code for the video output section */
  fprintf (outfile,"start:\n");
  fprintf (outfile,"loop:\n");
  GenerateStateCode (outfile);

  /* Output the code for the keyboard input section */
  fprintf (outfile,"keys:\n");
  fprintf (outfile,"\t\tendkeys\n");

  /* Close all the files and exit */
  fclose (outfile);
  return (1);
} /* ConvertTermcap */

/*
 * Define the structure of a finite state machine element.
 */
#define	NUM_CODES	10		/* Number of simultaneous codes */
struct	elem	{
		  int	 ch;		/* Character code to match */
		  int	 codes[NUM_CODES]; /* TERMCAP code for terminal node */
		  int	 label;		/* Label of this alternative */
		  struct elem *next;	/* Next character in this sequence */
		  struct elem *alt;	/* switch alternative to this char */
		};

/*
 * Define some special character codes.
 */
#define	CH_ROOT		0x100		/* Root of the state machine */

/*
 * Define some special terminal codes.
 */
#define	CODE_METABIT	0x8000		/* Bit to test for made up codes */
#define	CODE_ZEROSTR	0x8001		/* "\0" which is always ignored */
#define	CODE_DUMMY	0x8002		/* A dummy code for sequence splits */
#define	CODE_CLIENT	0x8003		/* UW/PC client sequence */

/*
 * Define the structure of a block of elements, so memory allocation
 * is more efficient when building the machines.
 */
#define	MAX_BLOCK_SIZE	64
struct	block	{
		  int	 used;		/* Number of elements in use so far */
		  struct elem elems[MAX_BLOCK_SIZE]; /* Elements in block */
		};

/*
 * Declare the global structures for the finte state machine.
 */
static	struct	block	*currentblock=NULL;	/* For memory allocation */
static	struct	elem	*machine=NULL;		/* Start of the machine */
static	int	stlabel=0;			/* Current label number */

/*
 * Allocate a new element for a finite state machine.
 * Returns NULL if out of memory.
 */
static	struct	elem *AllocateMachineElement (void)
{
  struct elem *newelem;
  int index;
  if (!currentblock || (currentblock -> used) >= MAX_BLOCK_SIZE)
    {
      if ((currentblock = (struct block *)malloc (sizeof (struct block)))
      			== NULL)
	return (NULL);
      currentblock -> used = 0;
    } /* if */
  newelem = &(currentblock -> elems[(currentblock -> used)++]);
  for (index = 0;index < NUM_CODES;++index)
    newelem -> codes[index] = 0;
  newelem -> label = 0;
  newelem -> next = NULL;
  newelem -> alt = NULL;
  return (newelem);
} /* AllocateMachineElement */

/*
 * Initialise the state machine.  Returns zero if
 * not enough memory.
 */
static	int	InitStateMachine (void)
{
  if (!(machine = AllocateMachineElement ()))
    return (0);
  machine -> ch = CH_ROOT;
  return (1);
} /* InitStateMachine */

#define	LOBYTE(x)	((x) & 0xFF)
#define	HIBYTE(x)	(((x) >> 8) & 0xFF)

/*
 * Insert a string into the state machine for a particular code.
 * Returns NULL if there is not enough memory.  The root of the
 * state machine should be passed to this function initially.
 * Note: any padding information, etc is assumed to be stripped
 * before this function is called.
 */
static	char	*InsertStateMachine (int code,char *str,struct elem *machine)
{
  int ch;
  char *backstr;
  struct elem *newelem,*lastelem;

  printf ("Inserting string: %c%c=%s\n",LOBYTE(code),HIBYTE(code),str);

  while (*str)
    {
      /* Fetch a character to be inserted into the finite state machine */
      backstr = str;
      if (*str == '\\')
        {
	  /* Match a backslash-quoted character */
	  ++str;
	  if (!(*str))		/* Abort if premature end of string */
	    break;
	   else if (*str >= '0' && *str <= '7')
	    {
	      /* Match an octal character representation: \nnn */
	      ch = 0;
	      while (*str >= '0' && *str <= '7')
	        ch = ch * 8 + (*str++ - '0');
	    } /* then */
	   else
	    switch (*str++)	/* Match a \x sequence */
	      {
	        case 'E': ch = 033; break;
		case 'n': ch = '\n'; break;
		case 'r': ch = '\r'; break;
		case 't': ch = '\t'; break;
		case 'b': ch = '\b'; break;
		case 'f': ch = '\f'; break;
		default:  ch = *(str - 1); break;
	      } /* switch */
	} /* then */
       else if (*str == '^')
        {
	  /* Match a control character */
	  ++str;
	  if (*str)
	    ch = (*str++) & 0x1F;
	   else
	    break;
	} /* then */
       else
        ch = *str++;		/* Get an ordinary character */

      /* Convert \200 into \0 because that's what TERMCAP wants */
      if (!(ch & 0x7F))
        ch = 0;

      /* Abort the loop if we have encountered a new escape sequence */
      if (ch == 033 && machine -> ch != CH_ROOT)
        {
	  str = backstr;	/* Rewind so ESC can be read in again */
	  code = CODE_DUMMY;	/* Insert a dummy terminal here */
	  break;
	} /* if */

      /* Insert the character into the state machine */
      if (machine -> next)
        {
	  /* Search the existing alternatives for the place to insert */
	  lastelem = NULL;
	  newelem = machine -> next;
	  while (newelem && newelem -> ch != ch)
	    {
	      lastelem = newelem;
	      newelem = newelem -> alt;
	    } /* while */
	  if (newelem)
	    machine = newelem;	/* The state already exists: stop here */
	   else
	    {
	      /* Create a new state for the new alternative */
	      if (!(newelem = AllocateMachineElement ()))
	        return (NULL);
	      newelem -> ch = ch;
	      lastelem -> alt = newelem;
	      machine = newelem;
	    } /* if */
	} /* then */
       else
        {
	  /* Create a new child node to insert the character into */
	  if (!(newelem = AllocateMachineElement ()))
	    return (NULL);
	  newelem -> ch = ch;
	  machine -> next = newelem;
	  machine = newelem;	/* Move onto the new machine state */
	} /* else */
    } /* while */

  /* Add the new code to the machine at the final place */
  if (machine -> ch == CH_ROOT)
    {
      /* Report a capability that is empty.  This won't necessarily */
      /* be a hassle, so it's just a warning instead of an error.   */
      fprintf (stderr,"Warning: Capability %c%c is an empty string\n",
      		LOBYTE(code),HIBYTE(code));
    } /* then */
   else if (machine -> codes[0] && !(machine -> codes[0] & CODE_METABIT) &&
   	    !(code & CODE_METABIT))
    {
      /* The sequence already has a code: issue a warning.  This  */
      /* isn't serious because usually it's because two things do */
      /* the same thing in TERMCAP.				  */
      fprintf (stderr,"Warning: Capability %c%c is the same string as %c%c\n",
      		LOBYTE(code),HIBYTE(code),
      		LOBYTE(machine -> codes[0]),HIBYTE(machine -> codes[0]));
    } /* if */

  /* Place the new code into the array of codes in the block */
  ch = 0;
  while (ch < NUM_CODES && machine -> codes[ch])
    ++ch;
  if (ch < NUM_CODES && (!(code & CODE_METABIT) || code == (int)CODE_ZEROSTR))
    machine -> codes[ch] = code;
  return (str);
} /* InsertStateMachine */

/*
 * Determine if the given code is a keyboard string instead
 * of a video string.
 */
static	int	IsKeyboardString (int code)
{
  int ch1,ch2;
  ch1 = LOBYTE(code);
  ch2 = HIBYTE(code);
  if (!isalpha (ch1) && !isdigit (ch2))
    return (1);
  if (ch1 == 'F' || ch1 == 'K' || ch1 == 'k')
    return (1);
  return (0);
} /* IsKeyboardString */

/*
 * Add all of the TERMCAP capability strings to the finite state machine.
 * Returns zero if this function runs out of memory.
 */
static	int	BuildStateMachine (void)
{
  int index=0;
  int code;
  char *str;

  /* Initialise the state machine and add the default strings */
  if (!InitStateMachine () ||
      !InsertStateMachine (CODE_ZEROSTR,"\200",machine) ||
      !InsertStateMachine (CODE_CLIENT,"\E|",machine))
    return (0);

  /* Loop around all capabilities in the entry */
  while (index < TermcapLen)
    {
      str = TermcapEntry + index;
      if (*str && *str != '=' && *str != '.' &&
          *(str + 1) && *(str + 1) != '=')
        {
	  code = (*str & 0x7F) + ((*(str + 1) & 0x7F) << 8);
	  str += 2;
	  if (*str == '=' && *(str + 1) != '/')
	    {
	      /* Skip any padding characters and then insert the string */
	      ++str;
	      while (*str && (isdigit (*str) || *str == '*' || *str == '.'))
	        ++str;
	      if (!IsKeyboardString (code))
	        {
		  /* Insert all partial escape sequences into the machine */
	          while (str && *str)
		    {
		      str = InsertStateMachine (code,str,machine);
		      code = CODE_DUMMY; /* Use a dummy code on next pass */
		    } /* while */
		  if (!str)
		    return (0);	/* We ran out of memory */
		} /* if */
	    } /* if */
	} /* if */
      index += strlen (TermcapEntry + index) + 1;
    } /* while */
} /* BuildStateMachine */

/*
 * Output a character value.
 */
static	void	GenCharValue (FILE *outfile,int ch)
{
  if (ch >= ' ' && ch < 0x7F && ch != '\'')
    fprintf (outfile,"'%c'",ch);
   else
    fprintf (outfile,"0x%02X",ch);
} /* GenCharValue */

/*
 * Generate output code for a single state.
 */
static	void	GenSingleState (FILE *outfile,struct elem *machine)
{
  struct elem *newelem;

  /* Output a label for the state plus a comment as to the capability's name */
  if (machine -> label)
    fprintf (outfile,"L%d:",machine -> label);
  if (machine -> codes[0] == (int)CODE_ZEROSTR)
    fprintf (outfile,"\t\t// NUL\n");
   else if (machine -> codes[0])
    {
      /* Output the names of the capabilities with this expansion */
      int index;
      fprintf (outfile,"\t\t// ");
      for (index = 0;index < NUM_CODES;++index)
        {
	  if (!(machine -> codes[index]))
	    continue;
	  if (index)
	    fputc (',',outfile);
	  fprintf (outfile,"%c%c",LOBYTE(machine -> codes[index]),
	  		HIBYTE (machine -> codes[index]));
	} /* for */
      fputc ('\n',outfile);
    } /* then */
   else if (!(machine -> next))
    fprintf (outfile,"\t\t// ??\n");
   else if (machine -> label)
    fputc ('\n',outfile);

  /* Output code to test the alternatives (if any) */
  if (!(machine -> next))
    {
      /* We've reached a terminal node */
      if (machine -> ch == CH_ROOT)
        fprintf (outfile,"\t\tsend\n");
      fprintf (outfile,"\t\tjmp\tloop\n");
    } /* then */
   else
    {
      /* We need to test for some alternatives */
      newelem = machine -> next;
      fprintf (outfile,"\t\tgetch\n");	/* Get a character to test */
      if (!(newelem -> label))
        newelem -> label = ++stlabel;	/* Assign a new state label */
      if (!(newelem -> alt))
        {
	  /* There's only a single alternative: generate code for it */
	  fprintf (outfile,"\t\tcmp\t");
	  GenCharValue (outfile,newelem -> ch);
	  fprintf (outfile,"\n\t\tje\tL%d\n",newelem -> label);
	  if (machine -> ch == CH_ROOT)
	    fprintf (outfile,"\t\tsend\n");
	  fprintf (outfile,"\t\tjmp\tloop\n");
	} /* then */
       else
        {
	  /* Generate a switch statement for the alternatives */
	  fprintf (outfile,"\t\tswitch\n");
	  while (newelem)
	    {
	      fprintf (outfile,"\t\t  ");
	      GenCharValue (outfile,newelem -> ch);
	      fprintf (outfile,",L%d\n",newelem -> label);
	      newelem = newelem -> alt;
	      if (newelem && !(newelem -> label))
	        newelem -> label = ++stlabel;
	    } /* while */
	  fprintf (outfile,"\t\tendsw\n");
	  if (machine -> ch == CH_ROOT)
	    fprintf (outfile,"\t\tsend\n");
	  fprintf (outfile,"\t\tjmp\tloop\n");
	} /* else */
    } /* else */

  /* Recursively descend to output the code for the alternatives */
  fputc ('\n',outfile);
  newelem = machine -> next;
  while (newelem)
    {
      GenSingleState (outfile,newelem);
      newelem = newelem -> alt;
    } /* while */
} /* GenSingleState */

/*
 * Output TERMCC code for the state machine.
 */
static	void	GenerateStateCode (FILE *outfile)
{
  GenSingleState (outfile,machine);
} /* GenerateStateCode */
