/*-------------------------------------------------------------------------

  CONFIG.H - Configuration declarations for the UW client.
 
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
     1.0    28/12/90  RW  Original Version of CONFIG.H
     1.1    01/01/90  RW  Clean up and remove __PROTO__

-------------------------------------------------------------------------*/

#ifndef __CONFIG_H__
#define	__CONFIG_H__

/* Force C calling conventions */
#ifdef	__STDC__
#define	_Cdecl
#else
#define	_Cdecl	cdecl
#endif

#ifdef	__cplusplus
extern	"C" {
#endif

/* Define the base name of the configuration file.  The current */
/* directory is tried first, and then the program directory.	*/
/* This variable is defined in the MAIN module of the program.	*/
extern	char	*_Cdecl	ConfigFile;

/* Define the status of the XON/XOFF keys.  If this variable is */
/* non-zero the XON and XOFF characters are sent direct.	*/
extern	int	_Cdecl	XonXoffDirect;

/* Read the configuration file and set the configurable parameters  */
/* Will abort the program if a configuration error is detected.	The */
/* program name is supplied so the configuration file can be found. */
void	_Cdecl	Configure	(char *progname);

#ifdef	__cplusplus
}
#endif

#endif	/* __CONFIG_H__ */
