#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF

#define moduleCount 33

#include <p18f4480.h>
#include <stdio.h>
#include "functions_Serial_JF.h"
#include "masterBPS.def"
#include "masterBPS_management.h"
#include "masterBPS_initialize.h"


void main (void)
{
	unsigned long int i;
	int current = 0,channelZeroInitial = 0, channelOneInitial = 0;
	unsigned long int checkinCounter = 0;
	unsigned char arrayActive = 1;
	unsigned int voltage[moduleCount];
	unsigned char temperature[moduleCount];
	unsigned char bpsCheckin[5];
	
	
	unsigned char start = 0;
	
	
	//set all of the used pins to input 
	//	(so that we don't output anything wrong until everything is initialized)
	//best option is to set the unconnected pins to output and set to 0;
	//the list of unused pins:
	//RA 1,4,5,6,7 (0,2,3 are the adc pins used for current sensor)
	//RB 0,1,4,5,6,7 (2,3 are the CAN pins)
	//RC 4,5,6,7 (0 is the LED, 1,2,3 are the relay controls)
	//RD 1,2,3,4,5 (6 and 7 are the uart port lines)
	//RE 0,1,2 (all of the other bits in TRISE are config bits not for actual ports)
	TRISA = 0b00000000;
	TRISB = 0b00000000;
	TRISC = 0b00001111;
	TRISD = 0b00000000;
	TRISE = 0b00000000;
	LATB = 0x00;
	LATC = 0x00;
	LATD = 0x00;
	LATE = 0x00;
	
	///////////// Initial Routine ///////////////
	//ensure the relays are in the correct states
	RELAY_Initialize();
	//open the serial port
	//openSerialPort();
	//initialize the CAN system
	CAN_Initialize();
	//ensure the CBS system is off
	//CBS_Initialize();
	//setup the LED(s)
	LED_Initialize();
	//check to determine the last state of the BPS
	CheckForPreviousError();
	//setup the Analog to Digital converter
	ADC_Initialize(&channelZeroInitial, &channelOneInitial);
	//Check to see if all values are in range
	current = checkcurrent(channelZeroInitial, channelOneInitial);
	start = BPS_Initialize(moduleCount, bpsCheckin, voltage, temperature, current);
	//if everything is okay then start the car and continue running otherwise don't start the car
	if(start == 1)
		successful_Start();
	else
	{
		led1=LEDON;
		while(1);
	}

	while(1)
	{
		//led1 = ~led1;//leaving off for real use to save power
		checkMessages(voltage, temperature, bpsCheckin);
		current = checkcurrent(channelZeroInitial, channelOneInitial);
		sendData(current);
		//printf("%u\r\n%u\r\n",voltage[0], voltage[1]);
		//arrayActive= checkArray(voltage, moduleCount, arrayActive);
		//checkCBS();
		//printf("checkin[0]=0x%x\r\n", bpsCheckin[0]);
		if(checkinCounter >= CHECKINTIMEOUT)
		{
			led1=~led1;
			checkinTest(bpsCheckin, moduleCount);
			checkinCounter = 0;
		}
		checkinCounter++;
		//we are going to need a delay in here I think
		//don't yet know how long I want the delay to be
		for(i=0; i<400; ++i); //max for unsigned long is 4,294,967,295
	}

	return; //should never get here
}