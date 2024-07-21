//-------------------------------------------------------------------------
//
// DIALDOS.CPP - Declarations for the dialing directory under DOS.
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
//    1.0    05/04/92  RW  Original Version of DIALDOS.CPP
//
//-------------------------------------------------------------------------

#include <dos.h>
#include "comms.h"
#include "config.h"
#include "uw.h"
#include "display.h"
#include "dialog.h"
#include "dial.h"

#define	LINES(w)	((w) -> height)
#define	COLS(w)		((w) -> width)

//
// Change the dialing comms parameters.
//
static	void	SetDialParams (int params)
{
  if (params != -1)
    {
      comparams (UWConfig.ComPort,params);
      DELAY_FUNC (100);
      UWConfig.ComParams = params;
      UWConfig.makeparams ();
      UWMaster.status ();
    } /* if */
} // SetDialParams //

//
// Dial a number from a dial string.  Anything after '|' is ignored.
//
static	void	DialANumber (char *str)
{
  int index = 0;
  while (str[index] && str[index] != '|')
    ++index;
  if (str[index])
    {
      // Strip the comment, send the string, and put the comment back on //
      str[index] = '\0';
      UWMaster.sendstring (str);
      str[index] = '|';
    } /* then */
   else
    UWMaster.sendstring (str);
} // DialANumber //

//
// Declare the structure of the dialog box to display the dialing directory.
//
class	UWDialDir : public UWDialogBox {

private:

	// Display a dialing directory line.
	void	dirline (int index,int y,char *line);

public:

	UWDialDir (UWDisplay *wind,int numlines);

	virtual	void	key	(int keypress);

};

UWDialDir::UWDialDir (UWDisplay *wind,int numlines) :
	UWDialogBox (wind,(COLS(wind) - 60) / 2,
			  (LINES(wind) - numlines - 1) / 2,
			  ((COLS(wind) - 60) / 2) + 60,
			  ((LINES(wind) - numlines - 1) / 2) + numlines + 1)
{
  int temp,line;
  showstring ((COLS(wind) - 23) / 2,(LINES(wind) - numlines - 1) / 2,
  		" Select Number to Dial ");
  line = 0;
  if (UWConfig.DialString[0])
    dirline (0,line++,UWConfig.DialString);
  for (temp = 0;temp < NUM_DIAL_STRINGS;++temp)
    {
      if (UWConfig.DialNumber[temp][0])
        dirline (temp + 1,line++,UWConfig.DialNumber[temp]);
    } /* for */
} // UWDialDir::UWDialDir //

// Display a dialing directory line.
void	UWDialDir::dirline (int index,int y,char *line)
{
  char number[3];
  int temp;
  if (index == 0)
    number[0] = 'Q';
   else if (index < 10)
    number[0] = index + '0';
   else
    number[0] = index - 10 + 'A';
  number[1] = ':';
  number[2] = '\0';
  showstring (dx1 + 2,dy1 + 1 + y,number,1);
  temp = 0;
  while (line[temp] && line[temp] != '|')
    ++temp;
  if (line[temp])
    {
      line[temp] = '\0';
      showstring (dx1 + 5,dy1 + 1 + y,line);
      showstring (dx1 + 19,dy1 + 1 + y," ");
      showstring (dx1 + 20,dy1 + 1 + y,line + temp + 1);
      line[temp] = '|';
    } /* then */
   else
    {
      showstring (dx1 + 5,dy1 + 1 + y,line);
      if (!index)
        showstring (dx1 + 19,dy1 + 1 + y," (Quick Dial String)");
    } /* else */
} // UWDialDir::dirline //

void	UWDialDir::key (int keypress)
{
  int index = -1;
  if (keypress == 'q' || keypress == 'Q')
    index = 0;
   else if (keypress >= '1' && keypress <= '9')
    index = keypress - '0';
   else if (keypress >= 'A' && keypress <= 'Z')
    index = keypress - 'A' + 10;
   else if (keypress >= 'a' && keypress <= 'z')
    index = keypress - 'a' + 10;
   else if (keypress == 033)
    terminate ();
  if (index >= 0 && index <= NUM_DIAL_STRINGS &&
      (index < 1 || UWConfig.DialNumber[index - 1][0]))
    {
      // An entry has been selected: output it //
      terminate ();
      if (index == 0)
        DialANumber (UWConfig.DialString); // Quick dial selected.
       else
        {
	  // Output a number surrounded by the dialing prefix and suffix //
	  SetDialParams (UWConfig.DialParams[index - 1]);
	  UWMaster.sendstring (UWConfig.DialPrefix);
	  DialANumber (UWConfig.DialNumber[index - 1]);
	  UWMaster.sendstring (UWConfig.DialSuffix);
	} /* else */
    } /* if */
} // UWDialDir::key //

//
// Popup the dialing directory so the user can select a number.
// If there are no dialing directory entries, then just do a quick dial.
//
void	DialingDirectory (UWDisplay *wind)
{
  int temp,numlines;
  numlines = 0;
  for (temp = 0;temp < NUM_DIAL_STRINGS;++temp)
    {
      if (UWConfig.DialNumber[temp][0])
        ++numlines;
    } /* for */
  if (!numlines)
    DialANumber (UWConfig.DialString);	// Do a quick dial.
   else
    new UWDialDir (wind,numlines + (UWConfig.DialString[0] ? 1 : 0));
} // DialingDirectory //
