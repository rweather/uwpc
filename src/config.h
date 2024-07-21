//-------------------------------------------------------------------------
//
// CONFIG.H - Configuration objects for UW/PC.
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
//    1.0    23/03/91  RW  Original Version of CONFIG.H
//    1.1    08/12/91  RW  Add international language support.
//    1.2    05/04/92  RW  Add dialing directory.
//
//-------------------------------------------------------------------------

#ifndef __CONFIG_H__
#define	__CONFIG_H__

//
// Define the available status line positions.
//
#define	STATUS_LEFT		0
#define	STATUS_RIGHT		1
#define	STATUS_CENTRE		2
#define	STATUS_LEFT_SQUASH	3
#define	STATUS_RIGHT_SQUASH	4
#define	STATUS_CENTRE_SQUASH	5

//
// Define the values for UWConfig.ObeyTerm.
//
#define	OBEY_IGNORE		0
#define	OBEY_ALWAYS		1
#define	OBEY_NOTFIRST		2

//
// Define the shell kinds for the "fixterm" option.
//
#define	SHELL_NONE		0
#define	SHELL_BOURNE		1
#define	SHELL_CSHELL		2
#define	SHELL_STRING		3

//
// Define the configuration class.  All configuration information
// for UW/PC is stored in the object "UWConfig" of this class.
//
#define	STR_LEN		101
#define	MAX_DESCS	5
#define	FONT_STR_LEN	16
#define	MAX_CHARS	256
#define	NUM_DIAL_STRINGS 20
#define	DIAL_STR_LEN	51
class	UWConfiguration {

private:

	int	linenum;		// Configuration line number.

	// Abort the configuration with an error.
	void	error	(char *msg);

	// Abort the configuration with an "illegal" config message.
	void	illegal	(char *msg);

	// Load a particular language into memory.
	void	loadlang (char *language);

	// Load a new terminal description from the given filename.
	void	*LoadTerminal (char *filename);

	// Find a terminal description in the loaded descriptions.
	// Returns NULL if the terminal type could not be found.
	unsigned char far *FindTerminal (char *type);

	// Parse a dialing directory comms parameter spec.  Returns
	// -1 if the string was illegal (it also generates an error msg).
	int	parsecom (char *str);

	// Process a configuration line.
	void	processline (char *line);

public:

	int	ComPort;		// COM port to be used in UW/PC.
	int	ComParams;		// Communication parameters.
	int	ComCtsRts;		// Non-zero for CTS/RTS handshaking.
	int	ComFossil;		// Non-zero to use a FOSSIL driver.
	int	ComEstBaud;		// Baud rate estimation for DSZ.
	int	ComDirect;		// Non-zero for a direct serial line.
	int	StripHighBit;		// Non-zero for high bit strip.
	char	DeviceParameters[16];	// String description of ComParams.
	int	DisableStatusLine;	// Non-zero to disable status line.
	unsigned char far *P0TermType;	// Protocol 0 terminal type.
	unsigned char far *P1TermType;	// Protocol 1 terminal type.
	char	InitString[STR_LEN];	// Initialisation string.
	char	HangupString[STR_LEN];	// Modem string for hangup.
	int	CarrierInit;		// Non-zero to send init with carrier.
	int	XonXoffFlag;		// Non-zero for encoded XON/XOFF.
	int	BeepEnable;		// Non-zero to enable beep.
	int	SwapBSKeys;		// Non-zero to swap DEL and BS.
	char	DialString[DIAL_STR_LEN]; // String to send for dial.
	char	DialPrefix[DIAL_STR_LEN]; // Prefix for dialing a number.
	char	DialSuffix[DIAL_STR_LEN]; // Suffix for dialing a number.
	char	DialNumber[NUM_DIAL_STRINGS][DIAL_STR_LEN]; // Dialing dir.
	int	DialParams[NUM_DIAL_STRINGS]; // New comms parameters in dir.
	char	CommandString[STR_LEN];	// String to send for "uw" command.
	char	FKeys[10][STR_LEN];	// Function key definitions.
	char	StatusFormat[STR_LEN];	// Format of the status line.
	int	StatusPosn;		// Position of the status line.
	char	FtpString[STR_LEN];	// String to send for "uwftp" command.
	void	*TermDescs[MAX_DESCS];	// New terminal descriptions.
	char	TermFiles[MAX_DESCS][STR_LEN]; // Filenames of descriptions.
	int	NumTermDescs;		// Number of new descriptions.
	unsigned char NewAttrs[5];	// The new screen attributes.
	int	PopUpNewWindow;		// Non-zero to pop-up new windows.
	int	DisableUW;		// Non-zero to disable server.
	char	ZModemCommand[STR_LEN];	// Name of the ZModem program.
	int	EnableMouse;		// Non-zero if mouse is enabled.
	char	MailBoxName[STR_LEN];	// Name of the user's mailbox.
	char	Password[STR_LEN];	// Password for login client.
	char	MailString[STR_LEN];	// Name of the "uwmail" command.
	int	CursorSize;		// Shape of the screen cursor.
	char	FontFace[FONT_STR_LEN];	// Name of font for Windows 3.0.
	int	FontHeight;		// Height of the font for Windows 3.0.
	unsigned char FontCharSet;	// Character set for Windows 3.0.
	char	KeyTransTable[MAX_CHARS]; // Table to translate keys.
	char	PrintTransTable[MAX_CHARS]; // Translation table for printing.
	char	TransFile[STR_LEN];	// Name of translation table file.
	int	NewTransFile;		// Non-zero if not default trans file.
	char	Language[STR_LEN];	// Foreign language being used.
	int	BigVideo;		// Non-zero for 43/50 line mode.
	int	BellFreq;		// Frequency of the terminal bell.
	int	BellDur;		// Duration of the terminal bell (ms).
	int	AnsiBright;		// Non-zero for bright ANSI colours.
	int	ObeyTerm;		// Terminal override mode.
	int	MaxProtocol;		// Maximum multi-window protocol.
	int	MaxTitleLen;		// Maximum displayed title length.
	int	ShellKind;		// Kind of shell for "fixterm" option.
	char	ShellString[STR_LEN];	// String for ShellKind==SHELL_STRING.

	UWConfiguration (void);		// Set the defaults.

	int	doconfig (char *argv0);	// Do the configuration.

	// Process ComParams and create the DeviceParameters string.
	void	makeparams (void);

	// Get a terminal emulation for a particular terminal type.
	unsigned char far *getterminal (int termtype);

	// Dump the configuration settings out to UW.CFG.  The old
	// configuration file is saved in UWCFG.OLD.
	int	dumpconfig (char *argv0);

};

//
// Define the main UW/PC configuration object.
//
extern	UWConfiguration	UWConfig;

#endif	/* __CONFIG_H__ */
