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

/* Protocol 2 specific definitions and macros */

#define	P2_FN_WOPT	0040	/*	communicate window options */

/* Window option declarations */

#define	WOG_END		0	/* end of options */
#define	WOG_VIS		1	/* visibility */
#define	WOG_TYPE	2	/* window emulation type */
#define	WOG_POS		3	/* window position on screen */
#define	WOG_TITLE	4	/* window title */
#define	WOG_SIZE	5	/* window size (in pixels) */
#define	WOG_6		6	/* reserved */
#define	WOG_7		7	/* reserved */

#define	WOTTY_SIZE	8	/* (row,col) terminal size */
#define	WOTTY_FONTSZ	9	/* font size index */
#define	WOTTY_MOUSE	10	/* mouse interpretation */
#define	WOTTY_BELL	11	/* audible, visible bell */
#define	WOTTY_CURSOR	12	/* cursor shape */

/* Window option management commands */

#define	WOC_SET		0	/* change value of option */
#define	WOC_INQUIRE	2	/* ask about current option value */
#define	WOC_DO		4	/* do report changes to option */
#define	WOC_DONT	5	/* don't report changes to option */
#define	WOC_WILL	6	/* will report changes to option */
#define	WOC_WONT	7	/* won't report changes to option */

/* Window option encoding and decoding declarations */

#define	WONUM_MIN	 1		/* minimum option number */
#define	WONUM_GENERIC	 7		/* maximum generic option number */
#define	WONUM_SHORT	 14		/* maximum short option number */
#define	WONUM_MAX	 31		/* maximum option number */
#define	WONUM_MASK	 (017<<3)	/* mask for option extraction */
#define	WONUM_SENCODE(n) (((n)&017)<<3)	/* short encoding function */
#define	WONUM_SDECODE(c) (((c)>>3)&017)	/* short decoding function */
#define	WONUM_LPREFIX	 (017<<3)	/* long encoding prefix */
#define	WONUM_LENCODE(n) ((n)+' ')	/* long encoding function */
#define	WONUM_LDECODE(c) (((c)&0177)-' ') /* long decoding function */
#define	WONUM_USELONG(n) ((n)>WONUM_SHORT) /* test if long encode to be used */
#define	WONUM_COMMAND(c) ((c)&07)	/* extract option command */

/* Define the standard UW terminal types */

#define	UWT_UNKNOWN	-1	/* Used by UW/PC for Protocol 1 */
#define	UWT_ADM31	0	/* ADM-31 cursor-addressible terminal */
#define	UWT_VT52	1	/* VT52 cursor-addressible terminal */
#define	UWT_ANSI	2	/* ANSI-compatible terminal */
#define	UWT_TEK4010	3	/* Tektronix 4010 graphics terminal */
#define	UWT_FTP		4	/* File transfer window */
#define	UWT_PRINT	5	/* Output to printer */
#define	UWT_PLOT	6	/* Plot window */
#define	UWT_NOTUW	127	/* Code for type not supported by UW */

#endif	/* __UWPROTO_H__ */
