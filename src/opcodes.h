/*-------------------------------------------------------------------------

  OPCODES.H - Opcodes for the Termcap Compiler's abstract machine.
 
    This file is part of the Termcap Compiler and UW/PC source code.
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
     1.0    24/03/91  RW  Original Version of OPCODES.H
     1.1    10/12/91  RW  Add support for secondary key tables
     			  and some extra stuff for VT100.

-------------------------------------------------------------------------*/

#ifndef __OPCODES_H__
#define	__OPCODES_H__

/*
 * Define the version of the terminal descriptions that are supported
 * by the UW/PC program that uses them.  Only descriptions with versions
 * between 0x100 and this number will be accepted.  This should roughly
 * correspond to the current version of TERMCC.
 */
#define	UW_TERM_VERSION		0x102

/*
 * Define the size of the terminal description header.  This is the
 * offset to find the name of the terminal emulation and the base
 * of the jump labels.
 */
#define	TERM_HEADER_SIZE	6

/*
 * Define the opcodes that can appear in a Termcap program.  To
 * ensure backwards compatibility of terminal descriptions, only
 * add opcodes to the end of this list.
 */
enum   TermcapOpcodes {
	OP_SEND= 1,OP_SEND52,OP_CR,OP_LF,OP_BS,OP_BSWRAP,OP_MOVE,
	OP_CLEAR,OP_CLREOL,OP_CLREOS,OP_CLRSOL,OP_CLRSOS,OP_INSLINE,
	OP_DELLINE,OP_INSCHAR,OP_DELCHAR,OP_SETATTR,OP_SETATTR_ACC,
	OP_SETSCRL,OP_SETSCRL_ACC,OP_GETCH,OP_SETX,OP_SETY,OP_LOAD,
	OP_LOAD_WORD,OP_LOAD_WIDTH,OP_LOAD_HEIGHT,OP_ADD,OP_ADD_WORD,
	OP_ADD_WIDTH,OP_ADD_HEIGHT,OP_SUB,OP_SUB_WORD,OP_SUB_WIDTH,
	OP_SUB_HEIGHT,OP_CMP,OP_CMP_WORD,OP_CMP_WIDTH,OP_CMP_HEIGHT,
	OP_SWITCH,OP_SWITCH_WORD,OP_JE,OP_JNE,OP_JA,OP_JAE,OP_JB,
	OP_JBE,OP_JMP,OP_JSR,OP_RET,OP_REMOTE,OP_ESCAPE,OP_ESCAPE_WORD,
	OP_TAB,OP_BELL,OP_SAVEXY,OP_RESTXY,OP_GETXY,OP_GETX,OP_GETY,
	OP_SCRLUP,OP_SCRLDN,OP_SET,OP_RESET,OP_TEST,OP_RESARR,OP_GETARG,
	OP_GETA,OP_DEC,OP_SHIFT,OP_SETC,OP_SAVEATTR,OP_RESTATTR,
	OP_INSBLANK,OP_CLIENT,OP_KEYTAB,OP_KEYTAB_NONE,OP_TABND,
	OP_REVLF
};

#endif	/* __OPCODES_H__ */
