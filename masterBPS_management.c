#include "ECANPoll.h"
#include "adc.h"	//to allow channel to be changed easily
#include "masterBPS.def"
#include "functions_ADC_JF.h"
#include "functions_EEPROM_JF.h"
#include <stdio.h>
#include <p18f4580.h>
#include <delays.h>

#include "masterBPS_management.h"

extern int glob_interrupt;
extern long glob_current;

//reads and returns the current
long checkcurrent()
{
	/*//code for original current sensor
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
	
	
	return current;//*/

/////////////////////////////////////////////////////////////////////////////////////
	//*//The following code works for the HAIS 50 P hall effect current sensor.
    //On current sensor: yellow wire is voltage reference of 2.5, and white wire is Vout
	// Code written on the 2012 race by Jimmy Frilling
	// Inserted into this master BPS code on 29-Apr-2013
	//Edited by Daniel Cambron on 1-May-2013
	//int currentCH0 = 0, currentCH1 = 0;
	unsigned char interruptstatus;
	int k = 0;
	long result = 0;	//return the final answer
	signed long int adcval = 0;	//need a long to store the value (long is 32 bits)
	signed long int ref = 0;
	long average = 0;
	
	//Calibration 1-May-2013 Daniel Cambron
	//tests show that Vout-Vref = 0.00001253*current in mA 
	//current in mA = (4.5/1024)*(average adc value)/.00001253 + 470
	// or approximately 348.477*(average adc value) + 470
	// instead of dividing to get the average adc value, we just run the loop 1024 times and find the sum, 
    // and then multiply the number down to 348.48.    348.48 = 1024 * 0.3403095, 
	//so run the loop 1024 times and multiply by 0.3403095 at the end 
	interruptstatus = GLOBALINTERRUPTS;
	GLOBALINTERRUPTS = INTERRUPTDISABLE;
	for(k=0;k<1024;++k){
		SetChanADC(ADC_CH1);	//channel has the voltage reference for the current sensor
      	ConvertADC();			//tell the ADC to run once
       	while(BusyADC());		//wait until the conversion is finished
       	ref = ReadADC();	//store the value that was converted into result
		SetChanADC(ADC_CH0);	//channel has the current sensor value
      	ConvertADC();			//tell the ADC to run once
       	while(BusyADC());		//wait until the conversion is finished
       	adcval = ReadADC();	//store the value that was converted into result
		average += adcval - ref;
	}
	GLOBALINTERRUPTS = interruptstatus;
	result = average;
	//a calibration offset of 470 mA, and a factor of 0.3403095197
	result = ( (float)result * 0.3403) + 470;

	if(result > CUTOFF_CURRENT_HIGH || result < CUTOFF_CURRENT_LOW){
		failure(CURRENTERR, 0xFF, result, 0x00);
	}
	return result;	
}

//long updateEnergy(long energy





//check for new CAN messages and if new messages are found then react accordingly
void checkMessages()
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
		if(addressReceived == SHUTDOWN){
			failure(addressReceived, 0xFF, 0x00, 0x00);
		}		
		//check for any more messages
		messageReceived = ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived);
	}	
	return;
}

void readSlaves(unsigned int *voltageArray, unsigned char *tempArray, unsigned char numModules)
{
	int i = 0, j = 0;
	char messageReceived = 0;
	unsigned char dataReceived[8];	//maximum length that can be recieved
	unsigned char lengthReceived, flagsReceived;
	unsigned long addressReceived = 0;
	unsigned long sendAddress;
	unsigned int receiveTimeoutCounter = 0;
	unsigned int slaveTimeoutCounter = 0;
	unsigned int newVoltage = 0;
	unsigned char newTemp = 0;
	
	for(i=0; i<numModules; ++i)
	{
        unsigned int timeOutCounterJohn = 1000; //how long we wait for a message
		slaveTimeoutCounter = 0;                //how many other messages we get before fail;
		sendAddress = (i << 2) | MASK_BPS_READING;
		//printf("i:%d\r\nadd:%lb\r\n", i, sendAddress);
		while(addressReceived != sendAddress)
		{
			receiveTimeoutCounter = 0;
			//ensure that the 0 transmit buffer is empty
			/*
			while((TXB0CON & 0b10000000) == 0)
			{
				for(j=0; j<100; ++j);
				arrayrelay=~arrayrelay;
				for(j=0; j<100; ++j);
				arrayrelay=~arrayrelay;
			}//*/
			//send the message until it is acknowledged by something
			timeOutCounterJohn = 1000;
			while(!ECANSendMessage((MASK_BPS_MASTER | sendAddress), NULL, 0, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME)  && timeOutCounterJohn) 
			{
				timeOutCounterJohn --;
                for(j=0; j<50; ++j);
				//led1=~led1;
				for(j=0; j<50; ++j);
				//led1=~led1;
			}
			
			//check for a reply from the master acknowleging the message
			while( (!ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived)) && receiveTimeoutCounter<=RECEIVETIMEOUT  && timeOutCounterJohn) 
			{
                 timeOutCounterJohn--;
            	//	++receiveTimeoutCounter;
				for(j=0; j<100; ++j);
				//led1=~led1;

			//	printf("\tRXcnt: %u\r\n", receiveTimeoutCounter);//need this here for timing
			}
			//printf("RXadd:%lb\r\n", addressReceived);
			
			//*
			++slaveTimeoutCounter;
			if(slaveTimeoutCounter >= SLAVETIMEOUT)
				{printf("Slave Timeout Error\r\n");failure(BPSERR, i, 0x00, 0x00);}//*/
		}
		//check and ensure that three byte has been recieved
		if(lengthReceived == 3)
		{
			//retrieve and store the values for voltage and temperature
			memcpy_reduced(&newVoltage, dataReceived);
			newTemp = dataReceived[2];
			
			//store the new values into their arrays
			voltageArray[i] = newVoltage;
			tempArray[i] = newTemp;
			//print out the values
			GLOBALINTERRUPTS = INTERRUPTDISABLE;
			printf("V[%.2d]=%u\n\r",i, newVoltage);
			printf("T[%.2d]=%.2d\n\r",i, newTemp);
			GLOBALINTERRUPTS = INTERRUPTENABLE;
			//if it is under voltage over voltage or over temperature then shut the car off
			if((newVoltage < CUTOFF_VOLTAGE_LOW) || 
				(newVoltage > CUTOFF_VOLTAGE_HIGH) ||
				(newTemp > CUTOFF_TEMP_HIGH))
			{
				failure(BPSERR, i, newVoltage, newTemp);
			}
		}
		else {printf("Message Corruption Error\r\n");failure(BPSERR, i, 0x00, 0x00);}
	
		//if there was an interrupt, check the current now.
		if(glob_interrupt && switch5 == SWITCHOFF) //switch5 disables current reading
		{
			glob_current=checkcurrent();
			printf("BC=%ld\n\r",glob_current);
			while(!ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING), &glob_current, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
			glob_interrupt = 0;
		}
	}
	return;
}

void checkCBS(unsigned int *voltageArray, unsigned char *currentModule, unsigned char numModules)
{
//*
	unsigned char action = BALANCEOFF;
	unsigned char dataReceived[8];	//maximum length that can be recieved
	unsigned char lengthReceived, flagsReceived;
	unsigned long addressReceived = 0;
	unsigned int my_CAN_id = ((unsigned int) *currentModule) << 2;
	unsigned int lowestVoltage = 50000;
	unsigned char lowestModule = *currentModule;
	unsigned char i;
	//determine the lowest module
	for(i=0; i<numModules;++i)
	{
		if(voltageArray[i]<lowestVoltage)
		{
			lowestVoltage = voltageArray[i];
			lowestModule = i;
		}
	}
	//turn off CBS power to the previously low module if it's actually charging
	//check to see if we have received a comfirm message from the slave
	if(*currentModule != MODULE_ID_NULL)
	{
		while(addressReceived != (MASK_CBS | my_CAN_id))
		{
			//keep sending the message until something responds
			while(!ECANSendMessage(MASK_BPS_MASTER | MASK_CBS | my_CAN_id, &action, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
			//read message
			while(!ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived));
		}
	}
	//turn on CBS power to the new low module if low module is low enough
	if(lowestVoltage < (CUTOFF_VOLTAGE_HIGH - 3000))
	{
		my_CAN_id = ((unsigned int)lowestModule) << 2;
		addressReceived = 0;
		action = BALANCEON;
		//check to see if we have received a comfirm message from the slave
		while(addressReceived != (MASK_CBS | my_CAN_id))
		{
			//keep sending the message until something responds
			while(!ECANSendMessage(MASK_BPS_MASTER | MASK_CBS | my_CAN_id, &action, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
			//read message
			while(!ECANReceiveMessage(&addressReceived, &dataReceived, &lengthReceived, &flagsReceived));
		}
		*currentModule = lowestModule;
	}
	else
	{
		*currentModule = MODULE_ID_NULL;
	}
	//print out the result
	printf("CBS=%d\n\r",*currentModule);
	while(!ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING|MASK_CBS), currentModule, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));	
	//*/
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

long checkSOC(unsigned int *voltageArray,unsigned char numModules)
{
	unsigned int lowestVoltage = 50000;
	unsigned char i;
	unsigned long volts;
	unsigned long charge;
	//determine the lowest voltage
	for(i=0; i<numModules;++i)
	{
		if(voltageArray[i]<lowestVoltage)
		{
			lowestVoltage = voltageArray[i];
		}
	}	
 	volts = (unsigned long)lowestVoltage;
	//fit this voltage to a function corresponding to the State Of Charge	
	charge = (volts*volts*5/100000-volts*2+20000);
	charge = charge * 288; //multiply by the number of cells
	return (long) charge; //this number should be in units of A*sec or coulombs. represents number of coulombs compared to total in fully charged pack
}
//if there was a bad reading then store the error and shut down the car.
void failure(unsigned char type, unsigned char address, unsigned int intVal, unsigned char charVal)
{
	unsigned int i=0;
	unsigned char action = BALANCEOFF;
	unsigned char send_data[4];
	memcpy_reduced(&(send_data[1]), &intVal);
	send_data[0] = address;
	send_data[3] = charVal;
	//send the message that the car is going to shut down and why
	while(!ECANSendMessage(MASK_MASTER_SHUTDOWN, send_data, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
	//Try to shut the CBS relay off for the module that's out of range. this should not be a problem if the relay fails to shut off.
	while(!ECANSendMessage(MASK_BPS_MASTER | MASK_CBS | ((unsigned int)(address << 2)), &action, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
	printf("Shutting Car Down\n\r");
	printf("Addr = %.2x\n\r",address);
	printf("Volt = %u\n\r",intVal);
	printf("temp = %.2d\n\r",charVal);
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
	//different PWM's for the led to let us know what type of error
	if(switch1 == SWITCHOFF) //switch1 casues data to continue being collected after the relay has been shut off
	{
		if(type==BPSERR){
			while(1){
				led1 = ~led1;
				Delay10KTCYx(0);
			}
		}
		else if(type == CURRENTERR){
			while(1){
				led1 = ~led1;
				Delay10KTCYx(0);
				led1 = ~led1;
				Delay10KTCYx(0);
				Delay10KTCYx(0);
			}
		}
		else if(type == SHUTDOWN){
			while(1){led1 = LEDOFF;}
		}
		else{
			while(1){
				led1 = ~led1;
				for(i=0; i<20; ++i){Delay10KTCYx(0);}
			}
		}
	}
	return;
}

void sendData(long current,unsigned char currentModule, long energy)
{
	printf("BC=%ld\n\r",current);
	while(!ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING), &current, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
	while(!ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING|MASK_CBS), &currentModule, 1, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
	while(!ECANSendMessage((MASK_BPS_MASTER|MASK_BPS_READING|MASK_ENERGY), &energy, 4, ECAN_TX_STD_FRAME | ECAN_TX_PRIORITY_0 | ECAN_TX_NO_RTR_FRAME));
	printf("CBS=%d\n\r",currentModule);	
	//printf("E=%ld\n\r",energy);
	return;
}

