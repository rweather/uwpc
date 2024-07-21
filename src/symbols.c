/*-------------------------------------------------------------------------

  SYMBOLS.C - Symbol manipulation routines for the Termcap Compiler.
 
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
     1.0    03/04/91  RW  Original Version of SYMBOLS.C

-------------------------------------------------------------------------*/

#include "symbols.h"		/* Declarations for this module */
#include <string.h>

char	*malloc	();

/*
 * Declare the structure of the symbol table.
 */
#define	MAX_SYMBOLS	500
struct	symbol	{
		  char *name;	/* Name of the symbol */
		  int	ref;	/* First reference in chain */
		  int	posn;	/* Position of label definition */
		};
static	struct	symbol	SymbolTable[MAX_SYMBOLS];
static	int		NumSymbols=0;
static	int		ScanTable=0;

/*
 * Add a new identifier to the symbol table and return its index.
 * Returns -1 if the symbol table is full.
 */
int	addident (ident)
char	*ident;
{
  int index=0;
  while (index < NumSymbols && strcmp (ident,SymbolTable[index].name))
    ++index;
  if (index >= NumSymbols)
    {
      if (NumSymbols >= MAX_SYMBOLS)
        return (-1);		/* Symbol table is full */
      SymbolTable[NumSymbols].name = malloc (strlen (ident) + 1);
      if (SymbolTable[NumSymbols].name == 0)
        return (-1);		/* Not enough memory for symbol */
      index = NumSymbols;
      strcpy (SymbolTable[NumSymbols].name,ident);
      SymbolTable[NumSymbols].posn = -1;
      SymbolTable[NumSymbols++].ref = 0;
    } /* if */
  return (index);
} /* addident */

/*
 * Get the name of an identifier.
 */
char	*getname (ident)
int	ident;
{
  return (SymbolTable[ident].name);
} /* getname */

/*
 * Add the position for an identifier to the symbol table.
 */
void	addposn (ident,address)
int	ident,address;
{
  SymbolTable[ident].posn = address;
} /* addposn */

/*
 * Get the position for an identifier (-1 if undefined).
 */
int	getposn (ident)
int	ident;
{
  return (SymbolTable[ident].posn);
} /* getposn */

/*
 * Add a new reference to the symbol table for a particular
 * identifier index.
 */
void	addref	(ident,address)
int	ident,address;
{
  SymbolTable[ident].ref = address;
} /* addref */

/*
 * Get the current reference in the symbol table for an identifier.
 */
int	getref	(ident)
int	ident;
{
  return (SymbolTable[ident].ref);
} /* getref */

/*
 * Get the first reference address for the first symbol
 * table entry.  Returns the address or -1 at the table's end.
 */
int	firstref ()
{
  ScanTable = 0;
  while (ScanTable < NumSymbols && !SymbolTable[ScanTable].ref)
    ++ScanTable;
  if (ScanTable < NumSymbols)
    return (ScanTable);
   else
    return (-1);
} /* firstref */

/*
 * Get the next reference address from the symbol table.
 */
int	nextref	()
{
  ++ScanTable;
  while (ScanTable < NumSymbols && !SymbolTable[ScanTable].ref)
    ++ScanTable;
  if (ScanTable < NumSymbols)
    return (ScanTable);
   else
    return (-1);
} /* nextref */
