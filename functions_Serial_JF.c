/*////////////////////////////////////////////////////////////////
JF

functions_Serial_JF.c
version 1.0.0

Made Spring 2012 for the University of Kentucky Solar Car Team
////////////////////////////////////////////////////////////////*/
#include <usart.h>
#include <stdio.h>	//used for the notification that the opening succeeded


void openSerialPort(void)
{
	//TRISC |= 0x80;
	//TRISC &= 0xBF;
	OpenUSART( USART_TX_INT_OFF &
	USART_RX_INT_OFF &
	USART_ASYNCH_MODE &
	USART_EIGHT_BIT &
	USART_CONT_RX &
	USART_BRGH_HIGH,71); //71 = baud rate of 19200 on a 22.1 mHz clock
	
	printf("Serial Port Online\n\r");	//print this out to inform that the serial port is up
	
	return;
}