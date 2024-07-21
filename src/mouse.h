//-------------------------------------------------------------------------
//
// MOUSE.H - Declarations for handling the mouse events.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1991  Rhys Weatherley
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 1, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Revision History:
// ================
//
//  Version  DD/MM/YY  By  Description
//  -------  --------  --  --------------------------------------
//    1.0    08/06/91  RW  Original Version of MOUSE.H
//    1.1    14/12/91  RW  Add Windows 3.0 button values.
//
//-------------------------------------------------------------------------

#ifndef __MOUSE_H__
#define	__MOUSE_H__

#include "client.h"		// Client declarations.

//
// Define constants for the mouse buttons.  Note that the
// middle mouse button is not supported by UW/PC.
//
#ifdef	UWPC_DOS
#define	MOUSE_LEFT	1
#define	MOUSE_RIGHT	2
#else	/* UWPC_DOS */
#define	MOUSE_LEFT	MK_LBUTTON
#define	MOUSE_RIGHT	MK_RBUTTON
#endif	/* UWPC_DOS */

//
// The following variable will be non-zero when the mouse status
// has been detectably changed.
//
extern	int	MouseChange;

//
// The following variable will be non-zero when the mouse is active.
//
extern	int	MouseActive;

//
// Initialise the mouse to be used by UW/PC.  This routine should
// be called after the screen has been initialised.  It returns
// a non-zero value if the mouse could be initialised, zero otherwise.
//
int	InitMouse	(void);

//
// Terminate the handling of the mouse.
//
void	TermMouse	(void);

//
// Send the current mouse status to the given client.  This may
// be called any time after initialisation, but usually after
// "MouseChange" has been set.
//
void	SendMouseEvent	(UWClient *client);

//
// Display the mouse if it is not already visible.
//
void	ShowMouse	(void);

//
// Remove the mouse from the screen.
//
void	HideMouse	(void);

#endif	/* __MOUSE_H__ */
