/*-----------------------------------------------------------------------------

   COMMS.C - Serial Communications Routines for Turbo C/C++

    This file is part of UW/PC - a multi-window comms package for the PC.
    Copyright (C) 1990-1991  Rhys Weatherley

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Revision History:
   ================

    Version  DD/MM/YY  By  Description
    -------  --------  --  ---------------------------------------------------
      1.0    ??/09/90  RW  Original version of COMMS.C
      1.1    10/11/90  RW  Added ability to call comcarrier before comenable,
      			   support for 57600 baud and automatic raising of
			   the DTR signal.
      1.2    17/11/90  RW  Add 'leavedtr' parameter to "comrestore".
      1.3    17/03/91  RW  Create 'comfix' to fix DOS shell-out bug.

-----------------------------------------------------------------------------*/

#include "comms.h"		/* Declarations for this module */
#include <dos.h>

/* Various PIC/COM masks and values */

#define PIC_MASK 0x21
#define	PIC_EOI  0x20
#define ERR_MSK	 0x9E

/* Definitions for interrupt handling */

/* COM port offsets */

#define COM_DATA	 0	/* Data received on this I/O address */
#define COM_IER  	 1	/* This register enables interrupts */
#define	COM_LCR		 3	/* Line control register */
#define COM_MCR		 4	/* Control Register (signals) */
#define COM_STAT	 5	/* Status Register */
#define	COM_MSR		 6	/* Modem Status Register */

/* Data for installing COM port interrupts */

#define COM_INT_1   0x0C	/* Interrupt 0x0C handles IRQ4 (or COM1) */
#define INT_MASK_1  0x10	/* Mask for PIC (programmable interrupt
				   controller) 8259A */
#define COM_INT_2   0x0B	/* Interrupt 0x0B handles IRQ3 (or COM2) */
#define INT_MASK_2  0x08	/* Mask for PIC (programmable interrupt
				   controller) 8259A */

/* Define the interrupt buffer structure */

#define	MAX_INT_BUFSIZ	4096
struct	IntBuf	{
		  char	buffer[MAX_INT_BUFSIZ];
		  int	input,output,size;
		  int	statport,dataport;
		  int	testbit;
		  int	oldier,oldmcr,oldlcr,oldbaud;
		};

static	struct	IntBuf	Com1Buf,Com2Buf;

/* Define the data for the interrupt handlers */

static void interrupt int_com1();
static void interrupt int_com2();
static void interrupt (*old_c1)();
static void interrupt (*old_c2)();

/* Change the test loop-back properties of a COM port (given MCR reg) */
static	int	_Cdecl	changetest (mcrport,on)
int	mcrport,on;
{
  if (on)
    {
      outportb (mcrport, inportb (mcrport) | 0x10);
      return (0x10);
    }
   else
    {
      outportb (mcrport, inportb (mcrport) & 0x0F);
      return (0);
    }
}

/* Turn a COM port's test loop-back mode on or off */
/* NOTE: comenable always turns the loop-mode off  */
void	_Cdecl	comtest (port,on)
int	port,on;
{
  switch (port)
    {
      case 1:	Com1Buf.testbit = changetest (Com1Buf.dataport + COM_MCR,on);
	        break;
      case 2:	Com2Buf.testbit = changetest (Com2Buf.dataport + COM_MCR,on);
	        break;
      default:	break;
    }
}

/* Return the number of available input chars on a COM port */
/* Will raise the DTR signal if it is not already raised.   */
int	_Cdecl	comavail (port)
int	port;
{
  switch (port)
    {
      case 1:	outportb (Com1Buf.dataport + COM_MCR,0x0B |
			  Com1Buf.testbit);	/* Raise DTR */
		return (Com1Buf.size);
      case 2:	outportb (Com2Buf.dataport + COM_MCR,0x0B |
			  Com2Buf.testbit);	/* Raise DTR */
		return (Com2Buf.size);
      default:	return 0;
    }
}

/* Flush all input from the COM port */
void	_Cdecl	comflush (port)
int	port;
{
  switch (port)
    {
      case 1:	disable ();	/* Disallow ints while flushing */
		Com1Buf.size = 0;
		Com1Buf.output = Com1Buf.input;
		enable ();
		break;
      case 2:	disable ();	/* Disallow ints while flushing */
		Com2Buf.size = 0;
		Com2Buf.output = Com2Buf.input;
		enable ();
		break;
      default:	break;
    }
} /* comflush */

/* Receive a character from the COM port: -1 if not ready */
/* Will raise the DTR signal if it is not already raised. */
int	_Cdecl	comreceive(port)
int	port;
{
  int ch;
  switch (port)
    {
      case 1:	outportb (Com1Buf.dataport + COM_MCR,0x0B |
				Com1Buf.testbit);	/* Raise DTR */
		if (Com1Buf.size == 0) return -1;
		else {
			ch = Com1Buf.buffer[Com1Buf.output];
			Com1Buf.output = (Com1Buf.output + 1) %
				MAX_INT_BUFSIZ;
			--Com1Buf.size;
			return ch;
		}
      case 2:	outportb (Com2Buf.dataport + COM_MCR,0x0B |
				Com2Buf.testbit);	/* Raise DTR */
		if (Com2Buf.size == 0) return -1;
		else {
			ch = Com2Buf.buffer[Com2Buf.output];
			Com2Buf.output = (Com2Buf.output + 1) %
				MAX_INT_BUFSIZ;
			--Com2Buf.size;
			return ch;
		}
      default:	return (-1);
    }
}

/*  COM1 Interrupt handler.  HARDWARE DEPENDENT */

static void interrupt int_com1()
{
  char ch;
  disable();	/* Make sure ints are disabled */
  /* If no error occured, then store the character */
  if ((ch = (inportb(Com1Buf.statport)&ERR_MSK)) == 0)
    {
      ch = inportb(Com1Buf.dataport);/* Get the character */
      if (Com1Buf.size < MAX_INT_BUFSIZ) /* Store the char */
        {
	  Com1Buf.buffer[Com1Buf.input] = ch;
	  Com1Buf.input = (Com1Buf.input + 1) % MAX_INT_BUFSIZ;
	  ++Com1Buf.size;
        }
    }
   else
    ch = inportb(Com1Buf.dataport);/* Remove the erroneous character */
  outportb(PIC_EOI,0x20);	/* Tell PIC we have handled the int */
}

/*  COM2 Interrupt handler.  HARDWARE DEPENDENT */

static void interrupt int_com2()
{
  char ch;
  disable();	/* Make sure ints are disabled */
  /* If no error occured, then store the character */
  if ((ch = (inportb(Com2Buf.statport)&ERR_MSK)) == 0)
    {
      ch = inportb(Com2Buf.dataport);/* Get the character */
      if (Com2Buf.size < MAX_INT_BUFSIZ) /* Store the char */
        {
	  Com2Buf.buffer[Com2Buf.input] = ch;
	  Com2Buf.input = (Com2Buf.input + 1) % MAX_INT_BUFSIZ;
	  ++Com2Buf.size;
        }
    }
   else
    ch = inportb(Com2Buf.dataport);/* Remove the erroneous character */
  outportb(PIC_EOI,0x20);	/* Tell PIC we have handled the int */
}

/* Enable a COM port given its I/O port address */
static	void	_Cdecl	doenable (port)
int	port;
{
  /* Set DLAB bit to zero to ensure that we */
  /* access the correct COM port registers  */
  outportb(port + COM_LCR,inportb(port + COM_LCR) & 0x7F);

  /* Turn off the chip's interrupts to start with */
  outportb(port + COM_IER,0);
  outportb(port + COM_MCR,8); /* Just DTR up for now */

  /* Read status and data ports to clear any outstanding errors */
  inportb(port + COM_STAT);
  inportb(port);

  /* Set ints for data available */
  outportb(port + COM_IER,1);
}

/* Enable a COM port for Interrupt Driven I/O by this module */
void	_Cdecl	comenable(port)
int	port;
{
  char ch;
  int dataport;
  switch (port)
    {
      case 1:	/* Initialise buffers for COM1 */
		Com1Buf.input = 0;
		Com1Buf.output = 0;
		Com1Buf.size = 0;
		dataport = *((int far *)0x400L);
		Com1Buf.dataport = dataport;
		Com1Buf.statport = Com1Buf.dataport + COM_STAT;
		Com1Buf.testbit = 0;

		doenable (dataport);	/* Setup the regs */

		/* Setup the ISR details */
		old_c1 = getvect(COM_INT_1); /* Save old COM1 int */
		setvect(COM_INT_1,int_com1); /* Install new int */

		/* Now turn on DTR, RTS and OUT2: all ready */
		outportb(dataport + COM_MCR,0x0B);

		/* Program the PIC to handle COM1 interrupts */
		ch = inportb(PIC_MASK);	/* Read current mask */
		ch &= (0xFF^INT_MASK_1);/* Reset mask for COM1 */
		outportb(PIC_MASK,ch);	/* Send it to the 8259A */
		break;
      case 2:	/* Initialise buffers for COM2 */
		Com2Buf.input = 0;
		Com2Buf.output = 0;
		Com2Buf.size = 0;
		dataport = *((int far *)0x402L);
		Com2Buf.dataport = dataport;
		Com2Buf.statport = Com2Buf.dataport + COM_STAT;
		Com2Buf.testbit = 0;

		doenable (dataport);	/* Setup the regs */

		/* Setup the ISR details */
		old_c2 = getvect(COM_INT_2); /* Save old COM2 int */
		setvect(COM_INT_2,int_com2); /* Install new int */

		/* Now turn on DTR, RTS and OUT2: all ready */
		outportb(dataport + COM_MCR,0x0B);

		/* Program the PIC to handle COM2 interrupts */
		ch = inportb(PIC_MASK);	/* Read current mask */
		ch &= (0xFF^INT_MASK_2);/* Reset mask for COM2 */
		outportb(PIC_MASK,ch);	/* Send it to the 8259A */
		break;
      default:	break;
    }
}

/* Save the current setting of a COM port to be restored later */
/* Call this function before calling 'comenable' on the port.  */
void	_Cdecl	comsave (port)
int	port;
{
  int dataport;
  switch (port)
    {
      case 1:	dataport = *((int far *)0x400L);
		Com1Buf.oldlcr = inportb (dataport + COM_LCR);
		Com1Buf.oldmcr = inportb (dataport + COM_MCR);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr & 0x7F);
		Com1Buf.oldier = inportb (dataport + COM_IER);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr | 0x80);
		Com1Buf.oldbaud = inport (dataport);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr);
      		break;
      case 2:	dataport = *((int far *)0x402L);
		Com2Buf.oldlcr = inportb (dataport + COM_LCR);
		Com2Buf.oldmcr = inportb (dataport + COM_MCR);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr & 0x7F);
		Com2Buf.oldier = inportb (dataport + COM_IER);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr | 0x80);
		Com2Buf.oldbaud = inport (dataport);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr);
      		break;
      default:	break;
    }
}

/* Set the communications parameters for a COM port */
void	_Cdecl	comparams (port,params)
int	port,params;
{
  static int divisors[12] =		/* COM port baud rate divisors */
	   {0x0417,0x0300,0x0180,0x00E0,0x0060,
	    0x0030,0x0018,0x000C,0x0006,0x0003,
	    0x0002,0x0001};
  int value,dataport;
  switch (port)
    {
      case 1: dataport = Com1Buf.dataport; break;
      case 2: dataport = Com2Buf.dataport; break;
      default: return;
    }
  if (params & BITS_8)
    value = 0x03;
   else
    value = 0x02;
  if (params & STOP_2)
    value |= 0x04;
  if (params & PARITY_ODD)
    value |= 0x08;
   else if (params & PARITY_EVEN)
    value |= 0x18;
  outportb (dataport + COM_LCR,value | 0x80);	/* Set params and DLAB */
  outport  (dataport,divisors[params & BAUD_RATE]); /* Set the baud rate */
  outportb (dataport + COM_LCR,value);		/* Clear DLAB bit */
}

/* Disable the interrupt I/O for a COM port */
/* If 'leavedtr' != 0, then leave DTR up    */
void	_Cdecl	comdisable(port,leavedtr)
int	port,leavedtr;
{
  char ch;
  switch (port)
    {
      case 1:	ch = inportb(PIC_MASK);	/* Get 8259A (PIC) Mask */
		ch |= INT_MASK_1;	/* Set Interrupt Mask COM1 */
		outportb(PIC_MASK,ch);	/* Write int. mask to 8259A*/

		/* Clear the interrupt enable register */
		outportb(Com1Buf.dataport + COM_IER,0);

		/* Clear OUT2, and set DTR as required */
		if (leavedtr)
		  outportb(Com1Buf.dataport + COM_MCR,1);
		 else
		  outportb(Com1Buf.dataport + COM_MCR,0);

		setvect(COM_INT_1,old_c1);/* Restore COM1 int */
		break;
      case 2:	ch = inportb(PIC_MASK);	/* Get 8259A (PIC) Mask */
		ch |= INT_MASK_2;	/* Set Interrupt Mask COM1 */
		outportb(PIC_MASK,ch);	/* Write int. mask to 8259A*/

		/* Clear the interrupt enable register */
		outportb(Com2Buf.dataport + COM_IER,0);

		/* Clear OUT2, and set DTR as required */
		if (leavedtr)
		  outportb(Com2Buf.dataport + COM_MCR,1);
		 else
		  outportb(Com2Buf.dataport + COM_MCR,0);

		setvect(COM_INT_2,old_c2);/* Restore COM2 int */
		break;
      default:	break;
    }
}

/* Restore the setting of a COM port - after 'comdisable'     */
/* If 'leavedtr' is non-zero then leave DTR up no matter what */
void	_Cdecl	comrestore (port,leavedtr)
int	port,leavedtr;
{
  int dataport;
  switch (port)
    {
      case 1:	dataport = *((int far *)0x400L);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr & 0x7F);
		outportb (dataport + COM_IER,Com1Buf.oldier);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr | 0x80);
		outport (dataport,Com1Buf.oldbaud);
		outportb (dataport + COM_LCR,Com1Buf.oldlcr);
		if (leavedtr)
		  Com1Buf.oldmcr |= 0x01;	/* Leave DTR in a high state */
		outportb (dataport + COM_MCR,Com1Buf.oldmcr);
      		break;
      case 2:	dataport = *((int far *)0x402L);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr & 0x7F);
		outportb (dataport + COM_IER,Com2Buf.oldier);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr | 0x80);
		outport (dataport,Com2Buf.oldbaud);
		outportb (dataport + COM_LCR,Com2Buf.oldlcr);
		if (leavedtr)
		  Com2Buf.oldmcr |= 0x01;	/* Leave DTR in a high state */
		outportb (dataport + COM_MCR,Com2Buf.oldmcr);
      		break;
      default:	break;
    }
}

/* Output a character to an I/O port */
static	void	_Cdecl	dosend(dataport,statport,ch)
int	dataport,statport,ch;
{
  while (!(inportb(statport) & 0x20))
    ;	/* wait till port is available */
  outportb(dataport,ch);
}

/* Output a character to a COM port.  Will raise */
/* the DTR signal if it is not already raised.   */
void	_Cdecl	comsend(port,ch)
int	port;
unsigned char ch;
{
  switch (port)
    {
      case 1:	outportb (Com1Buf.dataport + COM_MCR,0x0B |
				Com1Buf.testbit);	/* Raise DTR */
		dosend (Com1Buf.dataport,Com1Buf.statport,ch);
		break;
      case 2:	outportb (Com2Buf.dataport + COM_MCR,0x0B |
				Com2Buf.testbit);	/* Raise DTR */
		dosend (Com2Buf.dataport,Com2Buf.statport,ch);
		break;
      default:	break;
    }
}

/* Test to see if a carrier is present - can be called before comenable */
int	_Cdecl	comcarrier (port)
int	port;
{
  int dataport;
  switch (port)
    {
      case 1: dataport = *((int far *)0x400L);	/* Get dataport address */
      	      return ((inportb (dataport + COM_MSR) & 0x80) != 0);
      case 2: dataport = *((int far *)0x402L);	/* Get dataport address */
      	      return ((inportb (dataport + COM_MSR) & 0x80) != 0);
      default: return 0;
    }
}

/* Test to see if the DSR (Data Set Ready) signal is present */
int	_Cdecl	comdsr (port)
int	port;
{
  switch (port)
    {
      case 1: return ((inportb (Com1Buf.dataport + COM_MSR) & 0x20) != 0);
      case 2: return ((inportb (Com2Buf.dataport + COM_MSR) & 0x20) != 0);
      default: return 0;
    }
}

/* Test to see if the COM port is ready for a new */
/* character to transmit.  Will raise the DTR     */
/* signal if it is not already raised.		  */
int	_Cdecl	comready (port)
int	port;
{
  switch (port)
    {
      case 1: outportb (Com1Buf.dataport + COM_MCR,0x0B |
      				Com1Buf.testbit);	/* Raise DTR */
      	      return ((inportb (Com1Buf.statport) & 0x20) != 0);
      case 2: outportb (Com2Buf.dataport + COM_MCR,0x0B |
      				Com2Buf.testbit);	/* Raise DTR */
              return ((inportb (Com2Buf.statport) & 0x20) != 0);
      default: return 0;
    }
}

/* Drop the DTR signal on a COM port */
void	_Cdecl	comdropdtr (port)
int	port;
{
  switch (port)
    {
      case 1: outportb (Com1Buf.dataport + COM_MCR,0x0A |
				Com1Buf.testbit); break;
      case 2: outportb (Com2Buf.dataport + COM_MCR,0x0A |
				Com2Buf.testbit); break;
    }
}

/* Raise the DTR signal on a COM port */
void	_Cdecl	comraisedtr (port)
int	port;
{
  switch (port)
    {
      case 1: outportb (Com1Buf.dataport + COM_MCR,0x0B |
				Com1Buf.testbit); break;
      case 2: outportb (Com2Buf.dataport + COM_MCR,0x0B |
				Com2Buf.testbit); break;
      default: break;
    }
}

/* Set the BREAK pulse on a COM port to 0 or 1 */
void	_Cdecl	combreak (port,value)
int	port,value;
{
  switch (port)
    {
      case 1: outportb (Com1Buf.dataport + COM_LCR,
		(value ? inportb (Com1Buf.dataport + COM_LCR) | 0x40
		       : inportb (Com1Buf.dataport + COM_LCR) & 0xBF));
      case 2: outportb (Com2Buf.dataport + COM_LCR,
		(value ? inportb (Com2Buf.dataport + COM_LCR) | 0x40
		       : inportb (Com2Buf.dataport + COM_LCR) & 0xBF));
      default: break;
    }
}

/* Restore a COM port after a DOS shell-out, since */
/* a program may have disabled interrupts, etc.    */
void	_Cdecl	comfix (port)
int	port;
{
  int ch;
  switch (port)
    {
      case 1:	/* Restore the serial port interrupts for COM1 */
		outportb(Com1Buf.dataport + COM_IER,1);
		outportb(Com1Buf.dataport + COM_MCR,0x0B);
		ch = inportb(PIC_MASK);	/* Read current mask */
		ch &= (0xFF^INT_MASK_1);/* Reset mask for COM1 */
		outportb(PIC_MASK,ch);	/* Send it to the 8259A */
		break;
      case 2:	/* Restore the serial port interrupts for COM2 */
		outportb(Com2Buf.dataport + COM_IER,1);
		outportb(Com2Buf.dataport + COM_MCR,0x0B);
		ch = inportb(PIC_MASK);	/* Read current mask */
		ch &= (0xFF^INT_MASK_2);/* Reset mask for COM2 */
		outportb(PIC_MASK,ch);	/* Send it to the 8259A */
		break;
      default:	break;
    }
}
