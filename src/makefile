# Makefile for UW/PC and Borland C++
#  by Rhys Weatherley, 1991

#.swap

# Note: this should still compile with Turbo C++ but I don't use it anymore.

# You will need to change the "BGIOBJ" variable to point to your version
# of "BGIOBJ" before using this makefile with Borland C++.

# Note that BISON and FLEX are required to compile the source code for
# the Termcap Compiler.  If you do not have these, you can either contact
# the author for versions that have been modified for Borland C++, or
# you can remove the references for building TERMCC.EXE from this makefile,
# and just use the supplied TERMCC.EXE for compiling the capability files.

# -DTRANS_DEBUG		Turn on debugging of transmitted characters.

#CFLAGS=-ml -v -DTRANS_DEBUG
#CFLAGS=-ml -v
CFLAGS=-ml
COMMS=comms.c
OBJECTS=main.obj screen.obj comms.obj client.obj keys.obj uw.obj display.obj \
	config.obj terminal.obj vt52.obj adm31.obj ansi.obj timer.obj \
	dialog.obj files.obj xmodem.obj clipbd.obj mouse.obj clients.obj \
	mail.obj login.obj vt100.obj dialdos.obj dconfig.obj
COMPILE=tcc $(CFLAGS)
BGIOBJ=c:\tc\bgi\bgiobj
LEXOPTS=-s
BSNOPTS=-dy
TFLAGS=-w-rch -w-rvl -w-dup -w-par
TOBJECTS=lexyy.obj y_tab.obj termcc.obj symbols.obj tcap2cap.obj
TVINCS=c:\bc\tvision\include
TVLIBS=c:\bc\tvision\lib\tv.lib

#all:		noasm uw.exe termcc.exe vt102.trm avatar.trm
all:		uw.exe termcc.exe vt102.trm avatar.trm

# The following dummy target is for Borland C++ 3.0 which seems to think that
# if an .ASM file exists, it shouldn't recompile the .CPP file, but just
# assemble the .ASM file. :-(

noasm:
		-del *.asm

uw:		uw.exe

termcc:		termcc.exe

uwinstall:	uwinstal.exe

# Link the final executable for UW/PC.
uw.exe:		uw.lib
		$(COMPILE) -M -euw.exe uw.lib

# Create a library of all the objects for the UW/PC executable.
uw.lib:		$(OBJECTS) uw.rsp
		-del uw.lib
		tlib uw.lib /C @uw.rsp

# Link the final executable for the Termcap Compiler.
termcc.exe:	$(TOBJECTS)
		$(COMPILE) $(TFLAGS) -M -etermcc.exe $(TOBJECTS)

# Compile and link the installation program UWINSTAL.
uwinstal.exe:	uwinstal.cpp
		$(COMPILE) -I$(TVINCS) -M -euwinstal.exe uwinstal.cpp $(TVLIBS)

# Compile the module COMMS.C [or COMMTEST.C] (everything else is CPP)
comms.obj:	comms.h $(COMMS)
		$(COMPILE) -ocomms.obj -c $(COMMS)

# Compile the files for the Termcap Compiler.
termcc.obj:	termcc.c
		$(COMPILE) $(TFLAGS) -c termcc.c

symbols.obj:	symbols.h symbols.c
		$(COMPILE) $(TFLAGS) -c symbols.c

tcap2cap.obj:	tcap2cap.c
		$(COMPILE) $(TFLAGS) -c tcap2cap.c

lexyy.obj:	lexyy.c symbols.h
		$(COMPILE) $(TFLAGS) -c lexyy.c

y_tab.obj:	y_tab.c symbols.h opcodes.h uwproto.h
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

vt100.obj:	vt100.trm
		$(BGIOBJ) /F vt100.trm vt100.obj _VT100_Driver VT100_TEXT

# Create the .TRM files from the .CAP terminal capability files.
# The VT100 terminal type is compiled specially from the ANSI terminal type.

vt100.trm:	ansi.cap termcc.exe
		termcc -t vt100 ansi.cap vt100.trm

vt102.trm:	ansi.cap termcc.exe
		termcc -t vt102 ansi.cap vt102.trm

avatar.trm:	ansi.cap termcc.exe
		termcc -t avatar ansi.cap avatar.trm

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
		files.h comms.h clipbd.h mouse.h extern.h dial.h display.h
screen.obj:	screen.h config.h mouse.h extern.h
client.obj:	client.h display.h config.h uw.h comms.h extern.h clipbd.h \
		mouse.h keys.h
keys.obj:	keys.h config.h
uw.obj:		uw.h comms.h client.h config.h display.h keys.h uwproto.h \
		screen.h timer.h extern.h mouse.h opcodes.h
display.obj:	display.h screen.h extern.h config.h
config.obj:	config.h comms.h client.h opcodes.h screen.h extern.h \
			uwproto.h
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
dialdos.obj:	dial.h config.h uw.h display.h extern.h dialog.h client.h \
			comms.h
dconfig.obj:	extern.h config.h screen.h comms.h opcodes.h

# Create the SETMODE.EXE mode setting program.
setmode.exe:	setmode.c
		$(COMPILE) -esetmode.exe setmode.c

# Create the backup ZIP file UWPC202X.ZIP
backup:
		-pkzip -u uwpc202x.zip *.c *.cpp *.h *.l *.y *.doc uw.cfg \
			uweg.cfg makefile history *.cap readme

# Make a beta-release distribution (without server, etc) UWPC201B.ZIP
beta:		uw.exe setmode.exe
		-tdstrip uw.exe
		-pkzip -u uwpc202b.zip uw.exe *.doc uweg.cfg termcc.exe \
				uwftp.c setmode.exe history

# Create the distribution ZIP files: UWPC202E.ZIP, UWPC202S.ZIP
# and UWPCSERV.ZIP.  This is only guaranteed to work on my system. :-)
dist:		all
		mkdir dist
		-tdstrip uwinstal.exe
		-tdstrip uw.exe
		-tdstrip termcc.exe
		-tdstrip setmode.exe
		copy readme dist
		copy copying dist
		copy uwinstal.exe dist
		copy uw.exe dist
		copy ..\win3\uw-w.exe dist
		copy uw.doc dist
		copy uweg.cfg dist
		copy uw.lng dist
		copy ..\win3\uwfont.fon dist
		copy termcc.exe dist
		copy termcc.doc dist
		copy setmode.exe dist
		copy scripts.tar dist
		copy history dist
		del dist\uw-terms.zip
		-pkzip -a dist\uw-terms.zip vt102.trm avatar.trm *.cap
		del uwpc202s.zip
		del lexyy.c
		del y_tab.c
		del y_tab.h
		-pkzip -a uwpc202s.zip readme.src *.c *.cpp *.h *.l *.y *.cap \
				uw.rsp uwftp.doc makefile
		del uwpc202e.zip
		-pkzip -a uwpc202e.zip dist\*.*
		del uwpcserv.zip
		-pkzip -a uwpcserv.zip readme.srv uwserver.tar

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
