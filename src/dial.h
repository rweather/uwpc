//-------------------------------------------------------------------------
//
// DIAL.H - Declarations for the dialing directory.
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1990-1992  Rhys Weatherley
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
//    1.0    05/04/92  RW  Original Version of DIAL.H
//
//-------------------------------------------------------------------------

#ifndef __DIAL_H__
#define	__DIAL_H__

//
// Popup the dialing directory so the user can select a number.
// If there are no dialing directory entries, then just do a quick dial.
//
void	DialingDirectory (UWDisplay *wind);

#endif	/* __DIAL_H__ */
