.TH UWTOOL 1 "14 September 1986"
.UC 4
.SH NAME
uwtool \- command-in-window utility for UW
.SH SYNOPSIS
.B uwtool
[
.BI \-w type
] [
.BI \-t title
] [
.B \-v
] [ command [ arg1 arg2 ...  ] ]
.SH DESCRIPTION
.I Uwtool
is a utility program for use with the
.I uw
multiple-window interface to UNIX.
If no arguments are given,
it creates a new `terminal' running the shell named in the
environment variable `SHELL'.
If a process is named,
it will create a new window with that process running in it,
and when that process is terminated the window will disappear.
(i.e. `uwtool vi foo' will create a new window with vi,
editing the file foo,
but the window will go away when vi is exited)  
Any arguments after the process name are passed as arguments to that process.
.I Uwtool
exits as soon as the window is created.
.PP
Normally,
the terminal type of the new window is inherited from
the window in which
.I uwtool
is run.
It is possible to override this with the `\-w' option
(`w' stands for `window emulation type').
The following values are recognized:
.TP 8n
adm31
Lear Siegler ADM-31
.TP
adm3a
Lear Siegler ADM-3a (uses ADM-31)
.TP
tek4010
Tektronix 4010
.TP
tek
Tektronix 4010
.TP
vt52
Digital Equipment Corporation VT-52
.TP
ansi
ANSI-compatible terminal
.TP
aaa-24
24-line Ann Arbor Ambassador
(uses ANSI emulation)
.PP
If an unknown type is specified,
the ADM-31 emulation will be used.
.PP
The title of the newly-created window may be
specified with the `\-t' option.
If this option is omitted,
the window title will be the command name.
.PP
The `\-v' flag causes
.I uwtool
to print the 32-bit window identifier on
the standard output.
This value can be used as an argument to a subsequent
command,
for example
.IR uwtitle .
.SH LIMITATIONS
.I Uwtool 
is of no use on unix unless 
the server is running and
.I uw
is being run on the Macintosh.
.br
If there is a stream of output in one window there will be lag in 
recognizing characters typed in another.
.SH SEE ALSO
uw(L), uwtitle(L), uwterm(L)
.br
.I uw
Macintosh documentation
(`UW \(em A Multiple-Window Interface to UNIX')
.br
`The UW Programmer's Library'
.SH AUTHOR
Program written by John Bruner, Lawrence Livermore Laboratories 7/85,11/85,9/86
.br
This document is based upon a document created by
by Chris Borton, UC San Diego 11/13/85,
edited 9/86 by John Bruner.
