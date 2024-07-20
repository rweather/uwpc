.TH UWPLOT 1 "28 April 1987"
.UC 4
.SH NAME
uwplot \- plot filter for use with UW
.SH SYNOPSIS
.B uwplot
[
.BI \-t title
] [
.BI \-n serveraddr
]
.SH DESCRIPTION
.I Uwplot
is a utility program for use with the
.I uw
multiple-window interface to UNIX.
It creates a window,
reads plotting instructions from the standard input,
and displays the result in the window.
(The plotting commands are produced by programs
which use the standard UNIX
.I plot
package.)
.PP
The title of the newly-created window may be
specified with the `\-t' option.
If this option is omitted,
the title will be the name of the command
which created it
(\fIi.e.\fP `uwplot').
.PP
Normally
.I uwplot
will examine the environment for the variable `UW_INET'
and will connect to the
.I uw
server with that address.
The `\-n' flag can be used to specify an alternate
server network address.
The address should have the form `xxxxxxxx.ddd'
where `x' is a hexadecimal digit
and `d' is a decimal digit.
The hexadecimal number is the host's Internet address
and the decimal number is the port on which the
server is listening for connections.
.PP
`linemod' commands in the plot file are
intended to select the color in which the
plot is drawn;
however, this feature is untested.
The old-model Quickdraw colors may be
specified by name
(all lower-case).
The standard plot linemod names are mapped to
colors as follows:
.sp
.nf
.ta 1i
black	solid
red	shortdashed
green	dotdashed
blue	longdashed
yellow	dotted
.fi
.PP
The present implementation on the Macintosh
is a square window with vertical and horizontal
dimensions of 300 pixels.
Arcs are not drawn correctly when
the vertical and horizontal scaling factors
are unequal.
.SH LIMITATIONS
.I Uwterm 
is of no use on unix unless 
the server is running and
.I uw
is being run on the Macintosh.
.SH SEE ALSO
plot(1G), plot(3X), plot(5), uw(L)
.br
.I uw
Macintosh documentation
(`UW \(em A Multiple-Window Interface to UNIX')
.br
`The UW Programmer's Library'
.SH AUTHOR
John Bruner, Lawrence Livermore National Laboratory, 10/86.
