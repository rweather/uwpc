//-------------------------------------------------------------------------
//
// EXTERN.H - External DOS and Windows 3.0 declarations for UW/PC.
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
//    1.0    05/05/91  RW  Original Version of EXTERN.H
//
//-------------------------------------------------------------------------

#ifndef __EXTERN_H__
#define	__EXTERN_H__

#if !defined(_Windows)

// Define the declarations necessary for the DOS version of UW/PC //
#define	UWPC_DOS	1

#else /* _Windows */

// Define the declarations necessary for the Windows 3.0 version of UW/PC //
#define	UWPC_WINDOWS	1

#include <windows.h>

#endif /* _Windows */

#endif	/* __EXTERN_H__ */
