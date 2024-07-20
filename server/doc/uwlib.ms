.\" This file should be processed by nroff or troff with the -ms macro set
.ds uw \s-2UW\s0
.DA September 30, 1986
.TL
The UW Programmer's Library
.AU
John D. Bruner
.SH
Introduction
.PP
\*(uw is a multiple-window Macintosh interface to a 4.2BSD UNIX\**.
.FS
Macintosh is a trademark of McIntosh Laboratories which
is licensed to Apple Computer, Inc.
UNIX is a registered trademark of Bell Laboratories.
.FE
\*(uw version 3 comprises a server,
a set of utility programs,
and an program-level interface.
This manual describes the services which are available
in the \*(uw programmer's library.
This library allow programs to create,
communicate with,
and perform some control operations upon
windows on the Macintosh.
.SH
Background
.PP
Before the library routines themselves can be discussed,
it is necessary to consider some aspects of the \*(uw
server.
The server which was distributed with \*(uw versions 1.6 and 2.10
communicated with the Macintosh using a protocol which
is referred to as the ``original protocol.''
The version 3 server is capable of communicating in this protocol;
however,
it also supports an ``extended protocol.''
For convenience,
these protocols are assigned numbers:
protocol 1 is the original protocol
and protocol 2 is the extended protocol.
.PP
Protocol 1 provides a mechanism for
the creation and destruction of windows
as well as a means to multiplex a single
communications line among several windows.
It provides a mechanism for transmitting
control and ``meta'' characters,
and it also provides two ``maintenance functions''
which are used for startup and shutdown.
.PP
Protocol 2 provides two significant enhancements
relative to protocol 1.
First,
window creation messages specify the window emulation type
(\fIe.g.\fP adm-31, vt52).
Second,
additional information about windows,
called ``window options,''
is transmitted between the Macintosh client and the
server on the host.
.PP
Window options are an ``out-of-band'' channel of
information between the Macintosh and the host.
There are two types:
generic
(common to all window types)
and emulation-specific.
The following are generic:
.DS
window visibility
window type
window position (pixel address of top left corner)
window title
window size in pixels
.DE
The following window options are specific to
cursor-addressible terminal emulations:
.DS
terminal size (number of rows and columns)
font size index (small=0, large=1)
mouse handling (clipboard or encoded send-to-host)
bell characteristics (audible, visible)
cursor appearance (block or underscore)
.DE
.PP
The server distinguishes between two window classes \(em
internal and external.
Internal windows are handled entirely by the server.
They are always terminal emulations and are always
associated with a pseudo-terminal device.
.PP
By contrast,
an external window always involves some outside process.
The server communicates with this process through one
or two Internet domain network connections.
There is always a ``data'' connection,
through which the external process exchanges information
(indirectly, through the server)
with the Macintosh.
There may also be a ``control'' connection
through which the external process exchanges
window option information
(again indirectly)
with the Macintosh.
The server acts as a multiplexor and demultiplexor
for external windows.
It also caches window option information;
however,
it does not perform host-end emulation-specific tasks.
.PP
Internal and external windows meet different needs.
Terminal emulation on the local host is best performed
by internal windows,
because fewer processes are involved
(and response time is better).
External windows are suitable for remote processes
(\fIi.e.\fP those on another Internet host)
or for non-terminal tasks such as file transfer.
The \*(uw application library contains routines
to create and manipulate both classes of windows.
.SH
Window ID's and Network Addresses
.PP
A unique 32-bit identification number is associated
with each window that a server manipulates.
Some operations
(described below)
require the window number to be specified.
When the server creates a new internal window,
it passes the window ID as the environment variable
``UW_ID''.
.PP
The server creates two network sockets upon which to receive
incoming messages.
One socket receives UNIX-domain datagrams,
the other listens for Internet-domain stream connections.
The addresses of these sockets are placed in the environment
as the variables ``UW_UIPC''
(UNIX-domain)
and ``UW_INET''
(Internet domain).
.SH
Data Types and Data Structures
.PP
The \*(uw programmer's library uses a number
of simple and structured data types.
.IP uwid_t 1i
Unique window ID numbers are represented by the data type ``uwid_t''.
.IP UWIN
Library routines which operate upon external windows
put a range of window information into a structure.
The type ``UWIN'' is a pointer to the structure.
An object of this datatype is referred to as a ``window descriptor.''
This declaration is intended to be used as an abstract unit
(in the manner of the standard I/O library's ``FILE\ *'').
.IP uwtype_t
Window emulation types have the data type ``uwtype_t''.
The following emulation types are defined:
.DS
.ta 8n 24n 32n
#define	UWT_ADM31	0	/* ADM-31 cursor-addressible terminal */
#define	UWT_VT52	1	/* VT52 cursor-addressible terminal */
#define	UWT_ANSI	2	/* ANSI-compatible terminal */
#define	UWT_TEK4010	3	/* Tektronix 4010 graphics terminal */
#define	UWT_FTP	4	/* File transfer */
#define	UWT_PRINT	5	/* Output to Macintosh printer */
.DE
.IP uwopt_t
Window options are assigned numbers whose type is ``uwopt_t''.
The names of the options are:
.DS
.ta 8n 28n 32n
#define	UWOP_VIS	1	/* visibility */
#define	UWOP_TYPE	2	/* window type */
#define	UWOP_POS	3	/* window position */
#define	UWOP_TITLE	4	/* window title */
#define	UWOP_WSIZE	5	/* window size (in bits) */
#define	UWOP_TSIZE	8	/* terminal size (row,col) */
#define	UWOP_TFONTSZ	9	/* small/large font size */
#define	UWOP_TCLIPB	10	/* clipboard/mouse encoding */
#define	UWOP_TBELL	11	/* audible, visual bell */
#define	UWOP_TCURS	12	/* cursor shape */
.DE
.IP uwoptcmd_t
The window option commands
which are passed between the Macintosh and the host
have type ``uwoptcmd_t''.
These commands are:
.DS
.ta 8n 24n 32n
#define	UWOC_SET	0		/* set value of option */
#define	UWOC_ASK	2		/* ask for value of option */
#define	UWOC_DO	4		/* report changes in value */
#define	UWOC_DONT	5		/* don't report changes */
#define	UWOC_WILL	6		/* will report changes */
#define	UWOC_WONT	7		/* won't report changes */
.DE
.IP "union uwoptval"
When a function requires a window option value as an argument,
the value of the window option is placed into a
union declared as ``union uwoptval''.
The address of this union is passed to the function.
This union is declared as follows:
.DS
.ta 8n 16n 24n
union uwoptval {
	unsigned char	uwov_1bit;
	unsigned char	uwov_2bit;
	unsigned char	uwov_6bit;
	unsigned short	uwov_12bit;
	struct {
		unsigned short v,h;
	}		uwov_point;
	char		uwov_string[256];
};
.DE
The union member used for a particular option
depends upon the option number.
At present,
the types of the window options and
corresponding union members are:
.DS
.ta 1i
visibility	uwov_1bit
type	uwov_6bit
position	uwov_point
title		uwov_string (null terminated)
bit size	uwov_point
tty size	uwov_point
font size	uwov_1bit
clipboard	uwov_1bit
bell		uwov_2bit
cursor type	uwov_1bit
.DE
.IP "uwerr_t"
When a library routine returns an error indication,
further information about the type of error can be
obtained from the global variable ``uwerrno''.
(Depending upon the type of error,
the external variable ``errno'' may also contain
pertinent information.)
\*(uw error numbers have type ``uwerr_t'',
and are defined as follows:
.DS
.ta 8n 24n 32n
#define	UWE_NONE	0	/* no error */
#define	UWE_ERRNO	1	/* system call error, consult errno */
#define	UWE_NXTYPE	2	/* nonexistent window type */
#define	UWE_DUPID	3	/* window ID duplicated (in use) */
#define	UWE_NOTIMPL	4	/* operation not implemented yet */
#define	UWE_NXSERV	5	/* non-existent server */
#define	UWE_NOMEM	6	/* unable to allocate required memory */
#define	UWE_INVAL	7	/* invalid argument to function */
#define	UWE_NOCTL	8	/* no control file descriptor */
.DE
.SH
Internal Window Interface
.PP
When an internal window is created by an external process,
a UNIX-domain datagram is sent to the server.
This datagram contains
(as ``access rights'')
a file descriptor for the ``master'' side of a pseudo-terminal.
The server assumes that the external process
has started some program on the ``slave'' side of the pseudo-terminal.
After sending the datagram,
the sender has no direct handle to manipulate
the window.
It has,
in effect,
relinquished all control.
(It should close the master side of the pseudo-terminal
after sending the datagram.)
To provide some additional flexibility,
it is possible to change the value of a window option
for any window
(even ``external'' windows)
if the window's unique ID is known.
The creator of the window has no special privileges
in this regard.
.LP
[One thing which the internal window routines
in the \*(uw library completely ignore
is the fact that datagrams are not guaranteed to be reliable.
UNIX-domain datagrams almost always seem to work,
but they can fail.
In the author's experience this has never been a problem,
but let the user beware.]
.LP
The following routines are available:
.IP uw_fork 1i
This routine is similar in concept to the system call ``fork''.
It creates a new process
and returns twice \(em
once in the parent and once in the child.
In addition to creating a new process,
``uw_fork'' also arranges for the new process to be
associated with an internal window.
It opens a pseudo-terminal,
redirects the child's standard input,
standard output,
and standard error,
and sends a UNIX-domain datagram to the \*(uw server.
It returns the unique ID associated with the window
in the parent,
and returns 0 in the child.
(\(mi1 is returned if the routine fails.)
.DS
uwid_t uw_fork(uwtype_t wtype, int *pidp);
.DE
The first argument specifies the type of the new window.
If the second argument to ``uw_fork'' is a non-NULL pointer,
the process-ID of the child will be stored at that address
in the parent process.
(In the child, ``*pidp'' will be zero.)
.IP uw_cmd
This routine builds upon the functionality of the ``uw_fork'' routine.
It creates a new window with ``uw_fork''
and then executes a specified command.
It takes the window type,
the name of an executable file,
and an argument list
as parameters;
it uses these as arguments to ``uw_fork''
and the C library routine ``execvp''.
It returns the window ID number to its caller:
.DS
uwid_t uw_cmd(uwtype_t wtype, char *file, char **argv);
.DE
(\(mi1 is returned if the routine fails.)
.IP uw_shell
``uw_shell'' is similar to ``uw_cmd'' except that it
executes an arbitrary shell command:
.DS
uwid_t uw_shell(uwtype_t wtype, char *cmd);
.DE
(\(mi1 is returned if the routine fails.)
By default the Bourne shell is used;
however,
the shell may be changed by patching the global variable ``uwshellname''.
.DS
char *uwshellname = "/bin/sh";
.DE
.IP uw_rsetopt
This routine changes the value of a window option
for an existing window
(named as a window ID).
The window may be either internal or external.
The specified window option is set to a desired value.
Zero is returned if the appropriate UNIX-domain message
was successfully sent;
\(mi1 is returned if the operation failed.
(Since ``uw_rsetopt'' does not receive a reply from the server,
it is unable to determine whether or not the command ``succeeded''.
Rather,
it returns zero if the command was successfully transmitted.)
.DS
int uw_rsetopt(uwid_t uwid, uwopt_t optnum, union uwoptval *optval);
.DE
``optval'' points to a ``union uwoptval'' structure
(described above)
in which the member corresponding to ``optnum''
has been initialized.
.IP "uw_perror"
When an error is reported by a \*(uw library routine,
the cause of the error is saved in the external variable
``uwerrno''.
If the error was UWE_ERRNO,
the standard external variable ``errno''
will also be meaningful.
(The routines which operate upon external windows,
described in the following section,
also save this information in the window descriptor.)
The routine ``uw_perror'' may be used to decode and print
error messages:
.DS
void uw_perror(char *usermesg, uwerr_t uwerr, int err)
.DE
where ``usermesg'' is a pointer to a user-specified string,
``uwerr'' is the \*(uw error code
(usually ``uwerrno''),
and ``err'' is the system call error code
(usually ``errno'').
[System call error numbers are defined in ``/usr/include/errno.h''.]
.sp
The \*(uw error messages may also be accessed directly.
Two external variables aid in user-formatted error messages:
.DS
extern char *uwerrlist[];
extern unsigned uwnerr;
.DE
If the error number is greater than or equal to ``uwnerr'',
no error message string exists.
(This ``cannot happen.'')
Otherwise,
the error message string is obtained by indexing into
``uwerrlist''.
.PP
The preceeding routines are sufficient to implement
a (simplified) version of the ``uwtool'' program,
which creates a new
(internal)
window running a specified command:
.DS
.ta 8n 16n 24n 32n 40n 48n 56n 64n 72n
/*
 *      uwtool
 *
 * Copyright 1986 by John D. Bruner.  All rights reserved.  Permission to
 * copy this program is given provided that the copy is not sold and that
 * this copyright notice is included.
 */
#include <strings.h>
#include "uwlib.h"

main(argc, argv)
int argc;
char **argv;
{
        register uwid_t uwid;
        register char *fname, *term;
	register wtype_t wtype;
        char *av[2];
        union uwoptval optval;
        extern int errno;
        extern char *getenv();

        /*
         * If called with no arguments, create a new window using the
         * current shell according to the SHELL environment variable
         * (or "/bin/sh" if that doesn't exist).  If called with
         * arguments, argv[1] through argv[argc\-1] are the arguments
         * to the command.
         */
        if (argc == 1) {
                if ((fname = getenv("SHELL")) == (char *)0)
                        fname = "/bin/sh";
                av[0] = fname;
                av[1] = (char *)0;
                argv = av;
        } else
                fname = *++argv;
	
	if ((term=getenv("TERM")) != (char *)0)
		wtype = uw_ttype(term);
	else
		wtype = UWT_ADM31;
        
        if ((uwid = uw_cmd(wtype, fname, argv)) < 0) {
                (void)strncpy(optval.uwov_string, fname,
                    sizeof optval.uwov_string);
                (void)uw_rsetopt(uwid, UWOP_TITLE, &optval);
                return(0);
        } else {
                uw_perror("uwtool", uwerrno, errno);
                return(1);
        }
}
.DE
After the first part of the function has massaged the argument list,
the ``uw_cmd'' routine creates a new window
running the command ``fname'' with argument list ``argv''.
If the window ID is positive,
the window creation succeeded.
After copying the name of the program into a ``union uwoptval'',
the program calls ``uw_rsetopt'' to set the window title
to that string.
If the window ID returned by ``uw_cmd'' was \(mi1,
the window creation failed.
In this case,
the program calls ``uw_perror'' to report the error.
.SH
External Window Interface
.LP
The remainder of the \*(uw library routines provide access to
external windows.
In contrast to internal windows,
a client process creates an external window
by establishing an Internet-domain stream connection
to the server and sending the server a ``create window'' command.
The server will establish a second stream connection
back to the client.
Data is passed between the client and the server on the first connection,
while control information is passed through the second.
[Because the server and client communicate through one or
two stream connection(s)
instead of by sending datagrams,
the unreliability problems noted above for internal windows
do not apply to external windows.]
.LP
The \*(uw library provides mechanisms for creating external windows,
killing them,
and manipulating window options.
When a window is created a window descriptor
(item of type UWIN)
is returned;
this is used as an argument to all other external-window routines.
.LP
The following routines are provided:
.IP "uw_new" 1i
This function creates a new external window of the specified type.
The calling sequence is:
.DS
UWIN uw_new(uwtype_t uwtype, struct sockaddr_in *server)
.DE
where ``uwtype'' is the window type
and ``server'' is a pointer to a structure specifying
the Internet address of the server.
(If ``server'' is a NULL pointer,
the server will be determined by examining the
environment variable ``UW_INET''.)
If the window creation succeeded,
``uw_new'' will return a non-NULL window descriptor;
otherwise,
it will return NULL, and
the global variables ``uwerrno'' and ``errno''
may be examined to determine the cause of the error.
.IP "uw_detach"
This function ``detaches'' the window from the program
so that it no longer is able to perform control operations
upon the window.
The data connection to the window remains open.
This function should be used when the data connection to a window
will be handled by a different process
than the control connection,
.I e.g.
in a child process after a ``fork''.
It is strongly recommended that no more than one process
have control access to a window at any one time.
The calling sequence is
.DS
uw_detach(UWIN uwin);
.DE
where ``uwin'' is the window descriptor.
Zero is returned for success,
while \(mi1 is returned for failure.
If the routine fails,
the error status will be stored in the UWIN data item
as well as in the global variables ``uwerrno'' and ``errno''.
.IP "uw_close"
This function closes a window.
Both the control and data connections to the window are closed.
If multiple processes have access to a window
(\fIe.g.\fP because of a ``fork''),
then the window will be destroyed when the last connection
to it is closed.
The calling sequence is
.DS
uw_close(UWIN uwin);
.DE
where ``uwin'' is the window descriptor
.IP "uw_kill"
At times it may be desirable for one process to destroy
a window even if the window is in use by other processes.
The ``uw_kill'' function performs this task.
The caller must have control access to the window
(it must not be ``detached'').
The syntax is:
.DS
uw_kill(UWIN uwin);
.DE
where ``uwin'' is the window descriptor.
When a window is killed
(either by ``uw_kill'' or upon command from the Macintosh)
the server closes its data channel.
Any further attempts to read or write to the window
will produce end-of-file or error conditions,
respectively.
.IP "uw_optfn"
If a process has control access to a window,
then it will periodically receive
window option messages from the Macintosh client
(through the server).
The \*(uw library receives these messages by enabling
asynchronous I/O notification on the control channel
and providing a SIGIO signal handler.
Sometimes it is desirable for an external process
to field incoming option messages itself.
To do so,
it must notify the \*(uw library by calling the
routine ``uw_optfn'':
.DS
void (*uw_optfn(UWIN uwin, uwopt_t optnum, void (*optfn)())();
.DE
where ``uwin'' is the window descriptor,
``optnum'' is the desired window option,
and ``optfn'' is a pointer to a function which
will be called when a message about window option ``optnum''
is received.
``uw_optfn'' returns the previous function.
To disable processing for a window option,
specify a NULL pointer for ``optfn''.
The user-supplied ``optfn'' is called with the following arguments:
.DS
.ta 8n
void (*optfn)(UWIN uwin, uwopt_t optnum, uwoptcmd_t optcmd,
	union uwoptval *optval);
.DE
where ``uwin'' is the window descriptor,
``optnum'' is the window option number,
``optcmd'' is the window option command,
and
(if ``optcmd'' is UWOC_SET)
``optval'' is a pointer to the new value of the window option.
.sp
Because the \*(uw library provides a signal handler for SIGIO,
if other portions of the program wish to catch SIGIO,
then some care must be taken
to ensure that all signal handlers are called.
The \*(uw library saves the return value from
``signal'' when it installs its handler.
If this is not SIG_IGN,
then that routine will be called after \*(uw has
completed its signal processing.
In a similar fashion,
if the calling program establishes a signal handler,
it should save the previous value and call the indicated
function
(if not SIG_IGN).
For example,
if the caller uses ``signal'':
.DS
.ta 8n 16n
oldhandler = signal(SIGIO, myhandler);
\&...
myhandler(sig, code, scp)
int sig, code;
struct sigcontext *scp;
{
	... code to handle exception ...
	if (oldhandler != SIG_IGN)
		(*oldhandler)(sig, code, scp);
}
.DE
Although from time to time
the Macintosh may ask the server for the current
value of a window option,
the \*(uw server caches the current value of each
window option
and responds to these inquiries directly.
Therefore,
the major reason for establishing a window option function
with ``uw_optfn'' is to process incoming UWOC_SET messages,
.I i.e.
messages from the Macintosh that the value of a window
option has changed.
.IP "uw_optcmd"
This function allows a program with control access to a window
to send window option commands.
The calling sequence is
.DS
.ta 8n
uw_optcmd(UWIN uwin, uwopt_t optnum, uwoptcmd_t optcmd,
	union uwoptval *optval);
.DE
where ``uwin'' is the window descriptor,
``optnum'' is the window option number,
``optcmd'' is the command,
and ``optval'' is a pointer to the option value.
Of the six window option messages,
only the UWOC_SET,
UWOC_DO,
and UWOC_DONT
messages are very useful.
UWOC_SET changes the value of a window option
(``optval'' points to the new value).
UWOC_DO and UWOC_DONT instruct the Macintosh to
report or not report
(respectively)
when a user action changes the value of a window option there.
When it creates a window,
the \*(uw server instructs the Macintosh to report all
changes to window options.
Most programs will probably not need to issue UWOC_DO or UWOC_DONT commands.
.IP uw_gvis
This function fetches the current visibility
status of a specified window:
.DS
int uw_gvis(UWIN uwin, int *vp);
.DE
``vp'' is a pointer to an integer where the visibility status
(0 or 1 for invisible or visible, respectively)
is returned.
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_svis
This function changes the visibility status of a specified window:
.DS
int uw_svis(UWIN uwin, int v);
.DE
If ``v'' is nonzero then the window ``uwin'' will be made visible;
otherwise,
the specified window will be made invisible.
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_gpos
This function returns the current position on the screen
of a specified window:
.DS
.ta 8n 20n 28n
struct uwpoint {
	unsigned	uwp_v;	/* vertical component */
	unsigned	uwp_h;	/* horizontal component */
};

int uw_gpos(UWIN uwin, struct uwpoint *pp);
.DE
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_spos
This function sets the position of a specified window to
a desired location:
.DS
int uw_spos(UWIN uwin, struct uwpoint *pp);
.DE
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_gwsize
This function returns the current size in pixels
of a specified window.
The size is expressed as a ``uwpoint'',
as defined above.
.DS
int uw_gwsize(UWIN uwin, struct uwpoint *pp);
.DE
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_swsize
This function sets a specified window to a new size:
.DS
int uw_swsize(UWIN uwin, struct uwpoint *pp);
.DE
.IP uw_gtitle
This function returns the title of a specified window.
The title has type ``uwtitle_t'':
.DS
typedef char uwtitle_t[256];

int uw_gtitle(UWIN uwin, uwtitle_t ttl);
.DE
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_stitle
This function sets the title of a specified window:
.DS
int uw_stitle(UWIN uwin, uwtitle_t ttl);
.DE
.IP uw_gtype
This function returns the type of a specified window:
.DS
int uw_gtype(UWIN uwin, uwtype_t *tp);
.DE
``tp'' points to a variable which receives the window type.
Zero is returned for success,
while \(mi1 is returned for failure.
.IP uw_stype
This function sets the type of a specified window:
.DS
int uw_stype(UWIN uwin, uwtype_t t);
.DE
``t'' is the new window type.
Zero is returned for success,
while \(mi1 is returned for failure.
.IP "UW_DATAFD"
This macro extracts the file descriptor for the data connection
from a window descriptor:
.DS
int UW_DATAFD(UWIN uwin);
.DE
.IP "UW_ID"
This macro returns the unique window ID associated with a
window descriptor:
.DS
uwid_t UW_ID(UWIN uwin);
.DE
.IP "UW_PERROR"
When the \*(uw library detects an error
it always places the error information into the
global variables ``uwerrno'' and ``errno''.
If the error is associated with a valid window descriptor,
it will also store the information in the window descriptor.
The macro ``UW_PERROR'' is used to print an error message
according to the error status in a window descriptor:
.DS
void UW_PERROR(char *message, UWIN uwin);
.DE
where ``message'' is any user-supplied message and
``uwin'' is a window descriptor.
.SH
Copyright
.LP
This document copyright 1986 by John D. Bruner.
Permission to copy is given,
provided that the copies are not sold
and that this copyright notice is included.
