UW/PC - Unix Windows for PC
===========================

This is a historical project of mine from the early 1990's called UW/PC.

UW/PC was a multiple-window interface to Unix for IBM-PC's and compatible
computers.  It was structured as a terminal program that could be used to
dial up to the Unix machine via a modem.

After connecting, you would start the "uw" server on the Unix side of the
connection.  This server could create up to seven virtual sessions over
the dial-up serial link, with special control sequences to switch between
sessions.

If a program in one session is printing data, the control sequences switch the
client to write to that window's buffer.  Similarly, if the user types in a
window, then control sequences are sent to switch back to that session on the
server side to deliver the keystrokes to the correct pty on the Unix side.
UW/PC uses ALT+1, ALT+2, etc to switch between windows.

The protocol was very efficient.  Under normal interactive use, the user
would hardly notice the overhead; even on slow 2400 baud modems.  There were
performance issues if one window was outputting a lot of text or performing a
file transfer, but interactive use was very snappy.

The "uw" server was originally created by John Bruner with a client for
Macintosh computers.  I made the PC client for MS-DOS, and eventually a
client for Windows 3.0.

The Linux "screen" utility provides similar functionality these days,
but the Unix Windows protocol was fairly unique in the late 1980's and
early 1990's.

See [UW.TXT](UW.TXT) for the historical documentation.

Screenshots
-----------

Unfortunately I don't have the server working yet on a system I can "dial"
into, but may put up some extra screenshots when I can do that.  Here are
some of the basic screens.  There is no online configuration; that was
done by manually editing the "UW.CFG" file.

<img alt="UW/PC 2.01 Startup" src="images/uwpc-201-startup.png" width="860"/>

<img alt="UW/PC 2.01 Help" src="images/uwpc-201-help.png" width="860"/>

<img alt="UW/PC 2.01 Quit" src="images/uwpc-201-quit.png" width="860"/>

Source Code
-----------

The source code for UW/PC for MS-DOS is under the "src" directory.
The source code for the Unix Windows server from John Bruner is in the
"server" directory.

I have cleaned up the code and checked in the source code for all
MS-DOS versions.  Branches in the repository called "uwpc-x.xx" are
provided for each major version I was able to recover from my archives:

* 1.00 (no source code found, only executables)
* 1.01
* 1.02 (first public release)
* 1.03
* 1.04
* 1.05 (not found, probably never released due to the rewrite in 2.00)
* 2.00 (total rewrite in C++)
* 2.01
* 2.02 (started the Windows 3.0 port)
* 2.03 (final version, work in progress)

The Windows front end is not yet checked in, only the MS-DOS front end.

The original MS-DOS code was designed to be built with Turbo C++ and
Turbo Assembler.  I switched to Borland C++ for the later Windows version.
You will need Turbo C++ or something similar if you want to build it.
I was able to successfully build the MS-DOS version with
[DOSBox](https://www.dosbox.com/) after installing Turbo C++ 1.01 and
Turbo Assembler 4.0 into the DOSBox environment.

John Bruner's original server code was designed to be built on mid-1980's
BSD 4.3 systems and is written in K&R C.  It would need a lot of work to
build it for modern ANSI C and POSIX compatible systems.

See the [HISTORY.TXT](HISTORY.TXT) file for release notes on each version.

TERMCC
------

By the time the project finished, I had support for a number of terminal
types in the "adm" and "vt" series.

Due to the complexity of implementing the escape sequence state machines,
I invented a little interpreted language.  The ".CAP" files were converted
into bytecode ".TRM" files which were then built into the executable.

The "Termcap Compiler", or "TERMCC", takes care of the conversion,
with the parser implemented using flex and bison.

Executables
-----------

I have stripped the original MS-DOS executables out of this repository,
but if you want a copy to run on your old PC then let me know.

License
-------

The first version "UW/PC 1.00" was labelled as freeware but it wasn't
until 1.02 that I made a public release.  I switched to the
GNU General Public License Version 1 for the public releases.
See the [COPYING](COPYING) file.

John Bruner's server code is distributed under the following license:

    Copyright 1985,1986 by John D. Bruner.  All rights reserved.  Permission to
    copy this program is given provided that the copy is not sold and that
    this copyright notice is included.

Future Work
-----------

The UW/PC project started in December 1990 and continued until mid-1992.

I had grand plans, some of which are documented at the end of
[UW.TXT](UW.TXT) and [src/readme](src/readme).  Windows support with
multiple floating windows in variable sizes, TEK4010 graphics support,
integrated FTP-style file transfer, NNTP clients, scroll-back buffers,
protocol 2 for advanced window control, and much more.  Bits and pieces
of these were already in progress.

However, the dial-up dumb terminal era was coming to a close and TCP/IP was
about to consume everything.  I also got distracted by other things,
namely my "Helldiver" e-mail client and USENET reader.  And then I got a
real job in 1995, working in Silicon Valley during the Dotcom era.
And that was the end of that.

So what does the future of UW/PC hold now?  Here are some ideas:

* Rewrite the "uw" server from scratch.  The BSD 4.3 server code is very
  old and crufty and modern ANSI C compilers do not like it one bit.
* Port the client to Linux, with the UI in curses or Qt.
* Figure out how to run a Unix Windows session over ssh instead of serial,
  which may create a very nice text-based windowing environment for
  remote machine access.
* Support modern terminal types like "xterm" in place of the older
  "adm31", "vt52", and "ansi" types from the original Unix Windows protocol.
* Consider using something like [libtsm](https://www.freedesktop.org/wiki/Software/libtsm/)
  to handle the complexity of modern terminal emulation.
* Add online configuration, so that manual editing of "UW.CFG" is no
  longer required.

Contact
-------

For more information on this code, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).
