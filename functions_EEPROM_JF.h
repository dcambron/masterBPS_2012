/*////////////////////////////////////////////////////////////////
JF

functions_EEPROM_JF.h
version 1.0.0

Made Spring 2012 for the University of Kentucky Solar Car Team

This is the EEPROM  wrapper functions made to be used for basic 
	EEPROM functions.
	
The readByte and writeByte functions both follow exactly the code
	sequence given in the PIC 4480 documentation

The functions are described below. With an example code segment at
	the bottom.
////////////////////////////////////////////////////////////////*/
#ifndef _functions_serial_JF
#define _functions_serial_JF

/*///////////////////////////////////////////
Reads the byte at the given address from the
	EEPROM.
	
This function implements the code sequence
	given in the PIC 4480 datasheet (pg 107)
	
PIC 4480 and PIC 4580 both have 256 bytes
	of EEPROM addressed from 0x00 to 0xFF
///////////////////////////////////////////*/
unsigned char readByte(unsigned char address);


/*///////////////////////////////////////////
Stores the given byte at the given address 
	to the EEPROM.
	
This function implements the code sequence
	given in the PIC 4480 datasheet (pg 107)
	
PIC 4480 and PIC 4580 both have 256 bytes
	of EEPROM addressed from 0x00 to 0xFF
///////////////////////////////////////////*/
void writeByte(unsigned char address, unsigned char byte);

#endif

/*//other possible forms of the readByte function
unsigned char readByte(unsigned char address, unsigned char *byte);

void readByte(unsigned char address, unsigned char *byte);
//*/


/*/////////////////////EXAMPLE CODE///////////////////////////////

//This code will bring a number out of EEPROM print it, increment 
//	it, print it again and then put it back into the same EEPROM 
//	memory location.
//The effect that this will have is to increment the number every
//	time the PIC is powered on.

#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF

#include <p18f4480.h>
#include "functions_Serial_JF.h"
#include "functions_EEPROM_JF.h"
#include <stdio.h>

void main(void)
{
	unsigned char mytestnumber;
	unsigned char address = 0x0A;
	
	//read number from 0x0A and store into mytestnumber
	mytestnumber = readByte(address);

	openSerialPort();
	printf("mytestnumber = %d\n\r", mytestnumber);

	mytestnumber++;
	
	printf("mytestnumber = %d\n\r", mytestnumber);
	
	//put the incremented value back into EEPROM at 0x0A
	writeByte(address, mytestnumber);

	while(1);
	return;
}
/////////////////////END EXAMPLE CODE///////////////////////////*/


