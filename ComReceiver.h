#ifndef COMRECEIVER_H
#define COMRECEIVER_H

#include "Communication.h"
#include "CRC_Calc.h"
#include "ctype.h"
#include "ComReceiver.h"

enum{NOPARAMETER=0,STRING,UINT_8,UINT_16,UINT_32,FLOAT,BYTEARRAY};
enum {NO_ERROR = 0,ERROR_SPEICHER,ERROR_PARAMETER,ERROR_JOB,ERROR_TRANSMISSION};
enum{MEMORY_ERROR,PARAMETER_ERROR,UNKNOWN_ERROR,TRANSMISSION_ERROR,SECURITY_ERROR,CRC_ERROR,NO_ACTIVE_SENSOR};

enum{CRC_NIO,CRC_IO,CRC_NO,CRC_YES};
enum{ RCST_WAIT=0,RCST_L1,RCST_L2,RCST_HEADER,RCST_Z1,RCST_Z2,RCST_BR2,RCST_Q1,RCST_Q2,RCST_KEADER,RCST_WAIT_NODE,RCST_WAIT_FUNCTION,RCST_WAIT_ADDRESS,RCST_WAIT_JOB,RCST_DO_JOB,RCST_WAIT_END1,RCST_WAIT_END2,RCST_WAITCRC,RCST_CRC,RCST_GET_DATATYPE,RCST_GET_PARAMETER,RCST_NO_PARAMETER,RCST_ATTENTION};
enum{CUSTOMER,PRODUCTION,DEVELOPMENT};


#define MAX_TEMP_STRING 55

class ComReceiver;


struct Command
{
	char function;
	char job;
	char security;
	uint8_t ptype;
	uint8_t pLength;
	void  (*commandFunction)  (ComReceiver *comRec, char function,char address,char job, void *parameterMem);
};

typedef struct Command COMMAND;

struct Information
{
  const char *quelle;
	char function;
	char address;
	char job;
	uint8_t ptype;
	uint8_t pLength;
	void *targetVariable;
	void  (*gotNewInformation)  ();
	//void  (*commandFunction)  (ComReceiver *comRec, char function,char address,char job, void *parameterMem);
};

typedef struct Information INFORMATION;


class ComReceiver
{
  public:
    ComReceiver(Communication *output,const char *Node, COMMAND *commands, uint8_t numCommands, INFORMATION *information, uint8_t numInformation);
    virtual ~ComReceiver();

    void doJob();
    void comStateMachine();
    void *getMemory(uint8_t type,uint8_t num);
    void interpreteParameter();
    void free_parameter(void);
    uint8_t Getparameter_text_length() { return parameter_text_length; }
    void Setparameter_text_length(uint8_t val) { parameter_text_length = val; }
    uint8_t Getparameter_text_pointer() { return parameter_text_pointer; }
    void Setparameter_text_pointer(uint8_t val) { parameter_text_pointer = val; }
    char Gettemp_parameter_text() { return *temp_parameter_text; }
    void Settemp_parameter_text(char val) { *temp_parameter_text = val; }
    uint8_t Gettemp_parameter_text_length() { return temp_parameter_text_length; }
    void Settemp_parameter_text_length(uint8_t val) { temp_parameter_text_length = val; }
    char Getquelle() { return quelle[3]; }
    void Setquelle(char val) { quelle[3] = val; }
    uint8_t GetBroadcast() { return isBroadcast; }
    uint8_t GetSecurityLevel() { return SecurityLevel; }
    void SetSecurityLevel(uint8_t val) { SecurityLevel = val; }
    Communication *Getoutput(){return outCom;}
    void sendAnswerInt(char function,char address,char job,uint32_t wert,uint8_t noerror);
    void sendAnswerDouble(char function,char address,char job,uint32_t wert,uint8_t noerror);
    void sendAnswer(char const *answer,char function,char address,char job,uint8_t noerror);
    void sendPureAnswer(char function,char address,char job,uint8_t noerror);
  protected:

  private:
    uint8_t rec_state;
    uint8_t function;
    uint8_t job;
    uint8_t crc;
    bool encryption=false;
    uint8_t address;
    char *parameter_text;
    uint8_t parameter_text_length;
    uint8_t parameter_text_pointer;
    char *temp_parameter_text;
    uint8_t temp_parameter_text_length;
    uint8_t temp_parameter_text_pointer;
    uint8_t bootloader_attention;
    uint8_t reset_attention;
    char quelle[3];
    char node[3];
    uint8_t SecurityLevel;
    const char *fehler_text[7]={"memory errors","parameter error","unknown job","no transmission","command not allowed","CRC error","no active sensor"};
    uint8_t isBroadcast=false;
    COMMAND *commands;
    INFORMATION *information;
    Communication *outCom;
    uint8_t numCommands;
    uint8_t numInformation;
    CRC_Calc crcGlobal;
};

#endif // COMRECEIVER_H
