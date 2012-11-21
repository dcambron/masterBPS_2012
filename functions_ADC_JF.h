/*////////////////////////////////////////////////////////////////
JF

functions_ADC_JF.h
version 1.0.0

Made Spring 2012 for the University of Kentucky Solar Car Team

This is the analog to digital wrapper function set made to be used
	for the basic ADC functions that the team uses.
	
The functions utilize the standard C18 library and expand on them.

The functions are described below. With an example code segment at
	the bottom.
////////////////////////////////////////////////////////////////*/

#ifndef _functions_adc_JF
#define _functions_adc_JF


/*///////////////////////////////////////////
Configures the ADC to read from Channel 0 
	(Pin RA1) using the internal voltage
	referencing.

This function was primarily designed for use
	with the CAN boards that do not have
	voltage reference traces run to them.
	(all of the current CAN boards that we
	have at this time.)
///////////////////////////////////////////*/
void configureADC(void);


/*///////////////////////////////////////////
Configures the ADC to read from Channel 0 
	(Pin RA1) using external voltage
	referencing.

This function was primarily designed for 
	compatibility with the 1st and 2nd
	generation BPS boards (7 module 
	monitoring) that have the traces set up
	for using external voltage referencing.
///////////////////////////////////////////*/
void configureADC_external(void);


/*///////////////////////////////////////////
This takes a single sample and returns the 
	value that is read from the ADC.
	
This function was primarily designed to 
	allow the user to get the raw value that 
	the ADC would read on a single sample 
	easily.
///////////////////////////////////////////*/
unsigned int readConversion(void);


/*///////////////////////////////////////////
This takes 1024 samples and returns the 
	average of those samples.
	
NOTE: This function returns a raw (averaged)
	ADC value.
	
This function was designed to mimic the
	original way of taking readings that was
	used for a while on the team.
	
The 15 bit oversample function takes the same
	amount of time, but retains greater
	accuracy.
///////////////////////////////////////////*/
unsigned int read10bitAverage(void);


/*///////////////////////////////////////////
This takes 1024 samples and returns a 15 bit
	oversampled value of those values
	
NOTE: This function returns an (averaged)
	ADC value.
	
This function was designed to take more 
	accurate readings.
///////////////////////////////////////////*/
unsigned int read15bitOversample(void);


/*///////////////////////////////////////////
This utilizes the read10bitAverage function
	and does the conversion to a voltage
	offset from 0 (up to 5 volts).

NOTE: This funciton assumes that the voltage
	low reference is 0V and the voltage high
	referenc is 5V.
	
This function was made to make it easy to 
	write a quick segment of code to return 
	the voltage that was read.
	
RETURNS:
	An unsigned int representing the mV value
		that was read by the ADC.
	EXAMPLE: a value returned as 87 is the
		voltage value 87 mV
///////////////////////////////////////////*/
unsigned int read10bitVoltageOffset(void);


/*///////////////////////////////////////////
This utilizes the read15bitOversample function
	and does the conversion to a voltage
	offset from 0 (up to 5 volts).

NOTE: This funciton assumes that the voltage
	low reference is 0V and the voltage high
	referenc is 5V.
	
This function was made to make it easy to 
	write a quick segment of code to return 
	the voltage that was read.
	
RETURNS:
	An unsigned int representing the 100uV 
		value that was read by the ADC.
	EXAMPLE: a value returned as 874 is 
		the voltage value 87.4 mV
///////////////////////////////////////////*/
unsigned int read15bitVoltageOffset(void);

#endif


/*/////////////////////EXAMPLE CODE///////////////////////////////
#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF

#include <delays.h>	//for using the delay function

#include <usart.h>
#include <stdio.h>
#include "functions_ADC_JF.h"	//for declarations of ADC functions
#include "functions_serial_JF.h"	//for declarations of Serial Port functions

void main(void)
{	
	unsigned int adcResult = 0;	//variable to hold the value read in
	
	openSerialPort();	//open the serial port for output
	
	configureADC();	//open using internal voltage referencing
	
	while(1)
	{
		adcResult = read10bitAverage();	//just get the raw average
		printf("raw 10 bit result is %d\r\n", adcResult);	//display the raw average
		adcResult = read10bitVoltageOffset();	//get the averaged value
		printf("10 bit result is %d\r\n", adcResult);	//display the averaged value in mV
		adcResult = read15bitVoltageOffset();	//get the 15 bit accurate value
		printf("15 bit result is %d.%d\r\n", adcResult/10, adcResult%10);	//display the 15 bit value in mV
		Delay10KTCYx(500);	//delay to not take up all of the processor time
	}
}
/////////////////////END EXAMPLE CODE///////////////////////////*/