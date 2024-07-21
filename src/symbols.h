/*-------------------------------------------------------------------------

  SYMBOLS.H - Symbol manipulation routines for the Termcap Compiler.
 
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
     1.0    03/04/91  RW  Original Version of SYMBOLS.H

-------------------------------------------------------------------------*/

#ifndef __SYMBOLS_H__
#define	__SYMBOLS_H__

/*
 * Add a new identifier to the symbol table and return its index.
 * Returns -1 if the symbol table is full.
 */
int	addident ( /* char *ident */ );

/*
 * Get the name of an identifier.
 */
char	*getname ( /* int ident */ );

/*
 * Add the position for an identifier to the symbol table.
 */
void	addposn ( /* int ident,int address */ );

/*
 * Get the position for an identifier (-1 if undefined).
 */
int	getposn ( /* int ident */ );

/*
 * Add a new reference to the symbol table for a particular
 * identifier index.
 */
void	addref	( /* int ident,int address */ );

/*
 * Get the current reference in the symbol table for an identifier.
 */
int	getref	( /* int ident */ );

/*
 * Get the first reference address for the first symbol
 * table entry.  Returns the address or -1 at the table's end.
 */
int	firstref ( /* void */ );

/*
 * Get the next reference address from the symbol table.
 */
int	nextref	( /* void */ );

#endif	/* __SYMBOLS_H__ */
