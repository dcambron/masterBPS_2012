/*////////////////////////////////////////////////////////////////
JF

functions_ADC_JF.c
version 1.0.1

Made Spring 2012 for the University of Kentucky Solar Car Team
changes from 1.0.1
	- modified the configureADC_external function to setup both channels (0 and 1)
////////////////////////////////////////////////////////////////*/

#include "functions_ADC_JF.h"
#include <adc.h>	//needed for the analog to digital functions from the C18 library


void configureADC(void)
{
	TRISA |= 0x02;	//make sure that RA1 is set to input
	OpenADC(ADC_FOSC_32 &		//adc frequency (from old bps code)
		ADC_RIGHT_JUST &    //least significant bits
		ADC_12_TAD,
		ADC_CH0 &		//channel 0
		ADC_REF_VDD_VSS &		//use internal voltages as reference
		ADC_INT_OFF, ADC_4ANA);	//disable interrupts

	return;
}


void configureADC_external(void)
{

	TRISA |= 0x0F;		//need to ensure that the references are set to inputs
	ADCON1 = 0x3B;  //ADCON1 to now accept external reference voltage
	/*OpenADC( ADC_FOSC_32	&		//adc frequency (from old bps code)
		ADC_RIGHT_JUST	&     //least significant bits
		ADC_12_TAD,
		ADC_CH0		&		//channel 0
		ADC_VREFPLUS_EXT	&	//use external positive voltage reference
		ADC_VREFMINUS_EXT	&	//use external negative voltage reference
		ADC_INT_OFF,	ADC_4ANA	);		//interrupts disabled//*/
	OpenADC( ADC_FOSC_32       &  		
         ADC_RIGHT_JUST    &	 	
         ADC_20_TAD,	  		 
         ADC_CH0      &
    	 ADC_REF_VREFPLUS_VREFMINUS &	// VREFPLSU and VREFMINUS are tied to external reference
		 ADC_INT_OFF,  ADC_2ANA   );	// ADC_2ANA indicates that AN0 and AN1 are analog inputs

	return;
}


unsigned int readConversion(void)
{
	unsigned int result = 0;
	ConvertADC();	//tell the ADC to run once
	while(BusyADC());	//wait until the conversion is finished
	result = ReadADC();	//store the value that was converted into result
	
	return result;
}


unsigned int read10bitAverage(void)
{
	int k = 0;
	unsigned int result = 0;	//return the final answer
	unsigned long int average = 0;	//need a long to store the value (long is 32 bits)
	//get and sum the value 1024 (2^10) times
    for(k=0;k<1024;++k)
	{
      	ConvertADC();			//tell the ADC to run once
       	while(BusyADC());		//wait until the conversion is finished
       	average += ReadADC();	//store the value that was converted into result
	}
    result = average >> 10;        //averages the reads (divide result by 2^10)  (bitshift cheaper than divide)
	return result;
}


unsigned int read15bitOversample(void)
{
	int k = 0;
	unsigned int result = 0;	//return the final answer (int will be long enough it is 16 bits)
	unsigned long int average = 0;	//need a long to store the value (long is 32 bits)
	//get and sum the value 1024 (2^10) times
    for(k=0;k<1024;++k)
	{
      	ConvertADC();			//tell the ADC to run once
       	while(BusyADC());		//wait until the conversion is finished
       	average += ReadADC();	//store the value that was converted into result
	}
    result = average >> 5;        //divide result by 2^5 to retain the 15 bit accuracy
	return result;
}


unsigned int read10bitVoltageOffset(void)
{
	unsigned int result = 0;
	//ASSUMING
	//5 volt upper limit
	//0 volt lower limit
	//NOTE: I used alot of extra typecasting to ensure that the value would come out the way I intended
	//			It may be usesless I never checked it all.
	result = read10bitAverage() * ((float)( (float)(5-0) / (float)(1024) ) * 1000);//2^10 = 1024 for the 10 bit resolution			//1000 to display in mV
	return result;
}


unsigned int read15bitVoltageOffset(void)
{
	unsigned int result = 0;
	//ASSUMING
	//5 volt upper limit
	//0 volt lower limit
	//NOTE: I used alot of extra typecasting to ensure that the value would come out the way I intended
	//			It may be usesless I never checked it all.
	result = read15bitOversample() * ((float)( (float)(5-0) / (float)(32768) ) * 10000);//2**15 = 32768 for the 15 bit resolution			//10000 to display in .1 mV
	return result;
}
