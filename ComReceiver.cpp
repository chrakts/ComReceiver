#include "ComReceiver.h"

ComReceiver::ComReceiver(Communication *output,const char *Node, COMMAND *_commands, uint8_t _numCommands, INFORMATION *_information, uint8_t _numInformation)
{
  //ctor
  outCom = output;
  commands = _commands;
  information = _information;
  numCommands = _numCommands;
  numInformation = _numInformation;
  strcpy(node,Node);
  rec_state = RCST_WAIT;
  function=0;
  job=0;
  address=0;
  crc=CRC_NO;
  parameter_text=NULL;
  bootloader_attention = false;
  reset_attention = false;
}

ComReceiver::~ComReceiver()
{
  //dtor
}

void ComReceiver::doJob()
{
	if (  (rec_state == RCST_DO_JOB) && (job > 0) )
	{
    if(job==255) // das ist der CRC-Fehler-JOB
    {
      outCom->sendAnswer(fehler_text[CRC_ERROR],quelle,commands[0].function,address,commands[0].job,false);
    }
    else
    {
      interpreteParameter();
      if(isBroadcast==false)
      {
        if (SecurityLevel < commands[job-1].security)
        {
           outCom->sendAnswer(fehler_text[SECURITY_ERROR],quelle,commands[job-1].function,address,commands[job-1].job,false);
        }
        else
        {
          if ( job<=numCommands )
          {
            commands[job-1].commandFunction(this,commands[job-1].function,address,commands[job-1].job, parameter_text);
          }
        }
      }
      else // isBroadcast==true
      {
        switch( information[job-1].ptype )
        {
          case FLOAT:
            *( (float *)information[job-1].targetVariable ) = ((float *)parameter_text)[0];
          break;
          case UINT_32:
            *( (uint32_t *)information[job-1].targetVariable ) = ((uint32_t *)parameter_text)[0];
          break;
          case UINT_8:
            *( (uint8_t *)information[job-1].targetVariable ) = ((uint8_t *)parameter_text)[0];
          break;
          case STRING: // nicht getestet
            strncpy( (char *)information[job-1].targetVariable , (char *)parameter_text, (size_t)information[job-1].pLength);
          break;
        }
        if(information[job-1].gotNewInformation != NULL)
          information[job-1].gotNewInformation();
        _delay_ms(30);
      }
    }
		free_parameter();
		rec_state = RCST_WAIT;
		function = 0;
		job = 0;
	}
}

void ComReceiver::interpreteParameter()
{
uint8_t ptype,pLength,parameterCounter=0;
void *parameterPointer=nullptr;

  if( encryption==true )
  {
    uint8_t data[16];
    uint8_t i;
    for( i=0;i<16;i++)
      data[i] = ( (parameter_text[2*i]-65)<<4 )  +  (parameter_text[2*i+1]-65);
    outCom->decryptData(data);
    outCom->getEncryptData(data);
    for( i=0;i<16;i++)
      parameter_text[i] = data[i];
    parameter_text[16] = 0;
  }

  if( isBroadcast==false)
  {
    ptype = commands[job-1].ptype;
    pLength = commands[job-1].pLength;
  }
  else
  {
    ptype = information[job-1].ptype;
    pLength = information[job-1].pLength;
  }
  switch(ptype)
  {
    case FLOAT:
      parameterPointer = malloc(sizeof(float)*pLength);
    break;
    case UINT_8:
      parameterPointer = malloc(sizeof(uint8_t)*pLength);
    break;
    case UINT_16:
      parameterPointer = malloc(sizeof(uint16_t)*pLength);
    break;
    case UINT_32:
      parameterPointer = malloc(sizeof(uint32_t)*pLength);
    break;
  }

  switch(ptype)
  {
    case BYTEARRAY:
      if( encryption==false )
      {
        uint8_t i;
        for( i=0;i<pLength;i++)
          parameter_text[i] = ( (parameter_text[2*i]-65)<<4 )  +  (( parameter_text[2*i+1]-65)&0x0f );
      }
    break;
    case STRING:
    break;
    default:
      char *abschnitt;
      abschnitt = strtok(parameter_text, ";,");

      while(abschnitt != nullptr)
      {
        uint32_t wert;
        switch(ptype)
        {
          case UINT_8:
            wert = strtoul(abschnitt,NULL,0);
            if(wert<=255)
              ((uint8_t *)parameterPointer)[parameterCounter] = wert;
            else
              wert=0;
          break;
          case UINT_16:
            wert = strtoul(abschnitt,NULL,0);
            if(wert<=65535)
              ((uint16_t *)parameterPointer)[parameterCounter] = wert;
            else
              wert=0;
          break;
          case UINT_32:
            ((uint32_t *)parameterPointer)[parameterCounter] = strtoul(abschnitt,NULL,0);
          break;
          case FLOAT:
            ((float *)parameterPointer)[parameterCounter] = strtod(abschnitt,NULL);
          break;
        }
        abschnitt = strtok(NULL, ";,");
        parameterCounter++;
        if(parameterCounter==pLength)
          abschnitt = nullptr;
      }
    break;
  }
  if(parameterPointer!=nullptr)
  {
    free(parameter_text);
    parameter_text = (char *) parameterPointer;
  }
}

void ComReceiver::comStateMachine()
{
	uint8_t ready,i;
	char act_char,temp;
	static char crcString[5];
	static uint8_t crcIndex;
	uint8_t length;

	char infoType;
	if( outCom->getChar(act_char) == true )
	{
		if( false )
		{
			rec_state = RCST_L1;
		}
		else
		{
      PORTA.OUT = ~rec_state;
			switch( rec_state )
			{
				case RCST_WAIT:
					if( act_char=='#' )
					{
						crcIndex = 0;
						crcGlobal.Reset();
						isBroadcast = false;
						crcGlobal.Data(act_char);
						rec_state = RCST_L1;
 					}
				break;
				case RCST_L1:
					if( isxdigit(act_char)!=false )
					{
						if( act_char<58)
							length = 16*(act_char-48);
						else
						{
							act_char = tolower(act_char);
							length = 16*(act_char-87);
						}
						crcGlobal.Data(act_char);
						rec_state = RCST_L2;
					}
					else
						rec_state = RCST_WAIT;
				break;
				case RCST_L2:
					if( isxdigit(act_char)!=false )
					{
						if( act_char<58)
							length += (act_char-48);
						else
						{
							act_char = tolower(act_char);
							length += (act_char-87);
						}
						crcGlobal.Data(act_char);
						rec_state = RCST_HEADER;
					}
					else
						rec_state = RCST_WAIT;
				break;
				case RCST_HEADER:
					if ( (act_char&4)==4 )
					{
						crc=CRC_YES;
						crcGlobal.Data(act_char);
					}
					else
					{
						crc=CRC_NO;
					}
					if ( (act_char&2)==2 )
						encryption=true;
					else
						encryption=false;
					rec_state = RCST_Z1;
				break;
				case RCST_Z1:
					if(crc==CRC_YES)
						crcGlobal.Data(act_char);
          if( act_char==node[0] )
             rec_state = RCST_Z2;
          else
          {
              if( act_char=='B' )
                  rec_state = RCST_BR2;
              else
                  rec_state= RCST_WAIT;
          }
				break;
				case RCST_Z2:
					if( act_char==node[1] )
          {
						if(crc==CRC_YES)
							crcGlobal.Data(act_char);
						rec_state = RCST_Q1;
//						LED_ROT_ON;
          }
          else
          {
              rec_state= RCST_WAIT;
          }
				break;
				case RCST_BR2:
          if ( act_char=='R' )
          {
            if(crc==CRC_YES)
              crcGlobal.Data(act_char);
            rec_state = RCST_Q1;
            isBroadcast = true;
         }
          else
          {
            rec_state= RCST_WAIT;
          }
				break;
				case RCST_Q1:
					if(crc==CRC_YES)
						crcGlobal.Data(act_char);
					quelle[0]=act_char;
					rec_state = RCST_Q2;
				break;
				case RCST_Q2:
					if(crc==CRC_YES)
						crcGlobal.Data(act_char);
					quelle[1]=act_char;
					quelle[2]=0;
					rec_state = RCST_KEADER;
				break;
				case RCST_KEADER:
					infoType=act_char;
					if (infoType=='S')
					{
						if(crc==CRC_YES)
							crcGlobal.Data(act_char);
						rec_state=RCST_WAIT_FUNCTION;
					}
					else
					{
						rec_state=RCST_WAIT;
					}
				break;
				case RCST_WAIT_FUNCTION:
					rec_state = RCST_WAIT_ADDRESS;
					ready = false;
					temp = 0;
					i = 0;
					if(isBroadcast==false)
					{
            do
            {
              if(commands[i].function == act_char)
              {
                temp = act_char;
                ready = true;
                if(crc==CRC_YES)
                  crcGlobal.Data(act_char);
              }
              i++;
              if(i==numCommands)
                ready = true;
            }while (!ready);
					}
					else
					{
           do
            {
              if(  (information[i].function==act_char) & (information[i].quelle[0]==quelle[0]) & (information[i].quelle[1]==quelle[1])  )
              {
                temp = act_char;
                ready = true;
                if(crc==CRC_YES)
                  crcGlobal.Data(act_char);
              }
              i++;
              if(i==numInformation)
                ready = true;
            }while (!ready);
					}
					function = temp;
				break;
				case RCST_WAIT_ADDRESS:

					rec_state = RCST_WAIT_JOB;
					address = act_char;
                    if(crc==CRC_YES)
                        crcGlobal.Data(act_char);
				break;
				case RCST_WAIT_JOB:
					rec_state = RCST_NO_PARAMETER;
					ready = false;
					temp = 0;
					i = 0;
					if(isBroadcast==false)
					{
            do
            {
              if(commands[i].function == function)
              {
                if(commands[i].job == act_char)
                {
                  temp = i+1;  // Achtung: job ist immer eins größer als der Index
                  ready = true;
                  if(crc==CRC_YES)
                    crcGlobal.Data(act_char);
                }
              }
              i++;
              if(i==numCommands)
                ready = true;
            }while (!ready);
					}
					else
					{
            do
            {
              if(  (information[i].function == function) && (information[i].address == address) && (information[i].quelle[0]==quelle[0]) && (information[i].quelle[1]==quelle[1]) )
              {
                if(information[i].job == act_char)
                {
                  temp = i+1;  // Achtung: job ist immer eins größer als der Index
                  ready = true;
                  if(crc==CRC_YES)
                    crcGlobal.Data(act_char);
                }
              }
              i++;
              if(i==numInformation)
                ready = true;
            }while (!ready);
					}
					job = temp;
					if (job==0)
					{
						bootloader_attention = false;
						function = 0;
						job = 0;
						rec_state = RCST_WAIT;
					}
					else
					{
            uint8_t ptype;
            if( isBroadcast==false)
              ptype = commands[job-1].ptype;
            else
              ptype = information[job-1].ptype;
						if( ptype != NOPARAMETER )
						{
							parameter_text = (char*) getMemory(STRING,MAX_TEMP_STRING);
							if (parameter_text==NULL)
							{
								outCom->sendInfo("!!!!!Error!!!!!!","BR");
							}
							parameter_text_length = MAX_TEMP_STRING;
						rec_state = RCST_GET_DATATYPE;
						}
						else
							rec_state = RCST_NO_PARAMETER;
						parameter_text_pointer = 0;
						temp_parameter_text_pointer = 0;
					}
				break;
				case RCST_NO_PARAMETER: // dann muss der Datentyp = '?' sein
					if( act_char=='?' )
					{
						if(crc==CRC_YES)
						{
							crcGlobal.Data(act_char);
							rec_state = RCST_CRC;
						}
						else
							rec_state = RCST_WAIT_END1;
					}
					else
						rec_state = RCST_WAIT;
				break;
				case RCST_GET_DATATYPE: // einziger bekannter Datentyp : 'T'
					if( (act_char=='F') | (act_char=='t')  | (act_char=='T') )
					{
						if(crc==CRC_YES)
							crcGlobal.Data(act_char);
						rec_state = RCST_GET_PARAMETER;
					}
					else
						rec_state = RCST_WAIT;
				break;

				case RCST_CRC:
					if ( isxdigit(act_char) )
					{
						crcString[crcIndex] =  act_char;
						crcIndex++;
						if (crcIndex>=4)
						{
							crc=CRC_IO;
							if(crcGlobal.compare(crcString) != true )
							{
								job = 255;	// das ist der CRC-Error-Job
							}
							rec_state = RCST_WAIT_END1;
						}
					}
					else
					{
						rec_state = RCST_WAIT;
						job = 0;
						free_parameter();
					}
				break;

				case RCST_WAIT_END1:
					if( (act_char=='\r') | (act_char=='\n') )
						rec_state = RCST_WAIT_END2;
					else
					{
						rec_state = RCST_WAIT;
						job = 0;
						free_parameter();
					}
				break;
				case RCST_WAIT_END2:
					if( (act_char=='\r') | (act_char=='\n') )
						rec_state = RCST_DO_JOB;
					else
					{
						rec_state = RCST_WAIT;
						job = 0;
						free_parameter();
					}
				break;
				case RCST_GET_PARAMETER:
					if(crc==CRC_YES)
						crcGlobal.Data(act_char);
          if( (act_char=='<') )					// Parameterende
          {
            if(crc==CRC_YES)
              rec_state = RCST_CRC;
            else
              rec_state = RCST_WAIT_END1;

            parameter_text[parameter_text_pointer] = 0;
          }
          else
          {
            if( parameter_text_pointer < parameter_text_length-2 )
            {
              parameter_text[parameter_text_pointer] = act_char;
              parameter_text_pointer++;
            }
            else // zu langer Parameter
            {
              rec_state = RCST_WAIT;
              function = 0;
              free_parameter();
            }
          }
				break; // case RCST_GET_PARAMETER

			} // end of switch
		}
	}
}

void ComReceiver::sendAnswerInt(char function,char address,char job,uint32_t wert,uint8_t noerror)
{
  this->outCom->sendAnswerInt(quelle,function,address,job,wert,noerror);
}

void ComReceiver::sendAnswerDouble(char function,char address,char job,uint32_t wert,uint8_t noerror)
{
  this->outCom->sendAnswerDouble(quelle,function,address,job,wert,noerror);
}

void ComReceiver::sendAnswer(char const *answer,char function,char address,char job,uint8_t noerror)
{
  this->outCom->sendAnswer(answer,quelle,function,address,job,noerror);
}

void ComReceiver::sendPureAnswer(char function,char address,char job,uint8_t noerror)
{
  this->outCom->sendPureAnswer(quelle,function,address,job,noerror);
}

void *ComReceiver::getMemory(uint8_t type,uint8_t num)
{
uint8_t size=1;
void *mem=NULL;
	switch(type)
	{
		case STRING:
			size = 1;
		break;
		case BYTEARRAY:
			size = 2;
		break;
		case UINT_8:
			size = 1;
		break;
		case UINT_16:
			size = 2;
		break;
		case UINT_32:
			size = 4;
		break;
		case FLOAT:
			size = sizeof(double);
		break;
		default:
			size = -1;

	}
	if (size>0)
	{
		mem =  malloc(size*num);

	}
	return( mem );
}

void ComReceiver::free_parameter(void)
{
	if (parameter_text)
	{
		free( parameter_text );
		parameter_text = NULL;
		parameter_text_length = 0;
	}
	if (temp_parameter_text != NULL)
	{
		free( temp_parameter_text );
		temp_parameter_text = NULL;
		temp_parameter_text_pointer = 0;
	}
}
