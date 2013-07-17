//#include "slaveBPS.def"
#include "ECANPoll.h"
#include "functions_ADC_JF.h"
#include "masterBPS.def"
#include "adc.h"	//to allow channel to be changed easily

#include "masterBPS_initialize.h"
#include "functions_EEPROM_JF.h"
#include "masterBPS_management.h"

//set the CBS relay to off and set the direction registers and returns which module is being balanced
//	(for now there should be no module balanced so it returns 0)
//NOTE: I am assuming that we will not go over 40 modules so currentModule only has 5 elements (5*8=40)
void CBS_Initialize(void)
{
	//set whatever relay may be controlling the CBS supply to off
	
	//set the TRIS register for that relay to output

	//DC 2-May-2013: nothing to do here bacause we're powering CBS relay off main relay driver
	
	
	return;
}


void TIMER_Initialize(void)
{
	T0CONbits.TMR0ON = 0; // Stop the timer
	T0CONbits.T08BIT = 0; // Run in 16-bit mode
	T0CONbits.T0CS = 0; // Use system clock to increment timer
	T0CONbits.PSA = 0; // A prescaler is assigned for Timer0
	T0CONbits.T0PS2 = 1; // Use a 1:64 prescaler
	T0CONbits.T0PS1 = 0;
	T0CONbits.T0PS0 = 1;
	
	//INTCONbits.IPEN = 0; //priority disable
	INTCONbits.GIEH = 1; // Global Interrupt Enable
	INTCONbits.TMR0IE = 1; // Enable Timer0 interrupt (occurs on overflow)
	INTCONbits.TMR0IF = 0; // Clear Timer0 interrupt flag

	T0CONbits.TMR0ON = 1; // Start the timer

	return;
}



//sets the LED's to off initially and sets the direction registers
void LED_Initialize(void)
{
	//set to off before declaring it as an output
	//	(just good practice with outputs)
	led1 = LEDOFF;
	
	//set the direction register
	TRISC &= 0b11111110;//RC0 is the LED (set to output (0))
	
	//the following initializes the switches
	CMCON |= 0b00000111; //disable the comparator
	TRISD |= 0b01111100;
	TRISC |= 0b00100000;
}

//opens the CAN port
void CAN_Initialize(void)
{
	//ensure proper TRIS settings
	TRISB |= 0b00001000;//set the CAN input (RB3) to input
	TRISC &= 0b11111011;//set the CAN output (RB2) to output
			
	//actually start the CAN module
	ECANInitialize();
	
	return;
}

//sets up the analog to digital converter
void ADC_Initialize(int* channelZeroInitial, int* channelOneInitial)
{
	
	//sets up the ADC to read from external voltage references from channel 0
	configureADC_external();
	//configureADC();//for first tests I am going to use internal referencing
	
	SetChanADC(ADC_CH0);
	(*channelZeroInitial) = ((float) (read15bitOversample()  - 7.3284) * 0.001515656734 )* 100;      // Calculated new gains on 1/4/2012
	SetChanADC(ADC_CH1);
	(*channelOneInitial) = ((float) (read15bitOversample()  - 7.3284) * 0.001515656734 )* 100;      // Calculated new gains on 1/4/2012

	return;
}

void RELAY_Initialize(void)
{
	//set all relays to off
	precharge = RELAYOFF;
	mainrelay = RELAYOFF;
	arrayrelay = RELAYOFF;
	
	//set the TRIS registers
	TRISC &= 0b11110001;
	
	return;
}

void CheckForPreviousError(void)
{
	unsigned char dataByte;
	unsigned char errorType;
	unsigned char bpsMask = 0x3F;
	unsigned char send_data[4];
	//read in data from EEPROM
	dataByte = readByte(LOCATION_ERRTYPE);
	
	//check to see if any error flags were set
	//if flags were sent then send the error out on the CAN bus
	//	then clear the error from EEPROM
	//using the bitshift to reduce the number to 0, 1, 2, or 3
	errorType = (dataByte>>6);
	
	//if there was an error
	if(errorType > NOERR)
	{
		//determine which error
		//	read in the necessary data and then send it out over the CAN bus
		if(errorType == BPSERR)
		{
			//determine the BPS number of the error
			send_data[0] = dataByte & bpsMask;
			send_data[1] = readByte(LOCATION_INTVAL0);
			send_data[2] = readByte(LOCATION_INTVAL1);
			send_data[3] = readByte(LOCATION_CHARVAL);
			
			while(!ECANSendMessage(MASK_MASTER_SHUTDOWN, send_data, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
		}
		else if(errorType == CURRENTERR)
		{
			send_data[0] = 0xFF;
			send_data[1] = readByte(LOCATION_INTVAL0);
			send_data[2] = readByte(LOCATION_INTVAL1);
			send_data[3] = 0x00;
			
			while(!ECANSendMessage(MASK_MASTER_SHUTDOWN, send_data, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
		}
		else //if(errorType == SHUTDOWN)	//just going to let it default here
		{
			send_data[0] = 0xFF;
			send_data[1] = 0x00;
			send_data[2] = 0x00;
			send_data[3] = 0xFF;
			
			while(!ECANSendMessage(MASK_MASTER_SHUTDOWN, send_data, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
		}
		//since we have reported the error go ahead and clear the error bits
		//just so we can request the last error data I will leave all of the data
		//	untouched except for the errorType
		//(dataByte & bpsMask) will clear the error bits but leave the other data intact
		writeByte(LOCATION_ERRTYPE, (dataByte & bpsMask));
	}
	
	//otherwise there was no previous error just continue on
	
	return;
}

//use this function to run the relays in the startup sequence
void successful_Start(void)
{
	unsigned long int i = 0;
	//turn on precharge
	precharge = RELAYON;
	
	//wait for specified time
	for(i=0; i<PRECHARGETIME; ++i);
	
	//turn on main
	mainrelay = RELAYON;
	
	//wait for short time
	for(i=0; i<MAINWAITTIME; ++i);
	
	//turn off precharge
	precharge = RELAYOFF;
	
	//if appropriate turn on array
	arrayrelay = RELAYON;
	
	return;
}
