#ifndef CMULTISTANDARDCOMMANDS_H_INCLUDED
#define CMULTISTANDARDCOMMANDS_H_INCLUDED

#define cmultiStandardCommands		\
    {'-','-',CUSTOMER,NOPARAMETER,0,jobGotCRCError}, \
    {'S','A',PRODUCTION,STRING,16,setBootloaderAttention},   \
    {'S','B',PRODUCTION,NOPARAMETER,0,startBootloader},   \
    {'S','R',PRODUCTION,NOPARAMETER,0,doReset},   \
		{'S','K',CUSTOMER,STRING,16,jobSetSecurityKey}, \
		{'S','k',CUSTOMER,NOPARAMETER,0,jobGetSecurityKey}, \
		{'S','C',DEVELOPMENT,NOPARAMETER,0,jobGetCompilationDate}, \
		{'S','T',DEVELOPMENT,NOPARAMETER,0,jobGetCompilationTime}, \
		{'S','m',PRODUCTION,NOPARAMETER,0,jobGetFreeMemory}


void jobGotCRCError(ComReceiver *comRec, char function,char address,char job, void * pMem);
void setBootloaderAttention(ComReceiver *comRec, char function,char address,char job, void * pMem);
void startBootloader(ComReceiver *comRec, char function,char address,char job, void * pMem);
void doReset(ComReceiver *comRec, char function,char address,char job, void * pMem);
void jobGetCompilationDate(ComReceiver *comRec, char function,char address,char job, void * pMem);
void jobGetCompilationTime(ComReceiver *comRec, char function,char address,char job, void * pMem);
void jobSetSecurityKey(ComReceiver *comRec, char function,char address,char job, void * pMem);
void jobGetSecurityKey(ComReceiver *comRec, char function,char address,char job, void * pMem);
void jobGetFreeMemory(ComReceiver *comRec, char function,char address,char job, void * pMem);


#endif // CMULTISTANDARDCOMMANDS_H_INCLUDED
