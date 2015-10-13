//
// ������ "siem_sms.c"
//
// ������������ ��� �������� ������ ����� SMS - ��������� � ������� �������� Siemens.
//
// 1. ��� ������ ��������� ������� "time_wait_SMSPhone"
//    ������� ������ ���������������� �� ������� (������ �������) � ������ "Main.c"
// 2. ���������� ��������� �� ������ � ������������ UART�� (��. ���� << #define tx_data_sim tx_data_uart0>>).
//

#include <c8051F120.h>
#include <string.h>
#include <stdlib.h>
#include "Main.h"
#include "uart.h"
#include "io.h"
#include "rele.h"
#include "adc.h"
#include "io_sms.h"
#include "gprs\gprs.h"
#include "config.h"
#include "kod8.h"


// *****************************************************
// *   ��������� �� ������ � ���������� ������ uart    *
// *****************************************************
#define  tx_data_sim      tx_data_uart0
#define  sizeBuf_rx_Sim   sizeBuf_rx_uart0
#define  ClearBuf_Sim     ClearBuf_uart0
#define  Read_byte_sim    Read_byte_uart0
#define  Send_char_sim    send_char_uart0


// ******************************
// *         ���������          *
// ******************************

// �������� ��� �������� �� ���������
#define Com_B_QueryVersion  0x4A    // 0xDA  // ������ ������ 
#define Com_T_Version       0x3A    // 0xCA  // ����� ������
#define Com_B_NotReload     0x4B    // 0xDB  // ���������� �� ���������
#define Com_B_Reload        0x4C    // 0xCC  // ��������� ����������
#define Com_T_Restart       0x3C    // 0xCC  // �������� ���� �� ������� (� ���������)
#define Com_B_ReadyReload   0x4D    // 0xDD  // ���� ������ � �������
#define Com_T_QueryHeader   0x5A    // 0xEA  // �������� ����������� ���������
#define Com_T_QueryData     0x5B    // 0xEB  // �������� ����������� ���� ������
#define Com_T_NextBlock     0x5C    // 0xEC  // ���� ������ �������, ��������� �� ��������� ����
#define Com_B_OkNextBlock1  'O'     // 'O'   // ���� OK, ��������� �� ��������� ����
#define Com_B_OkNextBlock2  'K'     // 'K'   //
#define Com_T_EndReload     0x5E    // 0xEE  // �������� ��������� �������� ���� ������
#define Com_T_HardError     0x5D    // 0xED  // ��������� ������ ������
#define Com_T_Error         0x5F    // 0xEF  // ��������� ������

// ************************************************************************************************
//
// ************************************
// *            ����������            *
// ************************************
//


// Modem
// ****************
//            bit   md_f_modem_conect = no; // ���� "������� ��������� ����������"
              bit   md_f_need_send    = no; // ���� "������������� ������� ������ �� ������"
unsigned char xdata md_who_call       = 0;  // ��� ����� ����� (0-���� ���
                                            //                  1-��������� ��������
                                            //                  2-MD
                                            //                  3-��� ��������
                                            //                  4-VC)
unsigned char xdata md_mode           = 0;  // ����� (0-���������� ������ �� sms
                                            //        1-���������� ������ �� ������
                                            //        2-��������� �������� �� ������
                                            //       -3-����� ����� (����� ��) ���� 2 ���. ������� ��� ������ �� ������� (�� �������� �� SMS)
                                            //       -4-����� ����� (����� ����) ���������� ������� �����)
                                            //        5-��������� ����� �� ������
                                            //        6-GPRS
unsigned char xdata md_npack_mod_inc  = 0;  // ����� �������� ��������� ������
unsigned char xdata md_CS; // ����������� �����

unsigned char xdata md_Len; // ����� ����� ������ �����. �� ������

unsigned char xdata md_resetGSM = 0; //   0 - ������ �� ����
                                     //   1 - ���������� �������������������� GSM-�����
                                     // >=2 - ���������� ������� ����� GSM-������

//unsigned long xdata sTime_md_mode;
unsigned long xdata sTime_Net =0;

unsigned char xdata  count_atz=0; // ������� ������� ATZ
//unsigned char xdata  count_cops=0; // ������� ��� "AT+COPS=0"
unsigned char xdata  count_worksms=0; // ������� ��� ������ � ��������� ������.
unsigned char xdata  count_p;   // ������� ������� ������
//unsigned char xdata  count_dcd=0; // ������� ������� ��������� ���������� ��� ����������� DCD.
unsigned char xdata  count_error_no_reply_from_sms=0; // ������� ������� �������� ���������
unsigned char xdata  count_try_tn=0; // ������� ������� ��������� ������ ���������

unsigned  int xdata  count_uart_data; // ������� �����. ������ � ������ �� Simens
//unsigned char xdata  tx_data_sim; // �������� �
unsigned char xdata  rx_data_sim; // ����� �� usart0


unsigned char xdata  current_sms_signal_level=0xFF; // ������� ������� GSM
bit                 _flag_sms_signal_level_null=no; // ���� ������. �� 0 ������� GSM �������.

#define MAX_SIZE_BUF_READ_SIM 360
unsigned char xdata  buf_read_sim[MAX_SIZE_BUF_READ_SIM+1];

// ������ ��� �������� �� Simens
#define MAX_SIZE_TRANSMIT_BUFFER 190
unsigned char xdata  sim_line_tx[MAX_SIZE_TRANSMIT_BUFFER+1];
unsigned char xdata  sim_line_tx_len=0; // ����� sms - ��������� ��� �������

unsigned char xdata _c_ans_kom_rx=0; // ����� ������� (�����) ������
bit                 _flag_new_line_sim=no; // ���� ��� ������ ����� ������ �� Simens

unsigned char xdata  sms_receive_buffer[141]; // ����� ����� �������� SMS - ���������
unsigned char xdata  sms_receive_buffer_len=0; // ����� ��������� SMS - ���������
unsigned char xdata  n_mem_sms=0; // ����� ��������� sms � ������ simens
unsigned char xdata  index_sms=0; // ����� ������������ sms � ������ simens

unsigned char xdata  c_flag_sms_send_out=0; // ���� ��� ���������:
                                            // 1 - ����������
                                            // 0 - �� ����������
                                            // 2 - � �������� ��������
                                            // 3 - ����������, �� �� ����������.
bit  _flag_new_sms=no;           // ���� "������� ����� SMS - ���������".
bit  _flag_need_delete_sms=no;   // ���������� ������� �������� SMS - ���������.
bit  _flag_need_send_sms=no;     // ���������� ������� �������� SMS - ���������.
bit  _flag_need_wait_SR;         // ���������� ����� ������������� �������� SMS.
bit  _f_SMS_accept_type_8bit=no; // ���� ��� SMS ��������� 8-������.
bit  _f_SMS_noanswer=no;         // ���� sms ���������������� � ���������,
                                 // �� �������� �� ����.

bit  _flag_need_delete_badsms=no; // ���������� ������� �������� SMS - ���������.
unsigned char xdata  n_mem_badsms=0; // ����� ��������� ��������� � ������ simens


bit  _flag_tel_ready  =no; // ���� ���������� �������� (������ SC � DC �������).
bit  _flag_tel_ready_R=no; // ���� ���������� �������� ������ � ������ (������� SC � DC ���).

unsigned char xdata need_write_telDC_ix=0;


//#define KOL_NT  21    //���-�� ������� ���������
struct TEL_NUM xdata tel_num[KOL_NT]; // ����� sms-������ � ������ ����. �������

unsigned char xdata who_send_sms=0;    // �� ���� ������ SMS - ������ ������ (1-9)
                                       // ������ 10 - ����������� �����.

unsigned char xdata code_access_sms=0; // �� ���� ������ SMS - ��� (������) �������
                                       // 0 - ������ ��������
                                       // 1 - DC1-DC4
                                       // 5 - User1
unsigned char xdata sec_for_numtel;    // ������ �� ������� ���������.
                                       // ��������� (1) ��� �� ��������� (0) ����. ������
                                       // ��� 0 - SMS
                                       // ��� 1 - Data call
                                       // ��� 2 - Voice call

//unsigned char xdata tp_st_index =0; // TP-Status. ������ ��������� "������������� ��������"
//unsigned char xdata tp_st_state =0; // TP-Status. ��������� ��������� ��������� "������������� ��������"
                                    // 0-���� ���
                                    // 1-������ ������
                                    // 2-������������� ��������
unsigned char xdata tp_st_result=0; // TP-Status. ��������� "����������/������������"
                                    // 0-���� ����������
                                    // 1 - "����������"
                                    // 2 - "������������"


// ��������� ���. ������ ��� ��������� � ��������������.
unsigned char xdata tmp_telnum[kolBN];       // ���.����� ��������
unsigned char xdata len_telnum=0;            // ����� ���.������
unsigned char xdata tmp_telnum_ascii[kolCN]; // ���.����� � ascii
unsigned char xdata len_telnum_ascii=0;      // ����� ���.������ � ascii

unsigned char xdata tmp_telnum_pos;

bit _flag_need_read_all_tels=yes; // ������������� ��������� ��� ������ ��������� (����/���)

unsigned char xdata state_sim_rx=1; // ��������� �������� ��������� Usart
unsigned char xdata state_sim_wk=75; // ��������� �������� ������ c Simens
unsigned char xdata state_per_sim_wk=1; // ��������� �������� ������� Simens 
unsigned char xdata state_init_sim_wk; // ��������� �������� ������������� Simens 

unsigned char xdata Scounter;
unsigned char xdata strS[4];
//unsigned char xdata tmp_CCED[39];
/*
unsigned char xdata tmp_LAC[2]={0};
unsigned char xdata tmp_CI[2] ={0};
unsigned char xdata tmp_RxLev=0;
*/

//unsigned char xdata md_gprs_N=0;    // ��� ��������� ������ ��� gprs
//unsigned char xdata md_gprs_Name[10]={0}; //
//unsigned char xdata md_gprs_NameL=0;    //
//unsigned char xdata md_gprs_Pass[10]={0}; //

//_context_gsm xdata  context_gsm = {"0123456789ABCDEFGHIJ", 1, 20}; //for HTTP
_context_gsm xdata  context_gsm = {"", 0, 0}; //for HTTP


// ************************************************************************************************
void Command_3p (void);
void Command_ATH (void);
void Delete_sms (void);
void Restart_phone (void);
void RestartToLoader(void); // �������� ���������� ����������.


unsigned char xdata D_date;
unsigned char xdata M_date;
unsigned  int xdata G_date;

unsigned char code Month[12][3] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//
// ************************************
// *              �������             *
// ************************************
//

long int GetN (void)
{
  long xdata D = D_date;
  long xdata M = M_date;
  long xdata G = G_date;
  long xdata S;
  long xdata C;
  long xdata N;
  long xdata D1;

    S = (long)((12-M)/10);
    M = 12 * S + M - 2;
    G = G - S;
    C = (long)(G/4) - (long)(G/100) + (long)(G/400);
    D1 = 30 * (M-1) + D + (long)(0.59 * M);
    N = D1 + 365 * G + C;
    return N;
}


void Get_HTTP_Time(unsigned char xdata *pInput)
{
  unsigned long xdata N_day;
  xdata _t_int        NumWeek;
  unsigned char xdata H_time;
  unsigned char xdata M_time;
  unsigned char xdata S_time;
  unsigned char xdata DayWeek;

  unsigned char xdata c;

    TimeHttp.Year = G_date = atoi(&pInput[18]);
    TimeHttp.Day = D_date = atoi(&pInput[11]);
    // time
	TimeHttp.Hour = H_time = atoi(&pInput[23]);
    TimeHttp.Min  = M_time = atoi(&pInput[26]);
    TimeHttp.Sec  = S_time = atoi(&pInput[29]);

    for (c=0; c<12; c++)
        if (!strncmp(&pInput[14], Month[c], 3)) {M_date = c+1; break;}

    TimeHttp.Mon = M_date;

	N_day  = (unsigned long)(GetN());

   int_timer2_no;
    NumWeek.v_int = (unsigned int)((N_day-723126)/7);
    DayWeek = ((N_day+2)%7);
    Timer_inside = (((unsigned long)DayWeek)*24*60*60)+
                   (((unsigned long)H_time)*60*60)+
                   (((unsigned long)M_time*60))+
                    ((unsigned long)S_time);
    if (ForbiddenAll)
    {
        GetBack_m = 1; //��� ������ ����������� � ������� ��������� ����� � ������
        Day_timer = Timer_inside % 86400 / 60; //86400 - ������ � ������
    }
    Timer_inside += (unsigned long)NumWeek.v_int*604800L;
   int_timer2_yes;
}


// ������������� ������
//
void Init_sms_phone (void)
{
  unsigned char xdata i,j;

    for (j=0; j<KOL_NT; j++)
    {
        tel_num[j].len_SO = 0; // ���-�� ���� ������ � ������� SO.
        tel_num[j].sms_Al = 0; // ��������� sms-������� �� ��� ������ �� ��������.
        for (i=0; i<kolBN; i++) tel_num[j].num_SO[i] = 0xFF; // ���. ����� � ������� SO.
        tel_num[j].need_write = 0; // ������������� �������� ����� �������� � sim-�����.
        tel_num[j].accept     = 0; // ������ ����� � ������� ��������.
        tel_num[j].need_send_sms_Al=0; // ��� ������������� �������� �� ���� ����� ��������� sms-�������.
    }

    reset_wdt;

    ReadScratchPad1();
    if (FlashDump[8]=='S') sec_for_numtel = FlashDump[9];
    else                   sec_for_numtel = 0;
    reset_wdt;
}


// ����� ������ �� Simens (�� ������)
void Receive_sim (void)
{
    switch (state_sim_rx)
    {
        case 1: //state start
            if ((rx_data_sim == 0x0D)||(rx_data_sim == 0x0A)) return;
            buf_read_sim[0] = rx_data_sim;
            count_uart_data = 1;
            if (rx_data_sim=='m') state_sim_rx = 15;
            else
            if (rx_data_sim=='N') state_sim_rx = 15;
            else
                state_sim_rx = 10; // -> goto state receive
        break;

        case 10: //state receive
            // ��������� ������ �� Simens
            if ((rx_data_sim == 0x0D)||(rx_data_sim == 0x0A)) // ����� ������
            {
                buf_read_sim[count_uart_data]=0;
               _flag_new_line_sim = yes; // �������� ����� ������ �� Simens
                state_sim_rx = 1; // -> goto state start
                return;
            }
            // ������ ������ � �����.
            buf_read_sim[count_uart_data] = rx_data_sim;
            count_uart_data++;
            if (count_uart_data > MAX_SIZE_BUF_READ_SIM)
            {
                state_sim_rx = 99; // -> goto error
                return;
            }

            if ((buf_read_sim[0]=='>')&&(buf_read_sim[1]==' ')) // ��� ����������� ���������� sms - ���������
            {
               _flag_new_line_sim = yes; // �������� ����� ������ �� Simens
                state_sim_rx = 1; // -> goto state start
            }
        break;

        // ����� ������ �� ������
        case 15: 
            if (rx_data_sim=='d')
            {
                 buf_read_sim[count_uart_data++] = rx_data_sim;
                 state_sim_rx = 17;
                 //time_wait_phone_receive=0;
            }
            else
            if (rx_data_sim=='O')
            {
                 buf_read_sim[count_uart_data++] = rx_data_sim;
                 state_sim_rx = 16;
            }
            else state_sim_rx = 99;
        break;

        case 16: // ����� "NO CARRIER"
            buf_read_sim[count_uart_data++] = rx_data_sim;
            if (count_uart_data >= 10)
            {
                md_Len=10;
               _flag_new_line_sim = yes; // �������� ����� ������ �� Simens
                state_sim_rx = 1; // -> goto state start
            }
        break;

        case 17: // ����� ���������
            time_wait_SMSPhone2 = 0;
            //if (time_wait_phone_receive>03) state_sim_rx = 99;
            buf_read_sim[count_uart_data++] = rx_data_sim;
            if (count_uart_data >= 8)
            {
                md_Len=buf_read_sim[3]+8+1; // Data+Header+CS
                if (md_Len>149)
                   state_sim_rx = 99;
                else
                {
                   state_sim_rx = 20;
                   //time_wait_phone_receive=0;
                }
            }
        break;

        case 20: // ����� ������ �� ������

            time_wait_SMSPhone2 = 0;

          //if (time_wait_phone_receive>05) state_sim_rx = 99;

            buf_read_sim[count_uart_data++] = rx_data_sim;
            if (count_uart_data>=md_Len)
            {
               _flag_new_line_sim = yes; // �������� ����� ������ �� Simens
                state_sim_rx = 1; // -> goto state start
            }
        break;

        case 99: // state error after AT command
            if ((rx_data_sim == 0x0D)||(rx_data_sim == 0x0A)) state_sim_rx = 1; // -> goto state start
        break;

        default:
            state_sim_rx = 1; // -> goto state start
        break;
    }
}

// �������������� 'F' � 0x0F
unsigned char HexASCII_to_Hex (unsigned char HA)
{
    if ((HA >= '0') && (HA <= '9')) return HA-0x30;
    if ((HA >= 'A') && (HA <= 'F')) return HA-0x37;
    if ((HA >= 'a') && (HA <= 'f')) return HA-0x57;
    return 0xf0;
}

// �������������� 0xF0 � 'F'
unsigned char Hex_to_HexASCII_Hi (unsigned char Hx)
{
  unsigned char xdata HA;
    HA = (Hx&0xf0)>>4;
    if ((HA >= 0x00) && (HA <= 0x09)) return HA + 0x30;
    if ((HA >= 0x0A) && (HA <= 0x0F)) return HA + 0x37;
}

// �������������� 0x0F � 'F'
unsigned char Hex_to_HexASCII_Lo (unsigned char Hx)
{
  unsigned char xdata HA;
    HA = (Hx&0x0f);
    if ((HA >= 0x00) && (HA <= 0x09)) return HA + 0x30;
    if ((HA >= 0x0A) && (HA <= 0x0F)) return HA + 0x37;
}

// ****************************************************************
// ��������� ������ �������� � ������� SO �� ������� ASCII
// �� ������� ASCII: "+79027459999"
// � ������   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
// ���������� ���-�� ���� 7
unsigned char GetTel_ASCII_to_SO (unsigned char *tel_SO, unsigned char *tel_ascii, unsigned char Len)
{
  unsigned char xdata i,j;
    if (Len>kolCN) return 0;
    if (tel_ascii[0]=='+')
    {
        i=1;
        tel_SO[0] = 0x91;
    }
    else
    {
        i=0;
        tel_SO[0] = 0x81;
    }

    for (j=1; i<Len; )
    {
        tel_SO[j]  =  (HexASCII_to_Hex(tel_ascii[i++]))&0x0f;
        if (i<Len)
         tel_SO[j] |= ((HexASCII_to_Hex(tel_ascii[i++]))&0x0f)<<4;
        else
         tel_SO[j] |= 0xf0;
        j++;
    }
    return j;
}

// ��������� ������ ��������
// �� �������: "919720479599F9"
//  � ������: 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
unsigned char GetTel_ATSO_to_SO (unsigned char *tel_current, unsigned char *tel_Buf, unsigned char len)
{
  unsigned char xdata i,j;
    i = 0;
    j = 0;
    while (1)
    {
        if (i>len) { i=0; break; }
        if (tel_Buf[j+1]=='F') tel_current[i]  = 0x0F;
                          else tel_current[i]  = (((tel_Buf[j+1])-0x30)&0x0f);
        if (tel_Buf[j]  =='F') tel_current[i] |= 0xF0;
                          else tel_current[i] |= (((tel_Buf[j])  -0x30)&0x0f)<<4;
        i++;
        if ((tel_Buf[j]=='F') || (tel_Buf[j+1]=='F') || (i==len)) break;
        j += 2;
    }
    return i;
}
/*
// ��������� ������ ��������
// �� �������:      "919720479599F9"
//  � ������ ASCII: "+79027459999"
unsigned char GetTel_ATSO_to_ASCII (unsigned char *tel_ascii, unsigned char *tel_Buf)
{
  unsigned char xdata i=0;
  unsigned char xdata j=0;

    if ((tel_Buf[0]=='9')&&(tel_Buf[1]=='1'))
    {
        tel_ascii[0] = '+';
        i++;
    }
    j += 2;
    while (1)
    {
        if (i>kolCN) { i=0; break; }
        if (tel_Buf[j+1]=='F') break;
        tel_ascii[i++] = tel_Buf[j+1];
        if (tel_Buf[j] =='F') break;
        tel_ascii[i++] = tel_Buf[j];
    }
    return i;
}
*/
// ��������� ������ ��������
// �� ������� SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
// � ������   ASCII: "+79027459999"
unsigned char GetTel_SO_to_ASCII (unsigned char *tel_ascii, unsigned char *tel_SO, unsigned char len)
{
  unsigned char xdata i=0;
  unsigned char xdata j=0;

    if (tel_SO[0]==0x91)
    {
        tel_ascii[0] = '+';
        i++;
    }
    j++;

    while (1)
    {
        if (i>kolCN) { i=0; break; }
        if (((tel_SO[j])&0x0f)==0x0f) break;
        tel_ascii[i++] = Hex_to_HexASCII_Lo(tel_SO[j]);
        if (((tel_SO[j])&0xf0)==0xf0) break;
        tel_ascii[i++] = Hex_to_HexASCII_Hi(tel_SO[j]);
        j++;
        if (j>=len) break;
    }
    return i;
}

// ����� ���������� ����� �� ASCII � hex : "10," = 10 ��� 0x0A
unsigned char Get_byte_fr_ASCII (unsigned char *Buf)
{
//  unsigned char xdata i,j,A,B;
    if (Buf[0]==',') return 0;
    else
    if (Buf[1]==',') return Buf[0]-0x30;
    else
    if (Buf[2]==',') return (Buf[0]-0x30)*10+(Buf[1]-0x30);
    else
    if (Buf[3]==',') return (Buf[0]-0x30)*100+(Buf[1]-0x30)*10+(Buf[2]-0x30);
    return 0;
}

// ����� ���������� ����� �� ASCII ��� ������ ������ ������ � sms: "10," = 0x10
unsigned char Get_num_sms_fr_ASCII (unsigned char *Buf)
{
    if ((Buf[0]==',')||(Buf[0]==0)) return 0;
    else
    if ((Buf[1]==',')||(Buf[1]==0)) return Buf[0]-0x30;
    else
    if ((Buf[2]==',')||(Buf[2]==0)) return ((Buf[0]-0x30)<<4)+(Buf[1]-0x30);
    return 0;
}
/*
void S_Nxt(void)
{
  unsigned char xdata i=Scounter;
    while (1)
    {
        if (buf_read_sim[Scounter]==',') { Scounter++; return; }
        Scounter++;
        if ((Scounter-i)>=5) return;
    }
}
bit S_TakeNxt(void)
{
  unsigned char xdata i=0;

    strcpy(&strS[0], "0");

    while (1)
    {
        if ((buf_read_sim[Scounter]==',')||(buf_read_sim[Scounter]==0)) { Scounter++; return 1; }

        strS[i++] = buf_read_sim[Scounter++];
        strS[i] = 0;

        if (i>=5) return 0;
    }
}*/

/**
* @function Comp_telnum.
* ��������� ���� ���������� �������.
*/

bit Comp_telnum(unsigned char *ctelnum, unsigned char clen_telnum, unsigned char ID)
{
  unsigned char xdata i;

    if (tel_num[ID].len_SO<4) return (false);

    for (i=1; i<=clen_telnum-2; i++)
    {
        if (tel_num[ID].num_SO[tel_num[ID].len_SO-i] != ctelnum[clen_telnum-i])
        {
            // �� ��� �����
            return (false);
        }
    }
    // ��������� ������ �������
    return (true);
}

// ***********************************************
// * �������������� ������ �� Siemens ���������� *
// ***********************************************
void AnaliseLine (void)
{
  unsigned  int xdata i;
  unsigned char xdata j;
  unsigned char xdata iN;

  /*unsigned long xdata N_day;
  xdata _t_int        NumWeek;
  unsigned char xdata H_time;
  unsigned char xdata M_time;
  unsigned char xdata S_time;
  unsigned char xdata DayWeek;
  //signed char xdata Dif_time;*/

   _flag_new_line_sim = no;

    // ��� ������ �� Simens
   _c_ans_kom_rx = 0; // ���� Unknown

    //  1 - OK
    //  2 - RING
    //  3 - NO CARRIER
    //  4 - ERROR
    //  5 - CONNECT
    //  6 - NO DIALTONE
    //  7 - BUSY
    // 10 - "+CLCC: "  ����������� ������ ��������� �������� �� ����� �������.
    // 11 - "+CSCA: "  ������ ������ sms - ������
    // 12 - "+CPBR: "  ������ ������ �� �����. ������
    // 13 - "+CSQ: "   �������� �������
    // 22 - "+CREG: "  � ����
    // 24 - "+CREG: "  �� � ����
    // 22 - "+COPS: "  � ����
    // 24 - "+COPS: "  �� � ����
    // 17 - "^SMONC: " RxLev indication
    // 28 - "+CCID: "  ������ ID sim-�����.
    // 18 - "+CDSI: "  ������ ������ ��������� "������������� � �������� sms"
    // 18 - "+CDS: "   ������� ��������� "������������� � �������� sms"
    // 14 - "+CMGL: "  ������� ��������� sms - ���������
    // 15 - "+CMGS: "  sms - ��������� �������� �������
    // 16 - "> "       ����������� ���������� sms - ���������
    // 19 - "07"       ������� ������������� � �������� sms
    // 20 - "07"       sms - ���������
    // 21 - "07"       sms - uncknown

    // Com_B_QueryVersion (4A) - ������ ������
    // Com_B_NotReload    (4B) - ���������� �� ���������
    // Com_B_Reload       (4C) - ������� �� ���������
    // 6d - ������ �� ������

    if (strncmp("ERRO", &buf_read_sim[0], 4) == 0)
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else
    if (strncmp("OK",    &buf_read_sim[0], 2) == 0)
    {
        _c_ans_kom_rx = 1; // OK
    }
    else
/*  if (strncmp("RING",  &buf_read_sim[0], 4) == 0)
    {
        _c_ans_kom_rx = 2; // RING
        //DEBUG
        send_c_out_max(0xBF); //"closing connect (GSM)'"
        send_c_out_max(0x00);
    }
    else*/
    // 0 - Ready (ME allows commands from TA/TE)
    // 3 - Ringing (ME is ready for commands from TA/TE, but the ringer is active)
    // 4 - Call in progress (ME is ready for commands from TA/TE, but a call is in progress)
    if (strncmp("+CPAS: 0",  &buf_read_sim[0], 8) == 0)
    {
      //_c_ans_kom_rx = 2; // - call in progress (RING)
        _c_ans_kom_rx = 3; // - ready (NO CARRIER)
        //DEBUG
        //send_c_out_max(0xBF); //"closing connect (GSM)'"
        //send_c_out_max(0x01);
    }
    else
    //---------------------
    //+CRING: REL ASYNC
    //+CRING: ASYNC
    //+CRING: VOICE
    if (strncmp("+CRING",  &buf_read_sim[0], 6) == 0)
    {
        _c_ans_kom_rx = 2; // RING
        if (strncmp("VOI",  &buf_read_sim[8], 3) == 0)
            md_who_call = 4; //voice
        else
            md_who_call = 2; //data
    }
    else
    if (strncmp("NO CAR",  &buf_read_sim[0], 6) == 0)
    {
        _c_ans_kom_rx = 3; // NO CARRIER
    }
/*  else
    if (strncmp("+CPIN: RE", &buf_read_sim[0], 9) == 0) //+CPIN: READY
    {
        _c_ans_kom_rx = 27; // sim-����� ������
    }*/
    else
    if (strncmp("+CME ERR", &buf_read_sim[0], 8) == 0)
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else
    if (strncmp("+CMS ERR", &buf_read_sim[0], 8) == 0)
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else
    if (strncmp("*E2IPO:", &buf_read_sim[0], 7) == 0)
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else
    //  1 - PDP Invalid Context
    //  2 - PDP Account Invalid
    //  3 - PDP Shutdown Failure
    //  8 - PDP Setup Cancelled
    //  9 - PDP Too Many Active Acco
    // 10 - PDP Conflict with Higher Pri
    // 11 - PDP Too Many Active Users
    // 12 - PDP Non Existant Account
    // 13 - PDP Stop at User Request
    // 14 - PDP Authentication failed
    // 15 - PDP Bearer Failed Connect
    // 16 - PDP Remote Server Busy
    // 17 - PDP Remote Server Refused
    // 18 - PDP Bearer Busy
    // 19 - PDP Line Busy
    // 20 - PDP Unknown Error
    //255 - PDP Invalid Parameter
    if (strncmp("*E2IPA:", &buf_read_sim[0], 7) == 0)
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else
    if (strncmp("CONNE",    &buf_read_sim[0], 5) == 0)
    {
        _c_ans_kom_rx = 5; // CONNECT
    }
    else
/*  if (buf_read_sim[0] == 0x06) // ����� � ������ �� ���������.
    {
        _c_ans_kom_rx = 1; // OK
    }
    else
    if (buf_read_sim[0] == 0x19) // ����� � ������ ��. ������ ��������.
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else*/
    if (buf_read_sim[0] == Com_B_QueryVersion)
    {
        _c_ans_kom_rx = Com_B_QueryVersion; // Com_B_QueryVersion (4A) - ������ ������
    }
    else
    if (buf_read_sim[0] == Com_B_NotReload)
    {
        _c_ans_kom_rx = Com_B_NotReload; // Com_B_NotReload (4B) - ���������� �� ���������
    }
    else
    if (buf_read_sim[0] == Com_B_Reload)
    {
        _c_ans_kom_rx = Com_B_Reload; // Com_B_Reload (4C) - ������� �� ���������
    }
    else
    if ((buf_read_sim[0] == 'm')&&(buf_read_sim[1] == 'd')) // 6d 64   �����
    {
        _c_ans_kom_rx = 0x6d; // ������ �� ������
        if ((buf_read_sim[2]==0x01)&&(buf_read_sim[8]==0x0F)) // ������� �����
        {
            if (buf_read_sim[9]==0x1F)
            {
                ReadScratchPad1();
                FlashDump[5] = 0xAA;
                WriteScratchPad1();
            }
            
            Command_3p(); // ������� � ����� �������
            time_wait_SMSPhone = 0;
            GSM_DTR_OFF;
            while (time_wait_SMSPhone < 2) reset_wdt; // �����
            GSM_DTR_ON;

            Command_ATH(); // �������� ������
            time_wait_SMSPhone = 0;
            while (time_wait_SMSPhone < 2) reset_wdt; // �����

            reset_code = 0xB2;
            RSTSRC |= 0x10; // *** ����� ***
        }
    }
    else
    //
    // +CLIP: "89103010274",129,,,"",0
    //
    if (strncmp("+CLIP:", &buf_read_sim[0], 6) == 0) // ����������� ������ ��������� �������� �� ����� �������.
    {
        if (md_who_call==0) return; // ������� ������ ���� +CRING � ��� ������. ���� ��� ���, �� ����.

        if (buf_read_sim[7]=='"')
        {
            // ���������� ������ ��������
            len_telnum_ascii=0;
            while(1)
            {
                i = buf_read_sim[8+len_telnum_ascii];
                if (i=='"') break;
                else { tmp_telnum_ascii[len_telnum_ascii] = i; len_telnum_ascii++; }
                if (len_telnum_ascii>=kolCN) break;
            }
            len_telnum = GetTel_ASCII_to_SO(&tmp_telnum[0], &tmp_telnum_ascii[0], len_telnum_ascii);

            if (md_who_call==2) // 1-data call
            {
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 5)) md_who_call=2; // MD1
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 6)) md_who_call=2; // MD2D
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 9)) md_who_call=1; // BT
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 11)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 12)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 13)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 14)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 15)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 16)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 17)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 18)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 19)) md_who_call=5; // +10 MD � Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 20)) md_who_call=5; // +10 MD � Voice
                else
                if ((sec_for_numtel&0x02)==0) // ���� �� ����� �� ���� �������� �����, 
                {
                    md_who_call = 2; // �� ����� �������, ��� ��� MD1
                }
                else
                {   // ���� ����� �� ���� ������ �����, � ������ ���������� �� ����,            
                    md_who_call = 0; // �� ������ ������.
                }
                #ifndef MODE_5S
                    if (md_who_call==5) md_who_call=2; // MD (��� ������ �� 5 ��������)
                #endif
            }
            else // 0-voice
            {
/*              if ((Comp_telnum(&tmp_telnum[0], len_telnum, 5)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 6)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 7)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 8))) md_who_call=4; // VC
                else
                if ((Comp_telnum(&tmp_telnum[0], len_telnum, 11)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 12)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 13)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 14)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 15)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 16)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 17)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 18)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 19)) ||
                    (Comp_telnum(&tmp_telnum[0], len_telnum, 20))) md_who_call=4; // VC
                else
                if ((sec_for_numtel&0x04)==0) // ���� �� ����� �� ���� �������� �����, 
                {
                    md_who_call = 4; // �� ����� �������, ��� ��� VC
                }
                else*/
                {   // ���� ����� �� ���� ������ �����, � ������ ���������� �� ����,            
                    md_who_call = 0; // �� ������ ������.
                }
            }
        }
        else // ����� �� ���������
        {
            md_who_call = 0; // ������ ���� ������.
            if (md_who_call==2) // 1-data call
            {
                if ((sec_for_numtel&0x02)==0) // ���� �� ����� �� ���� �������� �����, 
                {
                    md_who_call = 2; // �� ����� �������, ��� ��� MD1
            }   }
            else // 0-voice
            {
                if ((sec_for_numtel&0x04)==0) // ���� �� ����� �� ���� �������� �����, 
                {
                    md_who_call = 4; // �� ����� �������, ��� ��� VC
        }   }   }
       _c_ans_kom_rx = 10;
    }
    else
    if (strncmp("+CSCA:", &buf_read_sim[0], 6) == 0) // ������ ������ sms - ������
    {
        // �������� ���:    +CSCA: "+79027459999"
        // ��������� ������ sms - ������ � ������� ASCII: "+79027459999"
        for (i=0; ;i++)
        {
            if (buf_read_sim[i+8]=='"') break;
            if (i>=kolCN) break;
            tmp_telnum_ascii[i] = buf_read_sim[i+8];
        }
        len_telnum_ascii = i;

        //_ERICSSON_
        if ((tmp_telnum_ascii[0]!='+') &&                 // ���� "�����" ���,
            (strncmp("145", &buf_read_sim[8+i+2], 3) == 0)) // � �� �����,
        {
            for (i==len_telnum_ascii; i>0; i--) tmp_telnum_ascii[i] = tmp_telnum_ascii[i-1]; // �� �������
            len_telnum_ascii++;
            tmp_telnum_ascii[0]='+';
        }

        // ��������� ������ sms - ������
        // �� ������� ASCII: "+79027459999"
        // � ������   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        tel_num[0].len_SO = GetTel_ASCII_to_SO (&tel_num[0].num_SO[0],
                                                &tmp_telnum_ascii[0],
                                                 len_telnum_ascii);
        //tel_num[0].need_read=no;
        if (len_telnum_ascii==0)
        {
           _c_ans_kom_rx=1;
            tel_num[0].accept = no;
        }
        else
        {
           _c_ans_kom_rx = 11; 
            tel_num[0].accept = yes;
        }
    }
    else
// ****************************************************************************************
//                              ������ ������ �� �����. ������
// ****************************************************************************************
    if (strncmp("+CPBR:", &buf_read_sim[0], 6) == 0)
    {
        // �������� ���:    +CPBR: i,"+79027459999",typ,text

        // ���������� ������� � ���. ������.
        iN = Get_byte_fr_ASCII(&buf_read_sim[7]);
        if (iN==19) iN=9; // BT - ��� ����. ��������
        else
        if (iN>=9)  iN+=2; // ���� ������ �9��18 ����� �11��20
        if (iN>20) return;

        // ������� ��������� �������
        for (i=0; ;i++)
        {
            if (buf_read_sim[i]=='"') { j = i+1; break; }
            if (i>10) break;
        }

        // ��������� ������ DC - ������ � ������� ASCII: "+79027459999"
        for (i=0; ;i++)
        {
            if (buf_read_sim[i+j]=='"') break;
            if (i>=kolCN) break;
            tmp_telnum_ascii[i] = buf_read_sim[i+j];
        }
        len_telnum_ascii = i; if (len_telnum_ascii==0) return;

        if ((tmp_telnum_ascii[0]!='+') &&                 // ���� "�����" ���,
            (strncmp("145", &buf_read_sim[j+i+2], 3) == 0)) // � �� �����,
        {
            for (i==len_telnum_ascii; i>0; i--) tmp_telnum_ascii[i] = tmp_telnum_ascii[i-1]; // �� �������
            len_telnum_ascii++;
            tmp_telnum_ascii[0]='+';
        }

        // ��������� ������ DC - ������
        // �� ������� ASCII: "+79027459999"
        // � ������   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        tel_num[iN].len_SO = GetTel_ASCII_to_SO (&tel_num[iN].num_SO[0],
                                                 &tmp_telnum_ascii[0],
                                                  len_telnum_ascii);
        if (tel_num[iN].len_SO==0) return;

      //tel_num[iN].need_read = no;
        tel_num[iN].accept    = yes; // ����� �������� iN ������, ������ ������ �� ����.

        _c_ans_kom_rx = 12; //
    }
    else
    if (strncmp("+CSQ:", &buf_read_sim[0], 5) == 0) // �������� �������
    {
       _c_ans_kom_rx = 13; //
        i = Get_byte_fr_ASCII(&buf_read_sim[6]);
        if (i<=31) current_sms_signal_level = i;
        if (i==99) current_sms_signal_level = 0xff;
    }
    else
// 0: not registered, ME is not currently searching for a new operator.
// 1: registered, home network.
// 2: not registered, ME currently searching for a new operator to register to.
// 3: registration denied.
// 4: unknown.
// 5: registered, roaming.
    if (strncmp("+CREG:", &buf_read_sim[0], 6) == 0) // � ���� ��� ��� ?
    {
        i = buf_read_sim[9]-0x30;
        if ((i==1)||(i==5)) _c_ans_kom_rx = 22; //    � ����
                       else _c_ans_kom_rx = 24; // �� � ����
    }
    else
// 0: automatic (default value)
// 1: manual
// 2: deregistration ; ME will be unregistered until <mode>=0 or 1 is selected.
// 3: set only <format> (for read command AT+COPS?)
// 4: manual / automatic (<oper> shall be present), if manual selection fails,
//    automatic mode is entered.
    if (strncmp("+COPS:", &buf_read_sim[0], 6) == 0) // � ���� ��� ��� ?
    {
        i = buf_read_sim[7]-0x30;
        /*
        if (i==2) _c_ans_kom_rx = 24; // �� � ����
             else _c_ans_kom_rx = 22; //    � ����
        */
        //_ERICSSON_
        if ((i==0)&&(buf_read_sim[8]==0)) _c_ans_kom_rx = 24; // �� � ����
                                     else _c_ans_kom_rx = 22; //    � ����
    }
//  else
//  if (strncmp("+CCED:", &buf_read_sim[0], 6) == 0) // RxLev indication
//  {
/*      Scounter=7;
        S_Nxt(); S_Nxt();
        if (!S_TakeNxt()) return;
        tmp_LAC[0] = (HexASCII_to_Hex(strS[0])<<4) | HexASCII_to_Hex(strS[1]);
        tmp_LAC[1] = (HexASCII_to_Hex(strS[2])<<4) | HexASCII_to_Hex(strS[3]);
        if (!S_TakeNxt()) return;
        tmp_CI[0]  = (HexASCII_to_Hex(strS[0])<<4) | HexASCII_to_Hex(strS[1]);
        tmp_CI[1]  = (HexASCII_to_Hex(strS[2])<<4) | HexASCII_to_Hex(strS[3]);
        S_Nxt(); S_Nxt();
        if (!S_TakeNxt()) return; tmp_RxLev = (unsigned char)(atoi(&strS[0]));
*/
//     _c_ans_kom_rx = 17;
//  }
    else
    if (strncmp("+CCID:", &buf_read_sim[0], 6) == 0) // ID sim-�����.
    {
        for (i=0; i<LENGTH_ID_SIM_CARD; i++)
        {
            if (buf_read_sim[8+i]=='"') break;
            context_gsm.sim[i] = buf_read_sim[8+i];
        }
        context_gsm.exist_simid = 1;
        context_gsm.simid_len   = i;
       _c_ans_kom_rx = 28;
    }
    else
    if (strncmp("*E2SSN:", &buf_read_sim[0], 7) == 0) // ID sim-�����.
    {
        for (i=0; i<LENGTH_ID_SIM_CARD; i++)
        {
            if (buf_read_sim[8+i]==0) break;
            context_gsm.sim[i] = buf_read_sim[8+i];
        }
        context_gsm.exist_simid = 1;
        context_gsm.simid_len   = i;
       _c_ans_kom_rx = 28;
    }
    else
    if (strncmp("+CDS:", &buf_read_sim[0], 5) == 0) // ���� ������������� � �������� sms.
    {
//        _c_ans_kom_rx = 18;
        n_mem_sms=0;
    }
    else
    if (strncmp("+CMGL:", &buf_read_sim[0], 6) == 0) // ������� ��������� sms - ���������
    {
       _c_ans_kom_rx = 14;
        if (_flag_new_sms==no)
        {
            // ����� ��������� ��������� � ������ simens
            n_mem_sms = Get_num_sms_fr_ASCII (&buf_read_sim[7]);
        }
    }
    else
    if (strncmp("+CMGS: ", &buf_read_sim[0], 7) == 0) // sms - ��������� �������� �������
    {
        _c_ans_kom_rx = 15;
    }
    else
    if (strncmp("> ", &buf_read_sim[0], 2) == 0) // ����������� ���������� sms - ���������
    {
        _c_ans_kom_rx = 16;
    }
    else
//  if (  (buf_read_sim[0] == '0')&& // sms - ���������
//        (  (buf_read_sim[1] == '0')||
//           (  ((buf_read_sim[2] == '9')||(buf_read_sim[2] == '8'))&&
//              (buf_read_sim[3] == '1')
//           )
//        )
//     )
    if (buf_read_sim[0] == '0') // sms - ���������
        //&&(n_mem_sms>0))
    {
       _c_ans_kom_rx = 21; // ������� sms, ��� ���� uncknown

        if (buf_read_sim[1] == '0')
        {
            i=3;
            len_telnum=0;
        }
        else
        if (((buf_read_sim[2] == '9')||(buf_read_sim[2] == '8'))&&
             (buf_read_sim[3] == '1'))
        {
            // ��������� ������ sms - ������
            // �� �������: "919720479599F9" � 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
            len_telnum = GetTel_ATSO_to_SO (&tmp_telnum[0], &buf_read_sim[2],
                                             HexASCII_to_Hex(buf_read_sim[1]));
            // ��������� ������ sms - ������
            // �� �������: "919720479599F9" � ASCII: "+79027459999"
            //len_telnum_ascii = GetTel_ATSO_to_ASCII(&tmp_telnum_ascii[0], &buf_read_sim[2]);

            i = len_telnum*2+3;
        }
        else // ����� sms ��� �� ����. �������.
        {
            if (n_mem_sms) _flag_need_delete_badsms=yes; return;
        }

        j = buf_read_sim[i] & 0x03; // message type (0-delivery, 1-submit, 2-status report)
        if ((j==3)||(j==1)) { if (n_mem_sms) _flag_need_delete_badsms=yes; return;}
        if (j==2) i+=5; // for Status report
             else i+=3; // for Delivery
        // ��������� ������ ���������
        // �� �������: "919720479599F9" � 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        len_telnum = GetTel_ATSO_to_SO (&tmp_telnum[0], &buf_read_sim[i], 
                                       (unsigned char)((HexASCII_to_Hex(buf_read_sim[i-1])+1)/2)+1);
        // ��������� ������ ���������
        // �� �������: "919720479599F9" � ASCII: "+79027459999"
        //len_telnum_ascii = GetTel_ATSO_to_ASCII(&tmp_telnum_ascii[0], &buf_read_sim[i]);

        i += len_telnum*2;

        if (j==2) // for Status report
        {
            // read Service-Centre-Time-Stamp
// ������ <1>
//              00 0665 0B919701478881F3 3060302102450C 3060302102450C 00
//                                       030603122054   030603122054   ok
// ������ <2>
//07919701479599F9 0665 0B919701478881F3 3060302102450C 3060302102450C 00
//                                       030603122054   030603122054   ok

            /*//if (ForbiddenAll) //����� ��� �� ���� ����������...
            //{
                G_date  = ((buf_read_sim[i+1]-0x30)&0x0f)*10;
                G_date += ((buf_read_sim[i]  -0x30)&0x0f);
                G_date +=2000;
                M_date  = ((buf_read_sim[i+3]-0x30)&0x0f)*10;
                M_date += ((buf_read_sim[i+2]-0x30)&0x0f);
                D_date  = ((buf_read_sim[i+5]-0x30)&0x0f)*10;
                D_date += ((buf_read_sim[i+4]-0x30)&0x0f);
                H_time  = ((buf_read_sim[i+7] -0x30)&0x0f)*10;
                H_time += ((buf_read_sim[i+6] -0x30)&0x0f);
                M_time  = ((buf_read_sim[i+9] -0x30)&0x0f)*10;
                M_time += ((buf_read_sim[i+8] -0x30)&0x0f);
                S_time  = ((buf_read_sim[i+11]-0x30)&0x0f)*10;
                S_time += ((buf_read_sim[i+10]-0x30)&0x0f);


                N_day  = (unsigned long)(GetN());

              int_timer2_no;
                NumWeek.v_int = (unsigned int)((N_day-723126)/7);
                DayWeek = ((N_day+2)%7);
                Timer_inside = (((unsigned long)DayWeek)*24*60*60)+
                               (((unsigned long)H_time)*60*60)+
                               (((unsigned long)M_time*60))+
                                ((unsigned long)S_time);
                if (ForbiddenAll)
                {
                    GetBack_m = 1; //��� ������ ����������� � ������� ��������� ����� � ������
                    Day_timer = Timer_inside % 86400 / 60; //86400 - ������ � ������
                }
                Timer_inside += (unsigned long)NumWeek.v_int*604800L;
              int_timer2_yes;
            //}*/

            i += 28;

            //tp_st_state=2; // ��������� "������������� ��������" ��������.

            j = buf_read_sim[i];
            // '0', '0'     ��������� ������ ���� (� ��������)
            //  000x xxxx - ����������
            if ((j=='0')||(j=='1')) tp_st_result = 1; //   ����������
                               else tp_st_result = 2; // ������������

//          if (buf_read_sim[1] != '0')
            if (n_mem_sms) _flag_need_delete_badsms = yes; // sms ���������� �������
            _c_ans_kom_rx = 19; // ������� ������������� � �������� sms
        }
        else // for Delivery
        {
            if (_flag_new_sms) return;
            if (n_mem_sms==0) return;


/*          if ((buf_read_sim[i]  =='0') && (buf_read_sim[i+1]=='0') &&
                (buf_read_sim[i+2]=='F') && (buf_read_sim[i+3]=='6'))
            {
                // ���� ��� SMS ��������� 8-������.
                _f_SMS_accept_type_8bit = yes;
            }
            else
            {
                // ���� ��� SMS ��������� ��������� (7-������).
                _f_SMS_accept_type_8bit = no;
                 // ���� ����� ��������, �� ��������� � ���������,
                 // ������� �������� �� ����.
                 if (len_telnum<=4) _f_SMS_noanswer=1; else _f_SMS_noanswer=0;
            }*/

            //DEBUG
            send_c_out_max(0xB7); //"new sms"
            send_c_out_max(0x00);

            if ((buf_read_sim[i] =='0') && (buf_read_sim[i+1]=='0'))
            {
                j=HexASCII_to_Hex(buf_read_sim[i+3]);
                if (((buf_read_sim[i+2]=='F')||(buf_read_sim[i+2]=='0'))
                      &&((j&0x0C)==4))
                {
                    // ���� ��� SMS ��������� 8-������.
                    _f_SMS_accept_type_8bit = yes;
                }
                else
                if (((buf_read_sim[i+2]=='F')||(buf_read_sim[i+2]=='0'))
                      &&((j&0x0C)==0))
                {
                    // ���� ��� SMS ��������� ��������� (7-������).
                   _f_SMS_accept_type_8bit = no;
                    // ���� ����� ��������, �� ��������� � ���������,
                    // ������� �������� �� ����.
                    if (len_telnum<=4) _f_SMS_noanswer=1; else _f_SMS_noanswer=0;
                }
                else
                {
                    // ����� sms ��� �� ����. �������.
                    if (n_mem_sms) _flag_need_delete_badsms=yes; return;
                }
            }

            i += 18;

reset_wdt;
            // ����� ��������� sms - ���������
            sms_receive_buffer_len  = ((HexASCII_to_Hex(buf_read_sim[i]))&0x0f)<<4;
            i++;
            sms_receive_buffer_len |=  (HexASCII_to_Hex(buf_read_sim[i]))&0x0f;
            i++; // 54 0x36
                 // sms_receive_buffer_len   25 0x19
            if (sms_receive_buffer_len>140) sms_receive_buffer_len=140;
            // ����� ����� �������� sms - ���������
            for (j=0; j<sms_receive_buffer_len; j++)
            {
                sms_receive_buffer[j]  = ((HexASCII_to_Hex(buf_read_sim[i]))&0x0f)<<4;
                i++;
                sms_receive_buffer[j] |=  (HexASCII_to_Hex(buf_read_sim[i]))&0x0f;
                i++;
reset_wdt;
            }
            for (j=sms_receive_buffer_len; j<140; j++) sms_receive_buffer[j]  = 0;
reset_wdt;

            // �������� �� ���� ������ sms
            code_access_sms = 0;
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 1)) { code_access_sms = 1; who_send_sms=1; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 2)) { code_access_sms = 1; who_send_sms=2; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 3)) { code_access_sms = 1; who_send_sms=3; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 4)) { code_access_sms = 1; who_send_sms=4; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 5)) { code_access_sms = 1; who_send_sms=5; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 6)) { code_access_sms = 1; who_send_sms=6; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 8)) { code_access_sms = 5; who_send_sms=8; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 11)) { code_access_sms = 5; who_send_sms=11; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 12)) { code_access_sms = 5; who_send_sms=12; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 13)) { code_access_sms = 5; who_send_sms=13; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 14)) { code_access_sms = 5; who_send_sms=14; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 15)) { code_access_sms = 5; who_send_sms=15; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 16)) { code_access_sms = 5; who_send_sms=16; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 17)) { code_access_sms = 5; who_send_sms=17; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 18)) { code_access_sms = 5; who_send_sms=18; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 19)) { code_access_sms = 5; who_send_sms=19; }
            else
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 20)) { code_access_sms = 5; who_send_sms=20; } // �������� �� ����
            else
            if ((sec_for_numtel&0x01)==0) // ���� �� ����� �� ���� ������ sms. 
            {
                code_access_sms = 1; // ��� ������� ��.
                who_send_sms=10;    tel_num[10].len_SO = len_telnum; 
                for (i=0; i<kolBN; i++) tel_num[10].num_SO[i] = tmp_telnum[i]; // ���������� �������
            }
            else
            {   // ���� ����� �� ���� ������ sms, � ������ ���������� �� ����,            
                code_access_sms = 0; // �� ��� ������� ������.
            }
           _flag_new_sms = yes; // ���� "�������� ����� sms-���������"
            index_sms = n_mem_sms;
           _c_ans_kom_rx = 20;  // ������� sms
        }
    }
}

void Send_str_sim(unsigned char code *pStr)
{
  unsigned char i;
    for (i=0; ; i++)
    {
        if (pStr[i]==0) break;
        tx_data_sim=pStr[i]; Send_char_sim();
        reset_wdt;
    }
}
/*
void Command_ATiW (void)
{
  unsigned char code AA[]="AT&W\r";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<5; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

void Command_IPR_9600 (void)
{
  unsigned char code AA[]="AT+IPR=9600\r";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

void Command_AT (void)
{
  unsigned char code AA[]={'A','T',13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<3; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_ATZ (void)
{
  unsigned char code AA[]={'A','T','Z',13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<4; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_CMGF (void)
{
  unsigned char code AA[]={'A','T','+','C','M','G','F','=','0',13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// ��������� ��� �������
void Command_ATE (void)
{
  unsigned char code AA[]={'A','T','E','0',13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<5; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
*/

// ���. ������ �� SIM ����� AT+CPBS=SM  (old AT^SPBS=SM)
void Command_CPBS_SM (void)
{
  unsigned char code AA[]={'A','T','+','C','P','B','S','=','"','S','M','"',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// SMS-�� � SIM ����� AT+CPMS="SM"
void Command_CPMS_SM (void)
{
  unsigned char i;

    //_ERICSSON_
    unsigned char code AA[]="AT+CPMS=\"SM\",\"SM\",\"SM\"\x0d";
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<23; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

/*void Command_AUDIO (void)
{
  unsigned char code AA[]="AT*E2APR=0,1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// ��������� ��������� ������
void Command_ATS10 (void)
{
  unsigned char code AA[]={'A','T','S','1','0','=','9','0',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_ATS7 (void)
{
  unsigned char code AA[]={'A','T','S','7','=','9','0',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// AT+CNMI=3,0,0,1
void Command_CNMI (void)
{
    //_ERICSSON_
    unsigned char code AA[]={'A','T','+','C','N','M','I','=','3',',','0',',','0',',','1',13};
    unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<16; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// AT+CBST=71,0,1
void Command_CBST (void)
{
  unsigned char code AA[]={'A','T','+','C','B','S','T','=','7','1', ',', '0', ',', '1', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<15; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� �� ������� ������� ����� DC-������.
void Command_read_telDC_sim (unsigned char N)
{
  unsigned char code AA[]={'A','T','+','C','P','B','R','='};
  unsigned char i;
  unsigned char j;
  unsigned char xdata M[8];
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }

    i = Hex_to_DecASCII_5(N, &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }
/*  if (N>=9)
    {
        tx_data_sim = '1'; Send_char_sim();
        tx_data_sim = '9'; Send_char_sim();
    }
    else
    {
        tx_data_sim = N+0x30; Send_char_sim();
    }*/
    tx_data_sim = 13; Send_char_sim();
}

// ������� �� ������� ������� ����� SMS-������.
void Command_read_telSC_sim (void)
{
  unsigned char code AA[]={'A','T','+','C','S','C','A','?',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

/*
// ������� ��������� �������.
void Command_device_off (void)
{
  unsigned char code AA[]={'A','T','+','C','P','O','F',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/
/*
// ������� �������� ��������.
void Command_speed (void)
{
  unsigned char code AA[]={'A','T','+','I','P','R','=','9','6','0','0',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// ������� �� ������� ������� ���-�� �������.
void Command_get_quality_signal (void)
{
  unsigned char code AA[]={'A','T','+','C','S','Q',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<7; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� "����� � ����?".
void Command_test_network (void)
{
  unsigned char code AA[]={'A','T','+','C','R','E','G','?',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� "����������� � ����?".
void Command_network_registration (void)
{
  unsigned char code AA[]={'A','T','+','C','O','P','S','?',13};
//unsigned char code AA[]={'A','T','+','C','O','P','S','=','0',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
//  for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� �� ������� ������� RxLev indication
/*void Command_get_RxLev (unsigned char N)
{
  unsigned char code AA[]={'A','T','+','C','C','E','D','=','0',','}; //,'1',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
    tx_data_sim = N;  Send_char_sim();
    tx_data_sim = 13; Send_char_sim();
}*/

// ������� �� ������� ������ ���� �� �������� ���������.
void Command_get_read_sms (void)
{
  unsigned char code AA[]={'A','T','+','C','M','G','L','=','4',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� �� ������� ������� ��������� N.
void Command_del_sms (unsigned char N)
{
  unsigned char code AA[]={'A','T','+','C','M','G','D','='};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
    // ����� ������ sms (����� ��������� ��� � 10 ����)
    if (N>=0x10) { tx_data_sim = Hex_to_HexASCII_Hi(N); Send_char_sim(); }
    tx_data_sim = Hex_to_HexASCII_Lo(N); Send_char_sim();
    tx_data_sim =  13; Send_char_sim();
}
/*
void Command_SPEAKER()
{
  unsigned char code AA[]={'A','T','+','S','P','E','A','K','E','R', '=', '1', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

void Command_WTONE()
{
  unsigned char code AA[]={'A', 'T', '+', 'W', 'T', 'O', 'N', 'E', '=',
                           '1', ',', '1', ',', '3', '0', '0', ',', '9', ',', '5', '0', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<22; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� AT+VGR
void Command_VGR (void)
{
  unsigned char code AA[]={'A','T','+','V','G','R','=','1','7','6',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ������� AT+VGT
void Command_VGT (void)
{
  unsigned char code AA[]={'A','T','+','V','G','T','=','2','4','3',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/
/*
void Command_VTS_OK (void)
{
  unsigned char code AA[]={'A','T','+','V','T','S','=','1',';','+','V','T','S','=','5',';','+','V','T','S','=','9',
                          13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<23; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
*/
// 1-��� ������������ DTR ��������� �����
// 2-��� ������������ DTR ������ �����
void Command_ATaD1 (void)
{
  unsigned char code AA[]="AT&D1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// ��������� �������� ����������, ����������� ������� ���������� (DSR).
// DSR �������� � ��������� ������, DSR ������� � ������ ������.
void Command_ATaS1 (void)
{
  unsigned char code AA[]="AT&S1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

void Command_3p (void)
{
  unsigned char code AA[]="+++";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<3; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_ATH (void)
{
  unsigned char code AA[]={'A','T','H', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<4; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// ��������� ������� ����������, �� gprs �� �������.
void Command_CHLD_1 (void)
{
  unsigned char code AA[]="AT+CHLD=1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

void Command_ATO (void)
{
  unsigned char code AA[]={'A','T','O', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<4; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_ATA (void)
{
  unsigned char code AA[]={'A','T','A', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<4; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
/*void Command_ATDT (void)
{
//  unsigned char code AA[]="ATDT89103010274\x0d"; //16
  unsigned char code AA[]="ATD*99***2#\x0d"; //12
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// ������ ID sim-�����
/*void Command_ATCCID (void)
{
  unsigned char code AA[]="AT+CCID\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// ������ ID sim-�����
void Command_IDSIM (void)
{
  unsigned char code AA[]="AT*E2SSN\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ��������� RI ��� ��������� ��. SMS
void Command_E2SMSRI (void)
{
  unsigned char code AA[]="AT*E2SMSRI=1000\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<16; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

//AT*E2IPS=
void Command_E2IPS (void)
{
  unsigned char code AA[]="AT*E2IPS=2,6,2,1020\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<20; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ����������� ��� IP. ��������� �� � GPRSe ?
/*void Command_testIP(void)
{
  unsigned char code AA[]="AT*E2IPI=0\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// *** ������ ***
//   0 - Ready (ME allows commands from TA/TE)
//   3 - Ringing (ME is ready for commands from TA/TE, but the ringer is active)
//   4 - Call in progress (ME is ready for commands from TA/TE, but a call is in progress)
// 
/*void Command_CPAS (void)
{
  unsigned char code AA[]="AT+CPAS\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

//at+crc=1  - ������ �� ������ ��� DATA
void Command_CRC (void)
{
  unsigned char code AA[]="AT+CRC=1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
//at+clip=1 - ����������� ������
void Command_CLIP (void)
{
  unsigned char code AA[]="AT+CLIP=1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

/*
void Command_CLCC (void)
{
  unsigned char code AA[]={'A','T','+','C','L','C','C', 13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// Define PDP Context
void Command_CGDCONT (void)
{
//unsigned char code AA[]="AT+CGDCONT=1,\"IP\",\"internet.beeline.ru\"\x0d";
  unsigned char code AA[]="AT+CGDCONT=1,\"IP\",\"";
  unsigned char i;
  unsigned char code *pAddr = F_ADDR_SETTINGS+110;

    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<19; i++) { tx_data_sim = AA[i]; Send_char_sim(); }

    for (i=0; i<64; i++)
    {
        if (pAddr[i]==0) break;
        tx_data_sim = pAddr[i]; Send_char_sim();
    }
    tx_data_sim = '"'; Send_char_sim();
    tx_data_sim =  13; Send_char_sim();
}

// Ericsson Internet Account Define
void Command_ENAD1 (void)
{
//unsigned char code AA[]="AT*ENAD=1,\"GPRS1\",\"beeline\",\"beeline\"\x0d";
  unsigned char code AA[]="AT*ENAD=1,\"GPRS1\",\"";
  unsigned char i;
  unsigned char code *pAddr1 = F_ADDR_SETTINGS+174;
  unsigned char code *pAddr2 = F_ADDR_SETTINGS+190;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;

    for (i=0; i<19; i++) { tx_data_sim = AA[i]; Send_char_sim(); }

    for (i=0; i<16; i++)
    {
        if (pAddr1[i]==0) break;
        tx_data_sim = pAddr1[i]; Send_char_sim();
    }
    tx_data_sim = '"'; Send_char_sim();
    tx_data_sim = ','; Send_char_sim();
    tx_data_sim = '"'; Send_char_sim();
    for (i=0; i<16; i++)
    {
        if (pAddr2[i]==0) break;
        tx_data_sim = pAddr2[i]; Send_char_sim();
    }
    tx_data_sim = '"'; Send_char_sim();
    tx_data_sim = ','; Send_char_sim();
    tx_data_sim = '1'; Send_char_sim();
    tx_data_sim = ','; Send_char_sim();
    tx_data_sim = '0'; Send_char_sim();
    tx_data_sim =  13; Send_char_sim();
}

/*void Command_CGCLASS (void)
{
  unsigned char code AA[]="AT+CGCLASS=\"B\"\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<15; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/
void Command_CGCEREP (void)
{
  unsigned char code AA[]="AT+CGEREP=1\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_E2IPA (void)
{
  unsigned char code AA[]="AT*E2IPA=1,1\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
/*
void Command_E2IPA0 (void)
{
  unsigned char code AA[]="AT*E2IPA=0,1\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
void Command_E2IPC (void)
{
  unsigned char code AA[]="AT*E2IPC\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
*/
void Command_E2IPO (void) // AT*E2IPO=1,"66.249.85.99",80\x0d
{
  unsigned char code AA[]="AT*E2IPO=1,\"";
  unsigned char i,j;
  unsigned char code *pAddr = F_ADDR_SETTINGS+206;
  unsigned int  code *pPort = F_ADDR_SETTINGS+218;
  unsigned char xdata M[8];
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }

    i = Hex_to_DecASCII_5((unsigned int)(pAddr[0]), &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }
    tx_data_sim = '.'; Send_char_sim();
    i = Hex_to_DecASCII_5((unsigned int)(pAddr[1]), &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }
    tx_data_sim = '.'; Send_char_sim();
    i = Hex_to_DecASCII_5((unsigned int)(pAddr[2]), &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }
    tx_data_sim = '.'; Send_char_sim();
    i = Hex_to_DecASCII_5((unsigned int)(pAddr[3]), &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }

    tx_data_sim = '"'; Send_char_sim();
    tx_data_sim = ','; Send_char_sim();
    i = Hex_to_DecASCII_5((unsigned int)(pPort[0]), &M[0]);
    for (j=0; j<i; j++) { tx_data_sim = M[j]; Send_char_sim(); }

    tx_data_sim = 0x0d; Send_char_sim();
}

// ������, ������� ����� �������������� MT ��� ������� MO SMS ���������
// '2'- GPRS
// '3'- ����� SMS
void Command_CGSMS (void)
{
  unsigned char code AA[]="AT+CGSMS=3\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// ����� Com_T_Restart (3�) - �������� �� ���������
/*void Command_1234 (void)
{
  unsigned char code AA[]={Com_T_Restart,'1','2','3','4',13};
  unsigned char i;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// ����� Com_T_Restart (3�) - �������� �� ���������
void Command_ver (void)
{
  unsigned char code AA[]={Com_T_Version,ID_TERM1,VERSION,ID_TERM2,BUILD,13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/


// ������ ��������� �������
void Analise_tel_num (void)
{
  unsigned char xdata i,j;
  unsigned char xdata n;

    // �������� �� ���� ���� ����� ?
    for (i=1,n=0; i<=9; i++) if (tel_num[i].len_SO) { n=i; break; }

    if (n!=0)
    {
        // ���� �� ���� ����� ����. �������� �� ������������� ������.
        for (i=1; i<=3; i++)
          if (tel_num[i].len_SO==0) // ���� ��� ������, �������.
          {
              tel_num[i].len_SO = tel_num[n].len_SO;
              for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
          }
        _flag_tel_ready_R = 0; // ������� ����� � � �������� � � ������.
    }
}
/*
// ������ ��������� 10 �������������� �������
void Analise_10d_tel_num(void)
{
  unsigned char xdata i;

    for (i=20; i>=11; i--)
    {
        tel_num[i].need_send_sms_Al=0; // ��� ������������� �������� �� ���� ����� ��������� sms-�������.
        tel_num[i].sms_Al = 0;         // ��������� sms-������� �� ���� ����� �� ��������.
    }
    if (tel_num[8].len_SO!=0) tel_num[8].sms_Al = 1; // ���� ����� �������� ��������� sms-������� �� ���� �����.
    for (i=20; i>=11; i--)
    {
        if (tel_num[i].len_SO==0) break;
        else
        tel_num[i].sms_Al = 1; // ���� ����� �������� ��������� sms-������� �� ���� �����.
    }
}*/

// ���� ������������� ��������� ��������� sms-�������.
/*void SetNeedSendSMSalert(void)
{
  unsigned char xdata i;

    if ((tel_num[8].len_SO!=0)&& // ���� ����� ����� ���� �
        (tel_num[8].sms_Al==1))  // �� ���� ��������� �������� ��������� sms-�������.
    {
        tel_num[8].need_send_sms_Al=1; // ���������� ������� �� ���� ����� ��������� sms-�������.
    }
    for (i=20; i>=11; i--)
    {
        if ((tel_num[i].len_SO!=0)&& // ���� ����� ����� ���� �
            (tel_num[i].sms_Al==1))  // �� ���� ��������� �������� ��������� sms-�������.
        {
            tel_num[i].need_send_sms_Al=1; // ���������� ������� �� ���� ����� ��������� sms-�������.
}   }   }
*/

// **************************
// * ������������� �������� *
// **************************
bit Automat_InitSim(void)
{
  unsigned char xdata i;
  unsigned char xdata j;
  unsigned char xdata n;

    switch (state_init_sim_wk)
    {
        case 1: // ��������� ��������� ��������� ������ � ���������
            count_atz = 0; // ������� ������� ������� ATZ
            count_p   = 0;
            //gprs.b.gprs_on=0;
            state_init_sim_wk = 3;
            tx_data_sim = 0x1a; Send_char_sim();
            tx_data_sim =   13; Send_char_sim();
            time_wait_SMSPhone = 0;
        break;

        // ***************
        // *     ATZ     *
        // ***************
        case 3: // ���� 2 ��� � �������� ATZ
            if (time_wait_SMSPhone >= 2)
            {
                count_atz++;
                if (count_atz >= 10)
                {
                    count_atz = 0;
                    state_sim_wk = 99; // Error
                    return 0;
                }
                Send_str_sim("ATZ\r\0");
                time_wait_SMSPhone = 0; state_init_sim_wk = 5;
            }
        break;

        case 5: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
              //count_atz = 0;
                StSYSTEM &= ~0x0030; // ����� �������� - ���� ������ ���.
                StatePB = 0xFF;
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 10;
            }
            if ((time_wait_SMSPhone >= 3)||(_c_ans_kom_rx==4))
            {
                state_init_sim_wk = 3;
                count_atz++;
                if (count_atz >= 3)
                {
                    StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0010; // ����� �� �������� - ������.
                    count_atz = 0;
                    StatePB = 0xFF;
                    state_sim_wk = 99; // Error
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // *****************
        // *   AT+CMGF=0   *  PDU mode
        // *****************
        case 10:
            if (time_wait_SMSPhone >= 2)
            {
                Send_str_sim("AT+CMGF=0\r\0"); // PDU mode
                time_wait_SMSPhone = 0; state_init_sim_wk = 12;
            }
        break;

        case 12: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
//              count_atz = 0;
                ClearBuf_Sim ();
                StSYSTEM &= ~0x0080; // ���� ������ ���.
                time_wait_SMSPhone = 0; state_init_sim_wk = 20;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                if (count_atz >= 3) { StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0080; } // ������ ������������� ������.
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ****************
        // *   ATS10=90   *
        // ****************
        case 20:
            if (time_wait_SMSPhone >= 5)
            {
                Command_ATS10(); time_wait_SMSPhone = 0; state_init_sim_wk = 22;
            }
        break;

        case 22:
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==4)) // ���� �����
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 23;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;
        // *****************
        // *    ATS7=90    *
        // *****************
        case 23:
            Command_ATS7(); time_wait_SMSPhone = 0; state_init_sim_wk = 24;
        break;
        case 24:
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==4)) // ���� �����
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 25;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        // ******************
        // *   AT+CPBS=SM   *
        // ******************
        case 25:
            if (time_wait_SMSPhone >= 3)
            { Command_CPBS_SM(); time_wait_SMSPhone = 0; state_init_sim_wk = 26; }
        break;

        case 26: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
              //count_atz = 0;
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 27;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 25;
                count_atz++;
                if (count_atz >= 8)
                {
//                  count_atz = 0;
                    state_init_sim_wk = 3;
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ******************
        // *   AT+CPMS=SM   *
        // ******************
        case 27:
            if (time_wait_SMSPhone >= 1)
            { Command_CPMS_SM(); time_wait_SMSPhone = 0; state_init_sim_wk = 28; }
        break;

        case 28: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 29;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 27;
                count_atz++;
                if (count_atz >= 8)
                {
                    state_init_sim_wk = 3;
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **************
        // *   AT+CSQ   *
        // **************
        case 29:
            Command_get_quality_signal(); time_wait_SMSPhone = 0; state_init_sim_wk = 30;
        break;

        case 30:
            if (_c_ans_kom_rx==1) // ��������
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 35;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        // *******************************************
        // *  AT+CNMI=2,0,0,1  ���  AT+CNMI=3,0,0,1  *
        // *******************************************
        case 35:
            Command_CNMI(); time_wait_SMSPhone = 0; state_init_sim_wk = 36;
        break;

        case 36: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 37;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ************
        // *   ATE0   *
        // ************
        case 37:
            if (time_wait_SMSPhone >= 1)
            {
                Send_str_sim("ATE0\r\0"); // ��������� ��� �������
                time_wait_SMSPhone = 0; state_init_sim_wk = 38;
            }
        break;

        case 38: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); 
                time_wait_SMSPhone = 0; state_init_sim_wk = 39;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **************
        // *  at+crc=1  *  ������ �� ������ ��� DATA
        // **************
        case 39:
            //if (time_wait_SMSPhone >= 1)
            {
                Command_CRC();
                time_wait_SMSPhone = 0; state_init_sim_wk = 40;
            }
        break;
        case 40: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); 
                time_wait_SMSPhone = 0; state_init_sim_wk = 41;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ***************
        // *  at+clip=1  *  ����������� ������
        // ***************
        case 41:
            //if (time_wait_SMSPhone >= 1)
            {
                Command_CLIP();
                time_wait_SMSPhone = 0; state_init_sim_wk = 42;
            }
        break;
        case 42: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); 
                time_wait_SMSPhone = 0; state_init_sim_wk = 46;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **********************
        // *   AT+CBST=71,0,1   *
        // **********************
        case 46:
            Command_CBST();
            time_wait_SMSPhone = 0; state_init_sim_wk = 47;
        break;
        case 47: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); 
                time_wait_SMSPhone = 0; state_init_sim_wk = 48;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        case 48:
            Command_CGSMS();
            time_wait_SMSPhone = 0; state_init_sim_wk = 49;
        break;
        case 49: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim (); 
                time_wait_SMSPhone = 0; state_init_sim_wk = 55;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        // *********************************************
        // *  AT+CGDCONT=1,"IP","internet.beeline.ru"  *
        // *********************************************
        case 55:
            Command_CGDCONT(); time_wait_SMSPhone = 0; state_init_sim_wk = 56;
        break;
        case 56:
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 57;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        // *******************************************
        // *  AT*ENAD=1,"GPRS1","beeline","beeline"  *
        // *******************************************
        case 57:
            Command_ENAD1(); time_wait_SMSPhone = 0; state_init_sim_wk = 58;
        break;
        case 58:
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 59;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        // *****************
        // *  AT+CGEREP=1  *
        // *****************
        case 59:
            Command_CGCEREP(); time_wait_SMSPhone = 0; state_init_sim_wk = 60;
        break;
        case 60:
            if (_c_ans_kom_rx==1) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 61;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;
        // *************
        // *   AT&D1   *
        // *************
        case 61:
            Command_ATaD1(); time_wait_SMSPhone = 0; state_init_sim_wk = 62;
        break;
        case 62:
            if (_c_ans_kom_rx==1) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 65;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;
        // *************
        // *   AT&S1   *
        // *************
        case 65:
            Command_ATaS1(); time_wait_SMSPhone = 0; state_init_sim_wk = 66;
        break;
        case 66:
            if (_c_ans_kom_rx==1) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 67;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;
        // ***********************
        // * ������ ID sim-����� *
        // ***********************
        case 67:
            Command_IDSIM(); time_wait_SMSPhone = 0; state_init_sim_wk = 68;
        break;
        case 68:
            if (_c_ans_kom_rx==28) // ���� ID
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 69;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 3)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;
        case 69:
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // ���� Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 70;
            }
        break;
        // **************************************
        // * ��������� RI ��� ��������� ��. SMS *
        // **************************************
        case 70:
            Command_E2SMSRI(); time_wait_SMSPhone = 0; state_init_sim_wk = 71;
        break;
        case 71:
            if (_c_ans_kom_rx==1) // OK
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 72;
            }
            if ((_c_ans_kom_rx==4)||(time_wait_SMSPhone >= 3))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 72:
            Command_E2IPS(); time_wait_SMSPhone = 0; state_init_sim_wk = 73;
        break;
        case 73:
            if (_c_ans_kom_rx==1) // OK
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 80;
            }
            if ((_c_ans_kom_rx==4)||(time_wait_SMSPhone >= 3))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

/*      case 74:
            Command_AUDIO(); time_wait_SMSPhone = 0; state_init_sim_wk = 75;
        break;
        case 75:
            if (_c_ans_kom_rx==1) // OK
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 80;
            }
            if ((_c_ans_kom_rx==4)||(time_wait_SMSPhone >= 3))
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
*/

        // *****************************
        // * ����� �������� sms ������ *
        // *****************************
        case 80: // ���������� ��������� ����� �������� sms ������. 
            if (_flag_need_read_all_tels==0) // ���� ��� ������������� ������ ������ ���������
            {                                // (��� ��� �������), ��
                count_try_tn = 0;     // ������� ������� ��������� ������ ���������
               _flag_tel_ready   = 1; // ��� ������ � ������
               _flag_tel_ready_R = 0; //
                state_init_sim_wk = 150;
                return 0;
            }
            Command_read_telSC_sim();
            time_wait_SMSPhone = 0; state_init_sim_wk = 85;
        break;

        case 85:
            if (_c_ans_kom_rx==11) // �������� ����� SMS - ������
            {
                tmp_telnum_pos = 1; // ������ ��������� ������ ��������� � ������� 1;
                count_try_tn = 0; // ������� ������� ��������� ������ ���������
                ClearBuf_Sim (); time_wait_SMSPhone1 = 0; state_init_sim_wk = 90; return 0;
            }
            if (_c_ans_kom_rx==1) // �� �������� ����� sms - ������
            {
                if (++count_try_tn >= 3) // ������� ������� ��������� ������ ���������
                {
                   _flag_tel_ready   = 1; // ������� ����� � ������, ��
                   _flag_tel_ready_R = 1; // ������ � ������ (������� SC � DC ���).
                    ClearBuf_Sim (); time_wait_SMSPhone = 0;
                    state_init_sim_wk = 150;
                    return 0;
                }
                Command_read_telSC_sim(); time_wait_SMSPhone = 0;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 10)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        // ************************
        // * ������ ��������� 1-9 *
        // ************************
        case 90:
            if (time_wait_SMSPhone1>=2)
            {
                // ���������� ��������� ������ ��������� 1-9.
                tel_num[tmp_telnum_pos].accept = no;
                Command_read_telDC_sim(tmp_telnum_pos);
                time_wait_SMSPhone = 0; state_init_sim_wk = 95;
            }
        break;

        case 95:
            if (_c_ans_kom_rx==12) // �������� ��������� �����
            {
                count_try_tn = 0; state_init_sim_wk = 90;

                i = tmp_telnum_pos;
                if (i==19) i=9;
                else
                if (i>8) i+=2;

                if (tel_num[i].accept)
                {
                    tmp_telnum_pos++;
                    if (tmp_telnum_pos>19) state_init_sim_wk = 100;
                }
                time_wait_SMSPhone1 = 0; return 0;
            }
            if (_c_ans_kom_rx==1) // �� �������� ��������� �����
            {
                count_try_tn = 0;
                tmp_telnum_pos++;
                if (tmp_telnum_pos>19) state_init_sim_wk = 100;
                                  else state_init_sim_wk = 90;
                time_wait_SMSPhone1 = 0; return 0;
            }
            if (_c_ans_kom_rx==4) // Error
            {
                if (++count_try_tn >= 30) // ������� ������� ��������� ������ ���������
                {
                    count_try_tn = 0;
                    tmp_telnum_pos++;
                    time_wait_SMSPhone1 = 0;
                    if (tmp_telnum_pos>19) state_init_sim_wk = 100;
                                      else state_init_sim_wk = 90;
                    return 0;
                }
                else
                {
                    time_wait_SMSPhone1 = 0; state_init_sim_wk = 90;
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 10)
            {
                time_wait_SMSPhone = 0; state_init_sim_wk = 3;
            }
        break;

        // ������ ��������� �������
        case 100:
            // �������� �� ���� ���� ����� ?
            for (i=1,n=0; i<=9; i++)
                if ((tel_num[i].accept)&&(tel_num[i].len_SO)) { n=i; break; }

            if (n!=0)
            {
                // ���� �� ���� ����� ����. �������� �� ������������� ������.
                for (i=1; i<=3; i++)
                  if ((tel_num[i].accept==no)||(tel_num[i].len_SO==0)) // ���� ��� ������, �������.
                  {
                      tel_num[i].len_SO = tel_num[n].len_SO;
                      for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
                  }
/*              for (i=1; i<=9; i++)
                  if ((i!=4)&& // ����������� ����� �� sms �������� �� ����.
                      (i!=8)&& // ����� ������������ US1 ���� �� ����
                      ((tel_num[i].accept==no)||(tel_num[i].len_SO==0)))
                  {
                    //tel_num[i].accept = yes;
                      tel_num[i].len_SO = tel_num[n].len_SO;
                      for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
                  }*/
                StSYSTEM &= ~0x00B0; // ����� ��������, ������ ���� - ���� ������ ���.
            }
            else // �� ��������� �� ������ ������.
            {
                StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0020; // ��� ������� - ������.
               _flag_tel_ready    = 1; // ������� ����� � ������, ��
               _flag_tel_ready_R  = 1; // ������ � ������ (������� ���).
                state_init_sim_wk = 150;
                return 0;
            }

/*          if (((tel_num[5].accept)&&(tel_num[5].len_SO))&&       // ���� MD1
                ((tel_num[6].accept==no)||(tel_num[6].len_SO==0))) // �� ��� MD2
            {
                // �������� MD2 - MD1.
                tel_num[6].len_SO = tel_num[5].len_SO;
                for (j=0; j<kolBN; j++) tel_num[6].num_SO[j] = tel_num[5].num_SO[j];
            }*/

            // ������ 10 �������������� �������
            //Analise_10d_tel_num();

           _flag_tel_ready   = 1; // ��� ������ � ������
           _flag_tel_ready_R = 0; //

            state_init_sim_wk = 150;
        break;

        // ��� ������ � ������
        case 150:
            //CurrentEvents.b.reset_cpu = 1; //��������� <����>������ ���������
            time_wait_SMSPhone = 0; return 1;
        break;

        default:
            state_init_sim_wk = 1;
        break;
    }
    return 0;
}

// ***********************************************
// * ������� ���������� ������� Siemens          *
// ***********************************************
void Automat_PerSimWork(void)
{
  unsigned char i;
  unsigned char code *pAddr = F_ADDR_SETTINGS+206;

    if ((md_resetGSM==1)&&(!_flag_need_send_sms)) //   1 - ���������� �������������������� GSM-�����
    {
        if (GSM_DCD) // �� � ����������, ����� ��������������������
        {
           _flag_need_read_all_tels=yes; // ���������� ��������� ��� ������ ���������.
            md_resetGSM=0;
            state_sim_wk = 1;
        }
        else // ���� ������� ����������
            state_sim_wk = 186;
        return;
    }
    else
    if (md_resetGSM>=2) // >=2 - ���������� ������� ����� GSM-������
//     ||(gprs.b.rst_gsm))
    {
        md_resetGSM=0;
//      gprs.b.rst_gsm=0;
        state_sim_wk = 99;
        return;
    }

//  if (_c_ans_kom_rx==2) state_per_sim_wk = 15; // RING (�������� �������� ������)
    if (_c_ans_kom_rx) _c_ans_kom_rx = 0;

    switch (state_per_sim_wk)
    {
        case 1:
            count_worksms=0;

        // ����� � ���� ��� ���?.
        case 2:
            Command_network_registration();
            time_wait_SMSPhone = 0; state_sim_wk = 195;//190;
            state_per_sim_wk = 4;
        break;

        case 4: // ����������� ����� � ���� ��� ���?
            if (time_wait_SMSPhone>=2)
            {
                Command_test_network(); //AT+CREG?   +CREG: 0,1
                time_wait_SMSPhone = 0; state_sim_wk = 62;
                state_per_sim_wk = 6;
            }
        break;

        case 6: // ����������� �������� �������
            if (time_wait_SMSPhone>=2)
            {
                Command_get_quality_signal ();
                time_wait_SMSPhone = 0; state_sim_wk = 60;
                state_per_sim_wk = 8;
            }
        break;

        case 8: // ���������� ������� SMS - ���������.
            // ���� ���� ������� ������ ���������, �� ���� ��������� �� ��������.
            for (i=0;  i<10; i++) if (tel_num[i].need_write) return;
            for (i=11; i<21; i++) if (tel_num[i].need_write) return;
            state_per_sim_wk = 10;
            if (_flag_need_send_sms==0) return;

            if (_flag_tel_ready_R)
            {
               _flag_need_send_sms = no; c_flag_sms_send_out = 0; return;
            }
            if (_flag_tel_ready) { time_wait_SMSPhone = 0; state_sim_wk = 15; }
        break;

        case 10: // ����������� ������� ����� SMS - ��������� � Simens.
            if (_flag_new_sms==no)
            {
                 state_sim_wk = 65;
            }
			
            time_wait_SMSPhone = 0; state_per_sim_wk = 11;
        break;

        case 11: // ������� RING
            ClearBuf_Sim(); _c_ans_kom_rx = 0;
            time_wait_SMSPhone = 0; state_sim_wk = 140;
            state_per_sim_wk = 12;
        break;

        case 12:
            if (time_wait_SMSPhone>1)
            {
                //count_worksms++;
                //if (count_worksms>=3) state_per_sim_wk = 15;
                //                else state_per_sim_wk = 2;

                if (!CurrentEvents.b.reset_cpu) state_per_sim_wk = 1;
                else
                if (++count_worksms<2) state_per_sim_wk = 8;
                else
                if (state_io_wk>5)     state_per_sim_wk = 8;
                else
                                       state_per_sim_wk = 15;
            }
        break;

        // 1. �� � gprs  dcd==1
        // 2. � ppp      dcd==1
        // 3. � tcp      dcd==0
        //
        case 15: // ������� GPRS
            // �������� �� � ���������� - ����� �� ������ ������ ("ATO").
            // ���� ��� - ������� �� ������ � ���� � GPRS.
            count_p = 0;
            //state_per_sim_wk = 13;

            if (time_nogprs) {state_per_sim_wk = 1; return;}

            // ���� IP-����� ����� 127.0.0.1 , �� � GPRS ����� �� ����
            if ((pAddr[0]==127)&&(pAddr[1]==0)&&(pAddr[2]==0)&&(pAddr[3]==1))
            {
                state_per_sim_wk = 1;
                return;
            }

            state_sim_wk = 170; //ATO ����� �� ���������� ������
/*
            if (GSM_DCD)
            {
                //count_dcd=0;
                state_sim_wk = 174;//104; //    ���� � gprs
            }
            else
            {
                //count_dcd++;
                //if (count_dcd>=3)
                //{
                //    count_dcd=0; md_resetGSM=2; // ����� ������
                //}
                //else
                    state_sim_wk = 170;//100; //ATO ����� �� ���������� ������
            }
*/
            state_per_sim_wk = 17;
//          if (gprs.b.discnct) state_sim_wk     = 188;//118; //����� �� GPRS.

            time_wait_SMSPhone = 0;
            
        break;

        case 17: // ����� ������ �������� ������ �� GPRS.
//          if (gprs.b.f_cycle) state_per_sim_wk = 1;
//                         else state_per_sim_wk = 15;
            state_per_sim_wk = 1;
        break;

        default:
            state_per_sim_wk = 1;
        break;
    }
}



// ***********************************************
// * ������� Siemens                             *
// ***********************************************
void Automat_sms_phone (void)
{
  unsigned char xdata i;
  unsigned char xdata j;
  unsigned char xdata M[8];
//unsigned  int xdata temp_int;

    if ((gprs.b.command)&&(StK8.b.fDataCon==0))
    {
        for (i=0; i<sizeBuf_rx_Sim; i++)
        {
            rx_data_sim = Read_byte_sim();
            Receive_sim();
            reset_wdt;
            if (_flag_new_line_sim) break;
        }

        if (_flag_new_line_sim)
        {
            if (_c_ans_kom_rx==0) AnaliseLine();
           _flag_new_line_sim = no;
        }
    }
    reset_wdt;


  switch (state_sim_wk)
    {
        // **************************
        // * ������������� �������� *
        // **************************
        case 1:
            state_init_sim_wk = 1; // ������������� ��������
            state_per_sim_wk = 1;  // ���������� �������
            state_sim_wk = 3;
        break;

        case 3:
            if (Automat_InitSim()==1) state_sim_wk = 10;
        break;

        // ******************************************************
        // *  �������� ����                                     *
        // ******************************************************
        case 10:

            // ***********************************
            // *   ���� ������� ����� � ������   *
            // ***********************************
            if (_flag_tel_ready)
            {
                // ���� ���������� ������� �������� SMS - ���������.
                if ((_flag_need_delete_sms)||(_flag_need_delete_badsms))
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 12; return;
                }
                // ���� ���������� �������� ����� �������� SMS ������
                if (tel_num[0].need_write)
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 30; return;
                }
                // ���� ���������� �������� ����� �������� DC �������
                for (i=1; i<10; i++) if (tel_num[i].need_write)
                {
                    need_write_telDC_ix = i;time_wait_SMSPhone = 0;state_sim_wk = 39;return;
                }
                // ���� ���������� �������� ������ ���. 10 ��������� MV �������
                for (i=11; i<21; i++) if (tel_num[i].need_write)
                {
                    need_write_telDC_ix = i;time_wait_SMSPhone = 0;state_sim_wk = 39;return;
                }
/*
                // ���������� ��������� ����������/��������� �������.
                if ((_flag_sms_signal_level_null==no) && // ���� ���������� ������� ��� �� ����
                    (Get_quality_signal()<1))            // ������ ������
                {
                    sTime_Net  = Rele_LCount; // �������� �����
                   _flag_sms_signal_level_null = yes; // ������ ������.
                }
                else
                if ((Get_quality_signal()>=1)&&(Get_quality_signal()<=4) && // ������ ��������
                    (_flag_sms_signal_level_null==yes))                     // � ���� ���������� �������
                {
                   _flag_sms_signal_level_null = no;
                    if ((Rele_LCount-sTime_Net)>=180L) // ���� �� ���� 3 ������
                    {
                        state_sim_wk = 99; // ���������� ������� ����/��� ��������.
                        return;
                    }
                }*/
            }
            // **************************************
            // *   ���� ������� �� ����� � ������   *
            // **************************************
            //else
            //{
            //}

            // *******************************************
            // *   ���������� �� ���������� ��������     *
            // *   ������������ �����������:             *
            // * 1. �������� �������                     *
            // * 2. RING                                 *
            // * 3. ������� ����� SMS-��������� � Simens *
            // *******************************************
            Automat_PerSimWork();

        break;

        // ***************************************************
        // ������� �������� sms - ��������� �� ������ ��������
        // ***************************************************
        case 12: 
            if (time_wait_SMSPhone >= 1)
            {
                Command_del_sms(n_mem_sms);
                time_wait_SMSPhone = 0; state_sim_wk = 14;
            }
        break;

        case 14: // �������� �������� ��������� sms - ���������
            if (_c_ans_kom_rx==1) // ���� ��������
            {
                if (_flag_need_delete_sms)
                {
                   _flag_need_delete_sms = no; _flag_new_sms = 0;
                }
                if (_flag_need_delete_badsms) _flag_need_delete_badsms = no;
                n_mem_sms = 0;
                count_p = 0;
                ClearBuf_Sim ();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
                return;
            }
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4)) // ������ ��������
            {
                time_wait_SMSPhone = 0;
                state_sim_wk = 12;
                count_p++;
                if (count_p >= 4)
                {
                    if (_flag_need_delete_sms)
                    {
                       _flag_need_delete_sms = no; _flag_new_sms = 0;
                    }
                    if (_flag_need_delete_badsms) _flag_need_delete_badsms = no;
                    n_mem_sms = 0;
                    count_p = 0;
                   _flag_need_read_all_tels = yes;
                    state_sim_wk = 1; // Error
                }
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // *********************************
        // * ������ �������� sms ��������� *
        // *********************************
        case 15: // ATZ
            if (time_wait_SMSPhone >= 1)
            {
                Send_str_sim("ATE0\r\0"); // ��������� ��� �������
                //Send_str_sim("AT\r\0");
                time_wait_SMSPhone = 0; state_sim_wk = 16;
            }
        break;

        case 16:
            if (_c_ans_kom_rx==1) // ���� OK
            {
                ClearBuf_Sim ();
              //count_atz = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 17; return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
                time_wait_SMSPhone = 0; state_sim_wk = 15; // ������
                count_atz++;
                if (count_atz >= 4)
                {
                   _flag_need_read_all_tels = yes;
                    count_atz = 0; state_sim_wk = 1; // Error
                }
            }
        break;

        case 17: // AT+CMGS=15
            if (time_wait_SMSPhone >= 1)
            {
                if (sim_line_tx_len==0)
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 1;
                    time_wait_SMSPhone = 0; state_sim_wk = 10;
                    return;
                }
                ClearBuf_Sim ();
               _c_ans_kom_rx = 0;
                tx_data_sim = 'A'; Send_char_sim();
                tx_data_sim = 'T'; Send_char_sim();
                tx_data_sim = '+'; Send_char_sim();
                tx_data_sim = 'C'; Send_char_sim();
                tx_data_sim = 'M'; Send_char_sim();
                tx_data_sim = 'G'; Send_char_sim();
                tx_data_sim = 'S'; Send_char_sim();
                tx_data_sim = '='; Send_char_sim();

                i = Hex_to_DecASCII_5(sim_line_tx_len-(tel_num[0].len_SO+1), &M[0]);
                for (j=0; j<i; j++)
                {
                    tx_data_sim = M[j]; Send_char_sim();
                }
                tx_data_sim =  13; Send_char_sim();
                time_wait_SMSPhone = 0; state_sim_wk = 18;
            }
        break;

        case 18:
            if (_c_ans_kom_rx==16) // ���� �����������
            {
                count_atz=0;
                ClearBuf_Sim (); _c_ans_kom_rx = 0; count_p=0;
                /*time_wait_SMSPhone = 0;*/ state_sim_wk = 19;
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 20)
            {
                count_error_no_reply_from_sms++; // �����������-�� �������� ������� �������� ���������
                time_wait_SMSPhone = 0; state_sim_wk = 15;
                if (count_error_no_reply_from_sms>=4)
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 0; // ���� ���������� ��� ��������� ������ �������� ���������.
                    count_error_no_reply_from_sms = 0;
                    state_sim_wk = 99; // ������
                }
            }
        break;

        case 19:
            if (cBufTxW < (SIZE_BUF_TX_UART0-1))
          //if (!cBufTxW)
            {
                tx_data_sim = Hex_to_HexASCII_Hi(sim_line_tx[count_p]); Send_char_sim();
                tx_data_sim = Hex_to_HexASCII_Lo(sim_line_tx[count_p]); Send_char_sim();
                if (++count_p>=sim_line_tx_len) state_sim_wk = 20;
            }
        break;

        case 20:
                tx_data_sim = 0x1a; Send_char_sim();
                time_wait_SMSPhone = 0; state_sim_wk = 21;
        break;

        case 21:
          //if ((_c_ans_kom_rx==15)||(_c_ans_kom_rx==1)) // sms - ��������� �������� �������
            if (_c_ans_kom_rx==15) // SMS - ��������� �������� �������
            {
                count_error_no_reply_from_sms = 0;
                if (_flag_need_wait_SR==no) // ������������� �� �����.
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 1; // ���� ��� ��������� ������� ����������.
                    time_wait_SMSPhone = 0; state_sim_wk = 10;
                    return;
                }
                time_wait_SMSPhone = 0; state_sim_wk = 22;
                StSYSTEM |= 0x0004; // ��������� �������� �������������
                return;
            }
            if ((time_wait_SMSPhone >= 30)||(_c_ans_kom_rx==4))
            {
                count_error_no_reply_from_sms++; // �����������-�� �������� ������� �������� ���������
                time_wait_SMSPhone = 0; state_sim_wk = 15;
                if (count_error_no_reply_from_sms>=3)
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 0; // ���� ���������� ��� ��������� ������ �������� ���������.
                    count_error_no_reply_from_sms = 0;
                    state_sim_wk = 99; // ������
                }
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ���� ��������� "������������� � ��������"
        case 22:
            if (_c_ans_kom_rx==19)
            {
               _flag_need_send_sms = no;
                if (tp_st_result==1) c_flag_sms_send_out = 1; // ��������� ���������� � ����������.
                else
                {
                    c_flag_sms_send_out = 3; // ��������� ����������, �� �� ����������.
                    // ���� ��� ������������ ������, �� ������� �� ���� �� �����.
                    if (tel_num[4].len_SO==0) c_flag_sms_send_out = 1; // ����� ������� ��� ����������
                  //if (tel_num[4].len_SO==0) c_flag_sms_send_out = 0; // ��������� ��������� �� �������.
                }
                state_sim_wk = 10;
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 90)
            {
               _flag_need_send_sms = no;
                state_sim_wk = 10;
              //c_flag_sms_send_out = 0;
                c_flag_sms_send_out = 1; // ����� ������� ��� ����������

/*              c_flag_sms_send_out = 3;
                // ���� ��� ������������ ������, �� ������� �� ���� �� �����.
                if (tel_num[4].len_SO==0) c_flag_sms_send_out = 0; // ��������� ��������� �� �������.
*/  
            }
        break;


        // ***********************************************
        // * ������ ������ �������� sms ������ � Siemens *
        // ***********************************************
        case 30:
          if (time_wait_SMSPhone >= 4)
          {
            // �������� ����� sms ������ � Siemens
            ClearBuf_Sim ();
           _c_ans_kom_rx = 0;
            tx_data_sim = 'A'; Send_char_sim();
            tx_data_sim = 'T'; Send_char_sim();
            tx_data_sim = '+'; Send_char_sim();
            tx_data_sim = 'C'; Send_char_sim();
            tx_data_sim = 'S'; Send_char_sim();
            tx_data_sim = 'C'; Send_char_sim();
            tx_data_sim = 'A'; Send_char_sim();
            tx_data_sim = '='; Send_char_sim();
            tx_data_sim = '"'; Send_char_sim();
//          len_telnum_ascii = GetTel_ATSO_to_ASCII(&tmp_telnum_ascii[0], &tel_num[0].num_SO[0]);
            len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                                  &tel_num[0].num_SO[0],
                                                   tel_num[0].len_SO);

            for (i=0; i<len_telnum_ascii; i++)
            {
                tx_data_sim = tmp_telnum_ascii[i]; Send_char_sim();
            }
            tx_data_sim = '"'; Send_char_sim();
            tx_data_sim =  13; Send_char_sim();
            time_wait_SMSPhone = 0; state_sim_wk = 36;
          }
        break;

        case 36:
            if (_c_ans_kom_rx==1) // Ok. ������� ����� sms ������ � Siemens
            {
                ClearBuf_Sim ();
                tel_num[0].need_write = no;
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4))  // ������
            {
                time_wait_SMSPhone = 0;
                state_sim_wk = 30;
                count_p++;
                if (count_p >= 3)
                {
                    count_p = 0;
                   _flag_need_read_all_tels=no;
                    state_sim_wk = 1; // Error
                }
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **********************************************
        // * ������ ������ �������� DC ������ � Siemens *
        // **********************************************
        case 39:
            if ((need_write_telDC_ix== 0)||
                (need_write_telDC_ix==10)||
                (need_write_telDC_ix >20))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            // �������� ����� DC ������ � Siemens
            ClearBuf_Sim ();
           _c_ans_kom_rx = 0;
            tx_data_sim = 'A'; Send_char_sim();
            tx_data_sim = 'T'; Send_char_sim();
            tx_data_sim = '+'; Send_char_sim();
            tx_data_sim = 'C'; Send_char_sim();
            tx_data_sim = 'P'; Send_char_sim();
            tx_data_sim = 'B'; Send_char_sim();
            tx_data_sim = 'W'; Send_char_sim();
            tx_data_sim = '='; Send_char_sim();

            if (need_write_telDC_ix==9)
            {
                tx_data_sim = '1'; Send_char_sim();
                tx_data_sim = '9'; Send_char_sim();
            }
            else
            if (need_write_telDC_ix==11)
            {
                tx_data_sim = '9'; Send_char_sim();
            }
            else
            if (need_write_telDC_ix>=12)
            {
                tx_data_sim = '1'; Send_char_sim();
                tx_data_sim = need_write_telDC_ix-12+0x30; Send_char_sim();
            }
            else
            {
                tx_data_sim = need_write_telDC_ix+0x30; Send_char_sim();
            }

            if (tel_num[need_write_telDC_ix].len_SO==0) // ����� ������� ����� �� ������
            {
                tx_data_sim =  13; Send_char_sim();
                time_wait_SMSPhone = 0; state_sim_wk = 45; return;
            }

            tx_data_sim = ','; Send_char_sim();
            tx_data_sim = '"'; Send_char_sim();
            len_telnum_ascii = GetTel_SO_to_ASCII(&tmp_telnum_ascii[0],
                                                  &tel_num[need_write_telDC_ix].num_SO[0],
                                                   tel_num[need_write_telDC_ix].len_SO);

            for (i=0; i<len_telnum_ascii; i++)
            {
                tx_data_sim = tmp_telnum_ascii[i]; Send_char_sim();
            }
            tx_data_sim = '"'; Send_char_sim();
            tx_data_sim = ','; Send_char_sim();
            if (tmp_telnum_ascii[0]=='+')
            {
                tx_data_sim = '1'; Send_char_sim();
                tx_data_sim = '4'; Send_char_sim();
                tx_data_sim = '5'; Send_char_sim();
            }
            else
            {
                tx_data_sim = '1'; Send_char_sim();
                tx_data_sim = '2'; Send_char_sim();
                tx_data_sim = '9'; Send_char_sim();
            }
            tx_data_sim = ','; Send_char_sim();
            tx_data_sim = '"'; Send_char_sim();
            if (need_write_telDC_ix<=4)
            {
                tx_data_sim = 'D'; Send_char_sim();
                tx_data_sim = 'C'; Send_char_sim();
                tx_data_sim = need_write_telDC_ix+0x30; Send_char_sim();
            }
            else
            if (need_write_telDC_ix<=6)
            {
                tx_data_sim = 'M'; Send_char_sim();
                tx_data_sim = 'D'; Send_char_sim();
                tx_data_sim = need_write_telDC_ix-4+0x30; Send_char_sim();
            }
            else
            if (need_write_telDC_ix==7)
            {
                tx_data_sim = 'V'; Send_char_sim();
                tx_data_sim = 'C'; Send_char_sim();
            }
            else
            if (need_write_telDC_ix==8)
            {
                tx_data_sim = 'U'; Send_char_sim();
                tx_data_sim = 'S'; Send_char_sim();
            }
            else
            if (need_write_telDC_ix==9)
            {
                tx_data_sim = 'B'; Send_char_sim();
                tx_data_sim = 'T'; Send_char_sim();
            }
            else
            if (need_write_telDC_ix<20)
            {
                tx_data_sim = 'M'; Send_char_sim();
                tx_data_sim = 'V'; Send_char_sim();
                tx_data_sim = need_write_telDC_ix-10+0x30; Send_char_sim();
            }
            else
            if (need_write_telDC_ix==20)
            {
                tx_data_sim = 'M'; Send_char_sim();
                tx_data_sim = 'V'; Send_char_sim();
                tx_data_sim = '1'; Send_char_sim();
                tx_data_sim = '0'; Send_char_sim();
            }
            tx_data_sim = '"'; Send_char_sim();
            tx_data_sim =  13; Send_char_sim();
            time_wait_SMSPhone = 0; state_sim_wk = 45;
        break;

        case 45:
            if (_c_ans_kom_rx==1) // Ok. ������� ����� DC ������ � Siemens
            {
                tel_num[need_write_telDC_ix].need_write = no;
                if (need_write_telDC_ix==9)
                {
                    _flag_tel_ready_R = 0; // ������� ����� � �� ����� � �� �������� (������ SC � DC ����).
                    _flag_tel_ready   = 1;
                }
                need_write_telDC_ix = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4))  // ������
            {
                state_sim_wk = 39;
                count_p++;
                if (count_p >= 3)
                {
                    count_p = 0;
                   _flag_need_read_all_tels = no;
                    state_sim_wk = 1; // Error
                }
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **************************
        // *    ����� ��� ������    *
        // **************************
        case 50:
            if (time_wait_SMSPhone >= 10)
            {
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
        break;

        // *********************************************
        // * �������� ������ ��� ������ ���-�� ������� *
        // *********************************************
        case 60: 
            if (_c_ans_kom_rx==1) // ��������
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
               _flag_need_read_all_tels = no;
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
        break;

/*        case 61:
            if (time_wait_SMSPhone >= 1)
            {
                Command_get_RxLev(); // AT+CCED=0,1
                time_wait_SMSPhone = 0; state_sim_wk = 62;
            }
        break;*/

/*      case 61:
            if (_c_ans_kom_rx==17) // �������� RxLev indication
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
               _flag_need_read_all_tels = no;
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
        break;*/

        // ***********************************
        // * �������� ����� � ���� ��� ��� ? *
        // ***********************************
        case 62: // ����� � ���� ��� ���?
            if (_c_ans_kom_rx==22) // ����� � ���� !!!
            {
                //StartRA; // ������������ ������� �����.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx==24)  // �� � ���� !!!
            {
                time_wait_SMSPhone = 0; state_sim_wk = 63; return;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 63;
//             _flag_need_read_all_tels = no;
//              time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 63:
            if (time_wait_SMSPhone > 5)
            {
                Command_test_network(); //AT+CREG?   +CREG: 0,1
                time_wait_SMSPhone = 0; state_sim_wk = 64;
            }
        break;
        case 64:
            if (_c_ans_kom_rx==22) // ����� � ���� !!!
            {
                //StartRA; // ������������ ������� �����.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx==24)  // �� � ���� !!!
            {
                //*------DEBUG------*
                send_c_out_max(0xBB); //"�� � ���� (CREG)"
                send_c_out_max(0x00);
                //*------DEBUG------*
                state_sim_wk = 99; // ������� ������� ��������
                return;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
               _flag_need_read_all_tels = no;
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;


        // ************************************************************************
        // * �������� ������ ��� ������/�������� "���� �� ����� SMS - ���������?" *
        // ************************************************************************
        case 65:
            if (time_wait_SMSPhone >= 1)
            {
                Command_get_read_sms(); state_sim_wk = 66;
            }
        break;

        case 66:
            if ((time_wait_SMSPhone >= 3)||(_flag_new_sms)|| // ����� ����� ��� ��� ������� 1�� sms
                (_c_ans_kom_rx==19))
            {
                Command_get_quality_signal(); // �������� ����� ������ sms
                time_wait_SMSPhone = 0; state_sim_wk = 67;
            }
            // 
			if ((_c_ans_kom_rx==1)&&(_flag_new_sms==0)) // OK ��� ��� (sms � ������ ���)
            {
                ClearBuf_Sim ();
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
			if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 67:
            if ((time_wait_SMSPhone > 2)||(_c_ans_kom_rx==1))  // ��������� ����� �� CSQ
            {
//              if (_flag_new_sms) // ���� ������� ����� sms
//              {
//                  // ���������, � ����� ��� ����� ?
//              }
                ClearBuf_Sim ();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;

        //_ERICSSON_
        // *******************************-------------------------------------------
        // *   ����/��� ������� ������   *  �����, �� ���������
        // *******************************
        case 72:
            GSM_OFF=0; time_wait_SMSPhone1=0; state_sim_wk=73;
        break;
        case 73:
            if (time_wait_SMSPhone1>=10)
            {
                GSM_OFF=1; time_wait_SMSPhone1=0; state_sim_wk=74;
            }
        break;
        case 74:
            if (time_wait_SMSPhone1>=10)
            {
                count_atz=0;
                state_sim_wk=75;
            }
        break;
        // ********************************-------------------------------------------
        // * ���������� ���������� ������ *
        // ********************************
        case 75:
            time_wait_SMSPhone1=0; state_sim_wk = 76;
        break;
        case 76:
            if (time_wait_SMSPhone1>10)
            {
                TEL_BUTTON=0;
                time_wait_SMSPhone1=0; state_sim_wk = 77;
            }
        break;
        case 77:
            if (time_wait_SMSPhone1>=15)
            {
                TEL_BUTTON=1;
                time_wait_SMSPhone1=0; state_sim_wk = 78;
            }
        break;
        case 78:
            if (time_wait_SMSPhone1>=30) state_sim_wk = 79;
        break;
        // *******************************
        // * ���������� ��������� ������ *
        // *******************************
        case 79:
            time_wait_SMSPhone1=0; state_sim_wk = 80;
            TEL_BUTTON=0;
        break;
        case 80:
            if (time_wait_SMSPhone1>=6)
            {
                TEL_BUTTON=1;
                gprs.b.command = 1;
                //gprs.b.rst_gsm = 0;
                gprs.b.gprs_er = 0;
                //gprs.b.abs_off = 1;
                //*------DEBUG------*
                send_c_out_max(0xBD); //"����� ������"
                send_c_out_max(0x00);
                //*------DEBUG------*
                time_wait_SMSPhone1=0; state_sim_wk = 85;
                count_p=0; /*count_atz=0;*/
            }
        break;
        // *******************************--------------------------------------------
        // * ����������� �������� ������ *
        // *******************************
        case 85:
            if (time_wait_SMSPhone1>=3)
            {
                set_com_speed(count_p); state_sim_wk = 86;
            }
        break;
        case 86:
            Send_str_sim("AT\r\0");
            time_wait_SMSPhone1=0; state_sim_wk = 87;
        break;
        case 87: // ���� Ok
            if (_c_ans_kom_rx==1) // ���� OK
            {
                count_atz=0;
                state_sim_wk = 88;
            }
            if (time_wait_SMSPhone1>=2)
            {
                count_p++;
                if (count_p>=10)
                {
                    //set_com_speed(1);/*9600*/ state_sim_wk=88;
                    if (++count_atz<50)
                    {
                        state_sim_wk=75; // ����� ������ ��� ���
                    }
                    else
                    {
                        state_sim_wk=72; // ����\��� ������� ������
                    }

                }
                else
                    state_sim_wk=85;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        // **********************************
        // * ��������� �������� ������ 9600 *
        // **********************************
        case 88:
            Send_str_sim("AT+IPR=9600\r\0"); time_wait_SMSPhone1 = 0; state_sim_wk = 89;
        break;
        case 89: // ���� Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=3)) // ���� OK
            {
                set_com_speed(0);
                time_wait_SMSPhone = 0; time_wait_SMSPhone1 = 0;
              //if (count_atz==0) // ������ ��� ���.�������� � ��������� ��� �� ����������.
              //    state_sim_wk = 90;
              //else
                if (count_p==0) // �������� ���������� - 9600.
                    state_sim_wk = 50;
                else            // �������� ������������ �� ����������, ���� ���������.
                    state_sim_wk = 92;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        
        // ********************************
        // * ����� ��������� �� ��������� *
        // ********************************
/*      case 90:
            if (time_wait_SMSPhone1>=5)
            {
                Send_str_sim("AT&F\r\0");
                count_atz=1;
                time_wait_SMSPhone1=0; state_sim_wk = 91;
            }
        break;
        case 91: // ���� Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=20))
            {
                //��������� �������� � ����� �� ��������������� ��������
                time_wait_SMSPhone1 = 0; state_sim_wk = 85;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;*/
        // ******************************
        // * ���������� �������� ������ *
        // ******************************
        case 92:
            if (time_wait_SMSPhone1>=5)
            {
                Send_str_sim("AT&W\r\0"); time_wait_SMSPhone1=0; state_sim_wk = 93;
            }
        break;
        case 93: // ���� Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=20))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 50;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;



// ���������������������������
// �                         �
// �     �������� ������     �
// �                         �
// ���������������������������
        // *****************************************
        // * ������ ������ connect � ����� ������� *
        // *****************************************
        case 100:
            Command_ATA(); // ������ ������
            time_wait_SMSPhone1 = 0; state_sim_wk = 105;
            time_wait_test = 0;
        break;
        case 105: // �������� CONNECTa
            /* ((md_who_call==4)&&(_c_ans_kom_rx==1)) // voice Ok
            {
                //c_gsmnet_cnt=0; // ����� �������� ��� �������� ��������� ������
                //_need_sleep_gps_charge=0; // ��� ������ �� ������� ������. ����� �������� GPS.
                time_wait_SMSPhone = 0; state_sim_wk = 106;
            }
            else*/
            if (_c_ans_kom_rx==5) // CONNECT
            {
                break_gsm_req();
                //http.b.io_busy = 0;
                //DEBUG
                send_c_out_max(0xB1); //"connect (GSM)'"
                send_c_out_max(0x01);
                send_c_out_max(0x00);
                //c_gsmnet_cnt=0; // ����� �������� ��� �������� ��������� ������
                // ����� ������
/*              if (md_who_call==1) md_mode = 2; // 2-��������� �������� �� ������
                else
                if (md_who_call==4) md_mode = 5; // 5-�����
                else
                if (md_who_call>1)
                {
                    md_mode = 1; // 1-���������� ������ �� ������
                    // ���� ����� "5 ������", �� ��� ����� ������ ������ 51.
                  #ifdef MODE_5S // ��� ����������� ������ 5 ������
                    if (md_who_call==5)
                    {
                        //sms_receive_buffer_len=2;
                        //sms_receive_buffer[0] = 0x51;
                        //sms_receive_buffer[1] = 0xFF;
                       _flag_new_sms = yes; // ���� "�������� ����� ���������"
                        index_sms=0;

                        sms_receive_buffer_len=140;
                        sms_receive_buffer[0]=0x31;
                        for (i=1; i<=72; i++) sms_receive_buffer[i]=0xff;
                        sms_receive_buffer[73]=4;
                        sms_receive_buffer[74]=0x48; sms_receive_buffer[75]=0x6F; sms_receive_buffer[76]=0x10;
                        sms_receive_buffer[77]=0x00; sms_receive_buffer[78]=0x04; sms_receive_buffer[79]=0xF0;

                        pChar = &Timer_inside;
                        sms_receive_buffer[80]=pChar[0];
                        sms_receive_buffer[81]=pChar[1];
                        sms_receive_buffer[82]=pChar[2];
                        sms_receive_buffer[83]=pChar[3];
                        sms_receive_buffer[84]=pChar[4];
                        sms_receive_buffer[85]=pChar[5];
                        for (i=86; i<=137; i++) sms_receive_buffer[i]=0;
                        Int=0;
                        for (i=0;  i<=137; i++) Int+=sms_receive_buffer[i];
                        pInt = &sms_receive_buffer[138];
                        (*pInt) = Int;
                    }
                  #endif
                }
                //md_f_modem_conect = yes; // ���� "�������� ����������"
               _online=no; // ���� ��� ������ online

                time_wait_SMSPhone2 = 0;
                time_wait_SMSPhone  = 0; state_sim_wk = 110; //return; // �������� �������
*/
                time_wait_SMSPhone1 = 0; state_sim_wk = 110;
            }
            if (((md_who_call==5)&&(time_wait_SMSPhone1>21)) //����� ���������,� �� �� ������ �� 5 ������.
                ||
                (time_wait_SMSPhone1>600) // �� ��������� ����������
                ||
                (_c_ans_kom_rx==3)) // ��� ���������� "no carrier"
            {
                md_mode = 0; // 0-���������� ������ �� sms
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // ������ ����������
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 110:
            if (GSM_DCD==0) // ���� ����������
            {
                StK8.b.fDataCon=1;
                state_sim_wk = 112;
            }
            if (time_wait_SMSPhone1>=150) // �� ���������
            {
                md_mode = 0; // 0-���������� ������ �� sms
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // ������ ����������
            }
        break;

        case 112:
            if (StK8.b.fDataCon==0) // ���������� ���������
            {
                md_mode = 0; // 0-���������� ������ �� sms
                c_flag_sms_send_out = 1; // ����� ����
                state_per_sim_wk = 1;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;

/*
        case 106:
            Command_CPAS();
            time_wait_SMSPhone1 = 0; state_sim_wk = 107;
        break;
        case 107:
            if (time_wait_SMSPhone1 >= 10) state_sim_wk = 106;
            if (time_wait_SMSPhone >= 180)
            {
                md_mode = 0; // 0-���������� ������ �� sms
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1 = 10; state_sim_wk = 120; // ������ ����������
            }
            if (_c_ans_kom_rx==3) // no carrier
            {
                md_mode = 0; // 0-���������� ������ �� sms
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone = 0; state_sim_wk = 10; // ����� ���������
            }
            _c_ans_kom_rx = 0;
        break;

        // Com_B_QueryVersion  0x4A    // ������ ������ 
        // Com_T_Version       0x3A    // ����� ������
        // Com_B_NotReload     0x4B    // ���������� �� ���������
        // Com_B_Reload        0x4C    // ��������� ����������
        //                     0x6D    // ������ �� ������
        // ***************************
        // *  ����� �������� ������  *
        // *     �������� ������     *
        // ***************************
        case 110:
            if (md_who_call==5) ; // � ������ 5 ������ �� �������� �� �������
            else
            if (_c_ans_kom_rx==Com_B_QueryVersion) // Com_B_QueryVersion (4A) - ������ ������
            {
                // ����� ������ ��������� � ����� (Com_T_Version)
                Command_ver();
                md_mode=2; // ����� ��������� ��������� ��������.
                time_wait_SMSPhone = 0; return;
            }
            else
            if (_c_ans_kom_rx==Com_B_NotReload) // Com_B_NotReload (4B) - ���������� �� ���������
            {
                md_mode = 0; // 0-���������� ������ �� sms
                break_gsm_req();
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; return; // ������ ����������
            }

            // ********************************
            // * ��������� �������� �� ������ *
            // ********************************
            if (md_mode==2) // ����. �������� ��������� ������ ��� ������� ������.
            {
                if (_c_ans_kom_rx==Com_B_Reload) // Com_B_Reload (4C) - ������� �� ���������
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 115; return;
                }
                else
                if (_c_ans_kom_rx==3) // no carier - ������ �����
                {
                    md_mode = 0; // 0-���������� ������ �� sms
                    ClearBuf_Sim();
                    c_flag_sms_send_out = 1; // ����� ����
                    time_wait_SMSPhone = 0; state_sim_wk = 10; return;
                }
            }

            // *******************************
            // * ���������� ������ �� ������ *
            // *******************************
            if ((md_mode==1)||(md_mode==5)) // �������� ������ ��� �����
            {
                if (_c_ans_kom_rx==3) // no carier - ������ �����
                {
                    md_mode = 0;
                    break_gsm_req();
                    c_flag_sms_send_out = 1; // ����� ����
                    state_per_sim_wk = 10;
                    ClearBuf_Sim();
                    time_wait_SMSPhone = 0; state_sim_wk = 10; return;
                }
                else
                // ���� �� � ������ "5 ������" � ������ ������� �� ��
                if ((md_who_call!=5)&&(_c_ans_kom_rx==0x6D)) // "md"
                {
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 155; return;
                }
                else
                if (md_f_need_send) // ���������� ������� ����� �� ��
                {
                    //ClearBuf_Sim();
                   //_c_ans_kom_rx = 0;
                    // ���� ��������� � ������ ������ �� gps, �� ������� CPU � �����
                    // ���������� ������� ����� �� ������ (�� ������� �������) ����
                    // ���� ������ �� ��.
                    //if (_need_sleep_gps_charge) time_sleep_gps=0;
                    md_f_need_send = 0;
                    md_npack_mod_inc++;
                    state_sim_wk = 160; return;
                }
            }

            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;

            if (
                // ���� �������� ������ � ������ "5���" �������������
                ((md_mode==1)&&(md_who_call==5)&&(time_wait_test>29)) ||
                // ���� � �� ��� ���������� � ������� 1 ������
                ((time_wait_SMSPhone >= 50)&&((md_mode==1)||(md_mode==2))) ||
                // ���� � ��������� ����� ������ 3 ������
                ((time_wait_SMSPhone >= 180)&&(md_mode==5)) // voice
                //(time_wait_SMSPhone2>=15)
               )
            {
                // ��������� �����
                md_mode = 0; // 0-���������� ������ �� sms
                md_f_need_send = 0;
                break_gsm_req();
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // ������ ����������
            }
            if (GSM_DCD) //���������� ��� ���������
            {
                state_per_sim_wk = 10;
                ClearBuf_Sim();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;

        case 115: // ������� 3C "1234" 0D (Com_T_Restart) � ������� �� ���������
            if (time_wait_SMSPhone >= 1)
            {
                // ����� Com_T_Restart (3�) - �������� �� ���������
                Command_1234();
                // ���������� ������� �� ���������.
                time_wait_SMSPhone = 0; state_sim_wk = 117; // ������ ����������
            }
        break;

        case 117:
            //if (SM_BUSY==0)
            {
                // ���������� ������� �� ���������.
                RestartToLoader();
            }
        break;
*/
        case 120: // ������ ����������
            if (time_wait_SMSPhone1 >= 12)
            {
               _online=no; // ���� ��� ������ online
                c_flag_sms_send_out = 1; // ����� ����
                Command_3p(); // ������� � ����� �������        <<< +++ >>>
                time_wait_SMSPhone1 = 0; state_sim_wk = 125;
                GSM_DTR_OFF;
            }
        break;

        case 125:
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1 >= 12)) // OK
            {
                GSM_DTR_ON;
                //Command_ATH(); // �������� ������
                Command_CHLD_1();
                time_wait_SMSPhone = 0; state_sim_wk = 126;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

        case 126:
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==3)||(time_wait_SMSPhone >= 2)) // NO CARRIER
            {
                md_mode=0;
                c_flag_sms_send_out = 1; // ����� ����
                ClearBuf_Sim();
state_per_sim_wk = 1;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

        // ****************************
        // * ��������� �������� RINGa *
        // ****************************
        case 140:
            if (_c_ans_kom_rx==10) // +CRING +CLIP
            {
                time_wait_SMSPhone = 0; state_sim_wk = 141;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 10) // �� ���������
            {
                ClearBuf_Sim(); _c_ans_kom_rx = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;
        case 141:
            if (_c_ans_kom_rx==10) // +CRING +CLIP
            {
                time_wait_SMSPhone = 0;
                state_sim_wk = 145;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 8) // �� ���������
            {
                ClearBuf_Sim(); _c_ans_kom_rx = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;
        // *********************************************************
        // * ����������� ������ ��������� �������� ����� ���� RING *
        // *********************************************************
        case 145:
            // ������ ��������� ��� ��������. �������� ���������.
            if (md_who_call==0)
            {
                // ��������� �� ������
                time_wait_SMSPhone1 = 5; state_sim_wk = 125; // ����� �����. ATH.
                return;
            }
            else
            // ��� ��������� �������� ��� �������� ������?
            {
                time_wait_SMSPhone = 0; state_sim_wk = 100; // ������� �� ������ ������
                return;
            }
        break;
/*
        // ***********************************************
        // *               ������ �� ������              *
        // ***********************************************
        case 155:
           _c_ans_kom_rx = 0;
            time_wait_SMSPhone2 = 0;

            sms_receive_buffer_len = buf_read_sim[3];
            md_CS = 0;
            // ������� ��
            for (i=0; i<(sms_receive_buffer_len+8); i++) md_CS ^= buf_read_sim[i];
            // �������� ��
            if (md_CS != buf_read_sim[sms_receive_buffer_len+8])
            { state_sim_wk = 156; return; } // �� �� �������, ��������� ������
          //{ state_sim_wk = 110; return; } // ���� ��������� �������
            // �� �������, ������� ������
            if (buf_read_sim[2]==1) // ������
            {
                //md_npack_mod_inc = 1;
                for (i=0; i<sms_receive_buffer_len; i++)
                    sms_receive_buffer[i] = buf_read_sim[i+8];
               _flag_new_sms = yes; // ���� "�������� ����� ���������"
                index_sms=0;
            }
            else
            if ((buf_read_sim[2]==0x06)||(buf_read_sim[2]==0x19))
            {
                time_wait_SMSPhone=0; state_sim_wk = 110; return; // ���� ��������� �������
            }
            else // ����������� �������, ��������� ������
            { state_sim_wk = 156; return; } // ��������� ERROR
          //{ state_sim_wk = 110; return; } // ���� ��������� �������

            // ���� ��������� �������
            time_wait_SMSPhone=0; state_sim_wk = 110; return;
        break;

        // ��������� ERROR
        case 156:
            tx_data_sim = 'm'; Send_char_sim(); // 6D
            tx_data_sim = 'd'; Send_char_sim(); // 64
            tx_data_sim =0x19;  Send_char_sim(); // type=0 - error
            tx_data_sim =  0;  Send_char_sim(); // len
            tx_data_sim =  0;  Send_char_sim(); // np
            tx_data_sim =  0;  Send_char_sim(); //
            tx_data_sim =  0;  Send_char_sim(); // 
            tx_data_sim =  0;  Send_char_sim(); // 
            tx_data_sim = 0x10; Send_char_sim(); // CS
            state_sim_wk = 110; // ���� ��������� �������
        break;

        // ***********************************************
        // *          ������� ������ �� ������           *
        // ***********************************************
        case 160:
            if (!cBufTxW)
            {
            tx_data_sim = 'm'; Send_char_sim();
            tx_data_sim = 'd'; Send_char_sim();
            tx_data_sim =  2; Send_char_sim();
            tx_data_sim = sim_line_tx_len; Send_char_sim();
            tx_data_sim = md_npack_mod_inc; Send_char_sim();
            tx_data_sim =  1; Send_char_sim(); // �������� ����� ����� �������������
            tx_data_sim =  0; Send_char_sim();
            tx_data_sim =  0; Send_char_sim();
            md_CS = 'm'^'d'^2^sim_line_tx_len^md_npack_mod_inc^1^0^0; // �����.����� ���������
            state_sim_wk = 161;
            count_p=0;
            }
        break;

        case 161:
            if (cBufTxW < SIZE_BUF_TX_UART0)
            {
                md_CS ^= sim_line_tx[count_p];
                tx_data_sim = sim_line_tx[count_p]; Send_char_sim();
                if (++count_p>=sim_line_tx_len) state_sim_wk = 162;
            }
        break;

        case 162:
            tx_data_sim = md_CS; Send_char_sim();

         c_flag_sms_send_out = 1; // ����� ����
         //DEBUG
         send_c_out_max(0xB8); //"send sms"
         send_c_out_max(0x0D);
         send_c_out_max(0x00);

            if ((sim_line_tx[0]==0x51)||(sim_line_tx[0]==0x67)
#ifdef MODE_5S // ��� ����������� ������ 5 ������
               ||(md_who_call==5)
#endif
               )
            {
                md_mode = 0; // 0-���������� ������ �� sms
                break_gsm_req();
                md_f_need_send = 0;
                c_flag_sms_send_out = 1; // ����� ����
                time_wait_SMSPhone1=0; state_sim_wk = 120; // ������ ����������
            }
            else
            {
                time_wait_SMSPhone=0;
                state_sim_wk = 110; // ���� ��������� �������
              //state_sim_wk = 161; // ����� ����, ���� �������������
            }
        break;
*/
// ����������������������
// �                    �
// �      G P R S       �
// �                    �
// ����������������������
        // ******************************************************
        // *                                                    *
        // *  ���� ���� � ��������� ������ (� PPP),             *
        // *  �� ������� ������� � ���������� TCP,              *
        // *  ����� ���� � PPP GPRS                             *
        // *                                                    *
        // ******************************************************
        case 170://100
            Command_ATO();
            time_wait_SMSPhone1=0; state_sim_wk = 171;
        break;
        case 171://101
            if (_c_ans_kom_rx==5) // Ok �� � ����������
            {
                //count_p = 0;
                //DEBUG
                send_c_out_max(0xB5); //"ATO"
                send_c_out_max(0x00);
                time_wait_SMSPhone = 0;
                state_sim_wk = 172;//102; // ����� ����� DCD
                return;
            }
            if ((_c_ans_kom_rx==4)||(_c_ans_kom_rx==3)) // ��� �� �� � ����������
            {
                count_p = 0;
                md_mode = 0; // 1-���������� ������ �� ������
                state_sim_wk = 174;//104; // ���� �������
                //gprs.b.gprs_on = 0;
            }
            if (time_wait_SMSPhone1>20) // �� ��������
            {
                //DEBUG
                send_c_out_max(0xB6); //"ATO error"
                send_c_out_max(0x00);
                count_p++;
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                    return;
                }
                state_sim_wk = 170;//100; // ��������� �� ��� �����.
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 172://102
            if (GSM_DCD==0) // ���� connect
            {
                count_p = 0;
                //DEBUG
                send_c_out_max(0xBA); //"DCD=0 - connect"
                send_c_out_max(0x00);
                md_mode = 6; // 6-GPRS //1-���������� ������ �� ������
                gprs.b.command = 0; // ����� ������ ��������
			//gprs.b.command = 1;
                state_sim_wk = 180;//110; // �������� ����� ������ �������� ������ �� GPRS
            }
            else
            if (time_wait_SMSPhone>=20)
            {
                count_p++;
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                    return;
                }
                state_sim_wk = 170;//100;
            }
        break;

        // *****************
        // *               *
        // *  ���� � GPRS  *
        // *               *
        // *****************
        case 174://104:
            StSYSTEM |= 0x0100; // ������ �������.
            Command_E2IPA(); // ���� � GPRS (PPP)
            time_wait_SMSPhone=0; state_sim_wk = 175;
        break;
        case 175://105:
            if (_c_ans_kom_rx==1) // OK
            {
                time_nogprs=0;
                count_p = 0;
                time_wait_SMSPhone=4;
                state_sim_wk = 176;//106;
            }
            else
            if //((_c_ans_kom_rx==4) || // error
                (time_wait_SMSPhone>8)
            {
                //DEBUG
                send_c_out_max(0xB9); //"�� ������ � gprs"
                send_c_out_max(0x00);
                md_mode = 0; // 1-���������� ������ �� ������
                count_p++;
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    time_nogprs=120; // GPRS ��� �������� 2 ������ �����, ����� ����� ��������� ���.
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                   _flag_need_read_all_tels=0; // �� ���� ������ ��� ������ ���������.
                    state_sim_wk = 1; // Error. �����������������.
                    return;
                }
                state_sim_wk = 174;//104; // ��� ���
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **********************************
        // *                                *
        // *  ����������� � ������� � GPRS  *
        // *                                *
        // **********************************
        case 176://106
            if (time_wait_SMSPhone>=5)
            {
                Command_E2IPO(); // ����������� � ������� � GPRS (TCP)
                time_wait_SMSPhone=0; state_sim_wk = 177;//107;
            }
        break;

        case 177://107:
            if (_c_ans_kom_rx==5) // CONNECT
            {
                time_nogprs=0;
                count_p = 0;
                md_mode = 6; // 6-GPRS //1-���������� ������ �� ������
                //DEBUG
                send_c_out_max(0xB1); //"connect (GPRS TCP)"
                send_c_out_max(0x00);

                time_wait_SMSPhone = 0;
                state_sim_wk = 172;//102; // �������� DCD � ����� ����� ������ �������� ������ �� GPRS
                return;
            }
            if ((time_wait_SMSPhone>76) || // �� ��������� ����������
                (_c_ans_kom_rx==3) ||      // ��� ���������� "no carrier"
                (_c_ans_kom_rx==4))
            {
                //*------DEBUG------*
                send_c_out_max(0xBE); //"Error"
                send_c_out_max(0x00);
                //*------DEBUG------*
                    //gprs.b.gprs_on = 0;
                md_mode = 0; // 1-���������� ������ �� ������
                count_p++;
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    time_nogprs=120; // GPRS ��� �������� 2 ������ �����, ����� ����� ��������� ���.
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; //118; // Close gprs.
                    state_per_sim_wk = 1;
                    //gprs.b.f_cycle=1;
                    return;
                }
                time_wait_SMSPhone = 0;
                state_sim_wk = 176;//106; // ��� ���
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ***************************************************
        // *  �������� ����� ������ �������� ������ �� GPRS  *
        // ***************************************************
        case 180://110
            if (gprs.b.command)
            //if (time_wait_SMSPhone>=10)
            //if ((!GSM_RI)||(_flag_need_send_sms))
            {
                md_mode = 0; // 1-���������� ������ �� ������
                count_p = 0;
                time_wait_SMSPhone1=0;
                state_sim_wk = 181;
            }
        break;
        case 181:
            if (time_wait_SMSPhone1>=5)
            {
                if (_flag_need_send_sms) //���� ��������� �� ���
                    state_per_sim_wk=8;
                else
                if ((gprs.b.gprs_er)||(timer_gprs_glob==0))
                    state_per_sim_wk=1;
                else
                    state_per_sim_wk=10;
                state_sim_wk = 182;
            }
        break;

        // ************************************
        // *   ������� � ����� ������ �       *
        // *   ������� � ����� ������ ������  *
        // ************************************
        case 182://112 // DTR 
            GSM_DTR_OFF;
            time_wait_SMSPhone1 = 0; state_sim_wk = 183;//113;
        break;
        case 183://113
            if (time_wait_SMSPhone1 > 5) //0.5 c
            {
                GSM_DTR_ON;
                time_wait_SMSPhone = 0; state_sim_wk = 184;//114;
            }
        break;
        case 184://114
            if (_c_ans_kom_rx==1) // OK ���� ����� ������ !
            {
                count_p = 0;
                md_mode = 0; // 1-���������� ������ �� ������
                //DEBUG
                send_c_out_max(0xB2); //"command mode ok"
                send_c_out_max(0x00);
                //if (gprs.b.gprs_er) state_sim_wk = 188;//118; // ���� ����� �� GPRS
                //else { state_sim_wk = 10; md_mode = 0; }
               _c_ans_kom_rx = 0;
                state_sim_wk = 10;
                time_wait_SMSPhone = 0;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
            if (time_wait_SMSPhone > 3)
            {
                //count_p++;
                //state_sim_wk = 112;
                //DEBUG
                send_c_out_max(0xB3); //"error command mode"
                send_c_out_max(0x00);
                //if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    md_mode = 0; // 1-���������� ������ �� ������
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; //188;//118; // ���� ����� �� GPRS
                }
            }
        break;

        // ****************************
        // *   ������ ����� �� GPRS   *
        // *                          *
        // ****************************
/*      case 186://116 // +++AT   (���� ���� CONNECT � �� � ��������� ������)
            if (time_wait_SMSPhone1 >= 12)
            {
                Command_3p(); // ������� � ����� �������        <<< +++AT >>>
                time_wait_SMSPhone = 0; state_sim_wk = 187;//117;
            }
        break;
        case 187://117
            if ((_c_ans_kom_rx==3)||(_c_ans_kom_rx==1)) // NO CARRIER ��� ok ���� ��� � ��������� ������
            {
                state_sim_wk = 188://118;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
            if (time_wait_SMSPhone >= 5) // �� ���������
            {
                count_p++;
                state_sim_wk = 186;//116;
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    md_mode = 0; // 1-���������� ������ �� ������
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                }
            }
        break;*/
/*
        case 188://118 // AT*E2PA=0,1 - ����� �� GPRS
            Command_E2IPA0();
            time_wait_SMSPhone = 0; state_sim_wk = 189;//119;
        break;
        case 189://119
            if (_c_ans_kom_rx==1) // OK
            {
                count_p = 0;
                //DEBUG
                send_c_out_max(0xB0); //
                send_c_out_max(0x02);
                send_c_out_max(0x00);
                time_wait_SMSPhone = 0;
                state_sim_wk = 190;//120;
            }
            if ((_c_ans_kom_rx==4)||(time_wait_SMSPhone>=3)) // Error
            {
                count_p++;
                state_sim_wk = 188;//118;
                //DEBUG
                send_c_out_max(0xB4); //"error. �� �����������"
                send_c_out_max(0x02);
                send_c_out_max(0x00);
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    md_mode = 0; // 1-���������� ������ �� ������
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                   _flag_need_read_all_tels=0; // �� ���� ������ ��� ������ ���������.
                    state_sim_wk = 1; // Error
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 190://120 // AT*E2PC - close connection
            Command_E2IPC();
            time_wait_SMSPhone = 0; state_sim_wk = 192;//121;
        break;
        case 191://121
            if (_c_ans_kom_rx==1) // OK
            {
                //count_p = 0;
                //DEBUG
                send_c_out_max(0xB0); //
                send_c_out_max(0x01);
                send_c_out_max(0x00);
                time_wait_SMSPhone = 0;
                state_sim_wk = 192;//122;
            }
            if ((_c_ans_kom_rx==4)||(time_wait_SMSPhone>=30)) // Error
            {
                count_p++;
                state_sim_wk = 190;//120; // !!!
                //DEBUG
                send_c_out_max(0xB4); //"error. �� �����������"
                send_c_out_max(0x01);
                send_c_out_max(0x00);
                if (count_p >= 3) // ��������� 3 ����, �� ���, ������ �� ������.
                {
                    md_mode = 0; // 1-���������� ������ �� ������
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 192://122 // ��������� DCD==1
            if (GSM_DCD==1) // no connect
            {
                count_p = 0;
                StSYSTEM &= ~0x0100; // ������ �����.
                //DEBUG
                send_c_out_max(0xB0); //"������������" "closing connect"
                send_c_out_max(0x00);
                md_mode = 0; // 1-���������� ������ �� ������
                c_flag_sms_send_out = 1; // ����� ����
                ClearBuf_Sim();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
            else
            if (time_wait_SMSPhone > 5)
            {
                //DEBUG
                send_c_out_max(0xB4); //"ATH - error"
                send_c_out_max(0x00);
                count_p++;
                if (count_p >= 3) // ��������� 5 ���, �� ���, ������ �� ������.
                {
                    md_mode = 0; // 1-���������� ������ �� ������
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                    return;
                }
                time_wait_SMSPhone = 0; state_sim_wk = 190;//120; // ��������� �� ��� �����.
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

*/


        // *****************
        // *   AT+COPS?    *  // ������� "����������� � ����?".
        // *****************
        case 195://190 // ����� � ���� ��� ���?
            if (_c_ans_kom_rx==22) // ����� � ���� !!!
            {
                StartRA; // ������������ ������� �����.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((_c_ans_kom_rx==24)||(_c_ans_kom_rx==4))  // �� � ���� !!!
            {
                time_wait_SMSPhone = 0; state_sim_wk = 196; return;
            }
            if (time_wait_SMSPhone >= 10)
            {
               _flag_need_read_all_tels = no;
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        case 196:
            if (time_wait_SMSPhone > 5)
            {
                Command_network_registration();
                time_wait_SMSPhone = 0; state_sim_wk = 197;
            }
        break;
        case 197:
            if (_c_ans_kom_rx==22) // ����� � ���� !!!
            {
                StartRA; // ������������ ������� �����.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((_c_ans_kom_rx==24)||(_c_ans_kom_rx==4))  // �� � ���� !!!
            {
                //*------DEBUG------*
                send_c_out_max(0xBC); //"�� � ���� (COPS)"
                send_c_out_max(0x00);
                //*------DEBUG------*
                state_sim_wk = 99; // ������� ������� ��������
                return;
            }
            if (time_wait_SMSPhone >= 10)
            {
               _flag_need_read_all_tels = no;
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // *******************************************
        // * �������� ������ ������������ � sms/gprs *
        // *******************************************
/*      case 200:
            if ((_c_ans_kom_rx)|| // �� (����� �������) ����������
                (time_wait_SMSPhone >= 5))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;*/

        // ***************************
        // * ������                  *
        // ***************************
        default:
            ClearBuf_Sim ();
            Restart_phone();
        break;
    }
}

// ������ ����� �������� (� ����������� �������)
//
void Restart_phone (void)
{
    StSYSTEM &= ~0x0100; // ������ �����.
    // �������� �� ������ �������� � ��������� � ����. �������� ������������ ���������� ������ ��������
    count_restart_gsm++;
    mResult.b.reset_gsm = 1; //����������� ������� �� reset'�:

    c_flag_sms_send_out = 0;
    current_sms_signal_level = 0xff;

   _flag_tel_ready         = no;  // ������� ���� �� �����
   _flag_tel_ready_R       = no;  // ����� ����� ������ �� �����

   _flag_need_read_all_tels=yes; // ���������� ��������� ��� ������ ���������.

    time_wait_SMSPhone = 0; state_sim_wk = 75;
}

// ******************************************************************
// ������� ����� � ������?
// Ret: 1-��, 0-���
// 
/*bit Is_tel_ready (void)
{
    return _flag_tel_ready;
}

// ******************************************************************
// ������� ����� � ������?
// Ret: 1-��, 0-���
// 
bit Is_tel_ready_R (void)
{
    return _flag_tel_ready_R;
}*/


// ******************************************************************
// �������� �������� �������.
// Ret: ���� ���-�� �������
// 0 - ��� ������� ��� ������ ������
// 1 - ������� �� ������� ������� � ��������
// 2 - ���� ������
// 3 - ���� ����������
//
unsigned char Get_quality_signal (void)
{
    //return current_sms_signal_level;
    //return current_sms_signal_level/6;
         if (current_sms_signal_level ==  0) return 0;
    else if (current_sms_signal_level <=  4) return 1; // 1-4
    else if (current_sms_signal_level <=  9) return 2; // 5-9
    else if (current_sms_signal_level <= 15) return 3; // 10-15
    else if (current_sms_signal_level <= 31) return 4; // 16-23
//  else if (current_sms_signal_level <= 31) return 5; //  x-31
    return 0xff;
}



// ******************************************************************
// ���� �� ����� sms ���������?
// Ret: 1-��, 0-���
//
bit Is_new_sms (void)
{
    if ((_flag_new_sms)&&((index_sms>0)||(md_mode!=0))) return 1;
    return 0;
}

// ��� 8 ������ sms ?
bit Is_new_sms_8bit (void)
{
    return (_f_SMS_accept_type_8bit);
}

// ������� �������� sms ��������� �� ������ ��������.
//
void Delete_sms (void)
{
   _flag_new_sms = no;
    sms_receive_buffer_len = 0;
    // ���������� ������� �������� sms - ���������.
    if ((md_mode==0)&&(index_sms>0)) { _flag_need_delete_sms = yes; n_mem_sms = index_sms; }
}

// ���� ����������� sms ��������� � ������ ��������?
// Ret: 1-��, 0-���
//
bit IsNonDelete_sms (void)
{
    return (_flag_need_delete_sms);
}

// ******************************************************************
// ��������� ����� sms ���������.
// Ret: ����� sms - ���������.
//
unsigned char Read_new_sms (void)
{
  unsigned char Len;
    if (_flag_new_sms==no) return 0;
    //_flag_new_sms = no;
    pBuf_read_data = &sms_receive_buffer[0];
    Len = sms_receive_buffer_len;
    Delete_sms();
    return Len;
}

void Send_modem (unsigned char *buf_sms, unsigned char len_sms)
{
  unsigned char i;
    for (i=0; i<len_sms; i++)
    {
        sim_line_tx[i] = buf_sms[i];
    }
    md_f_need_send = yes;
    c_flag_sms_send_out = 2; // ����� ��� �� ���������.
    sim_line_tx_len = len_sms;
}

void do_pack_tel_8b_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Fmt)
{
  unsigned char xdata i;
  unsigned char xdata j;
  unsigned char xdata n=0;
  unsigned char xdata ix;

// Fmt - ����:
//  7   - 0-��� �������������.   1-� ��������������.
//  6   - 0-(8-������ ��������). 1-(7-������ ��������).
//  5   - ------
//  4-0 - ������ ������ ��������.

    ix = Fmt & 0x1F; // ������ ������ ��������.

//  sim_line_tx[n++] = 0x00;
    sim_line_tx[n++] = tel_num[0].len_SO;
    for (i=0; i<tel_num[0].len_SO; i++) sim_line_tx[n++] = tel_num[0].num_SO[i];

/*!V!*/    _flag_need_wait_SR = yes; sim_line_tx[n++] = 0x31;
//    if (Fmt&0x80) { _flag_need_wait_SR = yes; sim_line_tx[n++] = 0x31; } // 0x31  �  ��������������
//             else { _flag_need_wait_SR =  no; sim_line_tx[n++] = 0x11; } // 0x11 ��� �������������
    sim_line_tx[n++] = 0x00;

    j = tel_num[ix].len_SO-1;
    if ((tel_num[ix].num_SO[j]&0xF0)==0xF0) j=j*2-1; else j=j*2;
    sim_line_tx[n++] = j;    //0x0B;

    for (i=0; i<tel_num[ix].len_SO; i++) sim_line_tx[n++] = tel_num[ix].num_SO[i];
    sim_line_tx[n++] = 0x00; //

    // ����� �� "�"
    if (Fmt&0x40) sim_line_tx[n++] = 0x00; // 00 - ��� 7-������� ���������.
             else sim_line_tx[n++] = 0xF6; // F6 - ��� 8-������� ���������. 

    sim_line_tx[n++] = 0xA8;

    // ����� �� "�"
    if (Fmt&0x40) sim_line_tx[n++] = (unsigned char)(((int)len_sms)*8/7); // 7-������ ��������.
             else sim_line_tx[n++] = len_sms;                             // 8-������ ��������.

    for (i=0; i<len_sms; i++)
    {
        sim_line_tx[n++] = buf_sms[i];
    }
    sim_line_tx_len = n;
}


// ������� sms ���������.
// Buf - ��������� �� ������ � sms - ����������.
// Len - ����� sms - ���������.
// Num - ����� Disp-������
//
void Send_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Num)
{
    // ���������� ������ ��� ��������
    do_pack_tel_8b_sms (&buf_sms[0], len_sms, Num);
    // ���������� ������� �������� sms - ���������.
    _flag_need_send_sms = yes;
    c_flag_sms_send_out = 2; // ����� ��� �� ���������.
}


// ���� ������������� �� �������� sms ���������?
// Ret: 1-��, 0-���, 2-���� ������
//
unsigned char Is_sms_send_out (void)
{
    return c_flag_sms_send_out;
}

// �������� � ������� (� ���������� ������) ����� �����.
// Buf - ��������� �� ������ � ���������� �������.
// ix  - ������ ����������� ������.
//
void Write_tel_to_phone (unsigned char ix, unsigned char *buf)
{
    unsigned char xdata i;

    if (buf[1]==0x0F) // ������ ����� (��� ������)
    {
        tel_num[ix].len_SO = 0; for (i=0; i<kolBN; i++) tel_num[ix].num_SO[i] = 0xff;
        tel_num[ix].need_write = yes; // ������������� �������� ����� �������� (����)
    }
    else
    {
        // ��������� ������ �������� �� ������ buf
        // � ������� SO:    0x0B 0x97 0x20 0x47 0x95 0x99 0xF9 ... 0x00 0x97 0x20 0x47 0x95 0x99 0xF9
        // "+" �� ����������.
        tmp_telnum[0] = 0x91; // '+'  ����������� �������������. � ����� ������.
        // ��� ������� "+79.." ���������� 0x91 0x97 .. �.�. "+79.." - ���������
        // ��� ������� "+88.." ���������� 0x91 0x88 .. �.�. "+88.." - ���������
        // ��� ������� "89..." ���������� 0x91 0x88 .. �.�. "+89.." - �� ��������� !!!
        i=1;
        while (1)
        {
            if (i>=kolBN) { i=0; break; }
            tmp_telnum[i] = buf[i];
//          if ((((tmp_telnum[i])&0x0f)==0x0f) ||
//              (((tmp_telnum[i])&0xf0)==0xf0)) { i++; break; }
            if   (tmp_telnum[i]       ==0x0f)        break;
            if (((tmp_telnum[i])&0xf0)==0xf0) { i++; break; }
            i++;
        }
        len_telnum = i;

        if (len_telnum<4) {tel_num[ix].len_SO = 0; return;}
        tel_num[ix].len_SO = len_telnum; for (i=0; i<len_telnum; i++) tel_num[ix].num_SO[i] = tmp_telnum[i];

        tel_num[ix].need_write = yes; // ������������� �������� ����� �������� (����)

        need_write_telDC_ix = 0;
    }
}

void full_buf_num_tel(unsigned char *buf)
{
// ****************
// ������ ���������
// ****************
  unsigned char xdata i;
  unsigned char xdata j;

    // 9 ������� (108 ����)
    for(i=0;i<108;i++) buf[i] = 0;
    for(j=0; j<9; j++) for(i=0; i<tel_num[j].len_SO; i++) buf[i+j*12] = tel_num[j].num_SO[i];
}
void full_buf_num_tel2(unsigned char *buf)
{
// ****************
// ������ ���������
// ****************
  unsigned char xdata i;
  unsigned char xdata j;

    // 11 ������� (132 ����)
    for(i=0;i<132;i++) buf[i] = 0;

    // 10 ������� � 11 �� 20
    for(j=0; j<10; j++) for(i=0; i<tel_num[j+11].len_SO; i++) buf[i+j*12] = tel_num[j+11].num_SO[i];
    //  1 �����   9 (BT)
    for(i=0; i<tel_num[9].len_SO; i++) buf[i+10*12] = tel_num[9].num_SO[i];
}


/**
* @function RestartToLoader
* �������� ���������� ����������
*/

//�������� ���������� ����������
void RestartToLoader(void)
{
  unsigned char data i = 0;
  unsigned char xdata * pwrite = 0x0000;

  //Loader's IVT address
  unsigned char code *pBeginAddr = 0x1FBB;

  //Copy Loader's IVT
    FlashDump[i++] = *pBeginAddr++;
    FlashDump[i++] = *pBeginAddr++;
    FlashDump[i++] = *pBeginAddr++;
    while (i < 3+22*8)
    {
        FlashDump[i++] = *pBeginAddr++;
        FlashDump[i++] = *pBeginAddr++;
        FlashDump[i++] = *pBeginAddr++;
        FlashDump[i++] = 0x00;
        FlashDump[i++] = 0x00;
        FlashDump[i++] = 0x00;
        FlashDump[i++] = 0x00;
        FlashDump[i++] = 0x00;
    }
    FlashDump[i++] = F_ADDR_SETTINGS >> 6; //bits |NNNNxxMM|: N-���. Flash, xx-�� ���., MM-start_mode

  //Writing to Flash:
    //0) Initialization
  global_int_no;
  reset_wdt;
    SFRPAGE = CONFIG_PAGE;
    CCH0CN |= 0x01; //"Block Write Enable"
    SFRPAGE = LEGACY_PAGE;
    //1) Erasing Flash Page:
    FLSCL |= 0x01;
    PSCTL |= 0x03;
    *pwrite = 0;
    PSCTL &= 0xFD;
  reset_wdt;
    //2) Writing Buffer to Flash:
    for (i=0; i<180; i++) *pwrite++ = FlashDump[i];
    PSCTL &= 0xFE;
    FLSCL &= 0xFE;

  //Reset
    reset_code = 0xB1;
    SetOscSlowMode();
    call_program_reset();
}
