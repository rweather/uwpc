.TH UWTERM 1 "20 September 1986"
.UC 4
.SH NAME
uwterm \- (possibly remote) terminal emulation for UW
.SH SYNOPSIS
.B uwterm
[
.BI \-w type
] [
.BI \-t title
] [
.BI \-n serveraddr
] [
.BI \-l loginname
] [ host ]
.SH DESCRIPTION
.I Uwterm
is a utility program for use with the
.I uw
multiple-window interface to UNIX.
It creates a window in which a terminal session
is conducted.
If no arguments are given
the terminal is created on the local machine.
A hostname may be specified;
in this case,
the terminal is created on the remote host.
.RI ( Uwterm
must be installed on the remote host
and permissions for
.I rsh
must have been set up correctly
in order for this to work.)
.I Uwterm
examines the `SHELL' environment variable
and executes the program named there.
The `\-l' option
can be used to specify the login name under which
the remote process will be executed
(the default is the current account).
.PP
Unlike
.IR uwtool ,
.I uwterm
does not exit until the window it has created
is destroyed;
hence,
it will usually be desirable to run it in the background.
.PP
Normally,
the terminal type of the new window is inherited from
the window in which
.I uwterm
is run.
It is possible to override this with the `\-w' option
(`w' stands for `window emulation type').
The window types are the same as those accepted by the
.I uwtool
program.
.PP
The title of the newly-created window may be
specified with the `\-t' option.
If this option is omitted,
the title will be the host name.
.PP
Normally
.I uwterm
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
The `\-n' flag is used by
.I uwterm
itself:
it creates a remote terminal by invoking itself on
the remote machine
(using
.IR rsh )
and specifying the network address
of the local server.
.SH LIMITATIONS
.I Uwterm 
is of no use on unix unless 
the server is running and
.I uw
is being run on the Macintosh.
.br
If there is a stream of output in one window there will be lag in 
recognizing characters typed in another.
.br
There are so many levels of buffering
that user-entered CTL-S/CTL-Q flow control is
practically useless,
even at relatively low baud rates.
.SH SEE ALSO
rsh(1), uw(L), uwtitle(L), uwtool(L)
.br
.I uw
Macintosh documentation
(`UW \(em A Multiple-Window Interface to UNIX')
.br
`The UW Programmer's Library'
.SH AUTHOR
John Bruner, Lawrence Livermore National Laboratory, 9/86.
.SH BUGS
There are so many levels of buffering that typing
XON and XOFF
to suspend output within a window
is futile.
