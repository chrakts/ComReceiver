#include "ComReceiver.h"

ComReceiver::ComReceiver(Communication *output,const char *Node, COMMAND *commands, uint8_t numCommands, INFORMATION *information, uint8_t numInformation)
{
  //ctor
  outCom = output;
  commands = commands;
  information = information;
  numCommands = numCommands;
  numInformation = numInformation;
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
		free_parameter();
		rec_state = RCST_WAIT;
		function = 0;
		job = 0;
	}
}

void ComReceiver::comStateMachine()
{
	uint8_t ready,i;
	uint8_t error = NO_ERROR;
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

/*					switch ( act_char )
					{
						case Node[0]:
							rec_state_KNET = RCST_Z2;
						break;
						case 'B':
							rec_state_KNET = RCST_BR2;
						break;
						default:
							rec_state_KNET= RCST_WAIT;
					}*/
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
/*

					if ( act_char==Node[1] )
					{
						if(crc_KNET==CRC_YES)
							crcGlobal.Data(act_char);
						rec_state_KNET = RCST_Q1;
						LED_ROT_ON;
					}
					else
					{
						rec_state_KNET= RCST_WAIT;
					}*/
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
            uint8_t ptype,pLength;
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
						if( ptype != NOPARAMETER )
						{
							parameter_text = (char*) getMemory(ptype,pLength);
							if (parameter_text==NULL)
							{
								outCom->sendInfo("!!!!!Error!!!!!!","BR");
							}
							parameter_text_length = pLength;
							if( ptype != STRING )
								temp_parameter_text = (char *) getMemory(STRING,MAX_TEMP_STRING);
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
								job = 1;	// das ist der CRC-Error-Job
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
					//if( act_char=='\n' )
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
          uint8_t ptype,pLength;
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
					if ( ptype==STRING )
					{
						if( (act_char=='<') )					// Parameterende
						{
							if(crc==CRC_YES)
								rec_state = RCST_CRC;
							else
								rec_state = RCST_WAIT_END1;

							parameter_text[parameter_text_pointer] = 0;
//							input->println("-----------------");
//							input->println(parameter_text);
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
								error = ERROR_PARAMETER;
								function = 0;
								free_parameter();
							}

						}
					} // if STRING
					else // if some Number-Parameter
					{
						if ((act_char=='<') || (act_char==','))
						{
							errno = 0;
							temp_parameter_text[temp_parameter_text_pointer] = 0;		// Zahlenstring abschießen
							if ( parameter_text_pointer < pLength )   // wird noch ein Parameter erwartet?
							{
								uint32_t wert;
								switch(ptype)
								{
									case UINT_8:
										uint8_t *pointer_u8;
										pointer_u8 =  (uint8_t*) parameter_text;
										wert = strtoul(temp_parameter_text,NULL,0);
										if(wert<256)
											pointer_u8[parameter_text_pointer] = (uint8_t) wert;
										else
											error = ERROR_PARAMETER;
									break;
									case UINT_16:
										uint16_t *pointer_u16;
										pointer_u16 =  (uint16_t*) parameter_text;
										wert = strtoul(temp_parameter_text,NULL,0);
										if(wert<65536)
											pointer_u16[parameter_text_pointer] = (uint16_t) wert;
										else
											error = ERROR_PARAMETER;
//										input->println(temp_parameter_text);	!!!!!!!!!!!!!!!!!auskommentiert!!!!!!!!!!!!!!!!!!!!
//										input->pformat("Wert: %\>d\n",wert);		!!!!!!!!!!!!!!!!!auskommentiert!!!!!!!!!!!!!!!!!!!!
									break;
									case UINT_32:
										uint32_t *pointer_u32;
										pointer_u32 =  (uint32_t*) parameter_text;
										pointer_u32[parameter_text_pointer] = strtoul(temp_parameter_text,NULL,0);
									break;
									case FLOAT:
										double *pointer_d;
										pointer_d =  (double*) parameter_text;
										pointer_d[parameter_text_pointer] = strtod(temp_parameter_text,NULL);
									break;
								}
							}
							else
								error = ERROR_PARAMETER;
							if( parameter_text_pointer < parameter_text_length-1 ) // Zeiger auf nächsten Parameter
							{
								parameter_text_pointer++;
								temp_parameter_text_pointer = 0;						// zurücksetzen für nächsten Parameter
							}
							else
                            {
                                    ;
                            } // hier noch abfangen falls zu viele Parameter eingeben wurden
							if ( errno != 0)
								error = ERROR_PARAMETER;
							if ((act_char=='<'))
							{
								if(crc==CRC_YES)
									rec_state = RCST_CRC;
								else
									rec_state = RCST_WAIT_END1;
							}
							/* hier noch abfangen, falls zu wenige Parameter eingegeben wurden ************************ */
						}
						else // weiterer Character eines Parameters
						{
							if( temp_parameter_text_pointer < MAX_TEMP_STRING-2 )
							{
								temp_parameter_text[temp_parameter_text_pointer] = act_char;
								temp_parameter_text_pointer++;
							}
							else // zu langer Parameter
								error = ERROR_JOB;
						}
						if ( error != NO_ERROR )
						{
								function = 0;
								rec_state = RCST_WAIT;
								free_parameter();
						}
					}
				break; // case RCST_GET_PARAMETER

			} // end of switch
//			input->pformat("State: %x, char:%x, job:%d\r\n",rec_state,act_char,job);
		}
	}
}

void ComReceiver::sendAnswerInt(char function,char address,char job,uint32_t wert,uint8_t noerror)
{
  this->outCom->sendAnswerInt(quelle,function,address,job,wert,noerror);
}

void ComReceiver::sendAnswer(char const *answer,char function,char address,char job,uint8_t noerror)
{
  this->outCom->sendAnswer(answer,quelle,function,address,job,noerror);
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
