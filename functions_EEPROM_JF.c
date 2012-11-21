/*////////////////////////////////////////////////////////////////
JF

functions_EEPROM_JF.c
version 1.0.0

Made Spring 2012 for the University of Kentucky Solar Car Team
////////////////////////////////////////////////////////////////*/

#include "functions_EEPROM_JF.h"

//included for register names needed for accessing EEPROM
#include <p18f4480.h>

unsigned char readByte(unsigned char address)
{
	unsigned char byte;	//byte to be read

	EEADR = address;	//put address to read in address register
	EECON1bits.EEPGD = 0;	//Point to Data memory
	EECON1bits.CFGS = 0;	//access EEPROM
	EECON1bits.RD = 1;	//instruct PIC to read the address
	byte = EEDATA;	//read byte from the data register

	return byte;
}

void writeByte(unsigned char address, unsigned char byte)
{
	EECON1bits.EEPGD = 0;	//Point to Data Memory
	EECON1bits.CFGS = 0;	//access EEPROM
	EECON1bits.WREN = 1;	//enable writing to EEPROM (normally disabled to prevent accidental writing)
	
	EEADR = address;	//put address to store to into the address register
	EEDATA = byte;	//put information to store in the data register
	
	//stuff to actually tell the PIC to store it (in the PIC documentation)
	EECON2 = 0x55;
	EECON2 = 0x0AA;
	EECON1bits.WR = 1;	//instruct to write
	
	EECON1bits.WREN = 0;	//disable writing to EEPROM again
	
	return;
}

/*//other possible forms of the readbyte function
unsigned char readByte(unsigned char address, unsigned char *byte)
{

	EEADR = address;	//put address to read in address register
	EECON1bits.EEPGD = 0;	//Point to Data memory
	EECON1bits.CFGS = 0;	//access EEPROM
	EECON1bits.RD = 1;	//instruct PIC to read the address
	(*byte) = EEDATA;	//read byte from the data register

	return (*byte);
}

void readByte(unsigned char address, unsigned char *byte)
{

	EEADR = address;	//put address to read in address register
	EECON1bits.EEPGD = 0;	//Point to Data memory
	EECON1bits.CFGS = 0;	//access EEPROM
	EECON1bits.RD = 1;	//instruct PIC to read the address
	(*byte) = EEDATA;	//read byte from the data register

	return (*byte);
}//*/