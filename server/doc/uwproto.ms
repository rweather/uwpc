.\" This file should be processed by nroff or troff with the -ms macro set
.ds uw "\s-2UW\s0
.de T=
.ie t .ta 8n 28n 36n 44n
.el .ta 8n 24n 32n 40n
..
.DA September 30, 1986
.TL
UW Protocol
.AU
John D. Bruner
.SH
Introduction
.PP
\*(uw is a multiple-window interface to UNIX.\**
.FS
UNIX is a registered trademark of American Telephone and Telegraph.
.br
Macintosh is a trademark of McIntosh Laboratories,
and is licensed to Apple Computer.
.br
ADM-31 is a trademark of Lear Siegler, Inc.
.br
VT52 is a trademark of Digital Equipment Corporation.
.br
The Tektronix 4010 is a graphics terminal manufactured
by Tektronix, Inc.
.FE
It comprises two parts:
a program which runs on a Macintosh
(referred to hereafter as ``the client'')
and a server program which runs on the UNIX system
(``the host'').
These two programs exchange information by sending
data across a serial communications line.
This information consists of data
and control and status messages relating to its presentation.
The structure of this information is defined by
the \*(uw protocol
and is the subject matter of this document.
.PP
\*(uw version 3 actually defines three protocols.
Internally they are assigned numbered,
while the user interface and documentation refer to them
by name.
The correspondence is as follows:
.IP 0
Protocol 0 is referred to as the ``serverless protocol''
or ``single terminal emulator protocol,''
because its use does not require a server on the host.
.IP 1
Protocol 1 is called the ``original \*(uw protocol,''
because it was the only protocol supported by the first
versions of \*(uw
(versions 1.6 and 2.10).
.IP 2
Protocol 2 is called the ``extended \*(uw protocol,''
or (sometimes)
the ``\*(uw version 3 protocol.''
.SH
Protocol 0 \(em The Serverless Protocol
.PP
Protocol 0 is not really a \*(uw protocol at all.
The client speaks protocol 0 when it is communicating
directly with a host,
rather than communicating through a server program
running on the host.
Protocol 0 is simply 7-bit or 8-bit ASCII.
Every byte transmitted in protocol 0 represents itself,
with the possible exception of the two flow-control characters
XON (control-Q)
and
XOFF (control-S).
Protocol 0 does not specify whether these characters are
to be used for flow-control purposes or for data transmission.
The client program on the Macintosh and the host's terminal driver
can be configured to use or ignore flow-control.
.PP
Protocol 0 does not specify whether data is transmitted
using 8-bit ASCII or 7-bit ASCII.
The user must choose the appropriate transmission format
and is reponsible for configuring both the client and the host
accordingly.
.SH
Protocol 1 \(em The Original \*(uw Protocol
.PP
Protocol 1 was the only protocol which was used in
the first versions of \*(uw
(versions 1.6 and 2.10).
It defines seven ``windows,''
each of which is an independent data stream.
Through the transmission of appropriate commands,
windows may be created or destroyed,
and data may be directed to a particular window.
Data which is transmitted from the client to the host
is referred to as ``input data,''
while data transmitted from the host to the client
is referred to as ``output data.''
In each direction a ``current window'' specifies
the recipient of data bytes.
(For example,
if the client wishes to send data to window 4,
it first sends a ``select window 4 as input window''
command and then sends the data.
Until the client sends another ``select input window'' command,
all further data that it transmits will be received
as data for window 4.)
The current input window and current output window may be different.
.PP
Protocol 1 encodes all information into 7-bit symbols;
it does not depend upon the value of the most-significant bit
(sometimes used for parity purposes).
Commands are encoded into two bytes:
a prefix byte (P1_IAC)
and a command byte.
Bit 7
(the second most-significant bit, octal 0100)
specifies whether the command was sent from the host to the client or
.I "vice versa:"
.DS
.T=
#define	P1_IAC	0001	/* intrepret following byte as a command */
#define	P1_DIR	0100	/* command direction: */
#define	P1_DIR_HTOM	0000	/*	from host to Mac (client) */
#define	P1_DIR_MTOH	0100	/*	from Mac (client) to host */
.DE
The command's function is encoded into the next three bits.
There are only seven commands:
.DS
.T=
#define	P1_FN	0070	/* function code: */
#define	P1_FN_NEWW	0000	/*	create new window */
#define	P1_FN_KILLW	0010	/*	kill (destroy) window */
#define	P1_FN_ISELW	0020	/*	select window for input data */
#define	P1_FN_OSELW	0030	/*	select window for output data */
#define	P1_FN_META	0050	/*	add META to next data character */
#define	P1_FN_CTLCH	0060	/*	send control character as data */
#define	P1_FN_MAINT	0070	/*	perform "maintenance function" */
.DE
(The client does not send the P1_FN_OSELW command;
similarly,
the host does not send the P1_FN_ISELW command.)
.PP
The least-significant three bits of the command byte
specify an argument to the function.
For the ``new window'' (P1_FN_NEWW),
``kill window'' (P1_FN_KILLW),
``select input'' (P1_FN_ISELW),
and
``select output'' (P1_FN_OSELW)
commands,
the low three bits specify a window number.
Window number zero is not used.
.PP
There are no arguments to the P1_FN_META command.
It directs that the next data byte to be transmitted
be considered a ``meta'' character;
.I i.e.
a byte with the most-significant bit
(octal 0200)
set.
.PP
The P1_FN_CTLCH command is used to encode three
data characters which cannot be transmitted directly as data.
These are P1_IAC
(which,
if encountered in a transmission,
indicates the start of a two-character command),
and the flow-control characters XON (021)
and XOFF (023).
The low-order three bits of the command byte specify the character:
.DS
.T=
#define	P1_CC	7	/* control character specifier: */
#define	P1_CC_IAC	1	/*	P1_IAC (001) */
#define	P1_CC_XON	2	/*	XON (021) */
#define	P1_CC_XOFF	3	/*	XOFF (023) */
.DE
A meta-control character is transmitted as a P1_FN_META
command followed by the appropriate P1_FN_CTLCH command.
Thus,
the octal character 0201 would be transmitted from the
host to the client as
a four-byte sequence:
.DS
.T=
0001	(P1_IAC)
0050	(P1_DIR_HTOM|P1_FN_META)
0001	(P1_IAC)
0061	(P1_DIR_HTOM|P1_FN_CTLCH|P1_CC_IAC)
.DE
Note that since the host does not send P1_FN_OSELW commands
and the client sends commands with the 0100 bit set,
the XON and XOFF control characters will never be sent as command bytes.
.PP
``Maintenance functions'' are defined for operations
which are performed infrequently.
Protocol 1 defines two maintenance functions:
.DS
.T=
#define	P1_MF	7	/* maintenance functions: */
#define	P1_MF_ENTRY	0	/*	start up */
#define	P1_MF_EXIT	7	/*	exit */
.DE
The server sends the P1_MF_ENTRY command when it starts up.
The client responds to this by killing any windows that it
has created
(silently,
.I i.e.
without sending P1_FN_KILLW messages to the host).
Either the client or the server may send the P1_MF_EXIT command
to terminate the session.
When the server on the host receives P1_MF_EXIT
it terminates;
when the client receives this command it will reset to a
simple known state.
.SH
Protocol 2
.PP
\*(uw version 3 provides a number of capabilities
that earlier versions of \*(uw did not.
Among these is an expansion of the host-client interaction.
In order to accomodate the increased flow of information
it was necessary to extend the \*(uw protocol.
One of the significant extensions in protocol 2
is support for a concept called ``window options.''
Window options are described in more detail in the next section.
.PP
Protocol 2 is very similar to protocol 1.
Like protocol 1,
protocol 2 multiplexes a single communications stream
among a maximum of seven windows.
Command bytes in protocol 2 are encoded in the same
fashion as in protocol 1:
a prefix byte
(P2_IAC)
followed by a command byte.
However,
unlike protocol 1,
some protocol 2 commands require more than one command byte.
.PP
The protocol 2 functions are:
.DS
.T=
#define	P2_FN	0070	/* function code: */
#define	P2_FN_NEWW	0000	/*	create new window */
#define	P2_FN_KILLW	0010	/*	kill (destroy) window */
#define	P2_FN_ISELW	0020	/*	select window for input data */
#define	P2_FN_OSELW	0030	/*	select window for output data */
#define	P2_FN_WOPT	0040	/*	communicate window options */
#define	P2_FN_META	0050	/*	add META to next data character */
#define	P2_FN_CTLCH	0060	/*	send control character as data */
#define	P2_FN_MAINT	0070	/*	perform "maintenance function" */
.DE
The P2_FN_KILLW,
P2_FN_ISELW,
P2_FN_OSELW,
and P2_FN_CTLCH
commands are identical to their counterparts in protocol 1.
.PP
The low-order three bits of the P2_FN_META command
represent a control character.
(The low-order three bits of the P1_FN_META command are ignored.)
The encoding is identical to the encoding
for the P2_FN_CTLCH command:
.DS
.T=
#define	P2_CC	7	/* control character specifier: */
#define	P2_CC_IAC	1	/*	P2_IAC (001) */
#define	P2_CC_XON	2	/*	XON (021) */
#define	P2_CC_XOFF	3	/*	XOFF (023) */
.DE
If the low-order three bits are zero,
then the P2_FN_META command acts like the P1_FN_META command \(em
the META bit is set in the next data byte.
If the low-order three bits are not all zero,
the the P2_FN_META command specifies a META-control character.
Thus, the following are all equivalent:
.DS
P1_IAC\ \ P1_FN_META\ \ P1_IAC\ \ P1_FN_CTLCH|P1_CC_IAC
P2_IAC\ \ P2_FN_META\ \ P2_IAC\ \ P2_FN_CTLCH|P2_CC_IAC
P2_IAC\ \ P2_FN_META|P2_CC_IAC
.DE
.PP
The P2_FN_NEWW command differs from the P1_FN_NEWW command
in that the protocol 2 command includes an extra byte.
The byte following the command byte
specifies the type of the window that is being created.
The numeric value of the window type is
added to the ASCII value for a space (blank);
hence,
the window type is always represented by a printable character.
As an example,
if the host wishes to create window 2
with window type 1 (VT-52),
the command sequence is:
.DS
.T=
0001	(P2_IAC)
0002	(P2_DIR_HTOM|P2_FN_NEWW|2)
0041	(`!')
.DE
.PP
The following maintenance functions (P2_FN_MAINT) are defined:
.DS
.T=
#define	P2_MF	7	/* maintenance functions: */
#define	P2_MF_ENTRY	0	/*	start up */
#define	P2_MF_ASKPCL	2	/*	request protocol negotiation */
#define	P2_MF_CANPCL	3	/*	suggest protocol */
#define	P2_MF_SETPCL	4	/*	set new protocol */
#define	P2_MF_EXIT	7	/*	exit */
.DE
The representations of
P2_MF_ENTRY and P2_MF_EXIT are identical to those in protocol 1.
The definition of the ``entry'' function is extended
slightly in protocol 2.
In protocol 1,
the P1_MF_ENTRY command is only sent by the server when it starts up.
The client recognizes this command and initializes itself.
In protocol 2,
the client is permitted to send the P2_MF_ENTRY command
to the server.
Upon receipt of this command,
the server issues the sequence of P2_FN_NEWW commands
and P2_FN_WOPT commands
(described below)
which will reconstruct all of the existing windows
on the client in their current state.
The client uses this command to ``restart'' itself after a crash
or other interruption on its end of the connection.
The three new maintenance functions are used for protocol negotiation.
Protocol negotiation is described in detail below.
.PP
Protocol 2 defines the new command
P2_FN_WOPT
to transmit window option information
between the client and server.
The P2_FN_WOPT command is followed by a variable-length
string of bytes which encode the window options information.
The next section describes the meaning and encoding of
window option information.
.SH
Window Options
.PP
Window options are window attributes
(the latter is a more meaningful name).
For each window,
a maximum of 31 window options may be defined.
These are divided into two categories:
generic and emulation-specific.
Generic window options are attributes which are common
to all window emulation types.
Emulation-specific options are meaningful only for some
subset of the available emulation types.
The following options are generic:
.DS
.T=
#define	WOG_END	0	/* [used as an endmarker] */
#define	WOG_VIS	1	/* visibility */
#define	WOG_TYPE	2	/* window emulation type */
#define	WOG_POS	3	/* window position on screen */
#define	WOG_TITLE	4	/* window title */
#define	WOG_SIZE	5	/* window size (in pixels) */
#define	WOG_6	6	/* [unassigned, reserved] */
#define	WOG_7	7	/* [unassigned, reserved] */
.DE
Terminal emulations define the following emulation-specific options:
.DS
.T=
#define	WOTTY_SIZE	8	/* (row,col) terminal size */
#define	WOTTY_FONTSZ	9	/* font size index */
#define	WOTTY_MOUSE	10	/* mouse interpretation */
#define	WOTTY_BELL	11	/* audible, visual bell */
#define	WOTTY_CURSOR	12	/* cursor shape */
.DE
Window option values are structured types composed
of the following primitive types:
.DS
fixed-length character vectors
variable-length character strings
specified-width unsigned integer data
.DE
.PP
The host and client may exchange the following commands
regarding window options:
.DS
.T=
#define	WOC_SET	0	/* change value of option */
#define	WOC_INQUIRE	2	/* ask about current option value */
#define	WOC_DO	4	/* do report changes to option */
#define	WOC_DONT	5	/* don't report changes to option */
#define	WOC_WILL	6	/* will report changes to option */
#define	WOC_WONT	7	/* won't report changes to option */
.DE
The ``set'' command is sent by either the client or the host
to specify the current value of a window option.
The ``inquire'' command is sent by either the client or the host
when it wishes to know the current value of an option.
The recipient of an ``inquire'' command responds with a
``set'' command that specifies the current option value.
.PP
The remaining four window option commands
are used by the host
to set up automatic reporting by the client
when the value of a window option changes.
If the host wishes to be informed
when the value of a window option changes
(\fIe.g.\fP when a window is retitled),
it sends a ``do'' command to the client.
The client responds to the ``do'' command
with a ``will'' command
followed immediately by a ``set'' command
(reporting the current value of the option).
Thereafter,
whenever the value of that option changes
the client will send a ``set'' command with the new value
to the host.
If the host wishes the client to stop sending
these ``set'' commands,
the host sends the client a ``don't'' command.
The client responds with a ``won't'' message.
.PP
The reporting status of generic window options is not affected
if the window emulation types changes;
however,
if the emulation type changes,
then reporting for emulation-specific options is ended.
If the host wishes the client to continue
reporting changes in some emulation-specific window options,
it must send the appropriate ``do'' commands.
.PP
Window option commands are grouped together and transmitted
collectively as part of a P2_FN_WOPT command.
That is,
the P2_IAC and P2_FN_WOPT command are immediately
followed by a variable-length string of bytes
which contain window option commands for one or
more options.
The end of a sequence of window option (sub)commands
is indicated by a command which specifies window option zero
(WOG_END).
.PP
All window option commands begin with a one or two
byte command specifier.
The one-byte form is called the ``short'' form,
while the two-byte form is the ``long'' form:
.DS
.ta 8n 32n 48n
#define	WONUM_MIN	1	/* minimum option number */
#define	WONUM_GENERIC	7	/* maximum generic option number */
#define	WONUM_SHORT	14	/* maximum short option number */
#define	WONUM_MAX	31	/* maximum option number */
#define	WONUM_MASK	(017<<3)	/* mask for extraction */
#define	WONUM_SENCODE(n)	(((n)&017)<<3)	/* short encoding function */
#define	WONUM_SDECODE(b)	(((b)>>3)&017)	/* short decoding function */
#define	WONUM_LPREFIX	(017<<3)	/*long encoding prefix */
#define	WONUM_LENCODE(n)	((n)+' ')	/* long encoding function */
#define	WONUM_LDECODE(c)	(((c)&0177)-' ')	/* long decoding function */
.DE
Commands
specifing options whose numbers are in the range WONUM_MIN to WONUM_SHORT
may use the short form.
In this case,
the window option number is encoded according to WONUM_SENCODE:
it is shifted left by three bits.
The command byte consists of a bitwise ``or'' of
the window option command
(\fIe.g.\fP WOC_INQUIRE)
and the encoded short option number.
.PP
Commands which specify options whose numbers are greater than WONUM_SHORT
must use the long form.
(The long form may be used for options whose numbers are less than WONUM_SHORT,
but there is no reason to do so.)
In this case,
the first byte contains a bitwise ``or'' of
the window option command
(\fIe.g.\fP WOC_INQUIRE)
and the special prefix WONUM_LPREFIX.
The second byte is encoded by WONUM_LENCODE:
the window option number is added to the ASCII code for a space
(thus this byte is always printable).
.PP
All of the window option commands begin with the
one or two byte option command specifier.
Unlike the other window option commands
(which use no additional bytes),
the WOC_SET command is followed by encoded data
(the value of the option).
Option values are constructed from three primitive
data types
(as noted above).
.IP chars 8n
The simplest type of data is a fixed-length character vector.
This is represented directly.
The vector must consist of printable characters.
[This restriction may be eliminated in the future.
The current implementation is able to process non-printable
characters
(including XON and XOFF)
correctly.]
.IP string
Like character vectors,
strings have a maximum length.
However,
unlike character vectors,
strings may contain non-printing characters.
Also,
while all characters in a character vector are sent,
a string may be shorter than its maximum length.
It is terminated by a null (000) byte.
[Hence,
a string may not contain an embedded null byte.]
.IP udata
The remaining data type is unsigned integer data.
This data has a fixed width measured in bits.
The value is encoded in ``little-endian'' fashion
into the low six bits of successive characters.
The (octal) 0100 bit of each character is set.
The number of characters required to hold an integer
varies from one
(for data which is one to six bits wide)
to six
(for data which is thirty-two bits wide).
.PP
The window options defined above have arguments as follows
(all integers are unsigned):
.IP WOG_VIS 20n
This is a 1-bit integer
which is nonzero iff the window is visible.
.IP WOG_POS
This consists of two 12-bit integers
which respectively specify the vertical and horizontal
offsets of the window on the client's screen.
.IP WOG_TITLE
This is a string of maximum length 256
which specifies the window's title.
.IP WOG_SIZE
This consists of two 12-bit integers
which respectively specify the vertical and horizontal
size of the window on the client's screen
(in pixels).
.IP WOTTY_SIZE
This consists of two 12-bit integers
which respectively specify the window size in rows and columns.
.IP WOTTY_FONTSZ
This is a 6-bit integer which is a font size index.
At present,
it specifies a ``small'' font if zero
and a ``large'' font if nonzero.
.IP WOTTY_MOUSE
This is a 1-bit integer
which is nonzero iff
mouse events are to be encoded and sent
as data to the host.
.IP WOTTY_BELL
This is a 2-bit integer.
The low-order bit is set iff the window should
display bells visually;
the other bit is set iff the client should report
bells within this window audibly.
.IP WOTTY_CURSOR
This is a 1-bit integer
which is zero if the window is using a block cursor
and nonzero if the window is using an underscore cursor.
.PP
One design decision which the author now regrets
is an overloading of the WOTTY_SIZE option.
If the host can handle window size changes on pseudo-terminals
(\fIe.g.\fP 4.3BSD can),
then the client is capable of changing the view size of a window
or its actual size,
according to the user's preference.
If the host cannot handle window size changes,
the client does not allow the view size to be changed.
The client assumes that the host can handle window size
changes if it receives a WOC_DO command for the WOTTY_SIZE option.
.SH
Protocol Negotiation
.PP
It is possible that at some time the versions of a \*(uw server
and client will not match;
.I e.g.
a version 2.10 client will be used with a version 3.4 server.
It is desirable that such combinations will work ``correctly'' \(em
that the server will communicate with the client using protocol 1
rather than trying to use protocol 2.
In order to accomplish this,
three new maintenance functions
are defined by which the server and client
may negotiate the protocol which is to be used.
Version 3 clients and servers recognize these maintenance
functions in both protocol 1 and protocol 2.
Older clients and servers do not recognize these functions
at all.
.PP
The protocol negotiation maintenance functions were
described above for protocol 2.
They are repeated here for protocol 1
(the encodings are identical):
.DS
.T=
#define	P1_MF_ASKPCL	2	/*	request protocol negotiation */
#define	P1_MF_CANPCL	3	/*	suggest protocol */
#define	P1_MF_SETPCL	4	/*	set new protocol */
.DE
P1_MF_ASKPCL is encoded in a single command byte
(following P1_IAC).
The P1_MF_CANPCL and P1_MF_SETPCL command bytes are
followed by an additional byte which names a protocol.
For the purposes of protocol negotiation,
protocols 1 and 2 are represented by the ASCII
characters space and exclamation-mark,
respectively.
.PP
The client and server always start operation in protocol 1.
(The user may have instructed the client to use protocol 2;
nonetheless,
it will use protocol 1 until protocol negotiations are complete.)
When the server is started
it will send a P1_MF_ENTRY maintenance command to the client.
If the client knows about protocol 2 and wishes to use it,
it will send a P1_MF_ASKPCL to the server.
If the client does not know about protocol 2,
it will not send P1_MF_ASKPCL,
protocol negotiation will never be started,
and both sides will continue to use protocol 1.
.PP
If the server can support something other than protocol 1
it will respond to the P1_MF_ASKPCL with a
P1_MF_CANPCL which names the most extensive protocol that it can support.
(At present,
this will be protocol 2;
however,
in the future it might name some other protocol.)
Old servers,
which do not recognize P1_MF_ASKPCL,
will ignore the maintenance function.
The client will time out after five seconds
and retry several times
(three in the present implementation);
if the server never responds
the client will ``give up'' and will
continue to use protocol 1 indefinitely.
.PP
When the client receives P1_MF_CANPCL from the server,
it will examine the server's suggested protocol.
If this protocol is unacceptable to the client,
it will respond with its own P1_MF_CANPCL,
naming the most extensive protocol that it can support.
The server,
upon receipt of this P1_MF_CANPCL,
will examine the client's suggested protocol.
If it is unacceptable to the server,
it will name the second-most extensive protocol that it can support.
Each time that the client or server receives a
P1_MF_CANPCL that names a protocol it cannot support,
it will respond with a different,
less extensive suggestion of its own.
Since the number of protocols is finite,
eventually someone will suggest protocol 1,
which both sides are required to support.
.PP
When the client or server receives a P1_MF_CANPCL
that names a protocol that it
.I can
support,
it will instruct its counterpart to start using that protocol
by sending a P1_MF_SETPCL that names that protocol.
Henceforth,
the new protocol will be used.
.PP
Protocol 2 allows the client to send a P2_MF_ENTRY
maintenance command to the server.
(This is encoded identically to a P1_MF_ENTRY command.)
If the server receives this maintenance command and
it is using a protocol other than protocol 1,
it will immediately respond with a P1_FN_SETPCL which
names the protocol that it is using.
It will then proceed to send ``new window'' and
(if applicable)
``window option'' commands to the client
to reconstruct the client's current state.
.SH
Postscript
.PP
There are a number of obvious problems with the mechanism
for protocol negotiation.
It is possible for one party to send a SETPCL
command and begin listening for input in a new protocol
while it is still receiving buffered commands in the old protocol
from the other party.
It probably would have been better to have established
a ``current protocol'',
similar to the ``current window'' scheme used for data transfer.
This scheme was born out of the desire to allow
old servers and clients to work with new ones.
It ``works'' if things are relatively quiescent,
as they are when the server first starts up
(before it creates its first window).
.PP
This document is still incomplete.
At this time it is more useful as a conceptual guide
to the \*(uw protocol
than a definitive reference.
.SH
Copyright
.LP
This document copyright 1986 by John D. Bruner.
Permission to copy is given,
provided that the copies are not sold
and that this copyright notice is included.
