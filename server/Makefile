#! /bin/make -f
#
#	Makefile for UW version 4.2
#
# Specify VERSION 4.3 for 4.3BSD, 4.2 for 4.2BSD (including Sun 3.x)
#
# DEFINES is a list of the site-dependent compile-time options:
# 	-DUTMP		attempt to update "/etc/utmp" (who) file
# 	-DV7TTYS	V7-format "/etc/ttys" (pre-4.3BSD)
#	-DSETOWN_BUG	fcntl(fd, F_SETOWN, pgrp) is broken (pre-4.3BSD)
#	-Dvoid=int	kludge around broken C compilers
#
# UTMP specifies that UW should attempt to update the "/etc/utmp" file.
# On some (foolish) systems this file is world-writeable.  Other
# installations may wish AFTER CAREFUL EXAMINATION OF THE ISSUES to
# install the UW server with group write access to the utmp file.
#
# V7TTYS should be used for V7-derived systems that do not have 4.3BSD's
# "/etc/ttys" file.  This includes 4.2BSD and Sun UNIX.  There is no
# support for USG's "/etc/inittab".
#
# SETOWN_BUG should be defined if the fcntl(fd, F_SETOWN, pgrp) system
# call is broken (as it is in 4.2BSD and Sun 3.0 UNIX).  On those
# machines the third argument must be negative to specify a process ID
# and positive to specify a process group.
#
# Some C compilers do not understand pointers to void functions.  The
# common kludge around this problem is to substitute "int" for "void"
# everywhere.  "lint" doesn't like this, but the program will compile
# into working code.
#
BINDIR	=	/usr/local/bin
INCDIR	=	/usr/include/uw
LIBDIR	=	/usr/local/lib
VERSION	=	4.3

DEFINES	=	-DUTMP

MKFILES	=	server/Makefile lib/Makefile utility/Makefile

all:	DEFINES $(MKFILES)
	cd server; make
	cd lib; make
	cd utility; make

depend:
	cd server; make depend
	cd lib; make depend
	cd utility; make depend

install:	all
	ln -s `pwd`/h $(INCDIR)
	install -s server/uw $(BINDIR)/uw
	install -s utility/uwtool $(BINDIR)/uwtool
	install -s utility/uwtitle $(BINDIR)/uwtitle
	install -s utility/uwterm $(BINDIR)/uwterm
	install -s utility/uwplot $(BINDIR)/uwplot
	cp lib/libuw.a $(LIBDIR); ranlib $(LIBDIR)/libuw.a

clean:
	cd server; make clean
	cd lib; make clean
	cd utility; make clean

DEFINES:	Makefile
	echo "" $(DEFINES) > DEFINES

server/Makefile:	Makefile server/Makefile_$(VERSION)
	cd server; cp Makefile_$(VERSION) Makefile; make depend

lib/Makefile:	Makefile lib/Makefile_$(VERSION)
	cd lib; cp Makefile_$(VERSION) Makefile; make depend

utility/Makefile:	Makefile utility/Makefile_$(VERSION)
	cd utility; cp Makefile_$(VERSION) Makefile; make depend
