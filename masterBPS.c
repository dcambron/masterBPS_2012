#pragma config OSC = HS     // Oscillator 
#pragma config PWRT = OFF   // Power on delay 
#pragma config WDT = OFF    // WatchDog Timer
#pragma config LVP = OFF    // Low Voltage Programming OFF

#define moduleCount 32	//actual number of modules to be checked

#include <p18f4580.h>
#include <stdio.h>
#include "functions_Serial_JF.h"
#include "masterBPS.def"
#include "masterBPS_management.h"
#include "masterBPS_initialize.h"
#include "isr.h"

long glob_energy = 0; //not used at the moment
long glob_current = 0; //current leaving the battery pack
int glob_interrupt = 0; //flag whenever an interrupt happens
int glob_CBSDelay = 0; //interrupt times the calling of the CBS function
unsigned char glob_currentModule = MODULE_ID_NULL; //module that the CBS is charging

////////INTERRUPT SERVICE ROUTINE///////////////////////////////
// start ISR code
#pragma code isr = 0x08 // store the below code at address 0x08
  // let the compiler know that the function isr() is an interrupt handler

void isr(void) //this function runs every 1/(((clock freq / 4) / prescaler) / (2^16)) seconds
{
    if(INTCONbits.TMR0IF) // if the interrupt flag has been set
    {
		glob_interrupt = 1;
		glob_CBSDelay++;
		INTCONbits.TMR0IF = 0;
    }    
	return;
}
#pragma interrupt isr 
#pragma code // return to the default code section
/////////////////////////////////////////// end ISR code////

void main (void)
{
	unsigned long int i;
	int channelZeroInitial= 0, channelOneInitial=0;	
	unsigned long int CBSDelay = 0; //CBS updates every so often
	unsigned char arrayActive = 1;
	unsigned int voltage[moduleCount];
	unsigned char temperature[moduleCount];
	//set all of the used pins to input 
	//	(so that we don't output anything wrong until everything is initialized)
	//best option is to set the unconnected pins to output and set to 0;
	//the list of unused pins:
	//RA 1,4,5,6,7 (0,2,3 are the adc pins used for current sensor)
	//RB 0,1,4,5,6,7 (2,3 are the CAN pins)
	//RC 4,6,7 (0 is the LED, 1,2,3 are the relay controls) 5 is switch3
	//RD 1, (6 and 7 are the uart port lines) 2,3,4,5 are switches
	//RE 0,1,2 (all of the other bits in TRISE are config bits not for actual ports)
	TRISA = 0b00000000;
	TRISB = 0b00000000;
	TRISC = 0b00101111;
	TRISD = 0b01111100;
	TRISE = 0b00000000;
	LATB = 0x00;
	LATC = 0x00;
	LATD = 0x00;
	LATE = 0x00;
	
	///////////// Initial Routine ///////////////
	//ensure the relays are in the correct states
	RELAY_Initialize();
	//open the serial port
	openSerialPort();
	//initialize the CAN system
	CAN_Initialize();
	//setup the LED(s)
	LED_Initialize();
	//check to determine the last state of the BPS
	CheckForPreviousError();
	//setup the Analog to Digital converter
	ADC_Initialize(&channelZeroInitial, &channelOneInitial);
	//Check to see if all values are in range
	if(switch5 == SWITCHOFF) glob_current = checkcurrent();
	readSlaves(voltage, temperature, moduleCount);
	//set up the timer which interrupts at a known frequency. (Used for Energy Calculation)
	TIMER_Initialize();
	//if we get here then all of the modules checked out alright and it is okay to start the car
	successful_Start();

	while(1)
	{
		readSlaves(voltage, temperature, moduleCount);
		arrayActive = checkArray(voltage, moduleCount, arrayActive);
		if(glob_CBSDelay > 60){
			checkCBS(voltage,&glob_currentModule, moduleCount);
			glob_CBSDelay = 0;
		}
		led1 = LEDOFF;
		if(switch1 == SWITCHON || switch2 == SWITCHON || switch5 == SWITCHON) led1 = LEDON;
		//sw1 cause program to keep runing after failure
		//sw5 disables current reading 
		for(i=0; i<400; ++i); //max for unsigned long is 4,294,967,295
	}
	return; //should never get here
}