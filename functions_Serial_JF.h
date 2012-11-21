/*////////////////////////////////////////////////////////////////
JF

functions_Serial_JF.h
version 1.0.0

Made Spring 2012 for the University of Kentucky Solar Car Team

This is the serial port wrapper function set made to be used for 
	basic Serial Port functions that the team uses.
	
The functions utilize the standard C18 library and expand on them.

The functions are described below. With an example code segment at
	the bottom.
////////////////////////////////////////////////////////////////*/
#ifndef _functions_serial_JF
#define _functions_serial_JF


/*///////////////////////////////////////////
Opens the Serial Port with the configuration
	that all of the boards use.

This sets a baud rate of 19200 on a 22.1 mHz
	oscillator (which is what all of the 
	boards at this time are using).
///////////////////////////////////////////*/
void openSerialPort(void);


#endif

/*/////////////////////EXAMPLE CODE///////////////////////////////
#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF

#include <stdio.h>
#include "functions_serial_JF.h"

void main(void)
{	
	openSerialPort();
	
	while(1)
	{
		printf("Good Morning Dave\n\r");
	}
	return;
}
/////////////////////END EXAMPLE CODE///////////////////////////*/