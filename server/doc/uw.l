.TH UW 1 "14 September 1986"
.UC 4
.SH NAME
uw \- multiple-window Macintosh interface to UNIX
.SH SYNOPSIS
.B uw [ 
.BI \-f filename
] [
.B \-n
] [
.B \-s
]
.SH DESCRIPTION
.I Uw
is a server program on UNIX that works with the program 
.I uw
on the Macintosh.
It provides the Macintosh program with access to
a maximum of seven independent I/O sessions.
An I/O session may be directly associated with a pseudo-terminal
or may simply be a communications channel to an external
UNIX process.
The host program multiplexes the input and output onto one RS\-232 line.
.PP
Most commonly,
sessions will be directly associated with pseudo-terminals.
The Macintosh program will emulate
a Lear Siegler ADM-31 terminal
(tset adm31),
a DEC VT52,
an ANSI-compatible terminal
(tset ansi or tset aaa-24),
and a Tektronix 4010.
Each window
(on the Macintosh)
has its own terminal emulation and can be resized at will.
Window size changes on the Macintosh can be propagated to the host,
or the Macintosh may be directed to display the lower left portion
of a larger logical terminal size.
.PP
If the file `.uwrc' exists in the user's home directory,
then
.I uw
will execute it when starting up.
If `.uwrc' is an executable file,
it will be directly invoked;
otherwise,
a shell will be spawned to interpret it.
An alternate startup file may be specified using the
`\-f' flag;
alternately,
the `\-n' flag instructs
.I uw
not to execute any startup file.
.PP
The `\-s' flag prevents
.I uw
from listening for UNIX-domain datagrams.
Thus,
it prevents external processes from manipulating
windows which they did not create.
This may be of value in an environment where
other users are considered `hostile.'
.SH LIMITATIONS
.I Uw 
is of no use on unix unless 
.I uw
is being run on the Macintosh.  
.br
If there is a stream of output in one window there will be lag in 
recognizing characters typed in another.
.SH SEE ALSO
uwtool(L), uwtitle(L), uwterm(L)
.br
.I uw
Macintosh documentation
(`UW \(em A Multiple-Window Interface to UNIX').
.br
`The UW Programmer's Library'
.SH AUTHOR
Program written by John Bruner, Lawrence Livermore Laboratories 7/85,11/85,9/86
.br
This document is based upon a document created by
by Chris Borton, UC San Diego 11/13/85,
edited 9/86 by John Bruner.
.SH BUGS
The `\-s' flag greatly reduces the utility of a `.uwrc' file,
since it prevents
.I uwtool
and
.I uwtitle
from working.
