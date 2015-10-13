#include <c8051f120.h>
#include <string.h>
#include <stdlib.h>
#include "gprs.h"
#include "http.h"
#include "..\main.h"
#include "..\uart.h"
#include "..\rmodem.h"
#include "..\io.h"


TIME_HTTP xdata TimeHttp;

/***************\
*               * \
*   D E B U G   *   >----------------------------------->
*               * /
\***************/

//�������� ���. ���������� � MAX31XX UART

#ifdef DEBUG_GPRS_MAX

 #define SIZE_BUF_OUT_MAX 0xFE
 unsigned char xdata buf_out_max[SIZE_BUF_OUT_MAX];

 void send_c_out_max(unsigned char C_out)
 {
     while (OutMAXPointer2 >= SIZE_BUF_OUT_MAX) {reset_wdt; send_max();}
     buf_out_max[OutMAXPointer2++] = C_out;
 }

 void debug_gprs_init()
 {
     pcontext = buf_out_max;
     Init_max(0x09); //set 19200 b/s
 }

#endif

//------------------------------------------------------<



union GPRSGL xdata gprs = {0};

unsigned char xdata StateInGPRS = 0x01;
unsigned char xdata StateOutGPRS = 0x01;
unsigned char xdata StateGlobGPRS = 0x01;

unsigned char xdata in_data;

//--GPRS Timers-Cnt's--                              |  *0.01s*  |
unsigned char data tick_gprs = 0x00; //[xx|xx|1s|0.1s|**|**|**|**]
unsigned char xdata timer_gprs_1 = 0;            //[*0.01s]
unsigned  int xdata timer_gprs_glob = 0;         //[*1s]
unsigned char xdata timer_http_delay = 0;        //[*0.01s]
unsigned char xdata timer_http_wait = 0;         //[*0.1s]
unsigned char xdata timer_gprs_in = 0;           //[*1s]

//unsigned char code HTTP_REPLY[] = "HTTP/1.# 200 OK";
//unsigned char code HTTP_LENGTH[] = "Content-Length: ";
unsigned char xdata bufInGPRS[SIZE_BUF_IN_GPRS];
unsigned char xdata cntInGPRS = 0;
unsigned char xdata bufHdrGPRS[SIZE_BUF_HDR_GPRS];
unsigned char xdata cntHdrGPRS = 0;
unsigned int xdata InLengthGPRS, cntInLengthGPRS;

unsigned int xdata cntOutGPRS;
unsigned int xdata SizeOutGPRS;


void automat_gprs_glob();
void automat_gprs_in();
void automat_gprs_out();


/*function is called from main*/
void automat_gprs_glob()
{
  unsigned char data c_tick;

    //*GPRS timers*
  int_timer2_no;
    c_tick = tick_gprs;
    tick_gprs = 0x00;
  int_timer2_yes;
    if (c_tick)
    {
        //*1s*
        if (c_tick & 0x20)
        {
            c_tick &= ~0x20;
            if (timer_gprs_glob) --timer_gprs_glob;
            if (timer_gprs_in)
                if (!(--timer_gprs_in)) gprs.b.t_input = 1;
            //*------DEBUG------*
            send_c_out_max(GSM_CTS?0xEE:0xFF); //"COUNT 1s"(+"CTS Busy")
            //*------DEBUG------*
        }
        //*0.1s*
        if (c_tick & 0x10)
        {
            c_tick &= ~0x10;
            if (timer_http_wait) --timer_http_wait;
        }
        //*0.01s*
        while (c_tick--)
        {
            if (timer_gprs_1) --timer_gprs_1;
            if (timer_http_delay) --timer_http_delay;
        }
    }

    reset_wdt;


  //������������������ ��������� (v3.1):
  // 1)  ������� �����������
  // 2)  ������� ������ (v)
  // 3)  ������������� ������ (v)
  // 4)  ��������� �������� SMS (v)
  // 5)  ���� � GPRS/PPP/IP/TCP (v)
         // 5a)  ��� �������� ��������� CONNECT ��������� ������ (v):
                 // gprs_on=1, gprs_md=1, command=0
  // 6)  ���������� ����� ����� RI (������ � ������� SMS) (m)
         // 6a)  ���� ������ SMS, ����� - gprs_md=0, command=1 (m), ��������� ����� � ��������� ����� (v)
         // 6b)  ��������� �������� SMS, ������� � ����� GPRS (v)
  // 7)  ���� ��������� ������ ��� �������� (IOStreamState.Out), �������� ������ �� HTTP (m)
         // 7a)  ���� ������ ������, ��������� ������� (m)
         // 7b)  ���� ������ ������������, ����� ����� � ��������� ����� (m)
  // 8)  ���� ��� ������ ��� ��������, ����� � ��������� ����� �� timeout (2���) (m)
  // 9)  �������� ���������� ��������� (?), ����. �������� ��������������� ������ �� SMS (v)

    //*GPRS mode processing*
    switch (StateGlobGPRS)
    {
        case 0x01: //wait for CONNECT to SERVER
            if (!gprs.b.command)
            {
                //*------DEBUG------*
                send_c_out_max(0x01); //"START"
                send_c_out_max(0x00);
                //*------DEBUG------*
                ClearBuf_uart0();
                StateHTTP = 0x01;
                cntInGPRS = 0;
                StateInGPRS = 0x01;
                StateOutGPRS = 0x01;
                StateGlobGPRS = 0x10;
                //*-----------------*
                gprs.b.gprs_md = 1; //* start automat_gprs_in(), automat_gprs_out()
                gprs.b.ret_cmd = 0;
                gprs.b.t_input = 0;
                gprs.b.f_cycle = 0;
                gprs.b.sending = 0;
                gprs.b.sendbuf = 0;
                if (gprs.b.gprs_er)
                    timer_gprs_glob = 60;
                else
                if (timer_gprs_glob==0)
                    timer_gprs_glob = 120;//900;//120;
            }
        break;

        case 0x10: //work in HTTP
            if (GSM_DCD) //����� ����������
            {
                StSYSTEM &= ~0xFE00;
                //*------DEBUG------*
                send_c_out_max(0x0D); //"return to SMS"
                send_c_out_max(0x21); //"Error - no DCD"
                send_c_out_max(0x00);
                //*------DEBUG------*
                gprs.b.gprs_md = 0; //* stop automat_gprs_in(), automat_gprs_out()
                if (gprs.b.gprs_er) gprs.b.f_cycle = 1;
                gprs.b.command = 1;
                gprs.b.gprs_er = 1;
                StateGlobGPRS = 0x01;
                break;
            }
            //*-----------------------*
            if (!GSM_RI || !timer_gprs_glob || //�������� ����� SMS ��� ����� �������
               ((IOStreamState.Out) & 0x8300)==0x8000) // ���� ��������� �� SMS                
            {
                gprs.b.ret_cmd = 1;
                gprs.b.f_cycle = 1;
            }
            //*-----------------------*
            if (gprs.b.ret_cmd && (StateHTTP==0x01)) //����� � Command, ��� ��������
            {
                StSYSTEM &= ~0x0100;
                gprs.b.gprs_er = 0;
                //*------DEBUG------*
                send_c_out_max(0x0D); //"return to SMS"
                send_c_out_max(0x10);
                send_c_out_max(0x00);
                //*------DEBUG------*
                gprs.b.gprs_md = 0; //* stop automat_gprs_in(), automat_gprs_out()
                gprs.b.command = 1;
                StateGlobGPRS = 0x15;
                break;
            }
            //*----------------------------------------------------------------*/
            if (((IOStreamState.Out & 0xC300) == 0x8100) && !gprs.b.ret_cmd) //������� ����� ��� �������� �� HTTP?
            {
                IOStreamState.Out |= 0x4000; //"����� �����"
                gprs.b.sendbuf = 1;
                //form SMS packet header
                switch (IOStreamState.Out & 0x3800)
                {
                    case 0x2000: //������
                        NpackInc(IOStreamBuf.Header.id = IOStreamState.ReqID);
                        IOStreamBuf.Header.sn = Get_npack_inc(IOStreamBuf.Header.id);
                        IOStreamBuf.Header.fmt = ((IOStreamState.ReqID==0x31)||(IOStreamState.ReqID==0x3D)) ? 0x80 : 0x00;
                    break;

                    case 0x0800: //���������
                        IOStreamBuf.Header.id = 0x32;
                        IOStreamBuf.Header.sn = ++npack_32_inc;
                        IOStreamBuf.Header.fmt = 0x80;
                    break;

                    default: //������������ �����
                        IOStreamState.Out = 0x0000;
                    return;
                }
                IOStreamBuf.Header.tail = 5 + SMS_Tail[0];
                IOStreamBuf.Header.res = 0x00;
                if ((IOStreamState.Out & 0x00FF) > SizeBufOut)
                {
                    IOStreamState.Out &= 0xFF00;
                    IOStreamState.Out |= SizeBufOut;
                }
                else
                    if (IOStreamState.Out & 0x0400) //��������� �����
                        IOStreamBuf.Header.fmt |= 0x01;
            }
        break;

        case 0x15: //temporary command mode
            if (GSM_DCD) //����� ����������
            {
                StSYSTEM &= ~0xFE00;
                //*------DEBUG------*
                send_c_out_max(0x0E); //"temp. COMMAND MODE & restart GPRS"
                send_c_out_max(0x21); //"Error - no DCD"
                send_c_out_max(0x00);
                //*------DEBUG------*
                StateGlobGPRS = 0x01;
                break;
            }
            if (!gprs.b.command)
            {
				StSYSTEM |= 0x8000;
                //*------DEBUG------*
                send_c_out_max(0x0C); //"END COMMAND MODE"
                send_c_out_max(0x00);
                //*------DEBUG------*
                ClearBuf_uart0();
                StateHTTP = 0x01;
                cntInGPRS = 0;
                StateInGPRS = 0x01;
                StateOutGPRS = 0x01;
                StateGlobGPRS = 0x10;
                //*-----------------*
                gprs.b.gprs_md = 1; //* start automat_gprs_in(), automat_gprs_out()
                gprs.b.ret_cmd = 0;
                gprs.b.f_cycle = 0;
                gprs.b.sending = 0;
                if (timer_gprs_glob==0) timer_gprs_glob = 120;//900;//120;
            }
        break;
    }

                            reset_wdt;

    //*GPRS input*
    automat_gprs_in();      reset_wdt;

    //*GPRS output*
    automat_gprs_out();     reset_wdt;

    //*HTTP processing*
    automat_http();
}

void automat_gprs_in()
{
  unsigned char data c;

  if (gprs.b.gprs_md) //----------->
  //-------------------------------> ����� ������
  while (Read_byte_uart0_GPRS())
  {
    reset_wdt;

    switch (StateInGPRS)
    {
        case 0x00: //����� ������ ��������� HTTP/1.1 X00 ...
        case 0x01:
            if ((in_data != '\r') && (in_data != '\n'))
            {
                if (cntHdrGPRS >= SIZE_BUF_HDR_GPRS) //error
                {
                    cntHdrGPRS = 0;
                    break;
                }
                bufHdrGPRS[cntHdrGPRS++] = in_data;
            }
            else
            {
                if (cntHdrGPRS >= 12)
                {
                    if (!strncmp(bufHdrGPRS, "HTTP/1.1 100", 12))
                        gprs.b.ackhttp = 1;
                    else
                    if (!strncmp(bufHdrGPRS, "HTTP/1.1 20x", 11)) // ��������� ��� ������ 20� - �
                    {
                        gprs.b.ackhttp = 1;
                        if (StateInGPRS) //0x01
                        {
                            InLengthGPRS = 0xFFFF; //����� ���� ����������
                            cntInGPRS = 1;
                            StateInGPRS = 0x05;
                        }
                    }
                }
                cntHdrGPRS = 0;
            }
        break;

        case 0x05: //����� Content-Length, \r\n\r\n
            if ((in_data != '\r') && (in_data != '\n'))
            {
                if (cntInGPRS >= SIZE_BUF_IN_GPRS) //error
                {
                    //cntInGPRS = 0;
                    break;
                }
                bufInGPRS[cntInGPRS++] = in_data;
                break;
            }
            if (cntInGPRS >= 16)
            {
                if (!strncmp(bufInGPRS, "Content-Length: ", 16))
                {
                    while (bufInGPRS[--cntInGPRS] != ' '); //�� ������ ��������
                    InLengthGPRS = atoi(&bufInGPRS[cntInGPRS]);
                }
                else
                if (!strncmp(bufInGPRS, "Date: ", 6) && (cntInGPRS == 35))
                {
                    Get_HTTP_Time(bufInGPRS);
                }
                cntInGPRS = 0;
                break;
            }
            if (!cntInGPRS && (in_data=='\r')) //������ \r\n\R\n
            {
                cntInLengthGPRS = 0;
                timer_gprs_in = TIMEOUT_HTTP_STREAM;
                gprs.b.t_input = 0;
                StateInGPRS = 0x09;
            }
            cntInGPRS = 0;
        break;

        case 0x09: //'\n'
            StateInGPRS = 0x10;
			// ������� ��� iteris
			#ifdef ITERIS_COD_MODE
				//IOStreamState.In = 0x8100 | cntInGPRS;
        		ClearBuf_uart0();
				timer_gprs_in  = 0; 
				gprs.b.t_input = 0;
        		gprs.b.new_req = 0;
    			cntInGPRS = 0;
				StateInGPRS = 0x01;
			#endif
        break;
		// ���� �� ������ �������� �����������
        case 0x10: //����� ����������� ��������� "KOD8#"
            if (++cntInLengthGPRS >= InLengthGPRS)
            {
                timer_gprs_in = gprs.b.t_input = 0;
                cntInGPRS = 0;
                StateInGPRS = 0x01;
                break;
            }
            bufInGPRS[cntInGPRS++] = in_data;
            timer_gprs_in = TIMEOUT_HTTP_STREAM;
            if ((in_data == '#') && (cntInGPRS >= INPAC_HEADER_LEN))
            {
                if (!strncmp(&bufInGPRS[cntInGPRS-INPAC_HEADER_LEN], INPAC_HEADER_STR, INPAC_HEADER_LEN))
                {
                    cntInGPRS = 0;
                    //---DEBUG---
                    //bufInGPRS[0] = 0x7B; //"7-bit" -
                    //cntInGPRS = 1; // - ��, ��� ����� ��������� �� ��� ��� �����. �������
                    //---DEBUG---
                    StateInGPRS = 0x11;
                    break;
                }
            }
            if (cntInGPRS >= SIZE_BUF_IN_GPRS-1) //error
            {
                cntInGPRS = 0;
                break;
            }
        break;

        case 0x11: //����� ������� ���
            timer_gprs_in = TIMEOUT_HTTP_STREAM;
            bufInGPRS[cntInGPRS++] = in_data;
            if ((++cntInLengthGPRS >= InLengthGPRS) || (cntInGPRS >= SIZE_BUF_IN_GPRS-1)) //�� �������
            {
                if (bufInGPRS[0]==0x7B) bufInGPRS[cntInGPRS++] = '\0'; //����� ASCII
                gprs.b.new_req = 1;
                timer_gprs_in = TIMEOUT_HTTP_REQ;
                gprs.b.t_input = 0;
                StateInGPRS = 0x00; //���� � ���������
            }
        break;

        default:
        break;
    }

    return;
  }
// ������ �� �������
if (gprs.b.new_req)
  {
    if (!(IOStreamState.In & 0xC000)) //����� �� �����
    {
        for (c=0; c<cntInGPRS; c++) IOStreamBuf.In[c] = bufInGPRS[c];
        IOStreamState.In = 0x8100 | cntInGPRS;
        timer_gprs_in = gprs.b.t_input = 0;
        gprs.b.new_req = 0;
        ClearBuf_uart0();
        cntInGPRS = 0;
        StateInGPRS = 0x01;
        return;
    }
  }
// 
if (gprs.b.t_input)
  {
    gprs.b.t_input = 0;
    /*if ((StateInGPRS == 0x11) && cntInGPRS) //������� ������ �� ���
    {
        gprs.b.new_req = 1;
        timer_gprs_in = TIMEOUT_HTTP_REQ;
        StateInGPRS = 0x00; //���� � ���������
        return;
    }*/
    gprs.b.new_req = 0;
    ClearBuf_uart0();
    cntInGPRS = 0;
    StateInGPRS = 0x01;
  }
}
/*send data to gprs buf_http[] */
void automat_gprs_out()
{
  if (gprs.b.gprs_md) //----------->
  //-------------------------------> �������� ������
  {
  switch (StateOutGPRS)
    {
        case 0x01: //start
            if (!(gprs.b.sending && FlagTX0))break;
            StateOutGPRS = 0x05;
			sendStringUART0(buf_http); // ����� ���������� 
			//cntOutGPRS = 0;
        //no break;
        case 0x05: //sending..
            /*if (cntOutGPRS >= SizeOutGPRS)
            {
                StateOutGPRS = 0xFF; //->�� default
                break;
            }
            tx_data_uart0 = buf_http[cntOutGPRS++]; send_char_uart0();*/ // 	���������� �� ������ �������, ��� ����� :)
        	/*while(cntOutGPRS < SizeOutGPRS)
				{ 
				tx_data_uart0 = buf_http[cntOutGPRS];
				send_char_uart0();
				cntOutGPRS++;
				reset_wdt;							  
				}*/
		//break;
        default:
            if (FlagTX0) //��� ����������
            {
                gprs.b.sending = 0;
                StateOutGPRS = 0x01;
            }
        break;
    }
  }
}