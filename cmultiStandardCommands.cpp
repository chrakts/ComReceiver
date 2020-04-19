#include "ComReceiver.h"
#include "secrets.h"


char Compilation_Date[] = __DATE__;
char Compilation_Time[] = __TIME__;

uint8_t bootloader_attention;		// nur wenn true, dann darf Bootloader gestartet werden.
void (*bootloader)( void ) = (void (*)(void)) (BOOT_SECTION_START/2);       // Set up function pointer
void (*reset)( void ) = (void (*)(void)) 0x0000;       // Set up function pointer


void jobGotCRCError(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
	comRec->sendAnswer("CRC-Fehler",function,address,job,false);
}

void setBootloaderAttention(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
	if (strcmp((char *)pMem,BOOTLOADER_ATTENTION_KEY)==0)
    bootloader_attention=true;
  else
    bootloader_attention=false;
  comRec->sendAnswer("BL Status",function,address,job,bootloader_attention);
}

void startBootloader(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
  if(bootloader_attention)
  {
    comRec->sendAnswer("Goto BL",function,address,job,true);
    _delay_ms(600);
    bootloader();
  }
  else
    comRec->sendAnswer("Wrong Status",function,address,job,false);
}

void doReset(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
  comRec->sendAnswer("Do Reset",function,address,job,true);
  _delay_ms(600);
  reset();
}

void jobSetSecurityKey(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
uint8_t ret = true;
	if (strcmp((char *)pMem,SECURITY_LEVEL_PRODUCTION_KEY)==0)
	{
    comRec->SetSecurityLevel(PRODUCTION);
	}
	else if(strcmp((char *)pMem,SECURITY_LEVEL_DEVELOPMENT_KEY)==0)
	{
    comRec->SetSecurityLevel(DEVELOPMENT);
	}
	else
	{
    comRec->SetSecurityLevel(CUSTOMER);
		ret = false;
	}
	comRec->sendAnswerInt(function,address,job,comRec->GetSecurityLevel(),ret);
}

void jobGetSecurityKey(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
	comRec->sendAnswerInt(function,address,job,comRec->GetSecurityLevel(),true);
}

void jobGetCompilationDate(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
char temp[20];
	strcpy(temp,Compilation_Date);
	comRec->sendAnswer(temp,function,address,job,true);
}

void jobGetCompilationTime(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
char temp[20];
	strcpy(temp,Compilation_Time);
	comRec->sendAnswer(temp,function,address,job,true);
}

void jobGetFreeMemory(ComReceiver *comRec, char function,char address,char job, void * pMem)
{
extern int __heap_start, *__brkval;
int v;
	char answer[15];
	sprintf(answer,"%d",(int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
	comRec->sendAnswer(answer,function,address,job,true);
}

