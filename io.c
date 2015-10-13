// ***********************************************************
// *                                                         *
// *  IO.C                                                   *
// *  ������������� ������ ��� ������ � ����� ������� �����  *
// *                                                         *
// ***********************************************************

#include <c8051F120.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#include "io.h"
#include "rele.h"
#include "io_sms.h"
#include "atflash.h"
#include "sensor.h"
#include "adc.h"
#include "kod8.h"
#include "gprs\gprs.h"

// ************************************************************************************************
//
// ************************************
// *            ����������            *
// ************************************
//

#define LEN_PACK   140
#define LEN_HEADER   5

unsigned char xdata npack_31_inc = 0; // ������������� ������
unsigned char xdata npack_32_inc = 0; // ������������� ������
unsigned char xdata npack_33_inc = 0; // ������������� ������
unsigned char xdata npack_34_inc = 0; // ������������� ������
unsigned char xdata npack_35_inc = 0; // ������������� ������
unsigned char xdata npack_36_inc = 0; // ������������� ������
unsigned char xdata npack_37_inc = 0; // ������������� ������
unsigned char xdata npack_39_inc = 0; // ������������� ������
unsigned char xdata npack_3B_inc = 0; // ������������� ������
unsigned char xdata npack_3D_inc = 0; // ������������� ������
unsigned char xdata npack_3F_inc = 0; // ������������� ������
unsigned char xdata npack_43_inc = 0; // ������������� ������
unsigned char xdata npack_45_inc = 0; // ������������� ������
unsigned char xdata npack_47_inc = 0; // ������������� ������
unsigned char xdata npack_49_inc = 0; // ������������� ������
unsigned char xdata npack_4B_inc = 0; // ������������� ������

bit _flag_need_do_restart=no; // ���������� ������� �������
bit _flag_send_p_last_pack;   // ���������� ������������� ����� (���� last ������ sms)
bit _flag_send_last_pack;     // ���������� ��������� �����
bit _flag_have_tail;          // �������� �����
bit _online=0;                // ����� online
bit _flag_need_to_loader=no;  // ���������� ������� �� ���������

//unsigned char xdata Channel='S'; // �� ��������� ������ ����� SMS

unsigned char xdata state_io_wk;

unsigned char xdata *pBuf;
unsigned int  xdata Len_buf;  // ����� ������ ��������
unsigned int  xdata count_all_buf; // ������� ������ ��������
unsigned char xdata count_tek_buf; // ������� ��������(�������������) ������
unsigned char xdata io_buf[141]; // ����� ��� ������ ����������
unsigned char xdata * pBuf_read_data; // ��������� �� ����� ��� ����� ����������
unsigned char xdata len_read_data;

// ��� ������������� ������ (1-�������/2-������/3-�������/4-���������)
unsigned char xdata TypeMesOut=0;

unsigned char xdata io_who_send_sms=0;    // �� ���� ������ SMS - ������ ������ (1-9)

unsigned char xdata pasw_sms[8];  // ������ ��� sms ������������
unsigned char xdata len_pasw_sms; // ����� ������ ��� sms ������������

//unsigned long xdata mAlert1_for_tsms; //0x21200020; // ����� ������� ��� ��������� sms-������.
//unsigned long xdata mAlert2_for_tsms; //0x00A01A30; // 
//unsigned long xdata alert1_for_tsms=0; // ������������ �� ���. ������� ��� ��������� sms-������.
//unsigned long xdata alert2_for_tsms=0; // 

// ************************************************************************************************
//
// ************************************
// *             �������              *
// ************************************

void Automat_io (void);
void Automat_sms (void);

// ������������� ������
//
void Init_io (void)
{
    Init_sms_phone();
    state_io_wk = 1;
    gprs.b.command = 1;
}

void Fill_buf_old(void)
{
    count_all_buf = 0; // ������� ������ �� ����� ������
    count_tek_buf = 1; // ������� ��������(�������������) ������

    if (io_buf[0]==0x20)
    {
        count_all_buf = 1;
      //count_tek_buf = 1;
        io_who_send_sms |= 0x40;
    }

    while (1) // ��������� ����� ��� ��������
    {
        io_buf[count_tek_buf++] = pBuf[count_all_buf++];
        // ���� ���������� ��������� �����
        if ((count_tek_buf==LEN_PACK)||(count_all_buf>=Len_buf)) return;
    }
}

unsigned char Get_npack_inc(unsigned char type)
{
         if (type == 0x31) return npack_31_inc;
    else if (type == 0x32) return npack_32_inc;
    else if (type == 0x33) return npack_33_inc;
    else if (type == 0x34) return npack_34_inc;
    else if (type == 0x35) return npack_35_inc;
    else if (type == 0x36) return npack_36_inc;
    else if (type == 0x37) return npack_37_inc;
    else if (type == 0x39) return npack_39_inc;
    else if (type == 0x3B) return npack_3B_inc;
    else if (type == 0x3D) return npack_3D_inc;
    else if (type == 0x3F) return npack_3F_inc;
    else if (type == 0x43) return npack_43_inc;
    else if (type == 0x45) return npack_45_inc;
    else if (type == 0x47) return npack_47_inc;
    else if (type == 0x49) return npack_49_inc;
    else if (type == 0x4B) return npack_4B_inc;
    return 0;
}

void Fill_buf_auto(void)
{
  unsigned char xdata npack_inc = Get_npack_inc(io_buf[0]);

    if (SMS_Tail[0]>100) SMS_Tail[0] = 0;

    count_all_buf = 0; // ������� ������ �� ����� ������

    // ������� � ����� HEADER
    io_buf[1] = LEN_HEADER+SMS_Tail[0]; // �������� ������ ������ �����
    io_buf[2] = npack_inc; // ������������� ������
    io_buf[3] = 0; // ������

    io_buf[4] = 0x00; //0x10 - ������� �����
    if (_flag_send_last_pack) io_buf[4] |= 0x01; // ���������� ��������� �����.

/*    if (io_buf[0] == 0x34)
    {
        io_buf[1] =   14; // �������� ��� �������
        io_buf[4] = 0x01; // ��������� ����� (����� ������)
    }*/
    if (io_buf[0] == 0x36)
    {
        io_buf[4] |= 0x80; // ������ ������
        if (TypeMesOut==1) // ������ �������
        {
            io_buf[1]  =    5; // ��������
            io_buf[4] |= 0x01; // ��������� ����� (����� ������)
        }
        else io_buf[4] |= 0x20; // �������, ��� ��� ����������� (�������������� �������)
    }
/*  if (io_buf[0] == 0x32)// ��� ����������
    {
        io_buf[4] = 0x01; // ��������� ����� (����� ������)
    }*/
    if ((io_buf[0] == 0x32)|| // ��� ����������
        (io_buf[0] == 0x31)|| // ��� �������
        (io_buf[0] == 0x3D))  // ��� ������� (������ ��� ������)
    {
        io_buf[4] |= 0x80; // ������ ������
    }

    count_tek_buf = LEN_HEADER; // ������� ��������(�������������) ������

    while (1) // ��������� ����� ��� ��������
    {
        io_buf[count_tek_buf++] = pBuf[count_all_buf++];
        // ���� ���������� ��������� �����
        if ((count_tek_buf==LEN_PACK)||(count_all_buf>=Len_buf)) return;
    }
}

void GetState(void)
{
  unsigned char xdata i;
    i = (unsigned char)((IOStreamState.Out>>11)&7);
// ��������� �� ������ � ������
// --------------------------------------------------------------
    if (i==1)
        pBuf = &IOStreamBuf.OutAuto[0]; // ��� ����������
    else
        pBuf = &IOStreamBuf.Out[0]; // ��� ���� ��������� �������

// ����� ������ (���-�� ���� ������ � ������)
// --------------------------------------------------------------
    Len_buf = IOStreamState.Out & 0x00FF;

// ���������� ��������� �����
// --------------------------------------------------------------
    if (IOStreamState.Out & 0x0400) _flag_send_last_pack = 1; else _flag_send_last_pack = 0;

// ������� ��������. ������ ������ ������ ����� ���� ��� �������.
// (������ �� ����������, � ����������)
// --------------------------------------------------------------
    if (Len_buf>(SizeBufOut))
        _flag_have_tail = 1; // �������� �����
    else
        _flag_have_tail = 0; // ��� ������

    if (i==2) // �������
    {
       _flag_have_tail = 0; // ��� ������
       _flag_send_last_pack = 1; // ���������� ��������� �����
    }

    // ���� ��������� ��� ������ ��� ����������� (�� ������� ������ ���� �� �����)
    // ���� ���������� ��������� ����� � ���� �����
    // ������ ��� �� ���������, � ������������� �����.
    if (((i==1)||(i==4)||(i==3)) &&
         (_flag_send_last_pack)&&(_flag_have_tail))
    {
        _flag_send_last_pack   = 0;
        _flag_send_p_last_pack = 1;
    }
    else _flag_send_p_last_pack = 0;
}

void ResetState(void)
{
    // ������� ��� "����� �����" � ��� �������� � ��� "��������� �����"
    // � ���. 0 �����
    IOStreamState.Out = IOStreamState.Out & 0x3B00;
    SMS_Tail[0] = 0; SMS_Tail[1] = 0;
    state_io_wk = 5;
}

/*bit Test_CS(void)
{
  unsigned char xdata i;
  unsigned  int xdata CS=0;
  unsigned int xdata *pCS;

    for(i=0; i<138; i++) CS+=pBuf_read_data[i];
    pCS = (unsigned int xdata*)&pBuf_read_data[138];
    if (CS==(*pCS)) return 1;
    return 0;
}
bit Test_PAS_0B(void)
{
  unsigned char xdata i;
    if (S_Password[0]==0xFF) return 1;
    for (i=0; i<8; i++) if (S_Password[i] != pBuf_read_data[130+i]) return 0;
    return 1;
}*/

/*void CS_error (unsigned char C)
{
    pBuf_read_data[0] = 0xCC;
    pBuf_read_data[1] = C;
    pBuf_read_data[2] = 0xCC;
}
void PAS_error (unsigned char C)
{
    pBuf_read_data[0] = 0xCC;
    pBuf_read_data[1] = C;
    pBuf_read_data[2] = 0xBB;
}*/

// �������� - ���� �������� ��� ��������?
bit Test_tel_to_phone(unsigned char xdata *pBufIO)
{
  unsigned char xdata i;
  unsigned char xdata n=0;

    if (pBufIO[1]==0x0F) return 0;

    for (i=1; i<10; i++) if (pBufIO[i*12+1]!=0x0F) n++;

    if (n>0) return 1;

    return 0;
}

// ��� ������ ��������� sms-������ ������� �� 7������� � 8������ ���.
void Data_decode(char xdata *src, char xdata *dest, int cnt)
{
    unsigned char xdata ofs = 0;
    unsigned char xdata mask, flag = 0;
    int xdata index, count, datalen;
    char xdata ch, TempSrc;
    TempSrc = *src;
    while (cnt--)
    {
        if (ofs) {
            mask = 0xff << (8-ofs);
            ch = (TempSrc & mask) >> (8-ofs);
            src++;
            TempSrc = *src;
            flag = 1;
        }
        else
        {
            ch = 0;
        }
        mask = 0xff >> ofs+1;
        ch |= (TempSrc & mask) << ofs;

        if ((flag)&&(ofs==0))
        {
            for (count=0; count < 2; count++)
            {
                datalen = strlen(src);
                for (index=0; index < datalen; index++)
                src[datalen - index] = src[datalen - 1 - index];
                src[datalen+1] = 0;
                src++;
            }
        }
        *dest++ = ch;
        if (ofs == 7) ofs = 0; else ofs++;
    }
    *dest = 0;
}

bit tstPhoneNum(unsigned char i, unsigned char cnt)
{
  unsigned char xdata l=0; // ����� ������
  unsigned char xdata j;

    while (1)
    {
        if (pBuf_read_data[l+i]=='"') break;
        tmp_telnum_ascii[l] = pBuf_read_data[l+i]; l++;
        if (l>=kolCN) // �� ���� ������� ���������
            return 0;
    }
    len_telnum_ascii = l;

    if (l==0) // ������ ����� (��� ������)
    {
        if ((cnt==0)||(cnt==1)) // �� ���� ������� ���������
            return 0;
        // ���������� ������� ����� � ���-�����
        tel_num[cnt].len_SO = 0; for (j=0; j<kolBN; j++) tel_num[cnt].num_SO[j] = 0xff;
        tel_num[cnt].need_write = yes; // ������������� �������� ����� �������� (����)            }
    }
    else
    {
        // ��������� ������ DC - ������
        // �� ������� ASCII: "+79027459999"
        // � ������   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        tel_num[cnt].len_SO = GetTel_ASCII_to_SO (&tel_num[cnt].num_SO[0],
                                                  &tmp_telnum_ascii[0],
                                                   len_telnum_ascii);

        tel_num[cnt].need_write = yes; // ������������� �������� ����� �������� (����)            }
        if (cnt==1) // DC1 �������� � DC2 � DC3
        {
            tel_num[3].len_SO = tel_num[2].len_SO = tel_num[1].len_SO;
            for (l=0; l<tel_num[1].len_SO; l++)
                tel_num[3].num_SO[l]=tel_num[2].num_SO[l]=tel_num[1].num_SO[l];
            tel_num[2].need_write = yes; // ������������� �������� ����� �������� (����)            }
            tel_num[3].need_write = yes; // ������������� �������� ����� �������� (����)            }
        }
    }
    return 1;
}

void Param_default(bit bPr2)
{
  unsigned int xdata CheckSum;
  unsigned int xdata * pCheckSum = &FlashDump[258];
  unsigned int xdata cnt;

    if (bPr2) ReadFlash(F_ADDR_SETTINGS, 260); reset_wdt;

    // *** password, pin ***
    //for (cnt=0; cnt<12; cnt++) S_Password[cnt] = 0xFF; //�� ���������
    //for (cnt=0; cnt<4;  cnt++) S_PinCode[cnt]  = 0xFF; //�� ���������
    // *** ������ ��� ��������� sms-������ ***
    FlashDump[91]=len_pasw_sms=8;
    for (cnt=0; cnt<8; cnt++) FlashDump[92+cnt]=pasw_sms[cnt]=cnt+1+0x30; // "12345678"
//  len_pasw_sms=0;
//  for (cnt=0; cnt<8; cnt++) pasw_sms[cnt]=0;
    // *** ����� ������� ��� ��������� sms-������ ***
    //mAlert1_for_tsms=0; mAlert2_for_tsms=0;

    // *** URL - BEELINE
    strcpy(&FlashDump[110],"internet.beeline.ru\0"); reset_wdt;
    // *** URL - MEGAFON
    //strcpy(&FlashDump[110],"internet.mc\0"); reset_wdt;
    // *** URL - MTS
    //strcpy(&FlashDump[110],"internet.mts.ru\0"); reset_wdt;

    // *** User Beeline
    strcpy(&FlashDump[174],"beeline\0"); reset_wdt;
    // *** Login
    strcpy(&FlashDump[190],"beeline\0"); reset_wdt;
    // *** IP server
    FlashDump[206] = 127; //213;
    FlashDump[207] = 0;   //59;
    FlashDump[208] = 0;   //64;
    FlashDump[209] = 1;   //229;
    // *** ID sensor1
    FlashDump[214] = 0x65; //1
    // *** ID sensor2
    FlashDump[215] = 0x65; //2
    // *** �������� ������ �������1
    FlashDump[216] = 60;
    // *** �������� ������ �������2
    FlashDump[217] = 60;
    // *** HTTP_SERVER_PORT ***
    FlashDump[218] = 0;  //Hi byte
    FlashDump[219] = 80; //Lo byte
    // *** HTTP_req
    //����� ������
  	//strcpy(&FlashDump[26],"POST /mon/nvgconhttp.dll?radar HTTP/1.1\r\n\0");
	strcpy(&FlashDump[26],"POST /IterisServer/webresources/psid/put_dt HTTP/1.1\r\n\0");
    //strcpy(&FlashDump[26],"POST /scripts/nvgconhttp.dll?data HTTP/1.1\r\n\0");
    //������ ������
    //strcpy(&FlashDump[26],"POST /radar/nvgconhttp.dll/data HTTP/1.1\r\n\0");
    reset_wdt;
    // * F_ADDR_SETTINGS Map *
    // ***********************
    // **  18- 25  - MicroLan SN
    // **  26- 90  - HTTP_REQ
    // **  91- 99  - User Password
    // ** 110-173  - PPP URL
    // ** 174-189  - PPP Login
    // ** 190-205  - PPP Password
    // ** 206-209  - IP Server
    // ** 210-213  - IP Server2 (no used)
    // ** 214-215  - ID Sensors
    // ** 216-217  - Time Req sensors
    // ** 218-219  - port
    // ** 220-257  - ...
    // ** 258,259  - ����������� �����
    // ***********************
    // **   http_port: 80   **
    // ***********************

    if (bPr2)
    {
        // ��������� ����� ����������� �����.
        for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
        *pCheckSum = CheckSum; // ����� ����������� �����.
        WriteFlash(F_ADDR_SETTINGS, 260);
    }
}

/*bit strscmp(unsigned char code * pSrc, unsigned char xdata * pFind)
{
    unsigned char xdata i,j;
    unsigned char xdata len=0;
    unsigned char xdata r;

    for (i=0; ((i<40)||(pFind[i]!=0)); i++);
    len=i;

    for (i=0; i<len; i++)
    {
        r=1; reset_wdt;
        for (j=0; pFind[j]!=0; j++)
        {
            if (pSrc[i+j]!=pFind[j]) r=0;
        }
        if (r==1) return 1;
    }
    return 0;
}*/

// �������� �������� �������� sms
bit Test_read_text(unsigned char xdata * pBuf_read_data)
{
    unsigned char xdata i,j,l;
    unsigned int  xdata cnt;
    unsigned int  xdata CheckSum;
    unsigned int  xdata * pCheckSum = &FlashDump[258];

        // ��������� ������ �� SMS      <PAS#...>
        for (i=0; i<len_pasw_sms; i++)
            if (pBuf_read_data[i]!=pasw_sms[i]) return 0; // ������ �� ������
        i++; // ���������� '#'

        for (j=0; j<10;j++)
        {
            if (pBuf_read_data[i+j]==' ') break;
            pBuf_read_data[i+j]=toupper(pBuf_read_data[i+j]);
            reset_wdt;
        }

        // ****************************
        //   <PAS#SETPASS "NEWPAS">   
        // ****************************
        if (strncmp("SETPASS \"", &pBuf_read_data[i], 9) == 0) // ����� ������
        {
            i+=9;
            for (j=0; ; j++)
            {
                if (pBuf_read_data[i+j]=='"') break;
                if (j>=8) return 0; // ������ ������� �������
            }
            // ���������� ������ �� ����
            ReadFlash(F_ADDR_SETTINGS, 260);
            FlashDump[91]=len_pasw_sms=j;
            for (l=0; l<j; l++) FlashDump[92+l]=pasw_sms[l]=pBuf_read_data[i+l]; // ����� ������

            // ��������� ����� ����������� �����.
            for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
            
            *pCheckSum = CheckSum; // ����� ����������� �����.

            WriteFlash(F_ADDR_SETTINGS, 260); reset_wdt;

            pBuf_read_data[0]='A';
            pBuf_read_data[1]=0x04;
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // ****************************
        //   <PAS#RESET M>  (M,C,F)
        // ****************************
        if (strncmp("RESET ",     &pBuf_read_data[i],  6) == 0) // �����
        {
            if ((pBuf_read_data[i+6]=='M')|| // ���������� ������� ����� GSM-������
                (pBuf_read_data[i+6]=='m'))
                md_resetGSM=2;

            if ((pBuf_read_data[i+6]=='C')|| // ���������� ������� ����� ���������
                (pBuf_read_data[i+6]=='c'))
               _flag_need_do_restart = 1;

            if ((pBuf_read_data[i+6]=='F')|| // ���������� ������� ����� ��������� � 
                (pBuf_read_data[i+6]=='f'))  // ������������ ��������� ���������. 
            {
               _flag_need_do_restart = 1;
                Param_default(1);
            }
            if ((pBuf_read_data[i+6]=='L')|| // ���������� ������� ����� ���������
                (pBuf_read_data[i+6]=='l'))
                _flag_need_to_loader=1; // ���������� ������� �� ���������.
            return 0;
        }
        else
        // ****************************
        //   <PAS#STATUS>
        // ****************************
        if (strncmp("STATUS",     &pBuf_read_data[i],  6) == 0) // ������
        {
             pBuf_read_data[0]='A';
             pBuf_read_data[1]=0x01;
             return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // ************************************************************************
        //   <PAS#GPRS "internet.beeline.ru","beeline","beeline","213.59.64.229","80">
        //   <12345678#GPRS "internet.mc","beeline","beeline","213.59.64.229","80">
        //      9   5     64+3                   16+3     16+3     12+2         = 133
        // ************************************************************************
        if (strncmp("GPRS \"",    &pBuf_read_data[i],  6) == 0) // ��������� ��� GPRS
        {
            ReadFlash(F_ADDR_SETTINGS, 260);

            i+=6;  //*** ������ ����  URL ***************
            l=0; FlashDump[110] = 0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='"') break;
                FlashDump[110+l] = pBuf_read_data[l+i]; l++;
                FlashDump[110+l] = 0;
                if (l>64) return 0; // ������
            }
            i+=l+3;//*** ������ ���� USER ***************
            l=0; FlashDump[174] = 0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='"') break;
                FlashDump[174+l] = pBuf_read_data[l+i]; l++;
                FlashDump[174+l] = 0;
                if (l>16) return 0; // ������
            }
            i+=l+3;//*** ������ ���� LOGIN **************
            l=0; FlashDump[190] = 0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='"') break;
                FlashDump[190+l] = pBuf_read_data[l+i]; l++;
                FlashDump[190+l] = 0;
                if (l>16) return 0; // ������
            }
            // ","192.168.0.153"
            i+=l+3;//*** ������ ���� <<1>> IP ������� *********
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='.') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[206] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� <<2>> IP ������� *********
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='.') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[207] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� <<3>> IP ������� *********
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='.') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[208] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� <<4>> IP ������� *********
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='"') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[209] = atoi(&pBuf_read_data[i]);
            i+=l+3;//*** ������ ���� ,"PORT *************
            if ((pBuf_read_data[i-2]==',') && //�.�. ������������ ���� PORT
                (pBuf_read_data[i-1]=='"') &&
                (pBuf_read_data[i]  >='0') &&
                (pBuf_read_data[i]  <='9'))
            {
                l=0;
                while (1)
                {
                    if (pBuf_read_data[l+i]=='"') { pBuf_read_data[l+i]=0; break; }
                    l++; if (l>5) return 0; // ������
                }
            }
            *(unsigned int xdata*)&FlashDump[218] = atoi(&pBuf_read_data[i]);

            // ��������� ����� ����������� �����.
            for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
            *pCheckSum = CheckSum; // ����� ����������� �����.

            WriteFlash(F_ADDR_SETTINGS, 260); reset_wdt;

            md_resetGSM=1; // ���������� �������������������� GSM-�����

            pBuf_read_data[0]='A';
            pBuf_read_data[1]=0x02; // ������� ��
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // *************************************************************************************
        //   <PAS#HTTP "POST /scripts/nvgconhttp.dll/pdu HTTP/1.1"> (\r\n\0 - ����������� ����)
        //      9   5     64+3 = 81
        // *************************************************************************************
        if (strncmp("HTTP \"",    &pBuf_read_data[i],  6) == 0) // ��������� ��� GPRS HTTP
        {
            ReadFlash(F_ADDR_SETTINGS, 260);

            i+=6;  //*** ������ ����  URL ***************
            l=0; FlashDump[26] = 0;
            while (1)
            {
                if (pBuf_read_data[l+i]=='"') break;
                FlashDump[26+l] = pBuf_read_data[l+i]; l++;
                if (l>61) return 0; // ������
            }
            FlashDump[26+l]   = 0x0d;
            FlashDump[26+l+1] = 0x0a;
            FlashDump[26+l+2] = 0;
            // ��������� ����� ����������� �����.
            for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
            *pCheckSum = CheckSum; // ����� ����������� �����.

            WriteFlash(F_ADDR_SETTINGS, 260); reset_wdt;

            //md_resetGSM=1; // ���������� �������������������� GSM-�����

            pBuf_read_data[0]='A';
            pBuf_read_data[1]=0x02; // ������� ��
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // **********************************
        //   <PAS#SPECTRUM 255,255,255,255>
        // **********************************
        if (strncmp("SPECTRUM ",  &pBuf_read_data[i],  9) == 0) // ������� ������� � ���������� ������ ��������
        {
            ReadFlash(F_ADDR_SETTINGS, 260);

            i+=9;  //*** ������ ���� ID1 ***************
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]==',') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[214] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� ID2 ***************
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]==',') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[215] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� INT1 **************
            l=0;
            while (1)
            {
                if (pBuf_read_data[l+i]==',') { pBuf_read_data[l+i]=0; break; }
                l++; if (l>3) return 0; // ������
            }
            FlashDump[216] = atoi(&pBuf_read_data[i]);
            i+=l+1;//*** ������ ���� INT2 **************
            l=0;
            while (1)
            {
                if ((pBuf_read_data[l+i] <'0')||
                    (pBuf_read_data[l+i] >'F'))
                { pBuf_read_data[l+i]=0; break; }

                l++; if (l>3) return 0; // ������
            }
            FlashDump[217] = atoi(&pBuf_read_data[i]);

            // ��������� ����� ����������� �����.
            for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
            *pCheckSum = CheckSum; // ����� ����������� �����.
            WriteFlash(F_ADDR_SETTINGS, 260); reset_wdt;

            pBuf_read_data[0]='A';
            pBuf_read_data[1]=0x02; // ������� ��
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // *****************************************************
        //   <PAS#SETPHONES 0,"+79107459999","+79107488183","+79107480005","+79107480006","+79107480008">
        //    mC  SC,  DC,   MD1,MD2D, US1
        //         0  1,2,3   5    6    8
        // ************************************** 9+10+2+22+22+22+22+22=131
        if (strncmp("SETPHONES ",  &pBuf_read_data[i], 10) == 0) // ������� �������� ������� (SMS-�����, DC)
        {
            i+=10; //*** ������ ����� �������� ������� ********
            sec_for_numtel = (pBuf_read_data[i]-0x30) & 0x07;
            // ����� ��������� �� ����� "����� �������� �������"
            ReadScratchPad1();
            FlashDump[8] = 'S'; 
            FlashDump[9] = sec_for_numtel;
            WriteScratchPad1();

            i+=3; //*** ������ ������� ������ ********

            if (tstPhoneNum(i,0)==1) // 0 - SC
            {
              i+=len_telnum_ascii+3; //��������� ����
              if (tstPhoneNum(i,1)==1) // 1 - DC
              {
                i+=len_telnum_ascii+3; //��������� ����
                if (tstPhoneNum(i,5)==1) // 5 - MD1
                {
                  i+=len_telnum_ascii+3; //��������� ����
                  if (tstPhoneNum(i,6)==1) // 6 - MD2D
                  {
                    i+=len_telnum_ascii+3; //��������� ����
                    tstPhoneNum(i,8); // 7 - US1
            } } } }

            pBuf_read_data[0]='A'; pBuf_read_data[1]=0x05; // ������� ������
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // *****************************************************
        //   <PAS#GETPHONES>
        // *****************************************************
        if (strncmp("GETPHONES",  &pBuf_read_data[i], 9) == 0) // �������� �������� ������ (SMS-�����, DC)
        {
            pBuf_read_data[0]='A'; pBuf_read_data[1]=0x05; // ������� ������
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // *****************************************************
        //   <PAS#GETVER>
        // *****************************************************
        if (strncmp("GETVER",     &pBuf_read_data[i], 6) == 0) // �������� ����� ������
        {
            pBuf_read_data[0]='A'; pBuf_read_data[1]=0x06; // ������� ����� ������
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
        // *****************************************************
        //   <PAS#RELE 1+,2-,4+>
        // *****************************************************
        if (strncmp("RELE ",     &pBuf_read_data[i], 5) == 0) // �������� ����� ������
        {
            i+=5;  //*** ������ ����  URL ***************

            for (j=0; j<4; j++)
            {
                if (pBuf_read_data[i]==(j+0x31))
                {
                    if (pBuf_read_data[i+1]=='+')
                    {
                        time_rele_on[j]=time_rele_work[j]=0; _c_rele_do[j]='1';
                        i+=3;
                    }
                    else
                    if (pBuf_read_data[i+1]=='-')
                    {
                        time_rele_on[j]=time_rele_work[j]=0; _c_rele_do[j]='0';
                        i+=3;
                    }
                    else
                        return 0; // ������
            }   }

            pBuf_read_data[0]='A'; pBuf_read_data[1]=0x02; // ������� ��
            return (~_f_SMS_noanswer); // ���� ���� �������� �� sms, �� return 1.
        }
        else
            return 0;
}

// �������� �������� ������
bit Test_read_data(void)
{
    unsigned char xdata Temp_Buf[144]; // 144-144/8=126
    unsigned char xdata i,j,l;
    unsigned int  xdata cnt;
    unsigned int  xdata CheckSum;
    unsigned int  xdata * pCheckSum = &FlashDump[258];

    if (_flag_tel_ready_R) // 0 - ������� ����� � �������� � ������
    {                        // 1 - ������� ����� ������ � ������
        if ((md_mode==0)&&((char)Is_new_sms_8bit()==no)) return 0;

        switch (pBuf_read_data[0])
        {
            case 0x0f: // ������� ������ ���������
            case 0x0b: // ������� ����� sms-������ � ����.������
                return 1;
        }
        return 0;
    }

    if ((md_mode==0)&&((char)Is_new_sms_8bit()==no)) // ��������� SMS
    {
        for (i=0; i<=125; i++) Temp_Buf[i]=pBuf_read_data[i];
        Temp_Buf[126]=0;

        Data_decode(Temp_Buf,pBuf_read_data,126); // ������������ 7������ �����.
        reset_wdt;
        return Test_read_text(pBuf_read_data);
    }

    switch (pBuf_read_data[0])
    {
        // ������� ������ ���������
        case 0x0f:
            if ((md_mode==0)&&(code_access_sms!=1)) return 0;
        return 1;

        case 0x0b: // ������� ����� sms-������ � ����.������
        case 0x5b: // ������� ����� 10 ������. ������� ����.������� (11-20)
        return 1;

        case 0x55: // ����� ������ ��� ��������� sms-������
            i=1;
            for (j=0; ; j++)
            {
                if ((pBuf_read_data[i+1+j]=='#') ||
                    (pBuf_read_data[i+1+j]== 0 )) break;
                if (j>=8) return 0; // ������ ������� �������
            }
            // ���������� ������ �� ����
            ReadFlash(F_ADDR_SETTINGS, 260);
            FlashDump[91]=len_pasw_sms=j;
            for (l=0; l<j; l++) FlashDump[92+l]=pasw_sms[l]=pBuf_read_data[i+10+l]; // ����� ������
            // ��������� ����� ����������� �����.
            for (CheckSum=0, cnt=0; cnt<258; cnt++) CheckSum += FlashDump[cnt];
            *pCheckSum = CheckSum; // ����� ����������� �����.
            WriteFlash(F_ADDR_SETTINGS, 260); reset_wdt;

            pBuf_read_data[0]='A';
            pBuf_read_data[1]=0x04;
            return 1;
        break;

        case 0x03: case 0x53:
        case 0x31: case 0x33: case 0x35: case 0x37: case 0x3D:
        case 0x43: case 0x45: case 0x47: case 0xDB:
        case 0x4D: // ������� ���������� ��.
            if ((md_mode==0)&&(code_access_sms!=1)) return 0;
            return 1;
        break;

        case 0x51:
            if (md_mode==1) return 1;
        return 0;

        // ������� ���/
        case 0x4F:
            if (md_mode==1) _online=yes; // ����� online (��� ��������)
            return 0;
        break;

        default:
            return 0;
        break;
    }
}

// �������� ���� �� �������� ���������
// � ���� ���� ����������
void Test_read_mes(void)
{
  unsigned char xdata i;

    // ���� ����� ������� ������� � ��������� ������
    if ((_flag_need_do_restart)&&(IsNonDelete_sms()==0))
    {
          //count_restart_cpu = count_restart_gps = count_restart_gsm = 0;
          //dRun_f.v_float = 0.f; // ����� �������
            reset_code = 0xB2;
            SOFT_RESET; //Software Reset uC
    }

    if ((_flag_need_to_loader)&&(IsNonDelete_sms()==0))
        RestartToLoader();

    // ���� ������ �� ������� ������? (�.�. ���� � ��� ��� �����)
    if ((IOStreamState.In & 0xC000)==0) // ������ ���, ����� ����.
    {
        IOStreamState.In = 0;
        if ((char)Is_new_sms()==1) // ���� �� �������� ��������� ? ��!
        {
            len_read_data = Read_new_sms();
            // ���� ��� 7 ������ (���������) sms,
            // � �� 'A', �� �� ����.
//          if ((md_mode==0)&&((char)Is_new_sms_8bit()==no)&&(pBuf_read_data[0]!=0x41)) return;
            // ���� ����� �� �������
            if (len_read_data>0)
            {
                //��������� ��������� ���������
                if ((char)Test_read_data()==1)
                {
                    io_who_send_sms = who_send_sms;
                    if ((char)Is_new_sms_8bit()==no) io_who_send_sms |= 0x40;

/*                  // ���� ������ �� sms, �� ������� �� ������� ������.
                    if (md_mode==0)
                    {
                       _need_sleep_gps=0; // ��� ������ �� ������� ������. ����� �������� GPS.
                    }
*/
                    //�������� ������� � ����� IOStreamBuf
                    for (i=0; i<len_read_data; i++) IOStreamBuf.In[i] = pBuf_read_data[i];
                    IOStreamState.In = 0x8000 | (len_read_data&0x00FF);

                }
                // ������� SMS-���������, ������ ���������������� ����� ����.
                StartRA;
    }   }   }
/*  else
    // ���� ������ �� ������� ������?
    if ((IOStreamState.In & 0x8000)&& // ������ ����.
        (IOStreamBuf.In[0]=='K'))     // ������ ������� � gprs (KOD8#...)
    {
        if (IOStreamState.In & 0x00FF)
        {
            pBuf_read_data = &IOStreamBuf.In[5];
            Test_read_text(); // ��������� ��������� ���������
            IOStreamState.In = 0;
        }
        else
            IOStreamState.In = 0;
    }*/
}

void NpackInc(unsigned char type)
{
    // ����������� �������� ������� ���������
         if (type == 0x31) npack_31_inc++;
    else if (type == 0x32) npack_32_inc++;
    else if (type == 0x33) npack_33_inc++;
    else if (type == 0x34) npack_34_inc++;
    else if (type == 0x35) npack_35_inc++;
    else if (type == 0x36) npack_36_inc++;
    else if (type == 0x37) npack_37_inc++;
    else if (type == 0x39) npack_39_inc++;
    else if (type == 0x3B) npack_3B_inc++;
    else if (type == 0x3D) npack_3D_inc++;
    else if (type == 0x3F) npack_3F_inc++;
    else if (type == 0x43) npack_43_inc++;
    else if (type == 0x45) npack_45_inc++;
    else if (type == 0x47) npack_47_inc++;
    else if (type == 0x49) npack_49_inc++;
    else if (type == 0x4B) npack_4B_inc++;
    else if (type == 0x0F) _flag_need_do_restart = yes;
}

// ***********************************************
// * ������� �����/������                        *
// ***********************************************

unsigned char xdata StatePB = 0x00; //state of PROG_BUTTON
void Automat_io (void)
{

    //*forced GSM reset processing*
    switch (StatePB)
    {
        case 0x00: //restarting..
        break;

        case 0x01:
            gprs.all = 0x0000;
            gprs.b.command = 1;
            StateGlobGPRS = 0x01;
            //*-----------*
            Restart_phone();
            //*-----------*
            StatePB = 0x00;
        break;

        default:
            if (!PROG_BUTTON) StatePB--; //�������
                         else StatePB = 0xFF;
        break;
    }
    //*---------------------------*

    if (GSM_DCD==0) time_nogprs=0;

    if (gprs.b.command==1)
    {
        Automat_sms_phone(); reset_wdt;
        Automat_sms ();      reset_wdt;
    }
}


void Automat_sms (void)
{
  unsigned char xdata i;
  unsigned int  xdata j;

    // �������� ���� �� �������� ���������
    // � ���� ���� ����������

    if (state_io_wk>3)
    {
        if ((md_mode==0)&&(time_wait_io2>=2)) // ���������� ������ �� sms
        {
            Test_read_mes();
            time_wait_io2=0;
        }
        else
        if (md_mode==1) // ���������� ������ �� ������
        {
            Test_read_mes();
            time_wait_io2=0;

            // ���� �� � 5�� ��� ������ ����� �� ������� ������
            //if (md_who_call!=5)
            //{
            //    _need_sleep_gps=0; // ��� ������ �� ������� ������. ����� �������� GPS.
            //}

            if ((_online) &&                      // ���� ����� online �
                (time_wait_io3>=2) &&             // ����� ���������� �
                ((IOStreamState.In & 0xC000)==0)) // �������� ����� �� �����
            {
                IOStreamBuf.In[0] = 0x4F;  // ����������� 4F
                IOStreamState.In = 0x8001;
                time_wait_io3=0;
    }   }   }

    switch (state_io_wk)
    {
        case 1: // ��������� ���������
            time_wait_io = 0;
            state_io_wk = 3;
        break;

        case 3:
            if (_flag_tel_ready) // ������� �����?
            {
                state_io_wk = 5;
            }
        break;
/*
        case 4:
            if (!_flag_tel_ready_R) // ������� ����� � ��������?
            {
                state_io_wk = 5;
            }
        break;
*/

        // ******************************************************
        // *  �������� ����                                     *
        // ******************************************************
        case 5:

            if (md_mode==0) { StSYSTEM &= ~0x0006; } // �������� ���������.

            if ((md_mode==0)&& //������ �� sms
                (_flag_tel_ready_R)) // ������� ���
                return;

            // ���� ��� ��������?
            if (((IOStreamState.Out) & 0x8300)==0x8000)
            {
                // ������ ��� "����� �����"
                IOStreamState.Out = IOStreamState.Out | 0xC000;
                state_io_wk = 20;
                //DEBUG
                send_c_out_max(0xB8); // need send sms
                send_c_out_max(0x00);
            }
/*          else
            if ((alert1_for_tsms>0) || (alert2_for_tsms>0))
            {
                count_tek_buf = do_txtpack_alert(&io_buf[0]);
                alert1_for_tsms = alert2_for_tsms = 0;
                SetNeedSendSMSalert();
                state_io_wk = 10;
            }*/
        break;

/*      case 10:
            //0x04 - ��� �����, 7 ������ ��������� sms.
            if (tel_num[8].need_send_sms_Al==1)
            {
led_GSM1 = 0; // ������ ���������.
                tel_num[8].need_send_sms_Al=0;
                Send_sms (&io_buf[0], count_tek_buf, 0x48); //0x40|8
                alert1_for_tsms = alert2_for_tsms = 0;
                time_wait_io = 0; state_io_wk = 12; return;
            }
            for (i=11; i<=20; i++)
               if (tel_num[i].need_send_sms_Al==1)
               {
led_GSM1 = 0; // ������ ���������.
                   tel_num[i].need_send_sms_Al=0;
                   Send_sms (&io_buf[0], count_tek_buf, (0x40|i));
                   alert1_for_tsms = alert2_for_tsms = 0;
                   time_wait_io = 0; state_io_wk = 12; return;
               }
            time_wait_io = 0; state_io_wk = 50;
        break;

        case 12: // ��������/�������� "��������� sms-������� ����?"
        {
            i = Is_sms_send_out();
            if (i==1) // ���� �����
            {
led_GSM1 = 1; led_GSM2 = 1; // �������� ���������.
                // ���������� SMS-���������, ������ ����� ����.
                StartRA;

                cLED2 &= 0x0f; // ����� ��������, ������ ����, SMS ���������� - ���� ������ ���.

                state_io_wk = 10; return;
            }
            else
            if ((i==0)||(i==3)) // �� ������� ��������� �����
            {
led_GSM1 = 1; led_GSM2 = 1; // �������� ���������.

                cLED2 &= 0x0f; cLED2 |= 0x40; // ��������� �������� SMS - ������.

                state_io_wk = 10; return;
            }
            else
            if (i==2) // ���� ���������� ��������� �� �����
            {
                // ����...
            }
            if (md_mode==1) // ���� ����� ������� (����� ��)
            {
               _flag_need_send_sms = no; // sms ���������� �� ����
                state_io_wk = 25; // ������� �� ������� ������ �� ��
            }
        }
        break;
*/
        // **** ������ StateSendSMS ****
        //[���R���r]:
        // � - ����� �� ��������� ������ � ������ �������� SMS;
        // � - ��������� ������� � ������ �������� SMS;
        // � - ��������� ������������� � ������ �������� SMS;
        // R - ��������� ��������� RA � ������ �������� SMS;
        // � - ����� �� ��������� ������ ��������� �� �������;
        // � - ��������� ������� ��������� �� �������;
        // � - ��������� ������������� ��������� �� �������;
        // r - ��������� ��������� RA ��������� �� �������
        case 20:
            GetState(); // pBuf, Len_buf, _flag_send_last_pack, _flag_have_tail.

            // ���� ���� ������������� �������� �� ����� ������,
            // �� ���� ������ �� �������� � ���������� ���
            if (Len_buf==0) { ResetState(); StateSendSMS &= 0x0F; return; }

/*          if (md_mode==0) // ��� ������� ����� ��� ��� ����������
            {
                ResetState();
                StateSendSMS &= 0x0F; StartRA; // ����� ����
                return;
            }
*/

// ******** ����������� ���� ��������� *********************************
            i = (unsigned char)((IOStreamState.Out>>11)&7);
            if (i==1)
            {
                io_buf[0] = 0x32;     // ���������
                // ��� ������������� ������ (1-�������/2-������/3-�������/4-���������/5-�����������)
                TypeMesOut = 4;
                StateSendSMS |= 0x20;
            }
            else
            if (i==2)
            {
                io_buf[0] = 0x36;     // �������
                TypeMesOut = 1;
                StateSendSMS |= 0x40;
            }
            else
            if (i==3)
            {
                io_buf[0] = 0x36;     // �����������
                TypeMesOut = 5;
                StateSendSMS |= 0x10;
                if ((IOStreamState.In & 0x8000)&& // ���� ��. ������.
                    (IOStreamBuf.In[0]!=0x31)&&   // ��
                    (IOStreamBuf.In[0]!=0x3D))    // �� ������ 31, 3D.
                {
                    ResetState();
                    StateSendSMS |= 0x01; // ����� ���, ����� �� ������� ���������.
                    StateSendSMS &= 0x0f;
                    return;
                }
            }
            else
            if (i==4)
            {
                io_buf[0] = IOStreamState.ReqID; // ����� �� ������
                TypeMesOut = 2;
                StateSendSMS |= 0x80;
            }
            else
            {
                ResetState();
                StateSendSMS &= 0x0F;
                return;
            }

            NpackInc(io_buf[0]);

// ******** ����������� ����������� �������� *********************************
            // ���������� ������ - ������ ������
            // ��� �������� ������ �� �����.
            if (((Get_quality_signal()<1)||  // - ������ ������
                 (!_flag_tel_ready)     // - �������� ������ �� �����
                )&& (md_mode==0))            // ��� ��� ������ ���� � ������ sms
            {
                ResetState();
                StateSendSMS |= (StateSendSMS>>4);
                StateSendSMS &= 0x0F;
                return;
            }

            // ���������� ������ ��� ��������
            if ((io_buf[0]==0x0B)||(io_buf[0]==0x03)||(io_buf[0]==0x0F)||
                (io_buf[0]==0x7B)||(io_buf[0]==0x20))
            {
               _flag_have_tail=0;  // ������ ���
               _flag_send_p_last_pack=0; // ��� �� ������������� �����
                Fill_buf_old();    // ���������� ���������� ������ (������ ��������)
                if (count_tek_buf<=5) {ResetState(); return;}
            }
            else
            {
                Fill_buf_auto();  // ���������� ���������� ������ (����� ��������)
            }

            // ���� ���� �����, �� ���� ��� �������� �� ������ ������.
            if (_flag_have_tail)
            {
                // SizeBufOut - ������ �������
                // Len_buf   - ����� �������
                for (j=SizeBufOut; j<Len_buf; j++)
                     pBuf[j-SizeBufOut] = pBuf[j];
                // ���� ����������� ��� ����� ��������
                SMS_Tail[0] = SMS_Tail[1];
                SMS_Tail[1] = 0;
                // ���� �������� ����� ����� ������
                IOStreamState.Out = (IOStreamState.Out & 0xFF00) |
                                  ((Len_buf-SizeBufOut) & 0x00FF);
            }
            else
            {
                // ���� �������� 0 ����� ������
                IOStreamState.Out = IOStreamState.Out & 0xFF00;
                SMS_Tail[0] = 0; SMS_Tail[1] = 0;
            }

            // ������� ��� "����� �����" � ��� �������� � ��� "��������� �����"
            IOStreamState.Out = IOStreamState.Out & 0x3BFF;

            // ���� ��� ������������� �����, �� ������������� ��� "����� �����",
            // ��� �������� � ��� "��������� �����"
            if (_flag_send_p_last_pack)
            {
                IOStreamState.Out = (IOStreamState.Out) | 0xC400;
            }

            if (md_mode==0) state_io_wk = 40; // 0-���������� ������ �� sms
            else
            if (md_mode==1) state_io_wk = 25; // 1-���������� ������ �� ������
            else
            {
                // ���������� ���� ������
                ResetState();
                StateSendSMS |= (StateSendSMS>>4);
                StateSendSMS &= 0x0F;
                return;
            }

            StSYSTEM |= 0x0002; // ������ ���������.
        break;

// ******************************************
//          ������� ������ �� ������
// ******************************************
        case 25: // �������� �����
            Send_modem (&io_buf[0], count_tek_buf);
            time_wait_io = 0; state_io_wk = 27;
        break;

        case 27: // ���� ����������
            i = Is_sms_send_out();
            if ((i==1)||(md_mode==0)||(time_wait_io>250)) // ����� ����
            {
                //NpackInc(io_buf[0]);

                StateSendSMS &= 0x0f; // ����� ���� ����� ���������
                // ���������� ���������, ������ ����� ����.
                StartRA;

                //time_wait_io3 = 0;
                //state_io_wk   = 51;
//led_GSM1 = 1; // �������� ���������.
                state_io_wk = 5;
            }
            /*if (i==0) // �� ������� ��������� �����
            {
                time_wait_io = 0;
                state_io_wk = 50;
            }*/
        break;

// ******************************************
//              ������� ������
// ******************************************
        case 40: // �������� �����
            // TypeMesOut ��� ������������� ������ (1-�������/2-������/3-�������/4-���������/5-�����������)
            if (TypeMesOut==1) i=0x81;//0x01; //8-� ��������������, 0-8���, 1-�������
            else
            if (TypeMesOut==4) i=0x83;//0x03; //8-� ��������������, 0-8���, 3-���������
            else
            if (TypeMesOut==5) i=0x81;//0x01; //8-� ��������������, 0-8���, 1-�������
            else
            if (io_buf[0]==0x0B) i = 0x82;
            else
            if ((io_buf[0]==0x20)&&(io_buf[1]==0x10)&&(io_buf[2]==0x14)) //"Phones" - �������� ������ �� DC2
               i=0x80|0x42;
            else
               i=0x80|io_who_send_sms; // ��� �����, 7 ��� 8 ���.

          #ifdef _SAGEM_
            i = i&0x7F; // � ������� ������������� �� ����������.
          #endif
            Send_sms (&io_buf[0], count_tek_buf, i);
            time_wait_io = 0; state_io_wk = 45;
        break;

        case 45: // ��������/�������� "����� ����?"
        {
            i = Is_sms_send_out();
            if (i==1) // ���� �����
            {
                StSYSTEM &= ~0x0006; // �������� ���������.

                //NpackInc(io_buf[0]); // ����������� �������� ������� ���������

                //DEBUG
                send_c_out_max(0xB8); // need send sms
                send_c_out_max(0x0C);
                send_c_out_max(0x00);

                StateSendSMS &= 0x0F; // ����� ���� ����� ���������
                // ���������� SMS-���������, ������ ����� ����.
                StartRA;

                StSYSTEM &= ~0x0070; // ����� ��������, ������ ����, SMS ���������� - ���� ������ ���.

                state_io_wk = 50;
                time_wait_io = 0;
                return;
            }
            else
            if ((i==0)||(time_wait_io>250)) // �� ������� ��������� �����
            {
                StSYSTEM &= ~0x0006; // �������� ���������.

                // ����� ���, ����� �� ������� ���������.
                StateSendSMS |= (StateSendSMS>>4);
                StateSendSMS &= 0x0F;

                state_io_wk = 50;
                time_wait_io = 0;
                StSYSTEM |= 0x0040; // ��������� �������� SMS - ������.
            }
            else
            if (i==3) // ��� �������������
            {
                // ���� ��������� �������������, ������ ��� ��� ������� ��� ����
                // � ���� i==0, �� ������ ���� ������. ����� DC2D
                // ������ �������� sms �� ����.
                if ((TypeMesOut==1)||(TypeMesOut==5))
                {
                    Send_sms (&io_buf[0], count_tek_buf, 4);
                    state_io_wk = 45;
                }
                else
                    state_io_wk = 50;
                time_wait_io = 0;
            }
            else
            if (i==2) // ���� ���������� ��������� �� �����
            {
                // ����...
            }
            if (md_mode==1) // ���� ����� ������� (����� ��)
            {
               _flag_need_send_sms = no; // sms ���������� �� ����
                state_io_wk = 25; // ������� �� ������� ������ �� ��
            }
        }
        break;

        case 50:
            if ((time_wait_io >20)||(md_mode==1)) state_io_wk = 5;
        break;

        default:
            state_io_wk = 1;
        break;
    }
}

/**********������� ��� ������� ������ ������ ������� �� GSM**********/
void break_gsm_req()
{
    if (!(IOStreamState.In & 0x0200)) //�� ����� �����������
        //����� ���������� ����� �����
        //�.�., ��������� ������ �� GSM �� ������ �� automat_request()
        IOStreamState.In = 0x0000;

    if (!(IOStreamState.Out & 0x0200)) //�� ����� �����������
    {
        //����� ���������� ����� ������,
        //� ����� �������� ���� ���� automat_request()
        //if ((IOStreamState.ReqID==0x3D)||(IOStreamState.ReqID==0x31)) // ������ ��� 0x31 � 0x3D
        if (IOStreamState.ReqID)
        {
            IOStreamState.Out = 0x0000;
            if (StateRequest > 0x01) //���� ��������� �������
            {
                StateRequest = 0xFF; //���������� ������
                SearchEnable = 0; //���������� ����� � �������
                StateSearch = 0x00; //���������� ����� � �������
            }
            else
            {
                StateRequest = 0x00; //���� ==0x01
            }
            IOStreamState.ReqID = 0;
        }
    }
}

// �������������� 2 ���� � ������ (5 �������� ���������� �����) (������: 0x001C � "...28").
unsigned char Hex_to_DecASCII_5 (unsigned int HA, unsigned char *M)
{
  unsigned char xdata i=0;
  unsigned char xdata c_0=0; // ������� �����
  unsigned  int xdata z;

    for (z=10000; z>=1; z/=10)
    {
        M[i] = HA/z;
        HA -= M[i] * z;
        M[i] += 0x30;
        if (M[i]!=0x30) c_0 = 1;
        if (c_0) i++;
    }
    if (i==0) i=1;
    return i;
}
/*
// ������ �����
unsigned char do_txtpack_M (unsigned char *pBuf)
{
  unsigned char xdata i;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;

    // ���������� �������
    pBuf[n++] = 0x20;
  //pBuf[n++] = 0x20;

    pBuf[n++] = ' ';
    pBuf[n++] = 'M';
    pBuf[n++] = ' ';
    pBuf[n++] = '=';
    pBuf[n++] = ' ';
    pBuf[n++] = 'p';
    if (mAlert1_for_tsms & 0x20000000) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 's';
    if (mAlert1_for_tsms & 0x00200000) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'j';
    if (mAlert1_for_tsms & 0x00000020) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'a';
    if (mAlert2_for_tsms & 0x00200000) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'i';
    if (mAlert2_for_tsms & 0x00800000) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'a';
    pBuf[n++] = 'i';
    if (mAlert2_for_tsms & 0x00000200) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'd';
    if (mAlert2_for_tsms & 0x00000800) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'c';
    if (mAlert2_for_tsms & 0x00001000) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'g';
    pBuf[n++] = 's';
    if (mAlert2_for_tsms & 0x00000010) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'g';
    pBuf[n++] = 'r';
    if (mAlert2_for_tsms & 0x00000020) pBuf[n++] = '1'; else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    pBuf[n++] = 'r';
    if (mAlert1_for_tsms & 0x01000000) pBuf[n++] = '1'; else pBuf[n++] = '0';

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}*/
/*
// ��������� ���������
// " Restart! PowerOut! SpeedUp! Jack! Alarm! Sensor1! Sensor2! Door! Capote! GuardOn! GuarOff!"
unsigned char do_txtpack_alert (unsigned char *pBuf)
{
  unsigned char xdata i;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;

    // ���������� �������
    pBuf[n++] = 0x20;
//  pBuf[n++] = 0x20;

    if (alert1_for_tsms & 0x20000000) //�������� ������� ���� �����
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'P';
        pBuf[n++] = 'o';
        pBuf[n++] = 'w';
        pBuf[n++] = 'e';
        pBuf[n++] = 'r';
        pBuf[n++] = 'O';
        pBuf[n++] = 'u';
        pBuf[n++] = 't';
        pBuf[n++] = '!';
    }
    if (alert1_for_tsms & 0x00200000) //�������� ���� ��������.
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'S';
        pBuf[n++] = 'p';
        pBuf[n++] = 'e';
        pBuf[n++] = 'e';
        pBuf[n++] = 'd';
        pBuf[n++] = 'U';
        pBuf[n++] = 'p';
        pBuf[n++] = '!';
    }
    if (alert1_for_tsms & 0x00000020) //����
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'J';
        pBuf[n++] = 'a';
        pBuf[n++] = 'c';
        pBuf[n++] = 'k';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00200000) //�������
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'A';
        pBuf[n++] = 'l';
        pBuf[n++] = 'a';
        pBuf[n++] = 'r';
        pBuf[n++] = 'm';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00800000) //������ ����� 1
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'S';
        pBuf[n++] = 'e';
        pBuf[n++] = 'n';
        pBuf[n++] = 's';
        pBuf[n++] = 'o';
        pBuf[n++] = 'r';
        pBuf[n++] = '1';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00000200) //������ ����� 2
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'S';
        pBuf[n++] = 'e';
        pBuf[n++] = 'n';
        pBuf[n++] = 's';
        pBuf[n++] = 'o';
        pBuf[n++] = 'r';
        pBuf[n++] = '2';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00000800) //�����
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'D';
        pBuf[n++] = 'o';
        pBuf[n++] = 'o';
        pBuf[n++] = 'r';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00001000) //�����
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'C';
        pBuf[n++] = 'a';
        pBuf[n++] = 'p';
        pBuf[n++] = 'o';
        pBuf[n++] = 't';
        pBuf[n++] = 'e';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00000010) //���������� ��� ������
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'G';
        pBuf[n++] = 'u';
        pBuf[n++] = 'a';
        pBuf[n++] = 'r';
        pBuf[n++] = 'd';
        pBuf[n++] = 'O';
        pBuf[n++] = 'n';
        pBuf[n++] = '!';
    }
    if (alert2_for_tsms & 0x00000020) //������ � ������
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'G';
        pBuf[n++] = 'u';
        pBuf[n++] = 'a';
        pBuf[n++] = 'r';
        pBuf[n++] = 'd';
        pBuf[n++] = 'O';
        pBuf[n++] = 'f';
        pBuf[n++] = 'f';
        pBuf[n++] = '!';
    }
    if (alert1_for_tsms & 0x01000000) //�������
    {
        pBuf[n++] = ' ';
        pBuf[n++] = 'R';
        pBuf[n++] = 'e';
        pBuf[n++] = 's';
        pBuf[n++] = 't';
        pBuf[n++] = 'a';
        pBuf[n++] = 'r';
        pBuf[n++] = 't';
        pBuf[n++] = '!';
    }

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}
*/

// ����� �� ������ ������
unsigned char do_txtpack_V (unsigned char *pBuf, unsigned char G)
{
  unsigned char xdata i,j;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;
  unsigned char xdata M[8];

    // ���������� �������
    pBuf[n++] = 0x20;
    pBuf[n++] = 0x20;

    pBuf[n++] = 'V';
    pBuf[n++] = 'e';
    pBuf[n++] = 'r';
    pBuf[n++] = ':';
    // VERSION
    i = Hex_to_DecASCII_5((unsigned int)(VERSION), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ',';
    // ID_TERM
//  i = Hex_to_DecASCII_5((unsigned int)(ID_TERM), &M[0]);
//  for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = Hex_to_HexASCII_Hi(BUILD);
    pBuf[n++] = Hex_to_HexASCII_Lo(BUILD);
    //pBuf[n++] = ',';
    // BUILD
//  i = Hex_to_DecASCII_5((unsigned int)(BUILD), &M[0]);
//  for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = Hex_to_HexASCII_Hi(ID_TERM);
    pBuf[n++] = Hex_to_HexASCII_Lo(ID_TERM);
    pBuf[n++] = ',';
    // ID_TERM1
    i = Hex_to_DecASCII_5((unsigned int)(ID_TERM1), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    //pBuf[n++] = ',';
    // ID_TERM2
    i = Hex_to_DecASCII_5((unsigned int)(ID_TERM2), &M[0]);
    if (i==1) pBuf[n++] = '0';
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = '.';

    if (G) return n;

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}

// ����� �� SMS-������� (���������)
unsigned char do_txtpack_T (unsigned char *pBuf, unsigned char G, unsigned char type)
{
  unsigned char xdata i;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;

    // ���������� �������
    pBuf[n++] = 0x20;
    pBuf[n++] = 0x20;

    if (type==0)
    {
        pBuf[n++] = 'O';
        pBuf[n++] = 'k';
        pBuf[n++] = '.';
        pBuf[n++] = ' ';
    }
    else //(1)
    {
        pBuf[n++] = 'S';
        pBuf[n++] = 't';
        pBuf[n++] = 'a';
        pBuf[n++] = 'r';
        pBuf[n++] = 't';
        pBuf[n++] = '.';
        pBuf[n++] = ' ';
    }

    if (G) return n;

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}

// ����� �� SMS-������� (���������) "����� ������"
unsigned char do_txtpack_P (unsigned char *pBuf, unsigned char G)
{
  unsigned char xdata i;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;

    // ���������� �������
    pBuf[n++] = 0x20;
    pBuf[n++] = 0x20;

//The password is changed
//The password has been changed.
    pBuf[n++] = 'P';
    pBuf[n++] = 'a';
    pBuf[n++] = 's';
    pBuf[n++] = 's';
    pBuf[n++] = 'w';
    pBuf[n++] = 'o';
    pBuf[n++] = 'r';
    pBuf[n++] = 'd';
    pBuf[n++] = ' ';
    pBuf[n++] = 'i';
    pBuf[n++] = 's';
    pBuf[n++] = ' ';
    pBuf[n++] = 'c';
    pBuf[n++] = 'h';
    pBuf[n++] = 'a';
    pBuf[n++] = 'n';
    pBuf[n++] = 'g';
    pBuf[n++] = 'e';
    pBuf[n++] = 'd';
    pBuf[n++] = '.';

    if (G) return n;

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}

// ����� �� SMS-������� (���������)
unsigned char do_txtpack_N (unsigned char *pBuf, unsigned char G) // �������� ������
{
  unsigned char xdata i;
  unsigned char xdata t;
  unsigned  int xdata b;
  unsigned char xdata n=0;

    // ���������� �������
    pBuf[n++] = 0x20;
    pBuf[n++] = 0x20;

    pBuf[n++] = 'P';
    pBuf[n++] = 'h';
    pBuf[n++] = 'o';
    pBuf[n++] = 'n';
    pBuf[n++] = 'e';
    pBuf[n++] = 's';
    pBuf[n++] = ':';
    pBuf[n++] = ' ';


    // ����� �������� ������
    pBuf[n++] = (sec_for_numtel&7) + 0x30;
    pBuf[n++] = ',';
    // 0 *****************************************************
    len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                          &tel_num[0].num_SO[0],
                                           tel_num[0].len_SO);
    pBuf[n++] = '"';
    for (i=0; i<len_telnum_ascii; i++) pBuf[n++] = tmp_telnum_ascii[i];
    pBuf[n++] = '"';
    pBuf[n++] = ',';

    // 1 *****************************************************
    len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                          &tel_num[1].num_SO[0],
                                           tel_num[1].len_SO);
    pBuf[n++] = '"';
    for (i=0; i<len_telnum_ascii; i++) pBuf[n++] = tmp_telnum_ascii[i];
    pBuf[n++] = '"';
    pBuf[n++] = ',';

    // 5 *****************************************************
    len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                          &tel_num[5].num_SO[0],
                                           tel_num[5].len_SO);
    pBuf[n++] = '"';
    for (i=0; i<len_telnum_ascii; i++) pBuf[n++] = tmp_telnum_ascii[i];
    pBuf[n++] = '"';
    pBuf[n++] = ',';

    // 6 *****************************************************
    len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                          &tel_num[6].num_SO[0],
                                           tel_num[6].len_SO);
    pBuf[n++] = '"';
    for (i=0; i<len_telnum_ascii; i++) pBuf[n++] = tmp_telnum_ascii[i];
    pBuf[n++] = '"';
    pBuf[n++] = ',';

    // 8 *****************************************************
    len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                          &tel_num[8].num_SO[0],
                                           tel_num[8].len_SO);
    pBuf[n++] = '"';
    for (i=0; i<len_telnum_ascii; i++) pBuf[n++] = tmp_telnum_ascii[i];
    pBuf[n++] = '"';

    if (G) return n;

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}

//���������� ���������� ��������� � ����� �� ������ 'A' (0x41)
//������ (������������ ����� = 107 ��������):
// "Sec! In=0000 Out=0000 ADC=127,127,127,127 Pow=12.5v Tc=100C Tm=100C GSM=4 ID1=100 ID2=101 Int1=255 Int2=255 Iteris=OK"
//
unsigned char do_txtpack_S (unsigned char *pBuf, unsigned char G) // Status
{
  unsigned char xdata i,j;
  unsigned char xdata t;
    signed char xdata sC;
  unsigned  int xdata b;
    signed  int xdata sI;
    signed  int xdata * psI;
  unsigned char xdata n=0;
  unsigned char xdata M[8];

    // ���������� �������
    pBuf[n++] = 0x20;
    pBuf[n++] = 0x20;
    // ----------------------------------------
    if (_flag_secbut_on) // *** "Guard" ***
    {
        pBuf[n++] = 'S';
        pBuf[n++] = 'e';
        pBuf[n++] = 'c';
        pBuf[n++] = '!';
        pBuf[n++] = ' ';
    }
    // ----------------------------------------
    pBuf[n++] = 'I';
    pBuf[n++] = 'n';
    pBuf[n++] = '=';
    if (DataLogger[5]&0x01) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x02) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x04) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x08) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'O';
    pBuf[n++] = 'u';
    pBuf[n++] = 't';
    pBuf[n++] = '=';
    if (DataLogger[5]&0x10) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x20) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x40) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    if (DataLogger[5]&0x80) pBuf[n++] = '1';
                       else pBuf[n++] = '0';
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'A';
    pBuf[n++] = 'D';
    pBuf[n++] = 'C';
    pBuf[n++] = '=';
    i = Hex_to_DecASCII_5((unsigned int)(DataLogger[6]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ',';
    i = Hex_to_DecASCII_5((unsigned int)(DataLogger[7]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ',';
    i = Hex_to_DecASCII_5((unsigned int)(DataLogger[8]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ',';
    i = Hex_to_DecASCII_5((unsigned int)(DataLogger[9]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ' ';
    // ----------------------------------------
    i = Hex_to_DecASCII_5((unsigned int)(DataLogger[10]), &M[0]);
    pBuf[n++] = 'P';
    pBuf[n++] = 'o';
    pBuf[n++] = 'w';
    pBuf[n++] = '=';
    for (j=0; j<i; j++)
    {
        if (((j+1)==i)&&(i>1)) pBuf[n++] = '.';
        pBuf[n++] = M[j];
    }
    pBuf[n++] = 'v';
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'T';
    pBuf[n++] = 'c';
    pBuf[n++] = '=';
    sC = (signed char)DataLogger[11];
    if (sC<0) { pBuf[n++] = '-'; sC = 0-sC; }
    i = Hex_to_DecASCII_5((unsigned int)sC, &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = 'C';
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'T';
    pBuf[n++] = 'm';
    pBuf[n++] = '=';
    if ((DataLogger[12]==0x07)&&(DataLogger[13]==0xFF))
    {
        pBuf[n++] = '?';
    }
    else
    {
        psI = &DataLogger[12];
        sI = (*psI) >> 4;
        if (sI<0) { pBuf[n++] = '-'; sI = 0-sI; }
        sI = sI>>4;
        i = Hex_to_DecASCII_5((unsigned int)sI, &M[0]);
        for (j=0; j<i; j++) pBuf[n++] = M[j];
        pBuf[n++] = 'C';
    }
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'G';
    pBuf[n++] = 'S';
    pBuf[n++] = 'M';
    pBuf[n++] = '=';
    pBuf[n++] = 0x30+GSM_signal;
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'I';
    pBuf[n++] = 'D';
    pBuf[n++] = '1';
    pBuf[n++] = '=';
    i = Hex_to_DecASCII_5((unsigned int)(K8_ID[0]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
//  Hex_to_HexASCII_Hi();
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'I';
    pBuf[n++] = 'D';
    pBuf[n++] = '2';
    pBuf[n++] = '=';
    i = Hex_to_DecASCII_5((unsigned int)(K8_ID[1]), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'I';
    pBuf[n++] = 'n';
    pBuf[n++] = 't';
    pBuf[n++] = '1';
    pBuf[n++] = '=';
    i = Hex_to_DecASCII_5((unsigned int)(K8_TIME0), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ' ';
    // ----------------------------------------
    pBuf[n++] = 'I';
    pBuf[n++] = 'n';
    pBuf[n++] = 't';
    pBuf[n++] = '2';
    pBuf[n++] = '=';
    i = Hex_to_DecASCII_5((unsigned int)(K8_TIME1), &M[0]);
    for (j=0; j<i; j++) pBuf[n++] = M[j];
    pBuf[n++] = ' ';
    // ----------------------------------------
	pBuf[n++] = 'I';
    pBuf[n++] = 't';
    pBuf[n++] = 'e';
    pBuf[n++] = 'r';
	pBuf[n++] = 'i';
	pBuf[n++] = 's';
    pBuf[n++] = '=';
	if(respondsIteris==retOk){
		pBuf[n++] = 'O';
    	pBuf[n++] = 'K';
		}else{
		pBuf[n++] = 'N';
    	pBuf[n++] = 'O';
		}
    pBuf[n++] = '.';
	// ----------------------------------------
    if (G) return n;

    b=0;
    for (i=0; i<n; i++)
    {
     // b   - ������� ��� ����� � ����� �������
     // --------------------------------------
     // i   - ���� � �������� �������
     // 0-6 - ���  � �������� �������
     // --------------------------------------
     // b%8 - ���  � ����� �������
     // b/8 - ���� � ����� �������
     // --------------------------------------
        t = pBuf[i];
        pBuf[i] = 0;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x01)   ) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x02)>>1) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x04)>>2) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x08)>>3) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x10)>>4) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x20)>>5) << (b%8); b++;
        pBuf[(unsigned char)(b/8)] |= ((t & 0x40)>>6) << (b%8); b++;
      //pBuf[(unsigned char)(b/8)] |= ((pBuf[i] & 0x80)>>7) << (b%8); b++;
    }
    if (b%8) return (b/8+1);
        else return (b/8);
}

// ���������� ��������� ��� �������� � ����� �� ������ 0x03, 0x0B ��� 0x0F
unsigned char do_pack030F (unsigned char *pBuf)
{
  unsigned char xdata n=0;

// ****************
// ��������� ������   8 ����
// ****************

    pBuf[n++] = 0; // ���-�� ����������� �����
    pBuf[n++] = 0; // ������� �����: 0 - ������ �����, 1 - ������ ���������.
                   // ������� �����: 0 - ��� ���������� ��������, 1 - ���� ���������� ��������.
    pBuf[n++] = 0; // �� ��������� ��� ������
    pBuf[n++] = VERSION; // ������ ��
    pBuf[n++] = ID_TERM; // ������ ���������
    pBuf[n++] = 0;//current_grab_satellite; // ������� �����:  ���-�� ����������� ���������.
                                        // ������� �����: ����������� ������ (3-2D, 4-3D)
//    if (current_sms_signal_level==99) pBuf[n++] = 0;
//    else pBuf[n++] = (current_sms_signal_level/5)&0x0f; // ������� �������� - ������� GSM ������� (0-4).
    pBuf[n++] = GSM_signal;
// ***********
// ���� ������    12 ����
// ***********
    pBuf[n++] = count_restart_cpu;  // Rst   - ���-�� ��������� ��������� (���������� �������� 0x0F).
    pBuf[n++] = 0;//count_restart_gps;  // rGPS  - ���-�� ������� (softreset) GPS (���������� �������� 0x0F).
    pBuf[n++] = count_restart_gsm;  // rGSM  - ���-�� ������� ������ GSM (���������� �������� 0x0F).
    SFRPAGE = LEGACY_PAGE;
    pBuf[n++] = RSTSRC; //������� �������� ����������
    pBuf[n++] = 0;//(unsigned char)_flag_gps_error; // GPS   - 1-���� ������� GPS, 0 - ��� GPS
/*    if (mAuto.b.delta_t)
    {
        pBuf[n++] = LimitsData.Each.delta_t % 60; // dTs   - ������������� ����� � ��� ���������� ����� �������.
        pBuf[n++] = (LimitsData.Each.delta_t / 60) >> 8; // dThi  - ������������� ����� � ��� ���������� ����� ������� - ������� ����� - int .
        pBuf[n++] = LimitsData.Each.delta_t / 60; // dTlo  - ������������� ����� � ��� ���������� ����� ������� - ������� ����� - int.
    }
    else*/
    {
        pBuf[n++] = 0;
        pBuf[n++] = 0;
        pBuf[n++] = 0;
    }
    pBuf[n++] = WrHeader.c[1]; // Kt_hi - �������� ������ � ������, ��. �����.
    pBuf[n++] = WrHeader.c[2]; // Kt_lo - �������� ������ � ������, ��. �����.
    pBuf[n++] = (unsigned char)(port_sensor_current&0xff); // PortS - ������� ��������  �� ��������� �������� ���� "����� �������".
    pBuf[n++] = (unsigned char)CurrentEvents.b.power_main_lo; // Pow   - ������� �������� �������: 0 - �������� �������, 1 - ���������� �������.

// ****************
// ������ ���������  108 ����
// ****************
    full_buf_num_tel(&pBuf[n]);


    //   0 -   7 - header
    //   8 -  19 - data
    //  20 - 127 - ������ ���������
    // 128       - ����� ������ �� ������� ���������
    // 129 - 130 - ������
    // 131 - 135 - ������, ���, ��, ��� �����
    
    pBuf[128] = sec_for_numtel;
    pBuf[129] = 0;//DataLogger[0x15];
    pBuf[130] = 0;//DataLogger[0x16];

    pBuf[131] = VERSION; // ������ ���������
    pBuf[132] = BUILD;   // �����������
    pBuf[133] = ID_TERM; // ������������� ���������
    pBuf[134] = ID_TERM1; // ���
    pBuf[135] = ID_TERM2; //     �����

    return 136;
}

// ���������� ��������� ��� �������� � ����� �� ������ 0x53, 0x5B.
unsigned char do_pack53 (unsigned char *pBuf)
{
  unsigned char xdata n=0;

// ****************
// ������ ���������  11� * 12� = 132 ����
// ****************
    full_buf_num_tel2(&pBuf[n]);

    //   0 -       53
    //   1 - 132 - ������ ���������
    // 133       - ����� ������ �� ������� ���������
    
    pBuf[132] = sec_for_numtel;
    pBuf[133] = 0x6D; //m
    pBuf[134] = 0x54; //T

    return 135;
}
