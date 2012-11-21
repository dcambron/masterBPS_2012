
#ifndef _masterBPS_management_JF
#define _masterBPS_management_JF


void checkMessages(unsigned int *voltageArray, unsigned char *tempArray, unsigned char *bpsCheckinArray);
void checkCBS(unsigned int *voltageArray, unsigned char *currentModule, unsigned char numModules);
void checkinTest(unsigned char *bpsCheckinArray, unsigned char numModules);
unsigned char checkArray(unsigned int *voltageArray, unsigned char numModules, unsigned char arrayActive);
void sendData(int current);

int checkcurrent(int channelZeroInitial, int channelOneInitial);


void memcpy_reduced(void *output, void *input);
void failure(unsigned char type, unsigned char address, unsigned int intVal, unsigned char charVal);


#endif