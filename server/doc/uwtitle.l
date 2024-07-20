.TH UWTITLE 1 "14 September 1986"
.UC 4
.SH NAME
uwtitle \- retitle UW window
.SH SYNOPSIS
.B uwtitle
[
.BI \-i id
]
string ...
.SH DESCRIPTION
.I Uwtitle
is a utility program for use with the
.I uw
multiple-window interface to UNIX.
It retitles an existing window.
The title is specified as one or more
strings
(in the same fashion as arguments to the
.I echo
program).
The `\-i' option can be used to specify
the ID of the window to be retitled;
otherwise,
the current window will be affected.
.SH LIMITATIONS
.I Uwtitle 
is of no use on unix unless 
the server is running and
.I uw
is being run on the Macintosh.
.SH SEE ALSO
echo(1), uw(L), uwtool(L), uwterm(L)
.br
.I uw
Macintosh documentation (`UW \(em A Multiple-Window Interface to UNIX')
.br
`The UW Programmer's Library'
.SH AUTHOR
John Bruner, Lawrence Livermore National Laboratory 9/86
