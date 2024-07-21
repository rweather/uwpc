/*-------------------------------------------------------------------------

  UWPROTO.H - Special declarations for the Unix Windows protocol.

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
     1.0    15/12/90  RW  Original Version of UWPROTO.H
     1.1    03/03/91  RW  Add protocol negotiation definitions.

-------------------------------------------------------------------------*/

#ifndef __UWPROTO_H__
#define	__UWPROTO_H__

/* Protocol 1 definitions and macros */

#define	P1_IAC		0001	/* interpret following byte as a command */
#define P1_DIR		0100	/* command direction: */
#define P1_DIR_HTOC	0000	/*	host to client */
#define	P1_DIR_CTOH	0100	/*	client to host */

#define	P1_FN		0070	/* function code: */
#define	P1_FN_ARG	0007	/*	extract function argument */
#define P1_FN_NEWW	0000	/*	create new window */
#define	P1_FN_KILLW	0010	/*	kill (destroy) window */
#define	P1_FN_ISELW	0020	/*	select window for input data */
#define	P1_FN_OSELW	0030	/*	select window for output data */
#define P1_FN_META	0050	/*	add META to next data character */
#define P1_FN_CTLCH	0060	/*	send control character as data */
#define	P1_FN_MAINT	0070	/*	perform "maintenance function" */

#define	P1_CC		0007	/* control character specifier: */
#define P1_CC_IAC	0001	/*	P1_IAC (001) */
#define	P1_CC_XON	0002	/*	P1_XON (021) */
#define	P1_CC_XOFF	0003	/*	P1_XOFF (023) */

#define P1_MF		0007	/* maintenance functions: */
#define	P1_MF_ENTRY	0000	/*	start up */
#define P1_MF_ASKPCL	0002	/*	request protocol negotiation */
#define P1_MF_CANPCL	0003	/*	suggest protocol */
#define P1_MF_SETPCL	0004	/*	set new protocol */
#define	P1_MF_EXIT	0007	/*	exit */

#endif	/* __UWPROTO_H__ */
