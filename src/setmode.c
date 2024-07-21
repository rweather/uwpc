/*-------------------------------------------------------------------------

  SETMODE.C - A program for setting the current screen mode.
 
    This file is part of the UW/PC project.
    Copyright (C) 1992  Rhys Weatherley

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
     1.0    14/03/92  RW  Original Version of SETMODE.C

-------------------------------------------------------------------------*/

#include <conio.h>
#include <stdlib.h>
#include <stdio.h>

int	main (argc,argv)
int	argc;
char	*argv[];
{
  if (argc != 2)
    fprintf (stderr,"Usage: setmode mode\n");
   else
    textmode (atoi (argv[1]));
  return (0);
} /* main */
