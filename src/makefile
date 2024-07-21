# Makefile for UW/PC and Turbo C++
#  by Rhys Weatherley, 1991

# You will need to change the "BGIOBJ" variable to point to your version
# of "BGIOBJ" before using this makefile with Turbo C++.

# Note that BISON and FLEX are required to compile the source code for
# the Termcap Compiler.  If you do not have these, you can either contact
# the author for versions that have been modified for Turbo C++, or
# you can remove the references for building TERMCC.EXE from this makefile,
# and just use the supplied TERMCC.EXE for compiling the capability files.

#CFLAGS=-mc -v
CFLAGS=-mc
COMMS=comms.c
OBJECTS=main.obj screen.obj comms.obj client.obj keys.obj uw.obj display.obj \
	config.obj terminal.obj vt52.obj adm31.obj ansi.obj timer.obj \
	dialog.obj files.obj xmodem.obj clipbd.obj mouse.obj clients.obj \
	mail.obj login.obj
COMPILE=tcc $(CFLAGS)
BGIOBJ=c:\tc\bgi\bgiobj
LEXOPTS=-s
BSNOPTS=-dy
TFLAGS=-w-rch -w-rvl -w-dup -w-par
TOBJECTS=lexyy.obj y_tab.obj termcc.obj symbols.obj

all:		uw.exe termcc.exe #setmode.com

# Link the final executable for UW/PC.
uw.exe:		uw.lib
		$(COMPILE) -M -euw.exe uw.lib

# Create a library of all the objects for the UW/PC executable.
uw.lib:		$(OBJECTS) uw.rsp
		tlib uw.lib /C @uw.rsp

# Link the final executable for the Termcap Compiler.
termcc.exe:	$(TOBJECTS)
		$(COMPILE) $(TFLAGS) -M -etermcc.exe $(TOBJECTS)

# Compile the module COMMS.C [or COMMTEST.C] (everything else is CPP)
comms.obj:	comms.h $(COMMS)
		$(COMPILE) -ocomms.obj -c $(COMMS)

# Compile the files for the Termcap Compiler.
termcc.obj:	termcc.c
		$(COMPILE) $(TFLAGS) -c termcc.c

symbols.obj:	symbols.h symbols.c
		$(COMPILE) $(TFLAGS) -c symbols.c

lexyy.obj:	lexyy.c symbols.h
		$(COMPILE) $(TFLAGS) -c lexyy.c

y_tab.obj:	y_tab.c symbols.h opcodes.h
		$(COMPILE) $(TFLAGS) -c y_tab.c

lexyy.c:	termcc.l y_tab.c
		flex $(LEXOPTS) termcc.l

y_tab.c:	termcc.y
		bison $(BSNOPTS) termcc.y

# Transform the terminal descriptions into .OBJ files ready for linking.
vt52.obj:	vt52.trm
		$(BGIOBJ) /F vt52.trm vt52.obj _VT52_Driver VT52_TEXT

adm31.obj:	adm31.trm
		$(BGIOBJ) /F adm31.trm adm31.obj _ADM31_Driver ADM31_TEXT

ansi.obj:	ansi.trm
		$(BGIOBJ) /F ansi.trm ansi.obj _ANSI_Driver ANSI_TEXT

# Create the .TRM files from the .CAP terminal capability files.
.cap.trm:
		termcc $<

# Define the .TRM -> .EXE dependencies
vt52.trm:	termcc.exe
adm31.trm:	termcc.exe
ansi.trm:	termcc.exe

# Define the .OBJ -> .CPP dependencies for module compilation
.cpp.obj:
		$(COMPILE) -c { $<}

# Define the .OBJ -> .H dependencies
main.obj:	screen.h client.h uw.h keys.h config.h timer.h dialog.h \
		files.h comms.h clipbd.h mouse.h extern.h
screen.obj:	screen.h config.h mouse.h extern.h
client.obj:	client.h display.h config.h uw.h comms.h extern.h clipbd.h \
		mouse.h
keys.obj:	keys.h config.h
uw.obj:		uw.h comms.h client.h config.h display.h keys.h uwproto.h \
		screen.h timer.h extern.h mouse.h
display.obj:	display.h screen.h extern.h config.h
config.obj:	config.h comms.h client.h opcodes.h screen.h extern.h
terminal.obj:	client.h display.h opcodes.h screen.h config.h extern.h \
		uw.h
timer.obj:	timer.h
dialog.obj:	dialog.h uw.h client.h screen.h display.h extern.h keys.h \
		config.h
files.obj:	files.h client.h uw.h keys.h extern.h
xmodem.obj:	files.h client.h uw.h timer.h display.h extern.h
clipbd.obj:	clipbd.h client.h display.h keys.h uw.h mouse.h extern.h
mouse.obj:	client.h mouse.h screen.h extern.h
clients.obj:	uw.h mail.h client.h login.h display.h extern.h
mail.obj:	uw.h mail.h client.h display.h extern.h config.h
login.obj:	uw.h client.h config.h login.h

# Create the small SETMODE.COM program from the debug script SETMODE.SCR
setmode.com:	setmode.scr
		debug <setmode.scr

# Create the backup ZIP file UWPC202X.ZIP
backup:
		-pkzip -u uwpc202x.zip *.c *.cpp *.h *.l *.y *.doc uw.cfg \
			uweg.cfg setmode.scr makefile history *.cap readme

# Make a beta-release distribution (without server, etc) UWPC201B.ZIP
beta:		uw.exe setmode.com
		-tdstrip uw.exe
		-pkzip -u uwpc202b.zip uw.exe *.doc uweg.cfg termcc.exe \
				uwftp.c setmode.com history

# Create the distribution ZIP file UWPC202.ZIP
dist:		uw.exe termcc.exe setmode.com
		mkdir dist
		-tdstrip uw.exe
		-tdstrip termcc.exe
		copy copying dist
		copy uw.exe dist
		copy uw.doc dist
		copy uweg.cfg dist
		copy uw.lng dist
		copy termcc.exe dist
		copy termcc.doc dist
		copy setmode.com dist
		copy history dist
		del dist\uw-src.zip
		del lexyy.c
		del y_tab.c
		del y_tab.h
		-pkzip -a dist\uw-src.zip readme *.c *.cpp *.h *.l *.y *.cap \
				uw.rsp setmode.scr uwftp.doc makefile
		copy uwserver.tar dist
		del uwpc202.zip
		-pkzip -a uwpc202.zip dist\*.*

# Clean up all non-essential generated compilation files.
clean:
		del *.obj
		del *.trm
		del *.map
		del uw.lib
		del uw.bak
		del lexyy.c
		del y_tab.c
		del y_tab.h

# Clobber all automatically generated compilation files
clobber:	clean
		del uw.exe
		del termcc.exe
