/*-------------------------------------------------------------------------

  UWLIB.H - Declarations for the UW Programmers library on the PC side.

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
     1.0    31/12/90  RW  Original Version of UWLIB.H
     1.1    01/01/90  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#ifndef __UWLIB_H__
#define	__UWLIB_H__

/* Force C calling conventions */
#ifdef	__STDC__
#define	_Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Define the window emulation types */
typedef	enum	{
		  UWT_ADM31	= 0,	/* ADM31 terminal emulation */
		  UWT_VT52	= 1,	/* VT52 terminal emulation */
		  UWT_ANSI	= 2,	/* ANSI terminal emulation - unused */
		  UWT_TEK4010	= 3,	/* TEK4010 - unused */
		  UWT_FTP	= 4,	/* File transfer - unused */
		  UWT_PRINT	= 5	/* Printer - unused */
		} uwtype_t;

#ifdef	__cplusplus
}
#endif

#endif	/* __UWLIB_H__ */
