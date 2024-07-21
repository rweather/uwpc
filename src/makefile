# Makefile for UW/PC and Turbo C++
#  by Rhys Weatherley, January 1991

# CFLAGS=-v
CFLAGS=
OBJECTS=main.obj device.obj comms.obj uw.obj term.obj keys.obj config.obj \
	ascii.obj
COMPILE=tcc -mc $(CFLAGS)

all:		uw.exe #setmode.com

# Link the final executable for UW/PC
uw.exe:		$(OBJECTS)
		$(COMPILE) -euw.exe $(OBJECTS)

# Define the .OBJ -> .C dependencies for module compilation
.c.obj:
		$(COMPILE) -c { $<}

# Define the .OBJ -> .H dependencies
comms.obj:	comms.h
device.obj:	device.h comms.h keys.h uw.h
main.obj:	device.h keys.h uw.h term.h config.h uwlib.h ascii.h
uw.obj:		uw.h uwproto.h device.h term.h uwlib.h
term.obj:	term.h uw.h device.h keys.h uwlib.h
keys.obj:	keys.h
config.obj:	config.h device.h comms.h term.h uwlib.h keys.h
ascii.obj:	ascii.h uw.h uwlib.h term.h

# Create the small SETMODE.COM program from the debug script SETMODE.SCR
setmode.com:	setmode.scr
		debug <setmode.scr

# Create the backup ZIP file UWPC104B.ZIP
backup:
		-pkzip -u uwpc104b.zip *.c *.h uw.doc uw.cfg uweg.cfg \
			setmode.scr makefile history

# Create the distribution ZIP file UWPC104.ZIP
dist:		uw.exe setmode.com
		mkdir dist
		-tdstrip uw.exe
		copy copying dist
		copy uw.exe dist
		copy uw.doc dist
		copy uweg.cfg dist
		copy setmode.com dist
		copy history dist
		del dist\uw-src.zip
		-pkzip -a dist\uw-src.zip *.c *.h setmode.scr makefile
		copy uwserver.tar dist
		del uwpc104.zip
		-pkzip -a uwpc104.zip dist\*.*

# Clean up all .OBJ files from this directory
clean:
		del *.obj

# Clobber all automatically generated compilation files
clobber:	clean
		del uw.exe
