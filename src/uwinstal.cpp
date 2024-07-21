//-------------------------------------------------------------------------
//
// UWINSTAL.CPP - Installation program for UW/PC.  This program uses
//		  Turbo Vision to make it look trendy. :-)
// 
//  This file is part of UW/PC - a multi-window comms package for the PC.
//  Copyright (C) 1992  Rhys Weatherley
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
//    1.0    07/04/92  RW  Original Version of UWINSTALL.CPP
//
//-------------------------------------------------------------------------

#pragma	warn	-par

#define Uses_TKeys
#define Uses_TApplication
#define Uses_TEvent
#define Uses_TRect
#define Uses_TDialog
#define Uses_TStaticText
#define Uses_TButton
#define Uses_TMenuBar
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#define Uses_TDeskTop
#define	Uses_TCheckBoxes
#define	Uses_TSItem
#define Uses_TLabel
#define Uses_TInputLine
#define	Uses_TTextDevice
#define Uses_TWindow
#define Uses_TTerminal
#define Uses_MsgBox
#define Uses_otstream
#include <tv.h>
#include <stdio.h>
#include <string.h>
#include <dir.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <alloc.h>
#include <fcntl.h>
#include <strstrea.h>
#include <fstream.h>
#include <conio.h>
#include <sys/stat.h>

//
// Define the installation flags.
//
#define	INST_MIN_FLAG		1	// Must be the smallest of the flags.
#define	INST_DOS_VERS		1
#define	INST_WIN_VERS		2
#define	INST_TERMCC		4
#define	INST_SETMODE		8
#define	INST_UWINSTAL		16
#define	INST_UWTERMS_OBJ	32
#define INST_UWTERMS_SRC	64
#define	INST_OTHER		128
#define	INST_CONFIG		256
#define	INST_SCRIPTS		512
#define	INST_MAX_FLAG		512	// Must be the largest of the flags.

//
// Declare the default installation options.
//
int	uwoptions = INST_DOS_VERS | INST_WIN_VERS | INST_TERMCC | INST_SETMODE
			| INST_SETMODE | INST_UWTERMS_OBJ;

//
// Declare the files to be copied for each of the installation options.
// Filenames that start with '!' need to be unzipped.  If UNZIP or PKUNZIP
// cannot be found, then the files are merely copied.
//
struct	uwprog	{
		  int	option;	// Option flag bit.
		  char	*files;	// Comma-separated list of files.
		  char	*desc;	// Description of files to copy.
		} uwprograms[] =
	{{INST_DOS_VERS,"UW.EXE",
		"DOS Version of UW/PC"},
	 {INST_WIN_VERS,"UW-W.EXE,UWFONT.FON",
	 	"Windows Version of UW/PC"},
	 {INST_CONFIG,"UW.DOC,UWEG.CFG,UW.LNG",
	 	"Configuration and Documentation files"},
	 {INST_TERMCC,"TERMCC.EXE,TERMCC.DOC",
	 	"Termcap Compiler and Documentation"},
	 {INST_SETMODE,"SETMODE.EXE",
	 	"Mode Setting Utility"},
	 {INST_UWINSTAL,"UWINSTAL.EXE",
	 	"UW/PC Installation Utility"},
	 {INST_UWTERMS_OBJ,"!UW-TERMS.ZIP:*.TRM",
	 	"Auxillary Terminal Emulations"},
	 {INST_UWTERMS_SRC,"!UW-TERMS.ZIP:*.CAP",
	 	"Auxillary Terminal Emulations (Source)"},
	 {INST_SCRIPTS,"SCRIPTS.TAR",
	 	"Unix Shell Scripts"},
	 {INST_OTHER,"README,COPYING,HISTORY",
	 	"Other Files"},
	 {0,NULL}};

//
// Declare the directory to install UW/PC in.
//
#define	STRING_LEN	128
char	UWInstDir[STRING_LEN];

//
// Declare the location of PKUNZIP or UNZIP so it can be used for UW-TERMS.
//
char	UWUnZipProg[STRING_LEN];

//
// Declare the directory to copy the files from.
//
char	UWSourceDir[STRING_LEN];

//
// Declare a buffer to build a command-line for "system" in.
//
char	UnZipBuffer[BUFSIZ];

const cmDoInstall = 100;	// Command constants for the Install menu.
const cmSetOptions = 101;
const cmAbout = 102;
const cmSetDest = 103;
const cmSetSource = 104;
const cmSetUnZip = 105;

class TInstallApp : public TApplication
{

public:

    TInstallApp();

    virtual void handleEvent( TEvent& event );
    static TMenuBar *initMenuBar( TRect );
    static TStatusLine *initStatusLine( TRect );
    virtual void idle ();

    void MessageBox (char *title,char **mesg);

private:

    void popupMenu();
    void optionBox();
    void directoryBox(char *title,char *prompt,char *string);
    void destBox() { directoryBox("Install Directory",
    				  "~D~irectory to install UW/PC in",
				  UWInstDir);
		     popupMenu(); };
    void sourceBox() { directoryBox("Source Directory",
    				    "~D~irectory to copy files from",
				    UWSourceDir);
		     popupMenu(); };
    void unzipBox() { directoryBox("Unzip",
    				   "~P~ath of Unzip program",
				   UWUnZipProg);
		     popupMenu(); };
    void copyWindow();

    int init;

};

TInstallApp	*uwapp;

static	char	*copyright[] =
    {"UWINSTAL version 1.00, Copyright (C) 1992 Rhys Weatherley",
     "UWINSTAL comes with ABSOLUTELY NO WARRANTY; see the file COPYING for",
     "details.  This is free software, and you are welcome to redistribute",
     "it under certain conditions; see the file COPYING for details.",
     NULL};

TInstallApp::TInstallApp() :
    TProgInit( &TInstallApp::initStatusLine,
               &TInstallApp::initMenuBar,
               &TInstallApp::initDeskTop
             )
{
  init = 0;
}

// Send a message to the application to pop-up the "Install" menu
// so it is displayed after any dialog box to remind the user to
// set the other options.
void TInstallApp::popupMenu()
{
  TEvent event;
  event.what = evKeyDown;
  event.keyDown.keyCode = kbAltI;
  handleEvent (event);
}

#define	setoptflag(opt,bit,cb,item) \
		((cb) -> mark (item) ? (opt) |= (bit) : (opt) &= ~(bit))

void TInstallApp::optionBox()
{
    // Build the dialog box for the installation options.
    TDialog *d = new TDialog( TRect( 19, 4, 62, 20 ), "Programs to Install" );
    TCheckBoxes *cb = new TCheckBoxes( TRect( 3, 3, 40, 12 ),
    			new TSItem( "~D~OS Version: UW.EXE",
			new TSItem( "~W~indows Version: UW-W.EXE",
			new TSItem( "~T~ermcap Compiler: TERMCC.EXE",
			new TSItem( "~M~ode Setter: SETMODE.EXE",
			new TSItem( "~I~nstaller: UWINSTAL.EXE",
			new TSItem( "~E~mulations: UW-TERMS.ZIP (TRM)",
			new TSItem( "Em~u~lations: UW-TERMS.ZIP (CAP)",
			new TSItem( "Uni~x~ Scripts: SCRIPTS.TAR",
			new TSItem( "~O~ther: README, COPYING, HISTORY",
			0))))))))));
    d -> insert (cb);
    d -> insert (new TLabel( TRect( 2, 2, 29, 3 ),
    			"~S~elect Programs to Install", cb ));
    d -> insert (new TButton ( TRect( 7, 13, 17, 15 ),
    			"~O~k", cmOK, bfDefault));
    d -> insert (new TButton ( TRect( 23, 13, 33, 15 ),
    			"~C~ancel", cmCancel, bfNormal));

    // Set the default checked boxes to all but the installer and other.
    ushort data = (ushort)uwoptions;
    d -> setData( &data );

    // Process the dialog box.
    if (deskTop->execView( d ) == cmCancel)
      {
        destroy (d);
	return;
      } /* if */

    // Copy the selected options back into the "uwoptions" variable.
    d -> getData (&data);
    uwoptions = (int)data;
    destroy (d);
}

void TInstallApp::directoryBox(char *title,char *prompt,char *string)
{
    // Build the dialog box for the directory selection.
    TDialog *d = new TDialog( TRect( 19, 4, 61, 12 ), title );
    TInputLine *b = new TInputLine (TRect (3,3,39,4), STRING_LEN);
    d -> insert (b);
    d -> insert (new TLabel( TRect( 2, 2, 2 + strlen (prompt), 3 ),
    			prompt, b ));
    d -> insert (new TButton ( TRect( 7, 5, 17, 7 ),
    			"~O~k", cmOK, bfDefault));
    d -> insert (new TButton ( TRect( 23, 5, 33, 7 ),
    			"~C~ancel", cmCancel, bfNormal));
    d -> setData( string );
    if (deskTop->execView( d ) == cmCancel)
      {
        destroy (d);
	return;
      } /* if */
    d -> getData ( string );
    strupr (string);	// Make string upper case.
    destroy (d);
}

void TInstallApp::MessageBox (char *title,char **mesg)
{
  // Find out the size of the message for creating the message box.
  int width,height,len,x,y;
  char **temp = mesg;
  width = 0;
  height = 0;
  while (*temp)
    {
      if ((len = strlen (*temp)) > width)
        width = len;
      ++height;
      ++temp;
    } /* while */
  width += 4;
  height += 6;
  x = (size.x - width) / 2;
  y = (size.y - height) / 2;

  // Build the dialog box for the message display.
  TDialog *d = new TDialog( TRect( x, y, x + width, y + height ), title );

  // Display the message within the message box.
  y = 2;
  while (*mesg)
    {
      d -> insert (new TStaticText( TRect( 2, y, 2 + strlen (*mesg), y + 1 ),
      				    *mesg ));
      ++y;
      ++mesg;
    } /* while */

  // Add an OK button to the message box.
  d -> insert (new TButton ( TRect( (width / 2) - 5, height - 3,
  				    (width / 2) + 5, height - 1 ),
				    "~O~k", cmOK, bfDefault));

  // Process the message box's messages.
  deskTop -> execView( d );
  destroy (d);
}

void TInstallApp::handleEvent( TEvent& event )
{
    TApplication::handleEvent( event );
    if( event.what == evCommand )
        {
        switch( event.message.command )
            {
	    case cmDoInstall:
	    	uwoptions &= ~INST_CONFIG;
		if (uwoptions & (INST_DOS_VERS | INST_WIN_VERS))
		  uwoptions |= INST_CONFIG;
	    	copyWindow();
	    	// popupMenu();
		clearEvent( event );
		break;
            case cmSetOptions:
                optionBox();
		popupMenu();
                clearEvent( event );
                break;
            case cmSetDest:
	    	destBox();
                clearEvent( event );
                break;
            case cmSetSource:
	    	sourceBox();
                clearEvent( event );
                break;
            case cmSetUnZip:
	    	unzipBox();
                clearEvent( event );
                break;
	    case cmAbout:
	    	MessageBox ("UW/PC Installer",copyright);
		popupMenu();
                clearEvent( event );
		break;
            default:
                break;
            }
        }
}

TMenuBar *TInstallApp::initMenuBar( TRect r )
{

    r.b.y = r.a.y+1;

    return new TMenuBar( r,
      *new TSubMenu( "~I~nstall", kbAltI ) +
        *new TMenuItem( "~S~ource Directory...", cmSetSource, kbAltS,
				hcNoContext, "Alt-S") +
        *new TMenuItem( "~D~estination Directory...", cmSetDest, kbAltD,
				hcNoContext, "Alt-D") +
        *new TMenuItem( "~P~rograms to Install...", cmSetOptions, kbAltP,
				hcNoContext, "Alt-P") +
        *new TMenuItem( "~U~nzip Program...", cmSetUnZip, kbAltU,
				hcNoContext, "Alt-U") +
         newLine() +
        *new TMenuItem( "~G~o and Install...", cmDoInstall, kbAltG,
				hcNoContext, "Alt-G") +
         newLine() +
        *new TMenuItem( "~A~bout...", cmAbout, kbNoKey ) +
         newLine() +
        *new TMenuItem( "E~x~it", cmQuit, cmQuit, hcNoContext, "Alt-X" )
        );

}

TStatusLine *TInstallApp::initStatusLine( TRect r )
{
    r.a.y = r.b.y-1;
    return new TStatusLine( r,
        *new TStatusDef( 0, 0xFFFF ) +
            *new TStatusItem( "~Alt-X~ Exit", kbAltX, cmQuit ) +
            *new TStatusItem( "~Alt-S~ Source", kbAltS, cmSetSource ) +
            *new TStatusItem( "~Alt-D~ Dest", kbAltD, cmSetDest ) +
            *new TStatusItem( "~Alt-P~ Programs", kbAltP, cmSetOptions ) +
            *new TStatusItem( "~Alt-U~ Unzip", kbAltU, cmSetUnZip ) +
            *new TStatusItem( "~Alt-G~ Go", kbAltG, cmDoInstall ) +
            *new TStatusItem( 0, kbF10, cmMenu )
            );
}

// Search the user's path for a file, but without the current
// directory like the standard Borland C++ searchpath does.
char	*newsearchpath (char *prog)
{
  static char checkpath[STRING_LEN];
  int index;

  // Get the text of the PATH variable.
  char *env = getenv ("PATH");
  if (!env)
    return (NULL);	// No path has been found.

  // Loop around checking each directory on the DOS path.
  while (*env)
    {
      // Skip semi-colons and white space in the path.
      if (*env == ';' || isspace (*env))
        {
	  ++env;
	  continue;
	} /* if */
      index = 0;
      while (*env && *env != ';' && !isspace (*env))
        checkpath[index++] = *env++;
      strcpy (checkpath + index,prog);
      if (access (checkpath,0) >= 0)
        return (checkpath);	// Found the file on the path.
    } /* while */
  return (NULL);		// Could not find the file on the path.
}

// Strip the filename componen from a pathname to leave the directory name.
void	StripFileName (char *str)
{
  int index = strlen (str);
  while (index > 0 && str[index - 1] != '\\' && str[index - 1] != '/')
    --index;
  if (index > 0)
    --index;
  str[index] = '\0';
}

// Get the default directories, pathnames, etc.
void	GetDefaults (char *progname)
{
  char *temp;

  // Determine the source directory name from the program name.
  strcpy (UWSourceDir,progname);
  StripFileName (UWSourceDir);

  // Try to find an UNZIP program to use on UW-TERMS
  // (and maybe UWPCXXXE.ZIP in a future version of UWINSTAL).
  if ((temp = searchpath ("PKUNZIP.EXE")) == NULL &&
      (temp = searchpath ("UNZIP.EXE")) == NULL &&
      (temp = searchpath ("UNZIP.COM")) == NULL)
    temp = "PKUNZIP.EXE";	// Use a default on a slim chance.
  strcpy (UWUnZipProg,temp);

  // See if UW/PC or UW/WIN are already somewhere on the path
  // and use that directory if they are.  Otherwise, just use C:\UW.
  if ((temp = newsearchpath ("UW.EXE")) == NULL &&
      (temp = newsearchpath ("UW-W.EXE")) == NULL &&
      (temp = newsearchpath ("TERMCC.EXE")) == NULL &&
      (temp = newsearchpath ("UWINSTAL.EXE")) == NULL &&
      (temp = newsearchpath ("UWWIN.EXE")) == NULL)
    temp = "C:\\UW\\";		// Select a default directory.
  strcpy (UWInstDir,temp);

  // Strip the final component off the found install directory.
  // i.e. remove the program name to get just the directory name.
  StripFileName (UWInstDir);
}

class TCopyWindow : public TWindow
{

public:

    TCopyWindow( TRect bounds, const char *winTitle );
    TTerminal *makeInterior( TRect bounds );
    virtual void handleEvent( TEvent& event );
    int docopy ();
    int donecopy;

private:

	TTextDevice *interior;
	int	copy_index;
	char	*copy_files;
	int	copy_srcfd,copy_destfd;
	int	copy_first;
	char	*source_file (char *path);
	char	*dest_file (char *path);
	long	source_posn;
	void	no_file (void)
		  {
		    interior -> do_sputn ("not found",9);
		  };
	void	file_error (void)
		  {
		    interior -> do_sputn ("error",5);
		  };

	void	InitCopy (void)
		  {
  		    copy_index = 0;
		    copy_files = "";
		    copy_srcfd = -1;
		    copy_destfd = -1;
		    copy_first = 1;
		    donecopy = 0;
		  }

};

TCopyWindow::TCopyWindow( TRect bounds,
                          const char *winTitle
                        ) :
    TWindowInit( &TCopyWindow::initFrame ),
    TWindow(bounds, winTitle, 0 )
{
    flags &= ~(wfMove | wfGrow | wfClose | wfZoom);
    interior = makeInterior( bounds );
    insert( interior );
    interior -> hideCursor ();
    mkdir (UWInstDir);
    InitCopy ();
}

TTerminal *TCopyWindow::makeInterior( TRect bounds )
{
    bounds = getExtent();
    bounds.grow( -1, -1 );
    return new TTerminal (bounds,0,
        standardScrollBar( sbVertical | sbHandleKeyboard ),
	8192);
}

TCopyWindow *copywin=NULL;

void	TCopyWindow::handleEvent( TEvent& event )
{
    if( event.what == evCommand )
        {
        switch( event.message.command )
            {
	    case cmClose:
	    	copywin = NULL;
		break;
            default:
                break;
            }
        }
    else if( event.what == evKeyDown )
        {
	switch( event.keyDown.keyCode )
	    {
	    case kbEnter:
	    	if (copy_srcfd < -1) donecopy = 1;
		break;
	    default:
	    	break;
	    }
	}
    TWindow::handleEvent( event );
}

void	TInstallApp::copyWindow()
{
  if (!copywin)
    {
      copywin = new TCopyWindow( TRect( 10, 4, 70, 18 ), "Copying Files" );
      deskTop -> insert (copywin);
    } /* if */
}

//
// Compute the name of a source file.  Returns NULL
// if the file does not exist.
//
char	*TCopyWindow::source_file (char *path)
{
  static char source[MAXPATH];
  char *end;
  strcpy (source,UWSourceDir);
  strcat (source,"\\");
  end = source + strlen (source);
  while (*path && *path != ':')
    *end++ = *path++;
  *end = '\0';
  if (access (source,0) < 0)
    return (NULL);
   else
    return (source);
}

//
// Compute the name of a destination file.
//
char	*TCopyWindow::dest_file (char *path)
{
  static char dest[MAXPATH];
  strcpy (dest,UWInstDir);
  strcat (dest,"\\");
  strcat (dest,path);
  return (dest);
}

int	TCopyWindow::docopy ()
{
  int len;
  static char basename[40];
  if (copy_srcfd < -1)
    return (1);
   else if (copy_srcfd < 0 && copy_files[0])
    {
      /* Find a new file to be copied in this string */
      len = 0;
      while (*copy_files && *copy_files != ',')
        basename[len++] = *copy_files++;
      basename[len] = '\0';
      if (*copy_files == ',')
        ++copy_files;
      if (basename[0] != '!')
        {
	  // Copy a plain file //
	  char *source,*dest;
	  interior -> do_sputn ("\n    ",5);
          interior -> do_sputn (basename,len);
	  interior -> do_sputn ("             ",13 - len);
	  if ((source = source_file (basename)) == NULL)
	    {
	      // File not found - just ignore it //
	      no_file ();
	    } /* then */
	   else
	    {
	      // Get the destination pathname and open the files //
	      dest = dest_file (basename);
	      if ((copy_srcfd = ::open (source,O_BINARY | O_RDONLY)) < 0)
		{
		  file_error ();
		  copy_srcfd = -1;
		  return (0);
		} /* if */
	      if ((copy_destfd = ::open (dest,O_BINARY | O_WRONLY |
				  	 O_CREAT | O_TRUNC,
					 S_IREAD | S_IWRITE)) < 0)
		{
		  file_error ();
		  ::close (copy_srcfd);
		  copy_srcfd = -1;
		  return (0);
		} /* if */
	      source_posn = 0;
	      interior -> do_sputn (".",1);
	    } /* else */
        } /* then */
       else
        {
	  // Extract some files from a ZIP file //
	  char *source;
	  len = 1;
	  while (basename[len] != ':')
	    ++len;
	  interior -> do_sputn ("\n    ",5);
	  source = source_file (basename + 1);
	  if (!source)
	    {
	      interior -> do_sputn (basename + 1,len - 1);
	      interior -> do_sputn ("             ",13 - (len - 1));
	      no_file ();
	      return (0);
	    } /* if */
	  interior -> do_sputn (UWUnZipProg,strlen (UWUnZipProg));
	  interior -> do_sputn (" -o ",4);
	  interior -> do_sputn (basename + 1,len - 1);
	  interior -> do_sputn (" ",1);
	  interior -> do_sputn (basename + len + 1,
	  			strlen (basename + len + 1));
	  strcpy (UnZipBuffer,UWUnZipProg);
	  strcat (UnZipBuffer," -o ");
	  strcat (UnZipBuffer,source);
	  strcat (UnZipBuffer," ");
	  strcat (UnZipBuffer,UWInstDir);
	  strcat (UnZipBuffer," ");
	  strcat (UnZipBuffer,basename + len + 1);
	  textattr (0x07);
	  clrscr ();
	  system (UnZipBuffer);
	  uwapp -> redraw ();
	} /* else */
    }
   else if (copy_srcfd < 0)
    {
      /* Find a new string of files to be processed */
      while (uwprograms[copy_index].option &&
      	    !(uwoptions & uwprograms[copy_index].option))
        ++copy_index;
      if (uwprograms[copy_index].option)
	{
	  len = strlen (uwprograms[copy_index].desc);
	  if (!copy_first)
	    interior -> do_sputn ("\n\n",2);
	  copy_first = 0;
	  interior -> do_sputn (uwprograms[copy_index].desc,len);
	  interior -> do_sputn (":\n",2);
	  copy_files = uwprograms[copy_index++].files;
	} /* then */
       else
        {
	  interior -> do_sputn ("\n\nPress RETURN to Continue",26);
	  copy_srcfd = -2;	// End of all files to be copied.
	  return (1);		// Indicate the window should be destroyed.
	} /* else */
    } /* then */
   else
    {
      // Copy some more of the current file //
      static char buffer[4096];
      if ((len = ::read (copy_srcfd,buffer,4096)) < 0)
        {
	  file_error ();
	  ::close (copy_srcfd);
	  ::close (copy_destfd);
	  copy_srcfd = -1;
	} /* then */
       else if (len == 0)
        {
	  ::close (copy_srcfd);
	  ::close (copy_destfd);
	  copy_srcfd = -1;
	} /* then */
       else if (::write (copy_destfd,buffer,len) != len)
        {
	  file_error ();
	  ::close (copy_srcfd);
	  ::close (copy_destfd);
	  copy_srcfd = -1;
	} /* then */
       else
        {
	  // Increment the output length and display a dot if 16k boundary //
	  source_posn += len;
	  if ((source_posn % 16384L) == 0L)
	    interior -> do_sputn (".",1);
	} /* else */
    } /* else */
  return (0);
}

void	TInstallApp::idle (void)
{
  TApplication::idle ();
  if (!init)
    {
      // Pop up the Copyright message box once the program has started //
      init = 1;
      MessageBox ("UW/PC Installer",copyright);
      popupMenu ();
    }
  if (copywin && copywin -> docopy () && copywin -> donecopy)
    {
      destroy (copywin);
      copywin = NULL;
      messageBox ("UW/PC Installation is Complete",mfConfirmation | mfOKButton);
    }
}

int main(int argc,char **argv)
{
    TInstallApp installUW;
    uwapp = &installUW;
    GetDefaults (argv[0]);
    installUW.run();
    return 0;
}
