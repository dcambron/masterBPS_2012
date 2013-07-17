
#ifndef _masterBPS_initialize_JF
#define _masterBPS_initialize_JF

void TIMER_Initialize(void);
void RELAY_Initialize(void);
void CBS_Initialize(void);
void CAN_Initialize(void);
void CheckForPreviousError(void);
void LED_Initialize(void);
void ADC_Initialize(int* channelZeroInitial, int* channelOneInitial);
void successful_Start(void);


#endif