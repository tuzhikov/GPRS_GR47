/*
_______________________________________________________________________________
This program is being written by SKB

Copyright (C) Navigator-Technology. Orel, Russia
_______________________________________________________________________________
*/

#include <c8051f120.h>
#include <math.h>
#include <intrins.h>
#include "MAIN.h"
#include "config.h"
#include "ADC.h"
#include "SENSOR.h"
#include "uart.h"
#include "io.h"
#include "io_sms.h"
#include "ds18b20.h"
#include "rele.h"
#include "atflash.h"
#include "rmodem.h"
#include "gprs\gprs.h"
#include "kod8.h"



#pragma FF (0)



bit FlagTX1 = 0;
bit FlagTX0 = 0;

#define m_SIZE 8 //����� (����) ����� mLogger, mAuto, mAlarm, ���������� � flash-������
unsigned char xdata FlashDump[1024]; //1024 ���� ��������� ��� ������ 1 �������� flash uC

unsigned char xdata time_wait;
unsigned char xdata time_wait_lan = 0;

unsigned char xdata port_ADC_latch = 0;


unsigned int xdata Beacon_timer = 0; //������� (� ���) ������� ���������� �����
unsigned int xdata Day_timer = 0; //������� ��������� ������� � �������

unsigned char xdata time_wait_io;
unsigned char xdata time_wait_io2;
unsigned char xdata time_wait_io3;
unsigned char xdata time_wait_SMSPhone;
unsigned  int xdata time_wait_SMSPhone1;
unsigned char xdata time_wait_phone_receive;
unsigned char xdata time_wait_SMSPhone2;
unsigned char xdata time_wait_test;
unsigned char xdata time_wait_txtsms;
unsigned char xdata time_nogprs = 0;


unsigned char xdata count_restart_cpu;
unsigned char xdata count_restart_gsm;



unsigned char xdata DataLogger[128] = {0x0};

unsigned char xdata cnt; //Counter ������ ����������

unsigned char xdata Char; //������ ����������
unsigned char xdata * pChar; //������ ����������
unsigned int xdata Int; //������ ����������
unsigned int xdata * pInt; //������ ����������
float xdata Float; //������ ����������
float xdata Float1; //������ ����������
//float xdata * pFloat; //������ ����������


//����� ������� � ������� �� ������� �����________________________________________

unsigned char xdata StateSearch = 0x00;
bit SearchEnable = 0; //�������� ������� ������

unsigned long xdata Search1_Time; //��������� �������� ������ - ������
unsigned long xdata Search2_Time; //��������� �������� ������ - �����

unsigned int xdata CountSearchPoints; //������� ��������� ����� �� ���-��� ������
unsigned int xdata AmountSearchPoints = 65535; //������������ ���������� ����� ��� ������

unsigned long xdata * pTimeL = FlashDump;

union EVENTS xdata MaskEvents; //����� ��� ������
union EVENTS xdata * pEvents = &FlashDump[0x7B]; //��. �� ������� ��������� ��������
//_________________________________________________________________________________


unsigned long xdata Timer_inside = 0L;
bit Timer_tick = 0; //���������, ��� ������ ����������


//����������
union EVENTS xdata CurrentEvents = {0x0}; //������� ���������
union EVENTS xdata PreviousEvents = {0x0}; //���������� ���������
union EVENTS xdata mLogger; //from Setup
union EVENTS xdata mResult = {0x0};  //CurrentEvents <XOR> PreviousEvents
union EVENTS xdata mStore = {0x0};  //��������� ���������� mResult
union LIMITS xdata LimitsData; //from Setup


//��������� ��������������� ������ ������ �� ������ �����
union EVENTS xdata mAuto;   //from Setup
union EVENTS xdata mAlarm;  //from Setup

unsigned char xdata DataRAM[(AlarmPointSize+8)*AMOUNT_DATARAM_POINTS]; //����� ���������� ������
unsigned int xdata DataRAMPointer = 0; //����� �������� �����
union EVENTS xdata AddAlarmEvents = {0x0}; //������� ������� ����� �������
union EVENTS xdata AlarmEvents = {0x0}; //������������� ������� ����� �������
unsigned char xdata AlarmTickCounter = 0; //������� ������ ��� ������ ������ � ����� DataRAM
#define NAlarmTicks 2 //�������� ������� � ������, �������������� AlarmTickCounter
unsigned char xdata AmountAlarmPoints = 0; //���������� ����� � ������ DataRAM[]


unsigned char xdata NTicks; //���������� ������, ���������� �� Setup,
                            //����� ������� ����������� ����������� �������
unsigned char xdata TickCounter = 0; //������� ������


unsigned char xdata FD_Auto[64];  //from Setup - ������� ������� ������ � ������ Auto
unsigned char xdata FD_Alarm[64]; //from Setup - ������� ������� ������ � ������ Alarm
unsigned char xdata FD_Req[64];   //from Setup - ������� ������� ������ � ������ Request

//unsigned char xdata S_Password[12]; //from Setup2 - Password
//unsigned char xdata S_PinCode[4];   //from Setup2 - Pin-code


//��������� ������� ������ FD:
struct { //max = 8
    unsigned char Sta : 1; //Status
    unsigned char DrR : 1; //Dry sensors, Relays
    unsigned char ADC : 1; //ADC-inputs, d/dt
    unsigned char Sys : 1; //System Parameters
    unsigned char Bl1 : 1; //Data Block1
    unsigned char Bl2 : 1; //Data Block2
       } xdata FD;
unsigned char xdata * pFD = &FD;
unsigned char xdata * pOutBuffer = DataLogger;

unsigned char xdata * pFD_Mask = FD_Alarm;


xdata struct {
          unsigned char In[140]; //����� ����� �� ������ �����
          struct SMS_HDR Header;
          unsigned char Out[SizeBufOut + AlarmPointSize+9]; //����� ������ �� ������ �����
          unsigned char OutAuto[SizeBufOut + AlarmPointSize+9]; //--||-- ��� ����������
             } IOStreamBuf = {0x0};
xdata struct {
          unsigned int In; //����� ��������� ������ �����
          unsigned int Out; //����� ��������� ������ ������
          unsigned char ReqID; //��� ��������������� �������
             } IOStreamState = {0x0000, 0x0800, 0x00}; //����� � ����������
bit OutBufChange; //����������, ���������� �� ������ � ������ ������
unsigned char xdata IOBufPointer = 0; //��������� �� ������� ������������ ���� � ������ ������
unsigned char xdata SMS_Tail[2] = {0x0}; //����� "�������" ��� ������� SMS (max == AlarmPointSize)


//AUTO �����_____________________________________________________________________________________
unsigned int xdata CountAuto = 0; //������� ����������� ����� �� ����� mAuto
bit Auto_Enable = 1; //������� ����� ��������������� ������ ������
unsigned int xdata saveIOBufPointer;
unsigned int xdata saveSMS_Tail;
unsigned int xdata saveIOStreamStateOut;
unsigned long xdata saveRdHeader;
unsigned char xdata saveStateSearch;
unsigned long xdata saveSearch2Time;
unsigned int xdata saveAmountSearchPoints;
unsigned int xdata saveCountSearchPoints;
//_______________________________________________________________________________________________


unsigned char xdata GSM_signal;
unsigned char xdata GSM_signal_prev = 0;


bit ForbiddenAll = 1; //����� ����������..
bit GetBack_m = 0;  //�������������� ����� mResult ��� ������ � ������

bit Alarm_Enable = 0; //��������� ������ �������� ������ �� �������
bit Lock_DataRAM = 0; //���������� ������ �������


//��������� �������� � ��� - automat_request()___________________________________________________
unsigned char xdata StateRequest = 0x00;
//_______________________________________________________________________________________________


bit RestoreSearch = 0; //��������� �� ������������� ����������� ������ �������/�������


//������� - automat_alarm()______________________________________________________________________
unsigned char xdata StateAlarm = 0x00;
unsigned char xdata AlarmPointNum; //����� ��������� ����� (������� � ���������)
//_______________________________________________________________________________________________


#define HystVolt  5 //���������� ��� ���������� �������
#define HystADC   2 //���������� ��� �������� ���, �������������� ���
#define HystTherm 3 //���������� ��� ����������� ����, ����������� ������� MicroLan

//���������� �� ���������� �������� ������� �����; ����������� � 0:00GMT
//���� ������� ���, ���� ������� � ��������� ������� ������ ONLINE
bit Beacon_Enable = 1;


unsigned long xdata Rele_LCount = 0;


bit DefaultProfile = 0; //"1" - ������������ ������� ���������� "�� ���������" -
                        //�����. ������������ ������, ��������� �� flash ([F_ADDR_SETUP])
unsigned char xdata reset_code; //��� �������� CPU


//������ ����� �������___________________________________________________________________________
bit CMask = 0;
unsigned char xdata CMask_cnt; //������� ������ ������ �����
unsigned char xdata CMask_buf[9];
//_______________________________________________________________________________________________


//�������������� �� ������� �� ������������ ������� ������_______________________________________
bit RA_Mode = 0; //������� �������������� ������� (��������� � Flash �����������)

//"Restore Alarms"' Address (EEPROM/DataFlash)
//RAA1 - ����� (� �������) ��������� ��������� (��������� �������������) �������
unsigned int xdata RAA1;
//RAA2 - ����� (� �������) ������ �������������� / ��������� ������������ �������
unsigned int xdata RAA2;
unsigned int xdata RAA3;
//RA_cnt - ������� ����������� �������������� ������
signed char xdata RA_cnt = 0;
#define RA_cnt_MAX 50
#define RA_cnt_MIN 25
signed char xdata RA_tmp = 0;

//������ StateSendSMS
//[���R���r]: � - ����� �� ��������� ������ � ������ �������� SMS;
//            � - ��������� ������� � ������ �������� SMS;
//            � - ��������� ������������� � ������ �������� SMS;
//            R - ��������� ��������� RA � ������ �������� SMS;
//            � - ����� �� ��������� ������ ��������� �� �������;
//            � - ��������� ������� ��������� �� �������;
//            � - ��������� ������������� ��������� �� �������;
//            r - ��������� ��������� RA ��������� �� �������
unsigned char xdata StateSendSMS = 0x00;
unsigned char xdata State__RA = 0x00; //��������� ������ ����� � ������ RA
unsigned char xdata TimeRA = 0xFE; //"0xFE" - ����������� ����� �� ��������
//_______________________________________________________________________________________________


unsigned int code LED_Mode[] = {
    ~0x0000,   //OFF
    ~0xFFFF,   //ON
    ~0x5555,   //NO TIME
    ~0x7777,   //NO TIME + USB
    ~0x0FF0,   //NORMAL
    ~0x3FFC,   //NORMAL + USB
    ~0x0001,   //SEND SMS
    ~0x0005    //SEND SMS 2
                               };

unsigned int code LED1_Mode[] = {
    ~0x0000,    //OFF (COMMAND MODE, GPRS OFF)
    ~0x5555,    //DIAL
    ~0x3333,    //PPP CONNECT/DISCONNECT
    ~0x0F0F,    //TCP CONNECT
    ~0xFFFF,    //HTTP WAIT
    ~0x0001,    //SEND HEADER
    ~0x0005,    //SEND DATA
    ~0x0015,    //TCP DISCONNECT
    ~0x0FF0     //COMMAND MODE, GPRS ON
                                };

unsigned int code LEDSTAT_Mode[] = {
    ~0x0000,   //OFF
    ~0xFFFF,   //ON
    ~0x0080,   //NO RESPONSE FROM MODEM
    ~0x0140,   //NO TEL NUMBERS
    ~0x0490,   //SMS SEND UNSUCCESSFUL
    ~0xFFFF    //MODEM INIT UNSUCCESSFUL
                                   };

unsigned char xdata StLED     = 0x02;
unsigned char xdata StLED1    = 0x00;
unsigned char xdata StLEDSTAT = 0x00;

unsigned int xdata StSYSTEM   = 0x0000;
// 0x0001 - USB power connected
// 0x0002 - �������� SMS
// 0x0004 - �������� ������������� SMS
// 0x0008 - �� ���.
// 0x0010 - ����� �� ��������
// 0x0020 - �� ����������� ����������� ���. �������
// 0x0040 - ��������� �������� SMS
// 0x0080 - ������ ������������� ������
// 0x0100 - ������ GPRS
// 0x0200 - �� ���.
// 0x0400 - TCP level
// 0x0800 - ���������� HTTP �����������
// 0x1000 - HTTP, �������� ������               \
// 0x2000 - HTTP, �������� ������ (send buffer) / 0x3000 - HTTP, �������� ������ (CTS busy)
// 0x4000 - �� ���.
// 0x8000 - ����� GPRS �������

//����� XOR �� ���������� �����. ����� ������ �������
unsigned char xdata mLogCS = _MLOGCSDEFAULT_;




void init_limits_data();

void automat_leds()
{
    //*LED*
    if (StSYSTEM & 0x0004)
        StLED = 7;
    else
    if (StSYSTEM & 0x0002)
        StLED = 6;
    else
    if (ForbiddenAll)
        StLED = 2 | StSYSTEM & 0x0001;
    else
        StLED = 4 | StSYSTEM & 0x0001;
    //*LED1*
    if (StSYSTEM & 0x8000)
        if (StSYSTEM & 0x0100)
        {
            //GPRS connect, GPRS active (PPP level)
            if (StSYSTEM & 0x0400)
                //TCP level
                if (StSYSTEM & 0x0800)
                    //HTTP level
                    switch (StSYSTEM & 0x7000)
                    {
                        case 0x1000: StLED1 = 5; break;
                        case 0x2000: StLED1 = 6; break;
                        case 0x3000: StLED1 = 7; break;
                            default: StLED1 = 4; break; //0, or incorrect state
                    }
                else
                    //TCP connect
                    StLED1 = 3;
            else
                //PPP connect/disconnect
                StLED1 = 2;
        }
        else
            //temporary command mode, GPRS stay active
            StLED1 = 8;
    else
        if (StSYSTEM & 0x0100)
            //dialing
            StLED1 = 1;
        else
            //command mode, GPRS off
            StLED1 = 0;        
    //*LEDSTAT*
    if (!(StSYSTEM & 0x00F0))
        StLEDSTAT = 0;
    else
    if (StSYSTEM & 0x0010)
        StLEDSTAT = 2;
    else
    if (StSYSTEM & 0x0080)
        StLEDSTAT = 5;
    else
    if (StSYSTEM & 0x0020)
        StLEDSTAT = 3;
    else
    if (StSYSTEM & 0x0040)
        StLEDSTAT = 4;
}


void ReadFlash(unsigned char code * pread, unsigned int amount) //������ ����� �� Flash-ROM ����������������
//pread - ��������� ����� ����������� ������ (0x0..0x7FFF), ������ ������ � BANK0
//amount - ���-�� ����, ���������� - ����� FlashDump[]
{
  unsigned int data i;
  reset_wdt; //?
    for (i=0; i<amount; i++) FlashDump[i] = *pread++;
  reset_wdt; //?
}

void WriteFlash(unsigned char xdata * pwrite, unsigned int amount) //������ ����� � Flash-ROM ����������������
{
  unsigned int data i;
//0) Initialization
    SFRPAGE = CONFIG_PAGE;
    CCH0CN |= 0x01; //"Block Write Enable" (!!! amount ������ ���� ������ 4 !!!)
    SFRPAGE = LEGACY_PAGE;
  reset_wdt;
//1) Erasing Flash Page:
    FLSCL |= 0x01;
  global_int_no;
    PSCTL |= 0x03;
    *pwrite = 0;
    PSCTL &= 0xFD;
  reset_wdt;
//2) Writing Buffer to Flash:
    for (i=0; i<amount; i++) *pwrite++ = FlashDump[i];
  reset_wdt;
    PSCTL &= 0xFE;
  global_int_yes;
    FLSCL &= 0xFE;
}

#ifdef GuardJTAG
#pragma disable
void LockJTAG() //��������� ������ ������ �� ������/������ �� JTAG
{
  unsigned char data c = PSBANK;
    SFRPAGE = CONFIG_PAGE;
    CCH0CN &= 0xFE; //"Block Write Disable"
    SFRPAGE = LEGACY_PAGE;
    PSBANK |= 0x30;
    FLSCL |= 0x01;
    PSCTL |= 0x01;
    *((unsigned int xdata*)0xFBFE) = 0x0000;
    PSCTL &= 0xFE;
    FLSCL &= 0xFE;
    PSBANK &= c; //return to Prev Bank
}
#endif

void ReadScratchPad1() //������ �� ScratchPad (bank1) 128 ���� � ����� FlashDump[]
{
  unsigned char data c = 0;
  unsigned char code * pread = 0x0000;
    SFRPAGE = LEGACY_PAGE;
    PSCTL |= 0x04;
    while (c < 128) FlashDump[c++] = *pread++;
    PSCTL &= 0xFB;
}

void WriteScratchPad1() //������ � ScratchPad (bank1) 128 ���� �� ������ FlashDump[]
{
  unsigned char data c = 0;
  unsigned char xdata * pwrite = 0x0000;
//0) Initialization
    SFRPAGE = CONFIG_PAGE;
    CCH0CN |= 0x01; //"Block Write Enable"
    SFRPAGE = LEGACY_PAGE;
    PSCTL |= 0x04;
  reset_wdt;
//1) Erasing Flash Page:
    FLSCL |= 0x01;
  global_int_no;
    PSCTL |= 0x03;
    *pwrite = 0;
    PSCTL &= 0xFD;
  reset_wdt;
//2) Writing Buffer to Flash:
    while (c < 128) *pwrite++ = FlashDump[c++];
    PSCTL &= 0xFA;
  global_int_yes;
    FLSCL &= 0xFE;
}


void WriteOutputData()
//������������ ����� ��������� ����� (max = ...)
//����� ������� ��������� ���������� pOutBuffer, *pFD
{
  unsigned char data i;
  unsigned char xdata * pStream; //��������� �� ������� ����� ������

    if (Auto_Enable) pStream = IOStreamBuf.OutAuto;
    else pStream = IOStreamBuf.Out;

                { for (i=0x00; i<=0x03; i++) pStream[IOBufPointer++] = pOutBuffer[i]; } //Time
    if (FD.Sta) {      i=0x04;               pStream[IOBufPointer++] = pOutBuffer[i]; } //Status
    if (FD.DrR) {      i=0x05;               pStream[IOBufPointer++] = pOutBuffer[i]; } //Dry s., Relays
    if (FD.ADC) { for (i=0x06; i<=0x09; i++) pStream[IOBufPointer++] = pOutBuffer[i]; } //ADC inputs
    if (FD.Sys) { for (i=0x0A; i<=0x0D; i++) pStream[IOBufPointer++] = pOutBuffer[i]; } //System par.
    if (FD.Bl1) { for (i=0x0E; i<=0x43; i++) pStream[IOBufPointer++] = pOutBuffer[i]; } //Data Block1
    if (FD.Bl2) { for (i=0x44; i<=0x79; i++) pStream[IOBufPointer++] = pOutBuffer[i]; } //Data Block2
}

unsigned char GetFDMask(union EVENTS EMask)
//EMask - ����� �������, �� ������� ��������� ����� ����� ������� ������
//������������ ����� �������, ������� ����� ������ ���� �������� � *pFD
//����� ������� ���������� pFD_Mask
//��� CMask ���������, ��������� �� ������ ����� EMask
//���� ����� ������ GetFDMask() CMask ������� ("1"->"0"), ����� ����� �� �������
{
unsigned char xdata cnt1;
unsigned char xdata cnt2;
unsigned char xdata Result = 0x00;
unsigned char xdata tmp;

  for (CMask_cnt=0,cnt1=0; cnt1<8; cnt1++)
  {
    tmp = EMask.c[cnt1];
    if (tmp)
      for (cnt2=0; cnt2<8; cnt2++)
        if (tmp & (0x1 << cnt2))
        {
          Result |= pFD_Mask[Char = (cnt1 << 3) + cnt2];
          if (CMask) //� SMS (0x31, 0x32) ������������ ����� �������
          {
            if (CMask_cnt >= 9) {CMask = 0; continue;} //����� ��������� � �������� ���� + 1 ����
            if (CMask_cnt) //�� 0-� ���� �����
                CMask_buf[CMask_cnt-1] |= 0x80;
            CMask_buf[CMask_cnt++] = Char;
          }
        }
  }
  return(Result);
}

void SetRdSearchRA() //������ ������ ����� � ������ RA
{
    BufAF = FlashDump; //����������
    ReadAF(); //�����

    if (RdHeader.t.Byte == 0x00) RdHeader.t.Byte = 0x80;
    else
    {
        RdHeader.t.Byte = 0x00;
        if (RdHeader.t.Page < MaxPageAF) RdHeader.t.Page++;
        else RdHeader.t.Page = 0x000;
    }
}

void automat__RA() //������ � ������ RA
{
    switch (State__RA)
    {
        case 0x01:
            RdHeader.t.Page = RAA2 >> 2;
            RdHeader.t.Byte = (RAA2 & 0x03) << 6;
            pOutBuffer = &FlashDump;
            RA_tmp = 0; //������� ������ � ������������ SMS-���������
            State__RA = 0x02;

        case 0x02:
        case 0x12:
            //����� ������ ����������; �����...
            if (IOBufPointer >= SizeBufOut)
            {
                IOStreamState.Out |= 0x9800;
                return;
            }
            if (State__RA == 0x12)
            {
                //���� SMS ����������, ����������� RAA2; ���������, ��� � ������ ���������
                //��������� ���������� ����� �����������, �.�. � RAA2 �������� �����
                //��������� ����� ����������� ���������
                State__RA = 0x02;
                RAA2 = RAA1;
                RA_cnt -= (RA_tmp-1);
                RA_tmp = 1;
            }
            RAA1 = (RdHeader.t.Page << 2) | (RdHeader.t.Byte >> 6);
            //������ ����� �� �������
            SetRdSearchRA();
            //�������� �����. �����
            for (Char=0,cnt=0; cnt<0x7F; cnt++) Char += FlashDump[cnt];
            if ((Char ^ mLogCS) != FlashDump[0x7F]) //��� ������; ���������...
            {
                IOStreamState.Out &= 0xFF00;
                IOStreamState.Out |= (0x9C00 | IOBufPointer); //���������� �������� �����
                State__RA = 0xFF;
                RA_Mode = 0;
                return;
            }
            //���� ��������� �����, �����. ��������� �������� �����, ����� ����������� ������
            if (RAA3 == RAA1) RAA3 = (RdHeader.t.Page << 2) | (RdHeader.t.Byte >> 6);
            //�������� �� ������� �������
            if (FlashDump[0x04] & 0x80) RA_tmp++;
            else return; //�� ������� - ������� �����
            //������������ ����� � ������ ������
            CMask = 1;
            IOStreamBuf.Out[IOBufPointer++] = *pFD = GetFDMask(*pEvents);
            if (CMask)
                for (cnt=0; cnt<CMask_cnt; cnt++)
                    IOStreamBuf.Out[IOBufPointer++] = CMask_buf[cnt];
            else
            {
                IOStreamBuf.Out[IOBufPointer++] = 0xC0; //"����� ��� ������"
                for (cnt=0; cnt<4; cnt++) IOStreamBuf.Out[IOBufPointer++] = (*pEvents).c[cnt];
                for (cnt=0; cnt<4; cnt++) IOStreamBuf.Out[IOBufPointer++] = 0x00;
            }
            WriteOutputData();
            //���������� ����� "������", ���� �������� ����� SMS, ���. ������ ������ SMS
            if (IOBufPointer >= SizeBufOut)
            {
                //���������� �������� ����� ������ � ������ + "������� SMS"
                IOStreamState.Out &= 0xFF00;
                IOStreamState.Out |= (0x9800 | IOBufPointer);
                SMS_Tail[1] = IOBufPointer - SizeBufOut;
                State__RA = 0x12;
            }

            return;

        case 0xFF: //����������
            State__RA = 0x00;
    }
}

void SetRdSearch() //���������� ��������� ������ ��� automat_search()
{
    //������� �� ��������� �����
    if (RdHeader.t.Byte) RdHeader.t.Byte = 0x00;
    else
    {
        RdHeader.t.Byte = 0x80;
        if (RdHeader.t.Page) RdHeader.t.Page--;
        else RdHeader.t.Page = MaxPageAF;
    }

    BufAF = FlashDump; //����������
    ReadAF(); //�����
}

void automat_search()
//�������� ������:
//Search1.XXX - ��������� ����� ������� �� �������
//Search2.XXX - �������� --||--
//MaskEvents.X.XXX - ����� ����� ������ (�� ���)
//AmountSearchPoints - ������������ ���������� ��������� �����
{
unsigned char data i;

  if (IOBufPointer >= SizeBufOut) IOStreamState.Out |= 0x8000;

  switch (StateSearch)
  {
    case 0x00: //����� ������

        RdHeader.l = WrHeader.l; //������ � ��������� ���������� �����
        pOutBuffer = &FlashDump;

        StateSearch = 0x01;
        return;

    case 0x01: //����� �������� �������� �� �������...

        //����� ������ ����������; �����...
        if (IOStreamState.Out & 0x8000) return;

        SetRdSearch();

        //�������� �����. �����
        for (Char=0, i=0; i<0x7F; i++) Char += FlashDump[i];
        if ((Char ^ mLogCS) != FlashDump[0x7F]) //��� ������; ���������...
        {
            StateSearch = 0x03;
            return;
        }

        if (*pTimeL > Search2_Time) return; //��������� ������...

    case 0x02: //����� �� �����; ������� �������� �� �������...

        //����� ������ ����������; �����...
        if (IOStreamState.Out & 0x8000) return;

        if (StateSearch == 0x02) SetRdSearch();
        else StateSearch = 0x02;

        //�������� �����. �����
        for (Char=0, i=0; i<0x7F; i++) Char += FlashDump[i];
        if ((Char ^ mLogCS) != FlashDump[0x7F]) //��� ������; ���������...
        {
            StateSearch = 0x03;
            return;
        }

        if (*pTimeL < Search1_Time) //����� ������� �� �������...
        {
            StateSearch = 0x03;
            return;
        }

        //����� �� ������� �����...
        if (((*pEvents).l.l1 & MaskEvents.l.l1) || //i.e. subset
            ((*pEvents).l.l2 & MaskEvents.l.l2))
        {
            //������������ � ����� �/� ������� ����� �������� �������������� ������� FD
            //���. ������ FD ��� ������� ����� � �������� ��� � ����� ������

            if (Auto_Enable) //!for GPRS only!
            {
                if (!gprs.b.abs_off) IOStreamState.Out |= 0x0100; //for GPRS
                pChar = IOStreamBuf.OutAuto;
                CMask = 0; //��� ������
                *pFD = 0x3F;
            }
            else
            {
                pChar = IOStreamBuf.Out;
                CMask = 0; //��� ������
                *pFD = 0x3F;
                //pChar[IOBufPointer++] = *pFD = GetFDMask(*pEvents);
            }

            if (CMask)
                for (cnt=0; cnt<CMask_cnt; cnt++) pChar[IOBufPointer++] = CMask_buf[cnt];
            else
            {
                pChar[IOBufPointer++] = 0xC0; //"����� ��� ������"
                for (cnt=0; cnt<4; cnt++) pChar[IOBufPointer++] = (*pEvents).c[cnt];
                for (cnt=0; cnt<4; cnt++) pChar[IOBufPointer++] = 0x00;
            }

            WriteOutputData();

            //if (Auto_Enable) //��� �������
            {
                IOStreamState.Out &= 0xFF00;
                IOStreamState.Out |= (0x8400 | IOBufPointer);
                SMS_Tail[1] = 0;
            }
            /*else
            //���������� ����� "������", ���� �������� ����� SMS, ���. ������ ������ SMS
            if (IOBufPointer >= SizeBufOut)
            {
                //���������� �������� ����� ������ � ������ + "������� SMS"
                IOStreamState.Out &= 0xFF00;
                IOStreamState.Out |= (0x8000 | IOBufPointer);
                SMS_Tail[1] = IOBufPointer - SizeBufOut;
            }*/

            //����������� ���. ��������� �����...
            if (++CountSearchPoints >= AmountSearchPoints) //���� ������ ��������... �����
            {
                if (Auto_Enable==0) IOStreamState.Out &= 0x7FFF; //(��������� ����� 0x31/0x3D)
                StateSearch = 0x03;
            }
        }

        //��������� ������...
        return;

    case 0x03: //���������� ������ ��������

        //���������� �������� ����� ������ � ������
        IOStreamState.Out &= 0xFF00;
        IOStreamState.Out |= IOBufPointer;

        SearchEnable = 0;
        StateSearch = 0x00;
  }
}

bit Test_In_CS()
{
  unsigned char data c = 0;
  unsigned int xdata CS = 0;

    for ( ; c<138; c++) CS += IOStreamBuf.In[c];
    if (CS != *((unsigned int xdata*)(&IOStreamBuf.In[138]))) return 0;
    return 1;
}

unsigned int xdata RNext; //���. �������� ��� ������� ��������

void automat_request() //��������� �������� � ���
{
unsigned char data i;
unsigned int xdata CheckSum;

    switch (StateRequest)
    {
        case 0x00: //�������� �������

            if (IOStreamState.In & 0x8000) //������ ������
            {
                IOStreamState.ReqID = IOStreamBuf.In[0];
                StateRequest = 0x01; //��������� ����� �� ���
            }

            return;

        case 0x01: //���������������� ��������

            if (IOStreamState.Out & 0x4000) return; //� ������ ������ ��������� ������������
                                                    //��� ������ SMS � �� ������ ����������!
            saveIOBufPointer = IOBufPointer;
            saveIOStreamStateOut = IOStreamState.Out;
            saveSMS_Tail = *(pInt = SMS_Tail);
            *pInt = 0x0000;

            if (RestoreSearch = SearchEnable) //�������� �����; ��������� ��� ��������� ������
            {
                saveRdHeader = RdHeader.l;
                saveStateSearch = StateSearch;
                saveSearch2Time = Search2_Time;
                saveAmountSearchPoints = AmountSearchPoints;
                saveCountSearchPoints = CountSearchPoints;

                SearchEnable = 0;
            }
            
            Auto_Enable = 0;
            //���������� � ������
            if (RM_Enable)
                RNext = IOStreamState.Out = 0x2200;
            else
                RNext = IOStreamState.Out = 0x2000 | IOStreamState.In & 0x0300;

            switch (IOStreamState.ReqID) //��������� ��� �������
            {
                case 0x31: case 0x3D: case 0x4F: case 0x51:
                case 0x33: case 0x35: case 0x37:
                case 0x03: case 0x0B: case 0x0F: case 0x5B: case 0x53:
                case 0x41:
                case 0x43: case 0x45: case 0x47:
                case 0x4D:
                case 0xDB: case 0x4B:
                case 0x7B:
                    StateRequest = IOStreamState.ReqID;
                break;

                default: //����������� ��� ������� - �� ��������������
                    IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
                    IOStreamState.Out |= 0x8400; //������ ������ �����
                    StateRequest = 0xFF; //���������
            }

            return;

        case 0x3D: //������ �� ������ ������ �� �������
        case 0x31: //������ �� ������ ������ �� �������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            Char = 1; //���������������� �������; ���. �������� �����. ����� N1

            for (i=0; i<8; i++) MaskEvents.c[i] = IOStreamBuf.In[Char++];

            for (i=0; i<64; i++) FD_Req[i] = IOStreamBuf.In[Char++];

            if (AmountSearchPoints = IOStreamBuf.In[Char++]);
            else AmountSearchPoints = 65535; //���� "0" - ������� ������������ ���-�� �����
            CountSearchPoints = 0;

            //�������������� �� ������� TSIP � ���. �������� (unsigned long), [���]
            pChar = &Float;
            for (i=0; i<4; i++) *pChar++ = IOStreamBuf.In[Char++];
            pChar = &Int;
            for (i=0; i<2; i++) *pChar++ = IOStreamBuf.In[Char++];
            Search1_Time = (unsigned long)Int*604800L + (unsigned long)Float;

            //�������������� �� ������� TSIP � ���. �������� (unsigned long), [���]
            pChar = &Float;
            for (i=0; i<4; i++) *pChar++ = IOStreamBuf.In[Char++];
            pChar = &Int;
            for (i=0; i<2; i++) *pChar++ = IOStreamBuf.In[Char++];
            Search2_Time = (unsigned long)Int*604800L + (unsigned long)Float;

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

            StateRequest = 0x02;
            return;

        case 0x02: //������� ������� ������, �� ���������� � ������
        case 0x09: //--||-- � ��������� ��������� �������
            
            IOBufPointer = 0;

            pFD_Mask = FD_Req;
            pOutBuffer = DataLogger;
            CMask = 0; //������ ����� �� �����������
            *pFD = GetFDMask(MaskEvents);

            //1) ��������� �� ������� ������
            IOStreamBuf.Out[IOBufPointer++] = *pFD; //������
            //"��� �������"
            IOStreamBuf.Out[IOBufPointer++] = 0x40;

            WriteOutputData();

            IOStreamState.Out &= 0xFF00;
            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

            if (StateRequest != 0x02) //������ 0x31/0x3D ��� ���������
            {
                StateRequest = 0xFF;
                return;
            }

            StateSearch = 0x00; //�� ������ ������
            SearchEnable = 1; //�����

            StateRequest = 0x08;
            return;

        case 0x33: //������� ��������� ����� mLogger, mAuto, mAlarm � ���������� ������� LimitsData

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            CountAuto = StateSearch = 00; //���������� ����� ���������� �� ��������
            RestoreSearch = 0; //(?)

            //������ ������ ������� 0x33 � 0x35 � �����
            ReadFlash(F_ADDR_SETUP, 260); //130 + 130

            Char = 0;
            for (i=0; i<8; i++) mLogger.c[i] = FlashDump[Char-1] = IOStreamBuf.In[++Char];
            for (i=0; i<8; i++) mAuto.c[i] = FlashDump[Char-1] = IOStreamBuf.In[++Char];
            for (i=0; i<8; i++) mAlarm.c[i] = FlashDump[Char-1] = IOStreamBuf.In[++Char];
            for (i=0; i<lim_SIZE; i++) LimitsData.All[i] = FlashDump[Char-1] = IOStreamBuf.In[++Char];
            for ( ; Char < 130; ) FlashDump[Char-1] = IOStreamBuf.In[++Char]; //?

            init_limits_data(); //���. ������������� ����� ������������� ����������

            reset_wdt;
            WriteFlash(F_ADDR_SETUP, 260);
            reset_wdt;

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            StateRequest = 0x05;
            return;

        case 0x35: //������� ��������� ����� ������� ������ ������ FD_Auto, FD_Alarm

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            //������ ������ ������� 0x33 � 0x35 � �����
            ReadFlash(F_ADDR_SETUP, 260); //130 + 130

            Char = 0;
            for (i=0; i<64; i++)
            {
                FD_Auto[i] = FlashDump[Char+129] = IOStreamBuf.In[++Char];
                FD_Alarm[i] = FlashDump[Char+129] = IOStreamBuf.In[++Char];
            }
            FlashDump[258] = IOStreamBuf.In[129];
            FlashDump[259] = IOStreamBuf.In[130];

            reset_wdt;
            WriteFlash(F_ADDR_SETUP, 260);
            reset_wdt;

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            StateRequest = 0x05;
            return;

        case 0x0B: //����� ������� SMS- � ������������� �������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            // ���� ��� �������, �� ������ �� �� �����
            if (Test_tel_to_phone(&IOStreamBuf.In[0]))
            {
                for (i=0; i<10; i++) Write_tel_to_phone (i, &IOStreamBuf.In[i*12]);
                // ���� �����-�� ������� ���, �� ��������� �� ���� ��������, ��� ����.
                Analise_tel_num();

                if (IOStreamBuf.In[121]!=0xFF)
                {
                    sec_for_numtel = IOStreamBuf.In[121] & 7;
                    //��������� �� ����� "����� �������� �������� ������ �� ������ ��������"
                    ReadScratchPad1();
                    FlashDump[8] = 'S'; 
                    FlashDump[9] = sec_for_numtel;
                    WriteScratchPad1();
                }
            }

            StateRequest = 0x03;
            return;

        case 0x0F: //����� ���������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            if (IOStreamBuf.In[1]==0x1F)
            {
                ReadScratchPad1();
                FlashDump[5] = 0xAA;
                WriteScratchPad1();
            }
            if (RM_Enable) StRM.b.fNeedRst = 1;
            else _flag_need_do_restart = 1;

            StateRequest = 0x03;
            return;

        case 0x03: //�����������

            if (IOStreamState.ReqID == 0x03)
                if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            IOBufPointer = 0;
            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

            IOBufPointer = do_pack030F(&IOStreamBuf.Out);
            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

            StateRequest = 0xFF;
            return;

        case 0x53: //������ ������� (10 ���������������)

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            IOBufPointer = do_pack53(&IOStreamBuf.Out);
            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����
            StateRequest = 0xFF;
            return;

        case 0x5B: //�����  ������� (10 ���������������)
            if (!Test_In_CS()) {StateRequest = 0xCC; return;}
            for (i=0; i<10; i++) Write_tel_to_phone (i+11, &IOStreamBuf.In[i*12]);
            //Analise_10d_tel_num(); // ������ 10 �������������� �������
            if (IOStreamBuf.In[121]!=0xFF)
            {
                sec_for_numtel = IOStreamBuf.In[121] & 7;
                // ����� ��������� �� ����� "����� �������� �������� ������ �� ������ ��������"
                ReadScratchPad1();
                FlashDump[8] = 'S'; 
                FlashDump[9] = sec_for_numtel;
                WriteScratchPad1();
            }
            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            IOBufPointer = do_pack53(&IOStreamBuf.Out);
            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����
            StateRequest = 0xFF;
            return;

        case 0x41: //'A'
            time_wait_txtsms=0;
            StateRequest = 0x10;

        case 0x10:
            if (time_wait_txtsms>1)
            {
                i = (unsigned char)((IOStreamState.In & 0x0300)==0x0100); // GPRS?
                switch (IOStreamBuf.In[1+i])
                {
                    case 1:// ����������/������, C�����
                        IOBufPointer = do_txtpack_S(&IOStreamBuf.Out,i);
                    break;
                    case 2:// ������� Ok
                        IOBufPointer = do_txtpack_T(&IOStreamBuf.Out,i,0);
                    break;
                    case 4:// ����� ������
                        IOBufPointer = do_txtpack_P(&IOStreamBuf.Out,i);
                    break;
                    case 5:// �������� ������
                        IOBufPointer = do_txtpack_N(&IOStreamBuf.Out,i);
                    break;
                    case 6:// ����� ������
                        IOBufPointer = do_txtpack_V(&IOStreamBuf.Out,i);
                    break;
                    case 7:// ��������� �������
                        IOBufPointer = do_txtpack_T(&IOStreamBuf.Out,i,1);
                    break;
                }

                IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

                IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

                IOStreamState.ReqID = 0x20;

                StateRequest = 0xFF;
            }
            return;

        case 0x43: //������� �� ��� ������, ������������ �� ������� 0x33; ����� �� ������ 0x33

            IOBufPointer = 0;

            //���� �������� ����� �� StateRequest == 0x05 (IOStreamState.ReqID == 0x33)
            if (IOStreamState.ReqID == 0x43)
            {
                if (!Test_In_CS()) {StateRequest = 0xCC; return;}
                IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            }
            else //(IOStreamState.ReqID == 0x33); ������� 0x33 <�� Default> �������
                DefaultProfile = 0;

            for (i=0; i<8; i++) IOStreamBuf.Out[IOBufPointer++] = mLogger.c[i];
            for (i=0; i<8; i++) IOStreamBuf.Out[IOBufPointer++] = mAuto.c[i];
            for (i=0; i<8; i++) IOStreamBuf.Out[IOBufPointer++] = mAlarm.c[i];
            for (i=0; i<lim_SIZE; i++) IOStreamBuf.Out[IOBufPointer++] = LimitsData.All[i];

            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

            StateRequest = 0xFF;
            return;

        case 0x45: //������� �� ��� ������, ������������ �� ������� 0x35; ����� �� ������ 0x35

            IOBufPointer = 0;

            //���� �������� ����� �� StateRequest == 0x05 (IOStreamState.ReqID == 0x35)
            if (IOStreamState.ReqID == 0x45)
            {
                if (!Test_In_CS()) {StateRequest = 0xCC; return;}
                IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            }

            for (i=0; i<64; i++)
            {
                IOStreamBuf.Out[IOBufPointer++] = FD_Auto[i];
                IOStreamBuf.Out[IOBufPointer++] = FD_Alarm[i];
            }

            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

            StateRequest = 0xFF;
            return;

        //����������� ��������� ������� 0x31
        case 0x08: //��������� ���������

            pFD_Mask = FD_Req; //?
            if (SearchEnable) {IOStreamState.Out |= RNext; return;} //�����...
            //����� �������
            if (IOStreamState.Out & 0x4000) return; //��������� �������������
            if (IOStreamState.Out & 0x00FF) //� ������ ���� ������
            {
                IOStreamState.Out |= 0x8400 | RNext; //���������� �������� �����
                StateRequest = 0xFF;
                return;
            }
            //� ������ ��� ������ - �������� ������� �������
            StateRequest = 0x09;
            return;

        //����������� ��������� �������� 0x33, 0x35
        case 0x05: //������ ������� �� Flash � ����� �� ���

            //���������� � �������� ����������� �����;
            //���� ������ ������ ���������, ������ ����� ����������� ������ 0x4X
            //1) �������� �����. ����� 1-� ����� ������� (0x33)
            ReadFlash(F_ADDR_SETUP, 130);
            for (CheckSum=0, i=0; i<128; i++) CheckSum += FlashDump[i];
            if (CheckSum != ((FlashDump[128] << 8) | FlashDump[129])) Char = 1; else Char = 0;
            reset_wdt;
            //2) �������� �����. ����� 2-� ����� ������� (0x35)
            ReadFlash(F_ADDR_SETUP+130, 130);
            for (CheckSum=0, i=0; i<128; i++) CheckSum += FlashDump[i];
            if (CheckSum != ((FlashDump[128] << 8) | FlashDump[129])) Char |= 0x2;
            //3) ������ ����� �� ������
            if (IOStreamState.ReqID == 0x33)
            {
                if (Char & 0x1)
                {
                    IOStreamBuf.Out[0] = 0x33; //��� �������������� �������
                    IOStreamBuf.Out[1] = 0xCE; //��� ������
                    IOStreamBuf.Out[2] = 0x35;
                    IOStreamBuf.Out[3] = (Char & 0x2) ? 0xCE : 0x00; //��� ������
                    IOStreamState.ReqID = 0x3F; //����� �� ������
                    IOStreamState.Out |= 0xA404; //���������� �������� �����
                    StateRequest = 0xFF;
                    return;
                }
                StateRequest = 0x43;
            }
            else
            {
                if (Char & 0x2)
                {
                    IOStreamBuf.Out[0] = 0x35; //��� �������������� �������
                    IOStreamBuf.Out[1] = 0xCE; //��� ������
                    IOStreamBuf.Out[2] = 0x33;
                    IOStreamBuf.Out[3] = (Char & 0x1) ? 0xCE : 0x00; //��� ������
                    IOStreamState.ReqID = 0x3F; //����� �� ������
                    IOStreamState.Out |= 0xA404; //���������� �������� �����
                    StateRequest = 0xFF;
                    return;
                }
                StateRequest = 0x45;
            }

            return;

        case 0x4D: //���������� ���� � ����������/������ ��/� ������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            UprRele(&IOStreamBuf.In[1]);

            time_wait_txtsms = 0;

            StateRequest = 0x7F; //������� ������� ��������� ���
            return;

        case 0x37: //��������� �������������� ����������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            //do_pack37();
            reset_wdt;
            //do_pack47();

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

            StateRequest = 0xFF;
            return;

        case 0x47: //������ �������������� ����������

            if (!Test_In_CS()) {StateRequest = 0xCC; return;}

            //do_pack47();

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

            StateRequest = 0xFF;
            return;
/*
        case 0x4B: //������� ��������� (GPRS)
            _f_SMS_noanswer=0;
            if (Test_read_text(&IOStreamBuf.In[5])==1)
            {         
                if (IOStreamBuf.In[6]==0x01) // ����������/������, C�����
                    IOBufPointer = do_txtpack_S(&IOStreamBuf.Out);

                if (IOStreamBuf.In[6]==0x02) // ������� Ok
                    IOBufPointer = do_txtpack_Ok(&IOStreamBuf.Out);

                if (IOStreamBuf.In[6]==0x04) // ����� ������
                    IOBufPointer = do_txtpack_P(&IOStreamBuf.Out);

                if (IOStreamBuf.In[6]==0x05) // �������� ������
                    IOBufPointer = do_txtpack_N(&IOStreamBuf.Out);

                IOStreamState.Out = (0xA500 | IOBufPointer); //���������� �������� �����
            }
            else
                IOStreamState.Out = 0x0000;

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������

            StateRequest = 0xFF;
            return;
*/
        case 0xCC: //�������� ����������� �����

            //������ ����������� ����� - ������� ���������� ��������, ������ ��������� �� ������
            IOStreamState.Out |= 0xA402; //���������� �������� ����� (2 �����)
            IOStreamBuf.Out[0] = IOStreamState.ReqID; //��� �������������� �������
            IOStreamBuf.Out[1] = 0xCC; //��� ������ (0xCC "������ �����. �����")
            IOStreamState.ReqID = 0x3F; //����� �� ������

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            StateRequest = 0xFF;
            return;

        case 0xFF: //����������
            
            if (IOStreamState.Out & 0x8000) return; //���� �� ������ �����...

            //������������ ��� ��������� ���������� � ������
            IOBufPointer = saveIOBufPointer;
            IOStreamState.Out = saveIOStreamStateOut;
            *(pInt = SMS_Tail) = saveSMS_Tail;
            Auto_Enable = 1;
            RM_Enable = 0;
            if (RestoreSearch) //��� ������� �����; ������������ ��������� ������
            {
                SearchEnable = 1;

                RdHeader.l = saveRdHeader;
                StateSearch = saveStateSearch;
                Search1_Time = 0L;
                Search2_Time = saveSearch2Time;
                MaskEvents.l.l1 = mAuto.l.l1;
                MaskEvents.l.l2 = mAuto.l.l2;
                AmountSearchPoints = saveAmountSearchPoints;
                CountSearchPoints = saveCountSearchPoints;

                pFD_Mask = FD_Auto;
            }
            pOutBuffer = FlashDump;

            IOStreamState.ReqID = 0x00;

            StateRequest = 0x00;
            return;

        case 0x51: //�������� ������ �� �����������
        case 0x7F: //������� ������� ��������� ���
          default: //"online" (0x4F), ��� ����������� ��� �������

            if (time_wait_txtsms <= 1) return;

            IOStreamBuf.Out[0] = 0xC0; //"����� ��� ������"
            for (IOBufPointer=1,cnt=0; cnt<8; cnt++) IOStreamBuf.Out[IOBufPointer++] = mResult.c[cnt];
            *pFD = 0x3F;
            pOutBuffer = DataLogger; //�� ������ �������
            WriteOutputData(); //�������� ���� �����

            IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
            IOStreamState.Out |= (0xA400 | IOBufPointer); //���������� �������� �����

            StateRequest = 0xFF;
            return;

        case 0x7B: //"7-bit" (GPRS)
            if (Test_read_text(&IOStreamBuf.In[1])) // ��������� ��������� ���������
            {
               _f_SMS_noanswer=0;
                StateRequest = 0x41;
            }
            else
            {
                IOStreamState.In = 0x0000; //�����; ������ �������� ����� ������ �������
                StateRequest = 0xFF;
            }
        break;
    } //switch (StateRequest)
} //void automat_request()

void automat_alarm()
{
    switch (StateAlarm)
    {
        case 0x00: //������ ��������� �������

            if (StateSendSMS & 0x40) return; //� ������ ������ ������������ �������

            if (StateSendSMS & 0x04) //������� �������� - ������������� � ����� RA
            {
                StateSendSMS &= 0xFB;
                if (RA_Mode == 0)
                {
                    RA_Mode = 1;
                    RA_cnt += RA_tmp;
                    TimeRA = 0; //������ ������ ���������� ����� ��������� �������� ������
                    StateAlarm = 0x11;
                    return;
                }
            }

            if (Alarm_Enable)
            //� ������ RA, ��� ��������� ����������� ������ SMS ���������,
            //��������� ���������� ��� Alarm_Enable, ������� Lock_DataRAM � "0"
            if (RA_Mode) StateAlarm = 0x21; //��������� �������������� ���������� ������
            else StateAlarm = 0x01; //������� ����� - ������� ����� �������

            return;

        case 0x11:

            //�������� � Flash ����������� ���� RA_Mode, ����� RAA2, ������� RA_cnt
            //RA_Mode: Scratchpad FLASH Memory, ����� [05], �������� 0x55
            //RAA2   : Scratchpad FLASH Memory, ����� [06:07]
            //RA_cnt  : Scratchpad FLASH Memory, ����� [10]
            ReadScratchPad1();
            FlashDump[5] = 0x55;
            *(pInt = &FlashDump[6]) = RAA2;
            FlashDump[10] = RA_cnt;
            WriteScratchPad1();

            AlarmEvents.l.l1 = AlarmEvents.l.l2 = 0;
            Lock_DataRAM = Alarm_Enable = 0;
            StateAlarm = 0x00;
            return;

        case 0x21: //���������� ���������� ����������
        case 0x01: //--||--
            
            if (IOStreamState.Out & 0x4000) return; //� ������ ������ ��������� ������������
                                                    //��� ������ SMS � �� ������ ����������!
            //��������� ����� ������ ���������� (�� ���� ������ ��������� �������)
            saveIOBufPointer = IOBufPointer;
            saveIOStreamStateOut = IOStreamState.Out;
            saveSMS_Tail = *(pInt = SMS_Tail);
            *pInt = 0x0000;

            if (RestoreSearch = SearchEnable) //�������� �����; ��������� ��������� ������
            {
                saveRdHeader = RdHeader.l;
                SearchEnable = 0;
            }

            Auto_Enable = 0;
            //IOStreamState.Out = (gprs.b.abs_off ? 0x1000 : 0x1100); //���������� � ������
            IOStreamState.Out = 0x1000;

            StateAlarm++; //0x01->0x02; 0x21->0x22
            return;

        case 0x02: //��������� ���. ��������
            
            IOBufPointer = 0;

            //��������� ���������� ����� ��������� ������
            if (DataRAMPointer) pOutBuffer = &DataRAM[DataRAMPointer - (AlarmPointSize+8)];
            else pOutBuffer = &DataRAM[(AlarmPointSize+8)*(AMOUNT_DATARAM_POINTS-1)];

            pFD_Mask = FD_Alarm;
            AlarmPointNum = AmountAlarmPoints;

            IOStreamState.Out |= 0x1000; //"�������"

            StateAlarm = 0x03;
            return;

        case 0x03: //������ ����� ����� � ����� ������

            pEvents = pOutBuffer + AlarmPointSize;
            if (((*pEvents).l.l1==0x0000) && ((*pEvents).l.l2==0x0000))
            {
                CMask = 0;
                IOStreamBuf.Out[IOBufPointer++] = *pFD = GetFDMask(AlarmEvents);
                IOStreamBuf.Out[IOBufPointer++] = 0x40; //"��� �������"
            }
            else
            {
                CMask = 1;
                IOStreamBuf.Out[IOBufPointer++] = *pFD = GetFDMask(*pEvents);
                if (CMask)
                    for (cnt=0; cnt<CMask_cnt; cnt++)
                        IOStreamBuf.Out[IOBufPointer++] = CMask_buf[cnt];
                else
                {
                    IOStreamBuf.Out[IOBufPointer++] = 0xC0; //"����� ��� ������"
                    for (cnt=0; cnt<8; cnt++) IOStreamBuf.Out[IOBufPointer++] = (*pEvents).c[cnt];
                }
            }

            WriteOutputData();

            if (pOutBuffer == DataRAM) pOutBuffer = &DataRAM[(AlarmPointSize+8)*(AMOUNT_DATARAM_POINTS-1)];
            else pOutBuffer -= (AlarmPointSize+8);

            if ((--AlarmPointNum) && (IOBufPointer < SizeBufOut)) return;

            if (AlarmEvents.b.reset_cpu)
            {
                if (StK8.b.fTabMode) //���� ���������� �����, ����� � ��������� ����������
                                     // �� ��������� ���������! ������ ������� �����������
                                     // ������ 0x41
                {
                    _f_SMS_noanswer = 0;
                    io_who_send_sms = 1; //�������� �������� �� ����� DC1
                    IOStreamBuf.In[0] = 0x41;
                    IOStreamBuf.In[1] = 0x07;
                    IOStreamState.In = 0x8002;
                    IOStreamState.Out = 0x0000;
                }
                else
                {
                    //��������� ����� ������ � ������ � �������� �������� ����������:
                    if (IOBufPointer < SizeBufOut)
                    {
                        IOStreamBuf.Out[IOBufPointer++] = WrHeader.c[1];
                        IOStreamBuf.Out[IOBufPointer++] = WrHeader.c[2];
                        IOStreamBuf.Out[IOBufPointer++] = WrHeader.c[3];
                        SFRPAGE = LEGACY_PAGE;
                        IOStreamBuf.Out[IOBufPointer++] = RSTSRC; //Reset Source Register
                        IOStreamBuf.Out[IOBufPointer++] = reset_code; //Reset Code State
                        reset_code = 0x01; //�������, �� ���������� ���������� ��������
                    }
                    IOStreamState.Out |= (0x9400 | IOBufPointer); //������� ��������; �������� �����
                }
            }
            else
                IOStreamState.Out |= (0x9400 | IOBufPointer); //������� ��������; �������� �����

            AlarmEvents.l.l1 = AlarmEvents.l.l2 = 0;

            Lock_DataRAM = 0; //�������������� ����� �������
            
            RAA2 = RAA1;
            RA_cnt = RA_tmp;
            RA_tmp = 0;

            StateAlarm = 0xFF;
            return;

        case 0x22: //������ �� ������ �� �������

            IOBufPointer = 0;

            pFD_Mask = FD_Alarm;

            IOStreamState.Out |= 0x1800; //������������������ ������ ("RA")
            State__RA = 0x01; //����� ��������

            //...RA_Mode==1...

            StateAlarm = 0x23;
            return;

        case 0x23: //�����...

            //(������������ ����� ������ IOStreamBuf.Out)
            if (IOStreamState.Out & 0xC000) return;
            if (StateSendSMS & 0x10) return; //SMS � ������ ��������
            if (StateSendSMS & 0x01) //SMS ��������� �� �������
            {
                StateSendSMS &= 0xFE;
                RA_Mode = 1;
                TimeRA = 0;

                ReadScratchPad1();
                FlashDump[5] = 0x55;
                *(pInt = &FlashDump[6]) = RAA2;
                FlashDump[10] = RA_cnt;
                WriteScratchPad1();

                Lock_DataRAM = 0;
                State__RA = 0x00;
                StateAlarm = 0xFF;
                return;
            }
            automat__RA(); //�����...
            if (State__RA) IOStreamState.Out |= 0x1800; //����� �� ��������
            else //��� ��������� ����������
            { //��������� ����� RA � Flash
                ReadScratchPad1();
                FlashDump[5] = 0xAA;
                WriteScratchPad1();
    
                StateAlarm = 0xFF;
            }

            return;

        case 0xFF: //����������

            if (IOStreamState.Out & 0x8000) return; //���� �� ������ �����...

            //���������� ������ � DataRAM[] � ����������� �������, ���� �� ���� ����� �������
            Alarm_Enable = Lock_DataRAM;
            if (RestoreSearch) //��� ������� �����; ������������ ��������� ������
            {
                SearchEnable = 1;
                RdHeader.l = saveRdHeader;
                pFD_Mask = FD_Auto;
            }
            pOutBuffer = FlashDump;

            pEvents = &FlashDump[0x7B];
            IOBufPointer = saveIOBufPointer; //��������������� ������� ������ ����������
            IOStreamState.Out = saveIOStreamStateOut;
            *(pInt = SMS_Tail) = saveSMS_Tail;

            Auto_Enable = 1;

            StateAlarm = 0x00;
            return;

        default:
            StateAlarm = 0x00; //?
    }
}


//�������� ��������� LimitsData, ���� ������ ������������ ��������
void init_limits_data()
{

}

void OpenDeviceProfile() //������ �� flash � ��������� ���� ���������� ���������� ����������
//���� ������ �� ���� �������� � flash, ������������� �������� �� ���������
{
unsigned int xdata CheckSum;
unsigned int xdata * pCheckSum = &FlashDump[128];
//unsigned long xdata * pLong;
unsigned int xdata Int;
bit bWrPr2=0;

    //������ ������� ����� - ����� �������, LimitsData

    Char = 0; //������� ������ FlashDump

    ReadFlash(F_ADDR_SETUP, 130);  reset_wdt;

    for (CheckSum=0, cnt=0; cnt<128; cnt++) CheckSum += FlashDump[cnt];

    if (CheckSum == *pCheckSum) //��������� ���������, �������� Setup`��
    {
        for (cnt=0; cnt<m_SIZE; cnt++)  mLogger.c[cnt]       = FlashDump[Char++];
        for (cnt=0; cnt<m_SIZE; cnt++)  mAuto.c[cnt]         = FlashDump[Char++];
        for (cnt=0; cnt<m_SIZE; cnt++)  mAlarm.c[cnt]        = FlashDump[Char++];

        for (cnt=0; cnt<lim_SIZE; cnt++) LimitsData.All[cnt] = FlashDump[Char++];
    }
    else //��������� ��������� �� ���������
    {
        DefaultProfile = 1; //����������� � ������ ���������, ��� 3

        mLogger.l.l1 = 0x8103000F;
        mLogger.l.l2 = 0x00000000;
        mAuto.l.l1   = 0x00000000; //0x80030000
        mAuto.l.l2   = 0x00000000;
        mAlarm.l.l1  = 0x00000000; //0x01000000 0x0100000F
        mAlarm.l.l2  = 0x00000000;

        LimitsData.Each.tem_critical_hi_n = 0x1;
        LimitsData.Each.tem_critical_hi = 50;
        LimitsData.Each.tem_critical_lo_n = 0x1;
        LimitsData.Each.tem_critical_lo = -20;
        LimitsData.Each.power_main_lo_n = 0x1;
        LimitsData.Each.power_main_lo = 90;
        LimitsData.Each.beacon_time_base = 0;
        LimitsData.Each.beacon_time = 5;

        LimitsData.Each.sensor_an1_hi_n = 0x1;
        LimitsData.Each.sensor_an1_hi = 127;
        LimitsData.Each.sensor_an2_hi_n = 0x1;
        LimitsData.Each.sensor_an2_hi = 127;
        LimitsData.Each.sensor_an3_hi_n = 0x1;
        LimitsData.Each.sensor_an3_hi = 127;
        LimitsData.Each.sensor_an4_hi_n = 0x1;
        LimitsData.Each.sensor_an4_hi = 127;

        LimitsData.Each.sensor_an1_lo_n = 0x1;
        LimitsData.Each.sensor_an1_lo = 0;
        LimitsData.Each.sensor_an2_lo_n = 0x1;
        LimitsData.Each.sensor_an2_lo = 0;
        LimitsData.Each.sensor_an3_lo_n = 0x1;
        LimitsData.Each.sensor_an3_lo = 0;
        LimitsData.Each.sensor_an4_lo_n = 0x1;
        LimitsData.Each.sensor_an4_lo = 0;

        LimitsData.Each.sensor_dry1_n = 0x2;
        LimitsData.Each.sensor_dry2_n = 0x2;
        LimitsData.Each.sensor_dry3_n = 0x2;
        LimitsData.Each.sensor_dry4_n = 0x2;

        LimitsData.Each.sensor_dry1_x = 0;
        LimitsData.Each.sensor_dry2_x = 0;
        LimitsData.Each.sensor_dry3_x = 0;
        LimitsData.Each.sensor_dry4_x = 0;

        LimitsData.Each.tem_microlan_hi_n = 0x1;
        LimitsData.Each.tem_microlan_hi = 80;
        LimitsData.Each.tem_microlan_lo_n = 0x1;
        LimitsData.Each.tem_microlan_lo = -15;

        LimitsData.Each.guarding_on_n = 0x1;
        LimitsData.Each.guarding_off_n = 0x1;
    }

    reset_wdt;

    init_limits_data();

    //������ ����� ������� ������ �� Setup:

    Char = 0;

    ReadFlash(F_ADDR_SETUP+130, 130);  reset_wdt;

    for (CheckSum=0, cnt=0; cnt<128; cnt++) CheckSum += FlashDump[cnt];

    if (CheckSum == *pCheckSum) //��������� ���������, �������� Setup`��
        for (cnt=0; cnt<64; cnt++)
        {
            FD_Auto[cnt]  = FlashDump[Char++];
            FD_Alarm[cnt] = FlashDump[Char++];
        }
    else //��������� ��������� �� ���������
        for (cnt=0; cnt<64; cnt++) FD_Auto[cnt] = FD_Alarm[cnt] = 0x3F; //bits [0..5]


    //��������� ���������
    ReadFlash(F_ADDR_SETTINGS, 260); reset_wdt;

    pCheckSum = &FlashDump[258];
    for (CheckSum=0, Int=0; Int<258; Int++) CheckSum += FlashDump[Int];

    reset_wdt;
    // ****************************************
    // ��������� ���������, �������� Setup`�� 2
    // ****************************************
    if (CheckSum == *pCheckSum)
    {
        // *** password, pin ****************************
        //for (cnt=0; cnt<12; cnt++) S_Password[cnt] = FlashDump[cnt];
        //for (cnt=0; cnt<4;  cnt++) S_PinCode[cnt]  = FlashDump[cnt+12];
        // *** ������ ��� ��������� sms-������ **********
        len_pasw_sms=FlashDump[91]; if (len_pasw_sms>8) len_pasw_sms=0;
        for (cnt=0; cnt<len_pasw_sms; cnt++) pasw_sms[cnt]=FlashDump[92+cnt];
        // *** ����� ������� ��� ��������� sms-������ ***
        //pLong = &FlashDump[101]; mAlert1_for_tsms = *pLong;
        //pLong = &FlashDump[105]; mAlert2_for_tsms = *pLong;
    }
    // ********************************************
    // ��������� �� ��������� (��������� ���������)
    // ********************************************
    else
    {
        Param_default(0); bWrPr2=1;
    }

    Char = 0;
    // ***************************
    // MicroLan Device serial code
    // ***************************
    //1) ������� SN � ���������� MicroLan
    StateLAN = 0x90; //���������� ����������� �����������
    while (1)
    {
        if (VLAN) automat_vlan(); else automat_lan();     reset_wdt;
        if (StLAN.b.fReadySN) break;
    }
    //2) �������� SN � ����� � Flash uC
    if (StLAN.b.fFailSN==0) //SN ������
    {
        for (cnt=0; cnt<8; cnt++)
            if (context_LAN[cnt] != ID_RM[cnt])
            {
                for (Char=cnt; Char<8; Char++) FlashDump[18+Char] = context_LAN[Char];
                reset_wdt;
                bWrPr2=1; //�������� ����� SN � Flash uC
                break;
            }
    }
    //3) ��������� CRC ���� � Flash uC
    for (cnt=0; cnt<8; cnt++)
        if (ID_RM[cnt] != 0xFF) //��� � Flash uC ����
        {
            data_crc_8[0] = 0;
            for (Char=0; Char<7; Char++) crc_8_lan(ID_RM[Char]); //������ CRC
            if (data_crc_8[0] != ID_RM[7]) //��� �������
            {
                for (Char=cnt; Char<8; Char++) FlashDump[18+Char] = 0xFF;
                reset_wdt;
                bWrPr2=1; //�������� [FF FF FF FF FF FF FF FF] � Flash uC
            }
            break;
        }
    reset_wdt;

    if (bWrPr2)
    {
        // ��������� ����� ����������� �����.
        for (CheckSum=0, Int=0; Int<258; Int++) CheckSum += FlashDump[Int];
        *pCheckSum = CheckSum; // ����� ����������� �����.
        WriteFlash(F_ADDR_SETTINGS, 260);
    }

    //---
    NTicks = 2;
}

void reset_state_log()
{
    port_sensor_latch = 0x0000; //����� ����������� �����
    port_ADC_latch = 0x00; //����� ����������� ����� ��� ���
}




extern switchbank(unsigned char bank_number);


//������� ��������� �� ���������
typedef (*function)();
void call_program_reset()
{
  function call_reset = (function)0x0000;

    PSBANK = 0x11;
    call_reset();
}


void main (void)
{
    //��������� �������������____________________________________________________________

   // switchbank(0);

    config(); reset_wdt;

    #ifdef GuardJTAG
        LockJTAG();
    #endif

    GSM_OFF = 1; //�����  ������ �����, ���������� 1
    SENSOR1WR = 0xFF; //����� (��� ��������)
    LED = 1; //����������
    //LED2C_RED = LED2C_GREEN = 0; //����������
    GSM_PWR_ON;
    TEL_BUTTON = 1;
    GSM_RTS = 0;
    GSM_DTR = 0;
////////////
//memset(TimeHttp,0,sizeof(TimeHttp));
TimeHttp.Year = 2015;
TimeHttp.Mon = 10;
TimeHttp.Day = 11;
TimeHttp.Sec = 0;
TimeHttp.Min = 0;
TimeHttp.Hour = 0;
////////////
    if(TAB_DET==0) StK8.b.fTabMode = 1; //���������� �����

    //reset AtFlash chip (min 10 us)
    RES_AT45 = 0;

    Init_uart0();
    Init_uart1(); reset_wdt;

    init_k8();
    
    #ifndef DEBUG_GPRS_MAX
     Init_max(0x0D); //set 1200 b/s
    #endif

    OpenDeviceProfile(); reset_wdt; //��������� ���������� ���������� ����������

    Beacon_timer = LimitsData.Each.beacon_time;

    Init_io(); reset_wdt;

    //reset AtFlash chip (min 10 us)
    RES_AT45 = 1;

    initialize_sensor(); //����. ���������
    initialize_rele();   //����. ����

    InitAF(); reset_wdt;

    reset_wdt;

    init_lan();

    for (Int=0; Int<(AlarmPointSize+8)*AMOUNT_DATARAM_POINTS; Int++) DataRAM[Int] = 0xFF;

    reset_wdt;

    #ifndef DEBUG_GPRS_MAX
     Init_max(0x0D); //set 1200 b/s
    #endif

    #ifdef DEBUG_GPRS_MAX
     debug_gprs_init(); //DEBUG GPRS
    #endif


    //���������� ��������� ��������� � Scratchpad FLASH

    //������ �� FLASH ���� ��������� ��������� ��� ������
    //����������� - Scratchpad Memory, ����� [00]
    //������ �������� ��������� CPU
    //����������� - Scratchpad Memory, ����� [01]
    ReadScratchPad1();

    _flag_secbut_on = (FlashDump[0] == 0x55);
    //������� ���� ������� �������������� ������, ��������� ����� �������, ������� ������
    if (FlashDump[5] == 0x55)
    {
        RA_Mode = 1;
        RAA2 = *(pInt = &FlashDump[6]);
        RA_cnt = FlashDump[10];
    }

    count_restart_cpu = FlashDump[1];
    FlashDump[1] = ++count_restart_cpu;

    WriteScratchPad1();




    while (1) //����������� ������� ����
    {
       reset_wdt;

       //AUTOMAT_IO____________________________________________________________
       OutBufChange = IOStreamState.Out & 0x8000; //����������, �������� �� ������� ������ � ������
       Automat_io(); reset_wdt; //�������� ���������� �� ������� ����� GSM
       automat_rmodem(); reset_wdt; //�������� ���������� �� �����������
       if (OutBufChange ^ (IOStreamState.Out & 0x8000)) //����� ���������
           IOBufPointer = IOStreamState.Out & 0x00FF; //��������������� ����� ������ ������

       if ((GSM_signal = Get_quality_signal()) == 0xFF) GSM_signal = GSM_signal_prev;

       //������������ ������� �� ��������� ������ �������:
       if (GSM_signal != GSM_signal_prev) //0xFF - �����������
       {
           switch (GSM_signal)
           {
               case 0:
               case 1:
                   mResult.b.level_gsm_lo = 1;
                   break;
               case 2:
               case 3:
               case 4:
                   mResult.b.level_gsm_norm = 1;
                   break;
           }

           GSM_signal_prev = GSM_signal;
       }

       //��������� ������� GSM
       DataLogger[0x04] = (GSM_signal & 0x07) << 4; //0x07 == �����. ������� GSM


       send_max(); //������� �������� ������ �� MAX


        //AUTOMAT_���__________________________________________________________
        StartConvertADC();
        if (FinishConvertADC()) //������ ����������...
        {
            if (ADCBuffer[7]<=10) ADCBuffer[7] = 0;
            Float1 = Kp * ADCBuffer[7];

            if (Float1 < 2.4) Char = 1; //1 - error
            else
            {
                Char = 0;
                Float = 12.0 / Float1; //����. ��� 1-4 ������� ���
            }

            //������ � DataLogger

            if ((port_ADC_latch & 0x01)==0)
            {
                /*if (!(LimitsData.Each.sensor_an1_hi_n & 0x08)) //�������������� �� ��������� �������
                {
                    if (Char) DataLogger[0x1D] = 0x00;
                    else
                    {
                        Int = Float*ADCBuffer[0];
                        DataLogger[0x1D] = Int>0x7F ? 0x7F : Int;
                    }
                }
                else*/
                    DataLogger[0x06] = ADCBuffer[0];
            }
            if ((port_ADC_latch & 0x02)==0)
            {
                /*if (!(LimitsData.Each.sensor_an2_hi_n & 0x08)) //�������������� �� ��������� �������
                {
                    if (Char) DataLogger[0x1E] = 0x00;
                    else
                    {
                        Int = Float*ADCBuffer[1];
                        DataLogger[0x1E] = Int>0x7F) ? 0x7F : Int;
                    }
                }
                else*/
                    DataLogger[0x07] = ADCBuffer[1];
            }
            if ((port_ADC_latch & 0x04)==0)
            {
                /*if (!(LimitsData.Each.sensor_an3_hi_n & 0x08)) //�������������� �� ��������� �������
                {
                    if (Char) DataLogger[0x1F] = 0x00;
                    else
                    {
                        Int = Float*ADCBuffer[2];
                        DataLogger[0x1F] = Int>0x7F ? 0x7F : Int;
                    }
                }
                else*/
                    DataLogger[0x08] = ADCBuffer[2];
            }
            if ((port_ADC_latch & 0x08)==0)
            {
                /*if (!(LimitsData.Each.sensor_an4_hi_n & 0x08)) //�������������� �� ��������� �������
                {
                    if (Char) DataLogger[0x20] = 0x00;
                    else
                    {
                        Int = Float*ADCBuffer[3];
                        DataLogger[0x20] = Int>0x7F ? 0x7F : Int;
                    }
                }
                else*/
                    DataLogger[0x09] = ADCBuffer[3];
            }

            if ((port_ADC_latch & 0x10)==0) DataLogger[0x0B] = Therm;

            //�������� �������
            if (Float1 > 25.5) DataLogger[0x0A] = 255; else DataLogger[0x0A] = 10*Float1 ;
            if (DataLogger[0x0A] < 10) DataLogger[0x0A] = 0;

/*
            //������� ������������ � CurrentEvents.b.sensor_anX_Y;
            //���� ��������, �������� � Setup, ���������, ��� �� ������� ������ ������� ��
            //��������������, �� ��� ������������ ������������ � PreviousEvents.b.sensor_anX_Y
            // *********************************
            // **   ��� 1 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an1_hi_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an1_hi_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1D] >= LimitsData.Each.sensor_an1_hi - CurrentEvents.b.sensor_an1_hi * HystADC)
                {
                    CurrentEvents.b.sensor_an1_hi = 1;
                    if (!PreviousEvents.b.sensor_an1_hi)
                    {
                        PreviousEvents.b.sensor_an1_hi = !(LimitsData.Each.sensor_an1_hi_n & 0x01);
                        if ((LimitsData.Each.sensor_an1_hi_n&0x01)&&
                            (mLogger.b.sensor_an1_hi))
                             port_ADC_latch |= 0x01;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an1_hi = 0;
                    if (PreviousEvents.b.sensor_an1_hi)
                    {
                        PreviousEvents.b.sensor_an1_hi = (bit)(LimitsData.Each.sensor_an1_hi_n & 0x02);
                        if ((LimitsData.Each.sensor_an1_hi_n&0x02)&&
                            (mLogger.b.sensor_an1_hi))
                             port_ADC_latch |= 0x01;
                    }
                }
            }
            // *********************************
            // **   ��� 1 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an1_lo_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an1_lo_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1D] <= LimitsData.Each.sensor_an1_lo + CurrentEvents.b.sensor_an1_lo * HystADC)
                {
                    CurrentEvents.b.sensor_an1_lo = 1;
                    if (!PreviousEvents.b.sensor_an1_lo)
                    {
                        PreviousEvents.b.sensor_an1_lo = !(LimitsData.Each.sensor_an1_lo_n & 0x01);
                        if ((LimitsData.Each.sensor_an1_lo_n&0x01)&&
                            (mLogger.b.sensor_an1_lo))
                             port_ADC_latch |= 0x01;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an1_lo = 0;
                    if (PreviousEvents.b.sensor_an1_lo)
                    {
                        PreviousEvents.b.sensor_an1_lo = (bit)(LimitsData.Each.sensor_an1_lo_n & 0x02);
                        if ((LimitsData.Each.sensor_an1_lo_n&0x02)&&
                            (mLogger.b.sensor_an1_lo))
                             port_ADC_latch |= 0x01;
                    }
                }
            }
            // *********************************
            // **   ��� 2 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an2_hi_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an2_hi_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1E] >= LimitsData.Each.sensor_an2_hi - CurrentEvents.b.sensor_an2_hi * HystADC)
                {
                    CurrentEvents.b.sensor_an2_hi = 1;
                    if (!PreviousEvents.b.sensor_an2_hi)
                    {
                        PreviousEvents.b.sensor_an2_hi = !(LimitsData.Each.sensor_an2_hi_n & 0x01);
                        if ((LimitsData.Each.sensor_an2_hi_n&0x01)&&
                            (mLogger.b.sensor_an2_hi))
                             port_ADC_latch |= 0x02;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an2_hi = 0;
                    if (PreviousEvents.b.sensor_an2_hi)
                    {
                        PreviousEvents.b.sensor_an2_hi = (bit)(LimitsData.Each.sensor_an2_hi_n & 0x02);
                        if ((LimitsData.Each.sensor_an2_hi_n&0x02)&&
                            (mLogger.b.sensor_an2_hi))
                             port_ADC_latch |= 0x02;
                    }
                }
            }
            // *********************************
            // **   ��� 2 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an2_lo_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an2_lo_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1E] <= LimitsData.Each.sensor_an2_lo + CurrentEvents.b.sensor_an2_lo * HystADC)
                {
                    CurrentEvents.b.sensor_an2_lo = 1;
                    if (!PreviousEvents.b.sensor_an2_lo)
                    {
                        PreviousEvents.b.sensor_an2_lo = !(LimitsData.Each.sensor_an2_lo_n & 0x01);
                        if ((LimitsData.Each.sensor_an2_lo_n&0x01)&&
                            (mLogger.b.sensor_an2_lo))
                             port_ADC_latch |= 0x02;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an2_lo = 0;
                    if (PreviousEvents.b.sensor_an2_lo)
                    {
                        PreviousEvents.b.sensor_an2_lo = (bit)(LimitsData.Each.sensor_an2_lo_n & 0x02);
                        if ((LimitsData.Each.sensor_an2_lo_n&0x02)&&
                            (mLogger.b.sensor_an2_lo))
                             port_ADC_latch |= 0x02;
                    }
                }
            }
            // *********************************
            // **   ��� 3 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an3_hi_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an3_hi_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1F] >= LimitsData.Each.sensor_an3_hi - CurrentEvents.b.sensor_an3_hi * HystADC)
                {
                    CurrentEvents.b.sensor_an3_hi = 1;
                    if (!PreviousEvents.b.sensor_an3_hi)
                    {
                        PreviousEvents.b.sensor_an3_hi = !(LimitsData.Each.sensor_an3_hi_n & 0x01);
                        if ((LimitsData.Each.sensor_an3_hi_n&0x01)&&
                            (mLogger.b.sensor_an3_hi))
                             port_ADC_latch |= 0x04;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an3_hi = 0;
                    if (PreviousEvents.b.sensor_an3_hi)
                    {
                        PreviousEvents.b.sensor_an3_hi = (bit)(LimitsData.Each.sensor_an3_hi_n & 0x02);
                        if ((LimitsData.Each.sensor_an3_hi_n&0x02)&&
                            (mLogger.b.sensor_an3_hi))
                             port_ADC_latch |= 0x04;
                    }
                }
            }
            // *********************************
            // **   ��� 3 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an3_lo_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an3_lo_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x1F] <= LimitsData.Each.sensor_an3_lo + CurrentEvents.b.sensor_an3_lo * HystADC)
                {
                    CurrentEvents.b.sensor_an3_lo = 1;
                    if (!PreviousEvents.b.sensor_an3_lo)
                    {
                        PreviousEvents.b.sensor_an3_lo = !(LimitsData.Each.sensor_an3_lo_n & 0x01);
                        if ((LimitsData.Each.sensor_an3_lo_n&0x01)&&
                            (mLogger.b.sensor_an3_lo))
                             port_ADC_latch |= 0x04;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an3_lo = 0;
                    if (PreviousEvents.b.sensor_an3_lo)
                    {
                        PreviousEvents.b.sensor_an3_lo = (bit)(LimitsData.Each.sensor_an3_lo_n & 0x02);
                        if ((LimitsData.Each.sensor_an3_lo_n&0x02)&&
                            (mLogger.b.sensor_an3_lo))
                             port_ADC_latch |= 0x04;
                    }
                }
            }
            // *********************************
            // **   ��� 4 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an4_hi_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an4_hi_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x20] >= LimitsData.Each.sensor_an4_hi - CurrentEvents.b.sensor_an4_hi * HystADC)
                {
                    CurrentEvents.b.sensor_an4_hi = 1;
                    if (!PreviousEvents.b.sensor_an4_hi)
                    {
                        PreviousEvents.b.sensor_an4_hi = !(LimitsData.Each.sensor_an4_hi_n & 0x01);
                        if ((LimitsData.Each.sensor_an4_hi_n&0x01)&&
                            (mLogger.b.sensor_an4_hi))
                             port_ADC_latch |= 0x08;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an4_hi = 0;
                    if (PreviousEvents.b.sensor_an4_hi)
                    {
                        PreviousEvents.b.sensor_an4_hi = (bit)(LimitsData.Each.sensor_an4_hi_n & 0x02);
                        if ((LimitsData.Each.sensor_an4_hi_n&0x02)&&
                            (mLogger.b.sensor_an4_hi))
                             port_ADC_latch |= 0x08;
                    }
                }
            }
            // *********************************
            // **   ��� 4 ������ ����� (1-4)  **
            // *********************************
            if (((LimitsData.Each.sensor_an4_lo_n & 4) && // ����������� ���� ����� ��� �������
                 (_flag_secbut_on)  &&  // ����� ��� �������
                 (_PowOnOS_wait2min)) ||  // �� ��������
                 ((LimitsData.Each.sensor_an4_lo_n & 4)==0)) // ��� ����������� � ����� ������
            {
                if (DataLogger[0x20] <= LimitsData.Each.sensor_an4_lo + CurrentEvents.b.sensor_an4_lo * HystADC)
                {
                    CurrentEvents.b.sensor_an4_lo = 1;
                    if (!PreviousEvents.b.sensor_an4_lo)
                    {
                        PreviousEvents.b.sensor_an4_lo = !(LimitsData.Each.sensor_an4_lo_n & 0x01);
                        if ((LimitsData.Each.sensor_an4_lo_n&0x01)&&
                            (mLogger.b.sensor_an4_lo))
                             port_ADC_latch |= 0x08;
                    }
                }
                else
                {
                    CurrentEvents.b.sensor_an4_lo = 0;
                    if (PreviousEvents.b.sensor_an4_lo)
                    {
                        PreviousEvents.b.sensor_an4_lo = (bit)(LimitsData.Each.sensor_an4_lo_n & 0x02);
                        if ((LimitsData.Each.sensor_an4_lo_n&0x02)&&
                            (mLogger.b.sensor_an4_lo))
                             port_ADC_latch |= 0x08;
                    }
                }
            }*/
reset_wdt;
            //���� ���������� ���� ������������, ����� "1", ����� - "0"
            //pow  21 �
            if (DataLogger[0x0A] <= LimitsData.Each.power_main_lo + CurrentEvents.b.power_main_lo * HystVolt)
            {
                CurrentEvents.b.power_main_lo = 1;
                if (!PreviousEvents.b.power_main_lo)
                    PreviousEvents.b.power_main_lo = !(LimitsData.Each.power_main_lo_n & 0x01);
            }
            else
            {
                CurrentEvents.b.power_main_lo = 0;
                if (PreviousEvents.b.power_main_lo)
                    PreviousEvents.b.power_main_lo = (bit)(LimitsData.Each.power_main_lo_n & 0x02);
            }
/*
            ///LED2C
            if (DataLogger[0x23] < 25) //��� ���.
            {
                if ((cLED2 & 0x07) != 0x02) cLED2 &= ~0x07;
                cLED2 |= 0x01;
                if ((cLED2 & 0x70) == 0x10) cLED2 &= ~0x10;
            }
            else
            {
                if (DataLogger[0x1F] < 30) //�.�. ��� SIM
                {
                    cLED2 &= ~0x70;
                    cLED2 |= 0x10;
                }
                else
                    if ((cLED2 & 0x70) == 0x10) cLED2 &= ~0x10;
            }*/
            //----------
            if (ADCBuffer[5] >= 50) //�.�. ���� USB
                StSYSTEM |= 0x0001;
            else
                StSYSTEM &= ~0x0001;

            // *****************************************
            // * ����������� ���� � ������ ����������� *
            // *****************************************
            if ((signed char)DataLogger[0x0B] >= LimitsData.Each.tem_critical_hi - CurrentEvents.b.tem_critical_hi * HystTherm)
            {
                CurrentEvents.b.tem_critical_hi = 1;
                if (!PreviousEvents.b.tem_critical_hi)
                {
                    PreviousEvents.b.tem_critical_hi = !(LimitsData.Each.tem_critical_hi_n & 0x01);
                    if ((LimitsData.Each.tem_critical_hi_n & 0x01)&&
                        (mLogger.b.tem_critical_hi))
                         port_ADC_latch |= 0x10;
                }
            }
            else
            {
                CurrentEvents.b.tem_critical_hi = 0;
                if (PreviousEvents.b.tem_critical_hi)
                {
                    PreviousEvents.b.tem_critical_hi = (bit)(LimitsData.Each.tem_critical_hi_n & 0x02);
                    if ((LimitsData.Each.tem_critical_hi_n & 0x02)&&
                        (mLogger.b.tem_critical_hi))
                         port_ADC_latch |= 0x10;
                }
            }
            if ((signed char)DataLogger[0x0B] <= LimitsData.Each.tem_critical_lo + CurrentEvents.b.tem_critical_lo * HystTherm)
            {
                CurrentEvents.b.tem_critical_lo = 1;
                if (!PreviousEvents.b.tem_critical_lo)
                {
                    PreviousEvents.b.tem_critical_lo = !(LimitsData.Each.tem_critical_lo_n & 0x01);
                    if ((LimitsData.Each.tem_critical_lo_n & 0x01)&&
                        (mLogger.b.tem_critical_lo))
                         port_ADC_latch |= 0x10;
                }
            }
            else
            {
                CurrentEvents.b.tem_critical_lo = 0;
                if (PreviousEvents.b.tem_critical_lo)
                {
                    PreviousEvents.b.tem_critical_lo = (bit)(LimitsData.Each.tem_critical_lo_n & 0x02);
                    if ((LimitsData.Each.tem_critical_lo_n & 0x02)&&
                        (mLogger.b.tem_critical_lo))
                         port_ADC_latch |= 0x10;
                }
            }
        } //if (FinishConvertADC())

        reset_wdt;

        //AUTOMAT_MICROLAN_____________________________________________________
        if (VLAN) automat_vlan(); else automat_lan();     reset_wdt;
        //����������� MicroLan � ������ �����������
        if (Lan_temper != 0x7F) //��� ��������� ������ MicroLan
        {
         if (Lan_temper >= LimitsData.Each.tem_microlan_hi - (CurrentEvents.b.tem_microlan_hi * HystTherm))
         {
             CurrentEvents.b.tem_microlan_hi = 1;
             if (!PreviousEvents.b.tem_microlan_hi)
                 PreviousEvents.b.tem_microlan_hi = !(LimitsData.Each.tem_microlan_hi_n & 0x01);
         }
         else
         {
             CurrentEvents.b.tem_microlan_hi = 0;
             if (PreviousEvents.b.tem_microlan_hi)
                 PreviousEvents.b.tem_microlan_hi = (bit)(LimitsData.Each.tem_microlan_hi_n & 0x02);
         }
         if (Lan_temper <= LimitsData.Each.tem_microlan_lo + (CurrentEvents.b.tem_microlan_lo * HystTherm))
         {
             CurrentEvents.b.tem_microlan_lo = 1;
             if (!PreviousEvents.b.tem_microlan_lo)
                 PreviousEvents.b.tem_microlan_lo = !(LimitsData.Each.tem_microlan_lo_n & 0x01);
         }
         else
         {
             CurrentEvents.b.tem_microlan_lo = 0;
             if (PreviousEvents.b.tem_microlan_lo)
                 PreviousEvents.b.tem_microlan_lo = (bit)(LimitsData.Each.tem_microlan_lo_n & 0x02);
         }
       } //if (Lan_temper != 0x7F)

       //AUTOMAT_IO_DOUBLE_2___________________________________________________
       
	   OutBufChange = IOStreamState.Out & 0x8000; //����������, �������� �� ������� ������ � ������
       Automat_io(); reset_wdt; //�������� ���������� �� ������� ����� GSM
       automat_rmodem(); reset_wdt; //�������� ���������� �� �����������
       
	   if(OutBufChange ^ (IOStreamState.Out & 0x8000)) //����� ���������
           IOBufPointer = IOStreamState.Out & 0x00FF; //��������������� ����� ������ ������

        //AUTOMAT_SENSOR_______________________________________________________
        automat_sensor();   reset_wdt; //������ � ���������

        //������� ������ � ��������������� ������������________________________
        automat_rele();     reset_wdt;

        //GPRS_________________________________________________________________
        OutBufChange = 0;
        automat_gprs_glob(); reset_wdt;
        if (OutBufChange) IOBufPointer = IOStreamState.Out & 0x00FF; //��������������� ����� ������ ������

        //�����_����������_����������__________________________________________
        automat_k8();       reset_wdt;

        //��������� ������� �����������________________________________________
        automat_leds();

        //������_�����_________________________________________________________
        if (!ForbiddenAll && (Day_timer >= 1440)) //00:00 - ����� GPS
        //����� ������ Day_timer
        {
            Day_timer = 0;
            Beacon_Enable = 0;
            //��������� �������� ��������� CPU
            count_restart_cpu = 0;
        }
        if (Beacon_Enable) //���� �������
        {
            if (Beacon_timer >= LimitsData.Each.beacon_time)
            {
                if (!ForbiddenAll) //����� ���������� - ����������� ������� �����
                {
                    mResult.b.beacon_time = 1;
                    Beacon_timer = 0;
                }
                else //����� ���������� - ����������� ������ ������ ONLINE
                if (PreviousEvents.b.reset_cpu && !(IOStreamState.In & 0xC000)) //����� �� �����, ����� �������� CPU
                {
                    IOStreamBuf.In[0] = 0x4F;
                    IOStreamState.In = 0x8101;
                    Beacon_timer = 0;
                }
            }
        }
        else //�� ��������� �����
        if (Beacon_timer)
        {
            if (Day_timer >= LimitsData.Each.beacon_time_base)
            {
                if (Day_timer == LimitsData.Each.beacon_time_base)
                    mResult.b.beacon_time = 1;
                Beacon_Enable = 1; //���. ������ ���������� ������� �����
                Beacon_timer = (Day_timer - LimitsData.Each.beacon_time_base) % LimitsData.Each.beacon_time;
            }
            else
                Beacon_timer = 0;
        }


        send_max(); //������� �������� ������ �� MAX


        //���������� ������ ���������
        DataLogger[0x04] |= ((unsigned char)_flag_secbut_on)        | //��� �������?
                          //((unsigned char)                  << 1) | //
                          //((unsigned char)                  << 2) | //
                            ((unsigned char)DefaultProfile    << 3) ; //������� "�� ���������"

        //������� ��������������  � � � � � � � � � �
        mResult.l.l1 |= (CurrentEvents.l.l1) ^ (PreviousEvents.l.l1);
        mResult.l.l2 |= (CurrentEvents.l.l2) ^ (PreviousEvents.l.l2);


      if (Timer_tick)
      {
        Timer_tick = 0;


        //��������� ������� �����
        pChar = &Timer_inside;
       int_timer2_no;
        for (cnt=0; cnt<4; cnt++) DataLogger[cnt] = pChar[cnt];
       int_timer2_yes;

        if (CurrentEvents.b.reset_cpu)
        {
        //���������� ������� �� ������� ~~��� � ���
        AddAlarmEvents.l.l1 |= (mResult.l.l1) & (mAlarm.l.l1);
        AddAlarmEvents.l.l2 |= (mResult.l.l2) & (mAlarm.l.l2);


        //���������� ������� ��������� ����� ���������, ����
        DataLogger[0x05] = (port_sensor_current & 0x0F) | get_sost_rele();


        //������ � ��������� ����� � ����������� ������� �� �������
        if (StateAlarm != 0x03) //��� �������� ������ DataRAM[] � IOStreamBuf.Out[]
        {
            if ((AddAlarmEvents.l.l1) || (AddAlarmEvents.l.l2)) //��������� �������
            {
                if (RA_Mode) //��� ����� GSM - ������� �� ����������
                {
                    RA_cnt++;
                    if (!Alarm_Enable) //������ �� �������������� �������������� �������
                    {
                        if (RA_cnt >= RA_cnt_MAX) //��������� ����. ���������� ���. �������������� ������
                        {
                            RAA2 = RAA3;
                            RA_cnt = RA_cnt_MIN; //���. ����������� ���. �������������� ������
                            StateAlarm = 0x11; //�������� � ScratchPad flash
                        }
                        if (RA_cnt == RA_cnt_MIN) //��������� ����� (� �������) ������� �������
                            RAA3 = (WrHeader.t.Page << 2) | (WrHeader.t.Byte >> 6); //(��� DataFlash 4 MBit)
                    }
                }
                else
                {
                    AlarmEvents.l.l1 |= AddAlarmEvents.l.l1; //�������� ����� � �������
                    AlarmEvents.l.l2 |= AddAlarmEvents.l.l2; //---//---//

                    if (Lock_DataRAM == 0) //������ ������� � ������
                    {
                        //��������� ����� (� �������), ���� ��������� ������ ������� (��� DataFlash 4 MBit)
                        RAA1 = (WrHeader.t.Page << 2) | (WrHeader.t.Byte >> 6);
                        RA_tmp = 1; //������� 1 �������������� �������
                    }
                    else RA_tmp++; //����������� ���. �������������� ������
                    Alarm_Enable = Lock_DataRAM = 1;
                }
                TickCounter = NTicks; //���������� �������� � ������ � �������� mResult
                //�������� � ������ ������� ������� (DataLogger[0x7E], ��� 7)
                DataLogger[0x04] |= 0x80;
                AlarmTickCounter = NAlarmTicks; //���������� �������� ��������� �����
            }
            else //�� ���� ������� �� �������
                if (Lock_DataRAM) AlarmTickCounter = 0; //����� ������������; ���������� ������

            if (AlarmTickCounter++ >= NAlarmTicks)
            {
                AlarmTickCounter = 1;

                for (cnt=0; cnt<AlarmPointSize; cnt++) DataRAM[DataRAMPointer++] = DataLogger[cnt];
                for (cnt=0; cnt<8; cnt++) DataRAM[DataRAMPointer++] = AddAlarmEvents.c[cnt];
                if (DataRAMPointer == (AlarmPointSize+8)*AMOUNT_DATARAM_POINTS) DataRAMPointer = 0;

                //��� ���������/�������� ���������� AmountAlarmPoints = 0
                if (AmountAlarmPoints < AMOUNT_DATARAM_POINTS) AmountAlarmPoints++;

                AddAlarmEvents.l.l1 = AddAlarmEvents.l.l2 = 0;
            }
        }

        reset_wdt;

        //������_�_�������_���������_�������_-_DataLogger[]____________________
        if (++TickCounter >= NTicks) //��������� ����������� ����� �������� �������,
                                     //������� ~~ 1 c��
        {
            TickCounter = 0;

            //* XXXXXX11 - ������ ����� �������� 1, 2 (KOD8)
            startK8 = 0x03;


            if (GetBack_m) //��� ��������� ������� GPS ��� ����� �������� ������ � EEPROM
            {
                ForbiddenAll = GetBack_m = 0;
                mResult.l.l1 |= mStore.l.l1; //�������������� ������� ��� ������ � DataLogger
                mResult.l.l2 |= mStore.l.l2; //--||--
                mStore.l.l2 = mStore.l.l1 = 0;
            } //if (GetBack_m)

            mResult.l.l1 &= mLogger.l.l1; //���������������� ������� ���������
            mResult.l.l2 &= mLogger.l.l2;

/*!!!*      if ((mResult.l.l1 & mAlert1_for_tsms) || // ��������� ������� ��� ��������� sms-�������.
                (mResult.l.l2 & mAlert2_for_tsms))
            {
                alert1_for_tsms |= mResult.l.l1 & mAlert1_for_tsms;
                alert2_for_tsms |= mResult.l.l2 & mAlert2_for_tsms;
            }
*/
            if (ForbiddenAll) //����� GPS ��� �� ��������
            {
                mStore.l.l1 |= mResult.l.l1; //��������� �������
                mStore.l.l2 |= mResult.l.l2; //--||--
            }
            else //����� GPS ����������; ��������� ����������� �������:____________
            if ((mResult.l.l1) || (mResult.l.l2)) //��������� �������
            {

                //���������� �����-��������� ������ � DataLogger
                for (cnt=0; cnt<4; cnt++)
                    DataLogger[0x7B + cnt] = mResult.c[cnt];

                //���������� ����������� �����
                for (Char=0,cnt=0; cnt<0x7F; cnt++) Char += DataLogger[cnt];
                DataLogger[0x7F] = Char ^ mLogCS;

                //������ � ����������������� ������ (������):
                BufAF = DataLogger; //��������
                WriteAF(); //�������...

                reset_wdt;

                //�������� ������� �������
                DataLogger[0x04] &= ~0x80;

                //������ �� ��������� CountAuto
                if (((mResult.l.l1) & (mAuto.l.l1)) || ((mResult.l.l2) & (mAuto.l.l2))) CountAuto++;

            } //if ((mResult.l.l1) || (mResult.l.l2))

            //����������� �����-���������
            PreviousEvents.l.l1 = CurrentEvents.l.l1;
            PreviousEvents.l.l2 = CurrentEvents.l.l2;

            //����� ������� ����� �������
            mResult.l.l1 = mResult.l.l2 = 0;

            //�������� ��������� (����������) ���������
            reset_state_log();

        } //if (++TickCounter >= NTicks)
        } //if (CurrentEvents.b.reset_cpu)
        else
            reset_state_log();
      } //if (Timer_tick)

        reset_wdt;

        //������� ������ ������ � �������______________________________________
        if (SearchEnable) automat_search();             reset_wdt;

        if (RM_Enable) automat_request(); //������ �� �����������
        else
        {
        //�������� ������� � ������� ����������� ������ ������������___________
        if (StateRequest == 0x00) automat_alarm();
        if (StateAlarm == 0x00) automat_request();

        reset_wdt;

        //START_AUTO_OUTPUT____________________________________________________
        if (Auto_Enable)
        {
        IOStreamState.Out |= 0x0800; //���������� � ������

        if (CountAuto && !SearchEnable) //���� �����, ����. �����
        { //������� ��������� CountAuto ����� �� ������� � �������� � ����� ������
            Search1_Time = 0L;
            Search2_Time = 0xFFFFFFFF;
            MaskEvents.l.l1 = mAuto.l.l1;
            MaskEvents.l.l2 = mAuto.l.l2;
            AmountSearchPoints = CountAuto;
            CountSearchPoints = 0;

            pFD_Mask = FD_Auto;

            SearchEnable = 1;
            StateSearch = 0x00;
            CountAuto = 0;

            automat_search(); //set read addr immediately
        } //if (CountAuto && !SearchEnable)
        } //if (Auto_Enable)
        } //if (RM_Enable)...else
    } //while (1)
} //void main (void)
