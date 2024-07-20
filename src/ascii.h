/*-------------------------------------------------------------------------

  ASCII.H - ASCII file transfer routines for UW/PC.

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
     1.0    26/01/91  RW  Original Version of ASCII.H

-------------------------------------------------------------------------*/

#ifndef __ASCII_H__
#define	__ASCII_H__

#include "uw.h"			/* Primary UW declarations */

/* Force C calling conventions */
#ifdef	__STDC__
#define _Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Start an ASCII send of a file in the given window */
void	_Cdecl	AsciiSend	(int window,char *filename);

/* Start an ASCII receive of a file in the given window */
void	_Cdecl	AsciiReceive	(int window,char *filename,int append);

#ifdef	__cplusplus
}
#endif

#endif	/* __ASCII_H__ */
