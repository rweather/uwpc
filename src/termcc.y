/*-------------------------------------------------------------------------

  TERMCC.Y - Grammar for the Termcap Compiler.
 
    This file is part of the Termcap Compiler source code.
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
     1.0    23/03/91  RW  Original Version of TERMCC.Y
     1.1    25/05/91  RW  Add more instructions to support ANSI

-------------------------------------------------------------------------*/

%token CMD_SEND CMD_SEND52 CMD_CR CMD_LF CMD_BS CMD_BSWRAP CMD_MOVE
%token CMD_CLEAR CMD_CLREOL CMD_CLREOS CMD_CLRSOL CMD_CLRSOS CMD_INSLINE
%token CMD_DELLINE CMD_INSCHAR CMD_DELCHAR CMD_SETATTR CMD_SETSCRL CMD_GETCH
%token CMD_SETX CMD_SETY CMD_LOAD CMD_ADD CMD_SUB CMD_CMP CMD_SWITCH
%token CMD_JE CMD_JNE CMD_JA CMD_JAE CMD_JB CMD_JBE CMD_JMP CMD_JSR CMD_RET
%token CMD_REMOTE CMD_ESCAPE CMD_ENDSW CMD_TAB CMD_BELL CMD_SAVEXY CMD_RESTXY
%token CMD_GETXY CMD_GETX CMD_GETY CMD_SCRLUP CMD_SCRLDN CMD_SET CMD_RESET
%token CMD_TEST CMD_NAME CMD_KEY CMD_ENDKEYS CMD_RESARR CMD_GETARG CMD_GETA
%token CMD_DEC CMD_SHIFT CMD_SETC CMD_SAVEATTR CMD_RESTATTR CMD_INSBLANK
%token NUMBER STRING IDENT COMMA SEMI COLON VAL_WIDTH VAL_HEIGHT

/* YYSTYPE is the semantic type for non-terminals and terminals */

%{

#include "opcodes.h"		/* Abstract machine opcodes */
#include "symbols.h"		/* Symbol manipulation routines */
#include <stdio.h>

#define yyoverflow	yyover	/* Where to go on overflows */
/* #define	YYDEBUG		1 */

extern	int	lexline;

int	numerrors=0;		/* Number of errors that occurred */
int	numwarnings=0;		/* Number of warnings that occurred */

#define	BUFFER_SIZE	16384

unsigned char	OutBuffer[BUFFER_SIZE];
int	OutBufPosn=0;

char	Buffer[100];

%}

%%

program:  cmds
	;

cmds:	  cmds cmd
	|
	;

cmd:	  CMD_SEND
		{ genbyte (OP_SEND); }
	| CMD_SEND52
		{ genbyte (OP_SEND52); }
	| CMD_CR
		{ genbyte (OP_CR); }
	| CMD_LF
		{ genbyte (OP_LF); }
	| CMD_BS
		{ genbyte (OP_BS); }
	| CMD_TAB
		{ genbyte (OP_TAB); }
	| CMD_BELL
		{ genbyte (OP_BELL); }
	| CMD_BSWRAP
		{ genbyte (OP_BSWRAP); }
	| CMD_MOVE
		{ genbyte (OP_MOVE); }
	| CMD_CLEAR
		{ genbyte (OP_CLEAR); }
	| CMD_CLREOL
		{ genbyte (OP_CLREOL); }
	| CMD_CLREOS
		{ genbyte (OP_CLREOS); }
	| CMD_CLRSOL
		{ genbyte (OP_CLRSOL); }
	| CMD_CLRSOS
		{ genbyte (OP_CLRSOS); }
	| CMD_INSLINE
		{ genbyte (OP_INSLINE); }
	| CMD_DELLINE
		{ genbyte (OP_DELLINE); }
	| CMD_INSCHAR
		{ genbyte (OP_INSCHAR); }
	| CMD_DELCHAR
		{ genbyte (OP_DELCHAR); }
	| CMD_SETATTR NUMBER
		{ genbyte (OP_SETATTR); genbyte ($2); }
	| CMD_SETATTR
		{ genbyte (OP_SETATTR_ACC); }
	| CMD_SETSCRL NUMBER
		{ genbyte (OP_SETSCRL); genbyte ($2); }
	| CMD_SETSCRL
		{ genbyte (OP_SETSCRL_ACC); }
	| CMD_GETCH
		{ genbyte (OP_GETCH); }
	| CMD_SETX
		{ genbyte (OP_SETX); }
	| CMD_SETY
		{ genbyte (OP_SETY); }
	| CMD_LOAD VAL_WIDTH
		{ genbyte (OP_LOAD_WIDTH); }
	| CMD_LOAD VAL_HEIGHT
		{ genbyte (OP_LOAD_HEIGHT); }
	| CMD_LOAD NUMBER
		{ genalt (OP_LOAD,OP_LOAD_WORD,$2); }
	| CMD_ADD VAL_WIDTH
		{ genbyte (OP_ADD_WIDTH); }
	| CMD_ADD VAL_HEIGHT
		{ genbyte (OP_ADD_HEIGHT); }
	| CMD_ADD NUMBER
		{ genalt (OP_ADD,OP_ADD_WORD,$2); }
	| CMD_SUB VAL_WIDTH
		{ genbyte (OP_SUB_WIDTH); }
	| CMD_SUB VAL_HEIGHT
		{ genbyte (OP_SUB_HEIGHT); }
	| CMD_SUB NUMBER
		{ genalt (OP_SUB,OP_SUB_WORD,$2); }
	| CMD_CMP VAL_WIDTH
		{ genbyte (OP_CMP_WIDTH); }
	| CMD_CMP VAL_HEIGHT
		{ genbyte (OP_CMP_HEIGHT); }
	| CMD_CMP NUMBER
		{ genalt (OP_CMP,OP_CMP_WORD,$2); }
	| CMD_SWITCH switches CMD_ENDSW
	| CMD_JE ref
		{ genbyte (OP_JE); doaddref ($2,OutBufPosn); }
	| CMD_JNE ref
		{ genbyte (OP_JNE); doaddref ($2,OutBufPosn); }
	| CMD_JA ref
		{ genbyte (OP_JA); doaddref ($2,OutBufPosn); }
	| CMD_JAE ref
		{ genbyte (OP_JAE); doaddref ($2,OutBufPosn); }
	| CMD_JB ref
		{ genbyte (OP_JB); doaddref ($2,OutBufPosn); }
	| CMD_JBE ref
		{ genbyte (OP_JBE); doaddref ($2,OutBufPosn); }
	| CMD_JMP ref
		{ genbyte (OP_JMP); doaddref ($2,OutBufPosn); }
	| CMD_JSR ref
		{ genbyte (OP_JSR); doaddref ($2,OutBufPosn); }
	| CMD_RET
		{ genbyte (OP_RET); }
	| CMD_REMOTE
		{ genbyte (OP_REMOTE); }
	| CMD_ESCAPE NUMBER
		{ genalt (OP_ESCAPE,OP_ESCAPE_WORD,$2); }
	| CMD_SAVEXY
		{ genbyte (OP_SAVEXY); }
	| CMD_RESTXY
		{ genbyte (OP_RESTXY); }
	| CMD_GETXY
		{ genbyte (OP_GETXY); }
	| CMD_GETX
		{ genbyte (OP_GETX); }
	| CMD_GETY
		{ genbyte (OP_GETY); }
	| CMD_SCRLUP
		{ genbyte (OP_SCRLUP); }
	| CMD_SCRLDN
		{ genbyte (OP_SCRLDN); }
	| CMD_SET NUMBER
		{ genbyte (OP_SET); genbyte ($2); }
	| CMD_RESET NUMBER
		{ genbyte (OP_RESET); genbyte ($2); }
	| CMD_TEST NUMBER
		{ genbyte (OP_TEST); genbyte ($2); }
	| CMD_NAME STRING
		{ genstring ($2); }
	| CMD_KEY NUMBER COMMA STRING
		{ genword ($2); genstring2 ($4); }
	| CMD_ENDKEYS
		{ genword (0); }
	| CMD_RESARR
		{ genbyte (OP_RESARR); }
	| CMD_GETARG
		{ genbyte (OP_GETCH); genbyte (OP_GETARG); }
	| CMD_GETA NUMBER COMMA NUMBER
		{ genbyte (OP_GETA); genbyte ($2); genbyte ($4); }
	| CMD_DEC
		{ genbyte (OP_DEC); }
	| CMD_SHIFT
		{ genbyte (OP_SHIFT); }
	| CMD_SETC
		{ genbyte (OP_SETC); }
	| CMD_SAVEATTR
		{ genbyte (OP_SAVEATTR); }
	| CMD_RESTATTR
		{ genbyte (OP_RESTATTR); }
	| CMD_INSBLANK
		{ genbyte (OP_INSBLANK); }
	| IDENT COLON
		{ addposn ($1,OutBufPosn); }
	| error
		{ parserr ("Illegal instruction"); }
	;

ref:	  IDENT
		{ $$ = $1; }
	;

switches: switches switch
	|
	;

switch:	  NUMBER COMMA ref
		{ genalt (OP_SWITCH,OP_SWITCH_WORD,$1);
		  doaddref ($3,OutBufPosn); }
	| error
		{ parserr ("Illegal switch"); }
	;

%%

/* Indicate an overflow of the parse stack and abort */
yyover ()
{
  parsefatal ("Parse stack overflow");
}

/* Report an error from FLEX/BISON - ignored in this version */
yyerror (msg)
char	*msg;
{
  /* fprintf (stderr,"%s (%d)\n",msg,lexline); */
}

/* Report a parsing error, and update the number of errors */
parserr (msg)
char	*msg;
{
  fprintf (stderr,"Error (%d): %s\n",lexline,msg);
  ++numerrors;
}

/* Report a warning, and update the number of warnings */
parsewarn (msg)
char	*msg;
{
  fprintf (stderr,"Warning (%d): %s\n",lexline,msg);
  ++numwarnings;
}

/* Report a fatal parsing error and abort */
parsefatal (msg)
char	*msg;
{
  fprintf (stderr,"Fatal (%d): %s\n",lexline,msg);
  exit (1);
}

/* Generate a single byte of code to the output buffer */
genbyte	(byte)
int	byte;
{
  OutBuffer[OutBufPosn++] = byte;
} /* genbyte */

/* Generate a 2-byte word of code to the output buffer */
genword (word)
int	word;
{
  genbyte (word & 255);
  genbyte ((word >> 8) & 255);
} /* genword */

/* Generate an instruction that has alternate byte and word types */
genalt (inst1,inst2,value)
int	inst1,inst2,value;
{
  if (value & 0xFF00)
    {
      genbyte (inst2);
      genword (value);
    } /* then */
   else
    {
      genbyte (inst1);
      genbyte (value);
    } /* else */
} /* genalt */

/* Generate a string from the symbol table */
genstring (ident)
int	ident;
{
  char *str;
  str = getname (ident);
  while (*str)
    genbyte (*str++);
  genbyte (0);
} /* genstring */

/* Generate a string starting with a length */
genstring2 (ident)
int	ident;
{
  char *str;
  str = getname (ident);
  genbyte (strlen (str) + 2);	/* Record string length + 2 for speed */
  while (*str)			/* when translating the keys */
    genbyte (*str++);
  genbyte (0);
} /* genstring2 */

/* Add a reference to a label and set links correctly */
doaddref (ident,address)
int	ident,address;
{
  int old;
  old = getref (ident);
  genbyte (old & 255);
  genbyte ((old >> 8) & 255);
  addref (ident,address);
} /* doaddref */

/* Fix up all label references */
fixups ()
{
  int ident,address,posn,temp;
  ident = firstref ();
  while (ident != -1)
    {
      address = getref (ident);
      posn = getposn (ident);
      if (posn == -1)
        {
	  sprintf (Buffer,"Label not defined: %s",getname (ident));
	  parserr (Buffer);
	} /* then */
       else
        while (address != 0)
          {
	    temp = OutBuffer[address] | (OutBuffer[address + 1] << 8);
	    OutBuffer[address] = posn & 255;
	    OutBuffer[address + 1] = (posn >> 8) & 255;
	    address = temp;
	  } /* while */
      ident = nextref ();
    } /* while */
  if ((ident = addident ("start")) == -1 ||
      (posn = getposn (ident)) == -1)
    {
      parserr ("No entry point 'start' defined");
      exit (1);
    } /* if */
  if ((ident = addident ("keys")) == -1 ||
      (posn = getposn (ident)) == -1)
    {
      parserr ("No entry point 'keys' defined");
      exit (1);
    } /* if */
} /* fixups */

extern	char	OutFileName[];

/* Dump the generated code to the output file */
dumpcode ()
{
  FILE *outfile;
  int posn,keys,index;
  if ((outfile = fopen (OutFileName,"wb")) == NULL)
    {
      fprintf (stderr,"\n");
      perror (OutFileName);
      exit (1);
    } /* if */
  posn = getposn (addident ("start"));
  fputc (posn & 255,outfile);
  fputc ((posn >> 8) & 255,outfile);
  keys = getposn (addident ("keys"));
  fputc (keys & 255,outfile);
  fputc ((keys >> 8) & 255,outfile);
  fputc (UW_TERM_VERSION & 255,outfile);
  fputc ((UW_TERM_VERSION >> 8) & 255,outfile);
  for (index = 0;index < OutBufPosn;++index)
    fputc (OutBuffer[index],outfile);
  fclose (outfile);
} /* dumpcode */
