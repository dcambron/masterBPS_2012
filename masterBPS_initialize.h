
#ifndef _masterBPS_initialize_JF
#define _masterBPS_initialize_JF

void RELAY_Initialize(void);
void CBS_Initialize(void);
void CAN_Initialize(void);
void CheckForPreviousError(void);
void LED_Initialize(void);
void ADC_Initialize(int* channelZeroInitial, int* channelOneInitial);
unsigned char BPS_Initialize(unsigned char numModules, unsigned char *bpsCheckinArray, unsigned int *voltageArray, unsigned char *tempArray, int current);
void successful_Start(void);


#endif