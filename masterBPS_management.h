
#ifndef _masterBPS_management_JF
#define _masterBPS_management_JF


void checkMessages();
void checkCBS(unsigned int *voltageArray, unsigned char *currentModule, unsigned char numModules);
unsigned char checkArray(unsigned int *voltageArray, unsigned char numModules, unsigned char arrayActive);
void sendData(long current, unsigned char currentModule,long energy);
void readSlaves(unsigned int *voltageArray, unsigned char *tempArray, unsigned char numModules);
long checkSOC(unsigned int *voltageArray,unsigned char numModules);
long checkcurrent();


void memcpy_reduced(void *output, void *input);
void failure(unsigned char type, unsigned char address, unsigned int intVal, unsigned char charVal);


#endif