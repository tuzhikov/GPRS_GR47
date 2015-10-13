//  --------------------------------------------------------------------------------
// | ------------------------------------------------------------------------------ |
// ||                                                                              ||
// ||     H T T P    C O M M U N I C A T I O N    L E V E L       (          )     ||
// ||                                                                              ||
// | ------------------------------------------------------------------------------ |
//  --------------------------------------------------------------------------------

#include "c8051f120.h"  
#include <stdio.h>
#include <stdlib.h>                                                   
#include <string.h>

#include "..\main.h"
#include "..\io_sms.h"
#include "..\io.h"
#include "..\kod8.h"
#include "gprs.h"

#include "setting.h"
#include "http.h"


#define SIZE_HTTP_REQUEST  0x0280//0x0140//0x01E0
unsigned char xdata buf_http[SIZE_HTTP_REQUEST];

unsigned char code http_req_str[64] _at_ (F_ADDR_SETTINGS+26);


// ����� ��� RTMS
#ifdef RTMS_COD_MODE
struct_str code http_header[] = {
                                  { "Host: #\r\n",                                       9 ,0 },
                                  { "Content-Type: application/octet-stream\r\n",       40 ,0 },    
                                  { "Content-Length: #\r\n",                            19 ,0 },    
                                  { "\r\n",                                              2 ,0 }     
                                };
struct_str code http_body[]   = { 
                                  { "id=#&",                                             5 ,0 },
                                  { "gmt[8]=#;",                                         9 ,0 },
                                  { "data[10]=#;",                                      11 ,0 },
                                  { "bin1[#]=#;",                                       10 ,0 },
                                  { "bin2[#]=#;",                                       10 ,0 },
                                  { "port[8]=#;",                                       10 ,0 },
                                  { "id=#&",                                             5 ,0 },
                                  { "txt[#]=#;",                                         9 ,0 },
                                  { "id=#&",                                             5 ,0 },
                                  { "all[#]=#;",                                         9 ,0 }
                                };
#endif

#ifdef ITERIS_COD_MODE
// POST /IterisServer/webresources/psid/put_dt HTTP/1.1..
struct_str code http_header[] = {
                                  { "Content-Type: application/x-www-form-urlencoded\r\n",49 ,0 },
								  { "Host: #:#\r\n",                                      11 ,0 },    
                                  { "Content-Length: #\r\n",                              19 ,0 },    
                                  { "\r\n",                                                2 ,0 }     
                                };
struct_str code http_body[]   = { 
                                  { "SimId=#&",                                      8 ,0 },
                                  { "Bin=#&",                                        6 ,0 },
                                  { "Timestamp=#",                                   11 ,0}
                                };
#endif

void IOStreamConf();

unsigned char xdata StateHTTP = 0x01;

unsigned char xdata cntPacInFrame = 0;

/*------------------------------------------------------------------------------------*/
unsigned int			formingHTTPHeaders();
unsigned int            formingHTTPHeadersGET();
static unsigned int formingHTTPContent(const char *pBuff,const unsigned short lengh);
//unsigned int			formingHTTPContent();
/*------------------------------------------------------------------------------------*/
void automat_http()
{
	if (!gprs.b.gprs_md) return;

    switch (StateHTTP)
    {
        case 0x01: //send HTTP header
            if (gprs.b.ret_cmd || GSM_CTS)
            {
                if (GSM_CTS) StSYSTEM |= 0x3000; //"HTTP, �������� ������ (CTS busy)"
                break;
            }
            StSYSTEM &= ~0x7000; //"HTTP, Idle"
            //* ���� ������ ��� �������� (IOStreamBuf.Out[]/.OutAuto[])
            //IOStreamState.Out |= 0x8800;
			if(!gprs.b.sending){
				//�������� �� HTTP
				if((IOStreamState.Out==0x0800)&&(continueSession==retOk)){//������ ����� ���� ����������
					IOStreamState.Out = 0xe583;
					}
				//
			 if((IOStreamState.Out & 0xC300) == 0xC100)
            	{
                StSYSTEM |= 0x1000; //"HTTP, �������� ������"
                SizeOutGPRS = formingHTTPHeaders();
                gprs.b.sending = 1;
                timer_http_wait = 10;//50; //[*0.1s]
                gprs.b.ackhttp = 0;
                //*------DEBUG------*
                send_c_out_max(0xD9); //"HTTP Send Packet Header"
                send_c_out_max(IOStreamBuf.OutAuto[2]);
                send_c_out_max(0x00);
                //*------DEBUG------*
                StateHTTP = 0x05;
				}
			}
        break;
        case 0x05: //send HTTP content
            if (gprs.b.ackhttp && !gprs.b.sending || !timer_http_wait)
            {
                SizeOutGPRS = formingHTTPContent(buf_http,sizeof(buf_http));
				gprs.b.sending = 1;
                //*------DEBUG------*
                send_c_out_max(0xDB); //"HTTP Send Packet Body"
                send_c_out_max(0x00);
                //*------DEBUG------*
                //timer_http_wait = 500; //5sec
				//sendStatusGPRS=retOk; // ��� ���� ��������, ���� �������� ���������
				StateHTTP = 0x10;
            }
        break;

        case 0x10: //send all
            if (!gprs.b.sending)//||(!timer_http_wait))
            {
                StSYSTEM &= ~0x1000;
				sendStatusGPRS = retOk; // �������� ����� ������ �� http, ��������� ����� ���������
				if (!timer_http_delay) cntPacInFrame = 0;
                timer_http_delay = 100;//250; //[*0.01s]
                if (++cntPacInFrame >= 3)
                {
                    StSYSTEM |= 0x2000; //"HTTP, �������� ������ (send buffer)"
                    cntPacInFrame = 0;
                    StateHTTP = 0x15;
                }
                else
                    StateHTTP = 0x01;
                
				//*------DEBUG------*
                send_c_out_max(0xDC); //"all sent"
                send_c_out_max(0x00);
                //*------DEBUG------*
                IOStreamConf();
                OutBufChange = 1;
            }
        break;

        case 0x15: //wait for set CTS=1
            if (!timer_http_delay) StateHTTP = 0x01;
        break;

        default:
            StateHTTP = 0x01;
        break;
    }
}

void IOStreamConf()
{
  unsigned char data c;

    SMS_Tail[0] = SMS_Tail[1];
    SMS_Tail[1] = 0;
    if(IOStreamState.Out & 0x0400)
    {
        if (!SMS_Tail[0]) //��� ������ ����
        {
            gprs.b.sendbuf = 0;
            IOStreamState.Out = 0x0000;
            return;
        }
        IOStreamState.Out &= 0xBF00;
    }
    else //�������� ����� ������
        IOStreamState.Out &= 0x3F00;
    //����� ������ � ������ ������
    pChar = ((IOStreamState.Out & 0x3800) == 0x0800) ? IOStreamBuf.OutAuto : IOStreamBuf.Out;
    for (c=0; c<SMS_Tail[0]; c++) pChar[c] = pChar[c+SizeBufOut];
    IOStreamState.Out |= SMS_Tail[0];
}


// ����� ��� Iteris --------------------------------------------------------------------------//
#ifdef ITERIS_COD_MODE
// GET
/*unsigned int formingHTTPHeadersGET()
{
unsigned int  data  i;
  unsigned char xdata g, S;
  unsigned int  xdata L;
  unsigned int  xdata LengContext = formingHTTPContent(buf_http,sizeof(buf_http));
  bit flInside = false;

    // copy URL
	memset(buf_http,0,sizeof(buf_http));
	buf_http[0] = 'G';
	buf_http[1] = 'E';
	buf_http[2] = 'T';
	for (i=3; i<64,http_req_str[i]; i++) buf_http[i] = http_req_str[i+1]; L = i+3;

    S = sizeof(http_header)/sizeof(struct_str);

    for (g=0; g<S; g++)
    {
        switch (g)
        {
            case One:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {
                    if(flInside==false){ 
						//add IP adress 
						for (cnt=0; cnt<4; cnt++)
                        	{
                            L += Hex_to_DecASCII_5(remoteIP.v[cnt], &buf_http[L]);
                            if (cnt<3) buf_http[L++] = '.';
                        	}
						flInside=true;
						}else{ 
						// add PORT adress
						flInside=false;
						L += Hex_to_DecASCII_5(remotePORT.Val,&buf_http[L]);
						}
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;
			// ����� ������
            case Two:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {// ������� ������ 
					L += Hex_to_DecASCII_5(LengContext, &buf_http[L]);   
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;
			// �������� ��� ���������
            default:
                for (i=0; i<http_header[g].len; i++) buf_http[L++] = http_header[g].str[i];
            break;
        }
    }

    return L;
}*/
// POST
unsigned int formingHTTPHeaders()
{
  unsigned int  data  i;
  unsigned char xdata g, S;
  unsigned int  xdata L;
  unsigned int  xdata LengContext = formingHTTPContent(buf_http,sizeof(buf_http));
  bit flInside = false;

    // copy URL
	memset(buf_http,0,sizeof(buf_http));
	for (i=0; i<64,http_req_str[i]; i++) buf_http[i] = http_req_str[i]; L = i;

    S = sizeof(http_header)/sizeof(struct_str);

    for (g=0; g<S; g++)
    {
        switch (g)
        {
            case One:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {
                    if(flInside==false){ 
						//add IP adress 
						for (cnt=0; cnt<4; cnt++)
                        	{
                            L += Hex_to_DecASCII_5(remoteIP.v[cnt], &buf_http[L]);
                            if (cnt<3) buf_http[L++] = '.';
                        	}
						flInside=true;
						}else{ 
						// add PORT adress
						flInside=false;
						L += Hex_to_DecASCII_5(remotePORT.Val,&buf_http[L]);
						}
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;
			// ����� ������
            case Two:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {// ������� ������ 
					L += Hex_to_DecASCII_5(LengContext, &buf_http[L]);   
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;
			// �������� ��� ���������
            default:
                for (i=0; i<http_header[g].len; i++) buf_http[L++] = http_header[g].str[i];
            break;
        }
    }

    return L;
}
// �������� ��������� http
static unsigned int formingHTTPContent(const char *pBuff,const unsigned short lengh)
{
unsigned char xdata Buff[25];
unsigned int  xdata L; 

memset(pBuff,0,lengh);
// sim id
strcpy(pBuff,"SimId=");
strncat(pBuff,&context_gsm.sim[0],context_gsm.simid_len);
strcat(pBuff,"&");
// DATA Iteris
L = strlen(pBuff);
cpyIterisData(&pBuff[L]);
// ��������� ����� ������� ������
strcat(pBuff,"Timestamp="); // yyyy.MM.dd HH.mm.ss 2015.10.11 17.16.15
// ��������� �����
memset(Buff,0,sizeof(Buff));
L = 0;
L +=Hex_to_DecASCII_5(TimeHttp.Year,&Buff[L]);
Buff[L++]='.';
L +=Hex_to_DecASCII_5(TimeHttp.Mon,&Buff[L]);
Buff[L++]='.';
L +=Hex_to_DecASCII_5(TimeHttp.Day,&Buff[L]);
Buff[L++]=' ';
L +=Hex_to_DecASCII_5(TimeHttp.Hour,&Buff[L]);
Buff[L++]='.';
L +=Hex_to_DecASCII_5(TimeHttp.Min,&Buff[L]);
Buff[L++]='.';
L +=Hex_to_DecASCII_5(TimeHttp.Sec,&Buff[L]);
strcat(pBuff,Buff);
L = strlen(pBuff);
return L;
}
#endif
// ����� ��� RTMS ----------------------------------------------------------------------------//
#ifdef RTMS_COD_MODE 

unsigned int formingHTTPHeaders()
{
  unsigned int  data  i;
  unsigned char xdata g, S;
  unsigned int  xdata L;

    for (i=0; i<64,http_req_str[i]; i++) buf_http[i] = http_req_str[i]; L = i;

    S = sizeof(http_header)/sizeof(struct_str);

    for (g=0; g<S; g++)
    {
        switch (g)
        {
            case 0:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {
                        for (cnt=0; cnt<4; cnt++)
                        {
                            L += Hex_to_DecASCII_5(remoteIP.v[cnt], &buf_http[L]);
                            if (cnt<3) buf_http[L++] = '.';
                        }
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;

            case 2:
                for (i=0; i<http_header[g].len; i++)
                {
                    if (http_header[g].str[i] == '#')
                    {
                        if ((IOStreamBuf.Header.id == 0x32) || //* ��������� KOD8
                            (IOStreamBuf.Header.id >= 0x4D) && (IOStreamBuf.Header.id <= 0x51)) //* ������ KOD8
                        {
                            //48 - '\0' + �����;
                            //context_gsm.simid_len - ����� ID SIM-�����;
                            //8 - ����� (4+4);
                            //10 - ���� Data[];
                            //4 - ������ ����� ���� �������� (2+2);
                            //8 - ���� Port[]
                            Char = 48 + context_gsm.simid_len + 8 + 10 + 4 + 8;
                            if ((IOStreamState.Out & 0x3800) == 0x0800)
                                pChar = &IOStreamBuf.OutAuto;
                            else
                                pChar = &IOStreamBuf.Out;
                            if (pChar[2] & 0x01) Char += 54; //�.�. ���� ������ � ������� 0
                            if (pChar[2] & 0x02) Char += 54; //�.�. ���� ������ � ������� 1
                            L += Hex_to_DecASCII_5(Char, &buf_http[L]);
                        }
                        else //* ��� ��������� ������
                        {
                            //������ ����� ���� Text[]
                            cnt = IOStreamState.Out & 0x00FF;
                            if (IOStreamBuf.Header.id != 0x20) cnt += 5; //* 0x20: ASCII
                            if (cnt <= 9) Char = 1;
                            else
                            if (cnt <= 99) Char = 2;
                            else
                                Char = 3;
                            //12 - '\0' + �����;
                            //context_gsm.simid_len - ����� ID SIM-�����;
                            //cnt - [SMS Packet Header (5)] + ����� ���� Text[]
                            Char += (12 + context_gsm.simid_len + cnt);
                            L += Hex_to_DecASCII_5(Char, &buf_http[L]);
                        }
                    }
                    else
                        buf_http[L++] = http_header[g].str[i];              
                }
            break;

            default:
                for (i=0; i<http_header[g].len; i++) buf_http[L++] = http_header[g].str[i];
            break;
        }
    }

    return L;
}

unsigned int formingHTTPContent()
{
  unsigned int  data  i;
  unsigned char xdata g, S;
  unsigned int  xdata L = 0;
  unsigned char xdata cnt_cr;

    //S = sizeof(http_body)/sizeof(struct_str);
    //if ((IOStreamState.Out & 0x3800) == 0x0800)
    if ((IOStreamBuf.Header.id == 0x32) || //* ��������� KOD8
        (IOStreamBuf.Header.id >= 0x4D) && (IOStreamBuf.Header.id <= 0x51)) //* ������ KOD8
    {g = 0; S = 5;}
    else
        if (IOStreamBuf.Header.id==0x20) {g = 6; S = 7;} //* text ASCII
        else {g = 8; S = 9;} //* ��� ��������� ������
    //*--------------*
    if ((IOStreamState.Out & 0x3800) == 0x0800)
        pChar = &IOStreamBuf.OutAuto;
    else
        pChar = &IOStreamBuf.Out;
    //*--------------*
    for ( ; g<=S; g++)
    {
        reset_wdt;

        switch (g)
        {
            case 0:
            case 6:
            case 8:
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        if (context_gsm.exist_simid)
                        {
                            memcpy(&buf_http[L], &context_gsm.sim[0], context_gsm.simid_len); 
                            L += context_gsm.simid_len; 
                        };
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];            
                }
            break;

            case 1:
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        int_timer2_no;
                        memcpy(&buf_http[L], &Timer_inside, 4); //����� ��������
                        int_timer2_yes;
                        L += 4;
                        memcpy(&buf_http[L], &pChar[9], 4); //�����
                        L += 4;
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            case 2:
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        memcpy(&buf_http[L], &pChar[13], 10); //��� ������ (����� �������� ������.)
                        L += 10;
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            case 3:
            case 4:
                cnt_cr = 0;
                cnt = g-3; //����� ������� 0/1
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        switch (cnt_cr++)
                        {
                            case 0: //Len=f(Event)
                                if (pChar[2] & (1<<cnt)) //�.�. ���� ������ � ������� 0/1
                                {
                                    buf_http[L++] = '5';
                                    buf_http[L++] = '4';
                                }
                                else
                                {
                                    buf_http[L++] = '0';
                                    buf_http[L++] = '0';
                                }
                            break;

                            case 1:
                                if (pChar[2] & (1<<cnt)) //�.�. ���� ������ � ������� 0/1
                                {
                                    memcpy(&buf_http[L], &pChar[23+54*cnt], 54);
                                    L += 54;
                                }
                            break;
                        }
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            case 5:
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        memcpy(&buf_http[L], &LOutK8, 4); //������� ������������ ���� � KOD8
                        L += 4;
                        int_uart1_no;
                        memcpy(&buf_http[L], &LInK8, 4); //������� �������� ���� �� KOD8
                        int_uart1_yes;
                        L += 4;
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            case 7:
                cnt_cr = 0;
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        switch (cnt_cr++)
                        {
                            case 0: //Len
                                Char = IOStreamState.Out & 0x00FF;
                                L += Hex_to_DecASCII_5(Char, &buf_http[L]);
                            break;

                            case 1: //Data
                                memcpy(&buf_http[L], IOStreamBuf.Out, Char);
                                L += Char;
                            break;
                        }
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            case 9:
                cnt_cr = 0;
                for (i=0; i<http_body[g].len; i++)
                {
                    if (http_body[g].str[i] == '#')
                    {
                        switch (cnt_cr++)
                        {
                            case 0: //Len
                                Char = 5 + (IOStreamState.Out & 0x00FF);
                                L += Hex_to_DecASCII_5(Char, &buf_http[L]);
                            break;

                            case 1: //Data
                                memcpy(&buf_http[L], &IOStreamBuf.Header, Char);
                                L += Char;
                            break;
                        }
                    }
                    else
                        buf_http[L++] = http_body[g].str[i];
                }
            break;

            default:
                for (i=0; i<http_body[g].len; i++) buf_http[L++] = http_body[g].str[i];
            break;
        }
    }

    //��������� '\0' ��� ���������� \x0D, \x0A
    buf_http[L++] = '\0';

    return L;
}
#endif