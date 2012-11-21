#include "ECANPoll.h"
#include "adc.h"	//to allow channel to be changed easily
#include "masterBPS.def"
#include "functions_ADC_JF.h"
#include "functions_EEPROM_JF.h"
#include <stdio.h>

#include "masterBPS_management.h"


/////////incomplete////////////
//reads and returns the current
int checkcurrent(int channelZeroInitial, int channelOneInitial)
{
	int currentCH0 = 0, currentCH1 = 0, current = 0;
	unsigned char interruptstatus;
	/////////CURRENT///////////
	//get the channel 0 current
	//ensure that interrupts are disabled while doing the conversion
	interruptstatus = GLOBALINTERRUPTS;
	GLOBALINTERRUPTS = INTERRUPTDISABLE;
	SetChanADC(ADC_CH0);
	
	//From dale's code for the shunt current sensor
	currentCH0 = ((float) (read15bitOversample()  - 7.3284) * 0.001515656734 )* 100;      // Calculated new gains on 1/4/2012
	
	SetChanADC(ADC_CH1);
	currentCH1 = ((float) (read15bitOversample()  - 7.3284) * 0.001515656734 )* 100;      // Calculated new gains on 1/4/2012
	//enable interrupts until next conversion
	GLOBALINTERRUPTS = interruptstatus;
		
	//determine which value to use
	currentCH0 = currentCH0 - channelZeroInitial;
	currentCH1 = currentCH1 - channelOneInitial; //I am considering channel 1 to be the negative current direction
	if(currentCH0 >= 0 && currentCH1 >= 0)
	{
		//if both are positive we will just use the higher value (for now)
		if(currentCH0 > currentCH1)
			current = currentCH0;
		else
			current = -currentCH1;
	}
	else if(currentCH0 <= 0 && currentCH1 <= 0) current = 0;
	else if(currentCH0 <= 0 && currentCH1 >= 0) current = -currentCH1; //go with the positive value
	else if(currentCH0 >= 0 && currentCH1 <= 0) current = currentCH0;
	else current = 0;//just a default case
	
	
	return current;
}

//check for new CAN messages and if new messages are found then react accordingly
void checkMessages(unsigned int* voltageArray, unsigned char* tempArray, unsigned char* bpsCheckinArray)
{
	char messageReceived = 0;
	unsigned char dataReceived[8];	//maximum length that can be recieved
	unsigned char lengthReceived, flagsReceived;
	unsigned long addressReceived;
	
	messageReceived = ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived);
	//while there are still messages in the buffer
	while(messageReceived == 1)
	{
		//if I get here then I have recieved a message
		//Now parse the identifier and decide what I need to do
		//if it is a reading from one of the slaves
		if ((addressReceived | MASK_BPS_SLAVE_READING)==MASK_BPS_SLAVE_READING)
		{
			unsigned char slaveAddr = 0;
			unsigned int newVoltage = 0;
			unsigned char newTemp = 0;

			//check and ensure that three byte has been recieved
			if(lengthReceived == 3)
			{
				
				//first calculate the address of the slave
				slaveAddr = (addressReceived & MASK_BPS_SLAVE) >> 2;
				
				//then retrieve and store the values for voltage and temperature
				memcpy_reduced(&newVoltage, dataReceived);
				newTemp = dataReceived[2];
				
				//store the new values into their arrays
				voltageArray[(int)slaveAddr] = newVoltage;
				tempArray[slaveAddr] = newTemp;
				/*
				printf("Message Recieved\r\nslaveaddr = %.2x\r\nvoltage = %u\r\ntemp = %u\r\n",
					slaveAddr, newVoltage, newTemp);
				printf("voltageArray[%d] = %u\r\n", slaveAddr, voltageArray[(int)slaveAddr]);//*/
				//if it is under voltage over voltage or over temperature then shut the car off
				if((newVoltage < CUTOFF_VOLTAGE_LOW) || 
					(newVoltage > CUTOFF_VOLTAGE_HIGH) ||
					(newTemp > CUTOFF_TEMP_HIGH))
				{
					failure(BPSERR, slaveAddr, newVoltage, newTemp);
				}
				
				//set the corresponding checkin bit
				//	slaveAddr/8 will select the correct char to place the bit in
				//	|=	"or equals" will leave all of the other bits alone except the one
				//	1 << (slaveAddr%8)	will put a 1 in the correct position of the char
				bpsCheckinArray[slaveAddr/8] |= 1 << (slaveAddr%8);
				//printf("bpsCheckinArray[%d] = 0x%x\r\n", slaveAddr/8, bpsCheckinArray[slaveAddr/8]);
			}
			//if it is a shutdown instruction
			else if(addressReceived == SHUTDOWN)
			{
				failure(addressReceived, 0xFF, 0x00, 0x00);
			}
			
		}
		
		//check for any more messages
		messageReceived = ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived);
	}
	return;
}

/////////incomplete////////////
void checkCBS(unsigned int *voltageArray, unsigned char *currentModule, unsigned char numModules)
{
	return;
}

void checkinTest(unsigned char *bpsCheckinArray, unsigned char numModules)
{
	unsigned char i;
	unsigned char comparebyte;
	//scan through the array
	//if a module has failed to check in then shut off the car
	//	otherwise clear the bit and continue on through the array
	for(i=0; i<numModules; ++i)
	{
		//if the bps has not checked in then report a failure
		comparebyte = 1 << (i%8);
		//yes, yes I know this is confusing
		//so the checkinArray has 5 chars the (i/8) picks which char I want to check
		//  anding (&) with comparebyte ensures that only a single "1" is left in the
		//	byte that I am checking
		//	bitshifting right (>>) by (i%8) pushes that one to the first bit position
		//	by doing this I am left with either a 0 or a 1 indicating the value of this
		//	specific bpscheckin bit
		//printf("comparebyte = 0x%x\r\n", comparebyte);
		//printf("i = %d\r\nbpsCheckinArray[%d] = 0x%x\r\n", i, i/8, bpsCheckinArray[i/8]);
		if(((bpsCheckinArray[(i/8)]&comparebyte)>>(i%8)) ==0)
			failure(BPSERR, i, 0x00, 0x00);
		//NOTE: if the above doesn't work you could just check each char for any zeros,
		//	but I wanted to be able to determine which BPS did not check in.
	}
	//if it makes it here then all was okay
	//clear the array and exit
	bpsCheckinArray[0] = 0;
	bpsCheckinArray[1] = 0;
	bpsCheckinArray[2] = 0;
	bpsCheckinArray[3] = 0;
	bpsCheckinArray[4] = 0;
	
	return;
}

//ripoff of memcpy so that I don't have to include the string library
void memcpy_reduced(void *output, void *input)
{
	*(char *)output = *(char *)input;
	*(char *)((char *)output+1) = *(char *)((char *)input+1);
	return;
}

//check to determine if the array should be turned on
//returns whether or not the array is active
unsigned char checkArray(unsigned int *voltageArray, unsigned char numModules, unsigned char arrayActive)
{
	//need separate on and off commands so that the array relay does not keep turning on and off
	//	(to prevent toggling between on and off)
	unsigned char overVoltage = 0;
	unsigned char underVoltage = 0;
	unsigned char i = 0;
	//check for any of the modules being over the voltage value
	for(i=0; i<numModules; ++i)
	{
		//printf("voltage[%d] = %u\r\n", i, voltageArray[i]);
		if(voltageArray[i]>=ARRAY_CUTOFF)
			overVoltage = 1;
		if(voltageArray[i]<=ARRAY_CUTON)
			underVoltage = 1;
	}

	//if any of the batteries are over the array limit then turn the array off
	//	otherwise make sure that the array is on
	if((overVoltage == 1) && (arrayActive == 1))	//over an on -> turn off
	{
		//want to change this to send a message to the MPPT controllers sometime
		arrayrelay = RELAYOFF;
		//turn all of the MPPTs off (may change it to dynamically turn them on and off later
		//ECANSendMessage(MPPT_CONTROLLER, 0x00, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME);
		arrayActive = 0;
	}
	else if((underVoltage == 1) && (arrayActive == 0))	//under and off -> turn on
	{
		//want to change this to send a message to the MPPT controllers sometime
		arrayrelay = RELAYON;
		//turn all of the MPPTs off (may change it to dynamically turn them on and off later
		//ECANSendMessage(MPPT_CONTROLLER, 0xFF, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME);
		arrayActive = 1;
	}
	//other options (over and off     under and on) remain the same
	
	return arrayActive;
}

//if there was a bad reading then store the error and shut down the car.
void failure(unsigned char type, unsigned char address, unsigned int intVal, unsigned char charVal)
{
	unsigned int i=0;
	unsigned char send_data[4];
	memcpy_reduced(&(send_data[1]), &intVal);
	send_data[0] = address;
	send_data[3] = charVal;
	//send the message that the car is going to shut down and why
	ECANSendMessage(MASK_MASTER_SHUTDOWN, send_data, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME);
	
	//store the error type (don't bother with type 0(not an error))
	if(type==BPSERR)
	{
		writeByte(LOCATION_ERRTYPE, ((type<<6) & address));
		writeByte(LOCATION_INTVAL0, send_data[1]);
		writeByte(LOCATION_INTVAL1, send_data[2]);
		writeByte(LOCATION_CHARVAL, charVal);
	}
	else if(type == CURRENTERR)
	{
		writeByte(LOCATION_ERRTYPE, (type<<6));
		writeByte(LOCATION_INTVAL0, send_data[1]);
		writeByte(LOCATION_INTVAL1, send_data[2]);
	}
	else if(type == SHUTDOWN)
	{
		writeByte(LOCATION_ERRTYPE, (type<<6));
	}

	//wait very short time for message to get through to LCD and telemetry
	for(i=0; i<1000; ++i);
	
	//shut down the car
	arrayrelay = RELAYOFF;
	mainrelay = RELAYOFF;
	
	//wait for car to shut down (yes it seems pointless until you run it on a power supply)
	while(1);
	
	return;
}

void sendData(int current)
{
	unsigned char send_data[2];
	memcpy_reduced(send_data, &current);
	
	ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING), send_data, 2, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME);
	return;
}