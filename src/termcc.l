/*-------------------------------------------------------------------------

  TERMCC.L - Lexical analysis declarations for the Termcap Compiler.
 
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
     1.0    23/03/91  RW  Original Version of TERMCC.L
     1.1    25/01/91  RW  Add more instructions to support ANSI

-------------------------------------------------------------------------*/

/* YYSTYPE is the semantic type to use in parsing */

%{

#include <stdio.h>
#include <ctype.h>
#include "y_tab.h"
#include "symbols.h"

#define	DEF_CHAR_CONST	'\0'	/* Default character constant on error */

extern	YYSTYPE	yylval;	/* Semantic value of current lexical symbol */
int	lexline=1;	/* Current line in the input file */

%}

%%
\/\/.*			/* Comment - ignore it (C++ comments) */
send			return (CMD_SEND);
send52			return (CMD_SEND52);
cr			return (CMD_CR);
lf			return (CMD_LF);
bs			return (CMD_BS);
tab			return (CMD_TAB);
bell			return (CMD_BELL);
bswrap			return (CMD_BSWRAP);
move			return (CMD_MOVE);
clear			return (CMD_CLEAR);
clreol			return (CMD_CLREOL);
clreos			return (CMD_CLREOS);
clrsol			return (CMD_CLRSOL);
clrsos			return (CMD_CLRSOS);
insline			return (CMD_INSLINE);
delline			return (CMD_DELLINE);
inschar			return (CMD_INSCHAR);
delchar			return (CMD_DELCHAR);
setattr			return (CMD_SETATTR);
setscrl			return (CMD_SETSCRL);
getch			return (CMD_GETCH);
setx			return (CMD_SETX);
sety			return (CMD_SETY);
load			return (CMD_LOAD);
add			return (CMD_ADD);
sub			return (CMD_SUB);
cmp			return (CMD_CMP);
switch			return (CMD_SWITCH);
endsw			return (CMD_ENDSW);
je			return (CMD_JE);
jne			return (CMD_JNE);
ja			return (CMD_JA);
jae			return (CMD_JAE);
jb			return (CMD_JB);
jbe			return (CMD_JBE);
jmp			return (CMD_JMP);
jsr			return (CMD_JSR);
ret			return (CMD_RET);
remote			return (CMD_REMOTE);
escape			return (CMD_ESCAPE);
savexy			return (CMD_SAVEXY);
restxy			return (CMD_RESTXY);
getxy			return (CMD_GETXY);
getx			return (CMD_GETX);
gety			return (CMD_GETY);
scrlup			return (CMD_SCRLUP);
scrldn			return (CMD_SCRLDN);
width			return (VAL_WIDTH);
height			return (VAL_HEIGHT);
set			return (CMD_SET);
reset			return (CMD_RESET);
test			return (CMD_TEST);
name			return (CMD_NAME);
key			return (CMD_KEY);
endkeys			return (CMD_ENDKEYS);
resarr			return (CMD_RESARR);
getarg			return (CMD_GETARG);
geta			return (CMD_GETA);
dec			return (CMD_DEC);
shift			return (CMD_SHIFT);
setc			return (CMD_SETC);
saveattr		return (CMD_SAVEATTR);
restattr		return (CMD_RESTATTR);
insblank		return (CMD_INSBLANK);
[a-z_A-Z][a-z_A-Z0-9]*	{ if ((yylval = addident (yytext)) == -1)
			    parsefatal ("Out of symbol table space");
			   else
			    return (IDENT);
			}
\"[^\"\n]*[\"\n]	{ if (!lexstring ('"'))
			    parserr ("String is too small");
			   else if ((yylval = addident (yytext)) == -1)
			    parsefatal ("Out of symbol table space");
			   else
			    return (STRING);
			}
'[^'\n]*['\n]		yylval = lexchar () & 255; return (NUMBER);
0[0-7]+			yylval = lexnoct (); return (NUMBER);
0x[0-9A-Fa-f]+		yylval = lexnhex (); return (NUMBER);
[0-9]+			yylval = atoi (yytext); return (NUMBER);
,			return (COMMA);
;			return (SEMI);
:			return (COLON);
[ \t\r]			/* Ignore white space */
\n			++lexline;	/* Update the line number */
.			parserr ("Illegal character");
%%

/* Initialise the lexical analysis for a file that is not stdin */
void	initlex (file)
FILE	*file;
{
  lexline = 1;		/* Send the line counter back to the start */
  yyrestart (file);	/* Restart the lexical analyser */
} /* initlex */

/* Translate an escape character code into an actual code */
static	int	lexesc (ch)
char	ch;
{
  switch (ch)
    {
      case 'n': return ('\n');
      case 'r': return ('\r');
      case 'b': return ('\b');
      case 't': return ('\t');
      case '0': return ('\0');
      default:  return (ch);
    } /* switch */
} /* lexesc */

/* Translate an octal character code into character */
static	int	lexoct (posn)
int	posn;
{
  return (((yytext[posn] - '0') << 6) |
          ((yytext[posn + 1] - '0') << 3) |
	   (yytext[posn + 2] - '0'));
} /* lexoct */

/* Analyse the string in 'yytext' to squash it */
/* Returns the length of the resultant string. */
static	int	lexstring (term)
char	term;
{
  int posn=1;		/* start at 1 to skip initial '"' */
  int squash=0;		/* Where to squash string into */
  char ch;
  while ((ch = yytext[posn]) != term && ch != '\n')
    {
      if (ch == '\\')	/* Escaped character? */
        {
	  if (isdigit (yytext[posn + 1]))
	    {
	      if (isdigit (yytext[posn + 2]) &&
	          isdigit (yytext[posn + 3]))
		{
		  yytext[squash++] = lexoct (posn + 1);
		  posn += 3;
		} /* then */
	       else
	        yytext[squash++] = lexesc (yytext[++posn]);
	    } /* then */
	   else
	    yytext[squash++] = lexesc (yytext[++posn]);
	} /* then */
       else
        yytext[squash++] = ch;
      ++posn;
    } /* while */
  if (ch != term)
    parserr ("Unterminated string constant");
  yytext[squash] = '\0';
  return (squash);	/* Return the length of the string */
} /* lexstring */

/* Analyse the '-delimited string in yytext to get a character */
static	int	lexchar ()
{
  if (lexstring ('\'') != 1)
    {
      parserr ("Character constant too long");
      return (DEF_CHAR_CONST);	/* Default character constant */
    }
   else
    return (yytext[0]);		/* Return the character at string start */
}

/* Convert an octal number into decimal and return it */
static	int	lexnoct ()
{
  int number,index;
  number = 0;
  index = 0;
  while (yytext[index])
    number = (number * 8) + (yytext[index++] - '0');
  return (number);
}

/* Convert a hexadecimal number (starting with "0x") */
/* into decimal and return it.			     */
static	int	lexnhex ()
{
  int number,index;
  number = 0;
  index = 2;
  while (yytext[index])
    {
      number *= 16;
      if (isdigit (yytext[index]))
        number += yytext[index++] - '0';
       else
        number += ((yytext[index++] - 'A') & 0x0F) + 10;
    }
  return (number);
}