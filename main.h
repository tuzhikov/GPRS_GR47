#ifndef MAIN_H
#define MAIN_H



#include <c8051f120.h>



// M A I N   # D e f i n e s


#define VERSION  48   // ������ ���������
#define BUILD    13   // �����������
#define ID_TERM1 10   // ���
#define ID_TERM2 00   // �����
#define ID_TERM  0x86 // KOD8 + ���



//!!!-!!!
#define DEBUG_GPRS_MAX //���������� ���������� �� GPRS �� ���� MAX31XX

//!!!-!!!
#define _MLOGCSDEFAULT_ 0x19 //����� �� �. ����� � ������� - �������� �� ��������� ��� ������� ������

//����� ������������� ������ ������ SMS _________________________#
#define SizeBufOut 135

//������ flash-������ �� ����� � ������� ����� JTAG _____________#
//#define GuardJTAG

// ����� �� 5 ��������
//#define MODE_5S

//~22118400 Hz:
//�������� �� ���� GSM __________________________________________#
//#define speedTel  0xB8  //19200 b/s
//#define speedTel 0x70   //9600 b/s
#define speedTel 0xF4     //115200 b/s
//�������� �� ���� KOD8__________________________________________#
//#define _GPS_9600
// �������� �� ���� ITERIS
#define _ITERIS_38400
//����-�� ��������� ���������� ������� __________________________#
//#define Kp 0.282
#define Kp 0.312558
#define Kb 0.0595

//����� Flash uC ��� ������� � ���������� ���-��� ���____________#
#define F_ADDR_SETUP    0x2400 //����� ���������� ���������� ��� � Flash uC (��������� SetupMTA)
#define F_ADDR_SETTINGS 0x2000 //����� ���������� ���������� ��� � Flash uC (�������������� ���������)



#define true   1
#define false  0
#define yes    1
#define no     0

#define global_int_no   EA = 0        //����. ����������� ����������
#define global_int_yes  EA = 1        //���. ����������� ����������
#define int_uart0_no    ES0 = 0       //����. ���������� UART0
#define int_uart0_yes   ES0 = 1       //���. ����������  UART0
#define int_uart1_no    EIE2 &= 0xBF  //����. ���������� UART1
#define int_uart1_yes   EIE2 |= 0x40  //���. ����������  UART1
#define int_ext0_no     EX0 = 0       //����. ���������� /INT0
#define int_ext0_yes    EX0 = 1       //���. ����������  /INT0
#define int_ext1_no     EX1 = 0       //����. ���������� /INT1
#define int_ext1_yes    EX1 = 1       //���. ����������  /INT1
#define int_SPI0_no     EIE1 &= 0xFE  //����. ���������� SPI0
#define int_SPI0_yes    EIE1 |= 0x01  //���. ����������  SPI0
#define int_timer2_no   ET2 = 0 //���� ���������� Timer2
#define int_timer2_yes  ET2 = 1 //��� ����������  Timer2

sbit P0_7 = P0 ^ 7;
sbit P0_6 = P0 ^ 6;
sbit P0_5 = P0 ^ 5;
sbit P0_4 = P0 ^ 4;
sbit P0_3 = P0 ^ 3;
sbit P0_2 = P0 ^ 2;
sbit P0_1 = P0 ^ 1;
sbit P0_0 = P0 ^ 0;

sbit P1_7 = P1 ^ 7;
sbit P1_6 = P1 ^ 6;
sbit P1_5 = P1 ^ 5;
sbit P1_4 = P1 ^ 4;
sbit P1_3 = P1 ^ 3;
sbit P1_2 = P1 ^ 2;
sbit P1_1 = P1 ^ 1;
sbit P1_0 = P1 ^ 0;

sbit P2_7 = P2 ^ 7;
sbit P2_6 = P2 ^ 6;
sbit P2_5 = P2 ^ 5;
sbit P2_4 = P2 ^ 4;
sbit P2_3 = P2 ^ 3;
sbit P2_2 = P2 ^ 2;
sbit P2_1 = P2 ^ 1;
sbit P2_0 = P2 ^ 0;

sbit P3_7 = P3 ^ 7;
sbit P3_6 = P3 ^ 6;
sbit P3_5 = P3 ^ 5;
sbit P3_4 = P3 ^ 4;
sbit P3_3 = P3 ^ 3;
sbit P3_2 = P3 ^ 2;
sbit P3_1 = P3 ^ 1;
sbit P3_0 = P3 ^ 0;

sbit P4_7 = P4 ^ 7;
sbit P4_6 = P4 ^ 6;
sbit P4_5 = P4 ^ 5;
sbit P4_4 = P4 ^ 4;
sbit P4_3 = P4 ^ 3;
sbit P4_2 = P4 ^ 2;
sbit P4_1 = P4 ^ 1;
sbit P4_0 = P4 ^ 0;

sbit P5_7 = P5 ^ 7;
sbit P5_6 = P5 ^ 6;
sbit P5_5 = P5 ^ 5;
sbit P5_4 = P5 ^ 4;
sbit P5_3 = P5 ^ 3;
sbit P5_2 = P5 ^ 2;
sbit P5_1 = P5 ^ 1;
sbit P5_0 = P5 ^ 0;

sbit P6_7 = P6 ^ 7;
sbit P6_6 = P6 ^ 6;
sbit P6_5 = P6 ^ 5;
sbit P6_4 = P6 ^ 4;
sbit P6_3 = P6 ^ 3;
sbit P6_2 = P6 ^ 2;
sbit P6_1 = P6 ^ 1;
sbit P6_0 = P6 ^ 0;

sbit P7_7 = P7 ^ 7;
sbit P7_6 = P7 ^ 6;
sbit P7_5 = P7 ^ 5;
sbit P7_4 = P7 ^ 4;
sbit P7_3 = P7 ^ 3;
sbit P7_2 = P7 ^ 2;
sbit P7_1 = P7 ^ 1;
sbit P7_0 = P7 ^ 0;



sfr16 RCAP4 = 0xCA;
//#define RCAP4wr      SFRPAGE=TMR4_PAGE;RCAP4
//#define RCAP4rd      SFRPAGE=TMR4_PAGE,RCAP4


// Tab Rx/Tx Switch _____________________________________________#
#define TANG_SWITCH  P3_4

//Tab Detector Pin ______________________________________________#
#define TAB_DET     (SFRPAGE=CONFIG_PAGE,P5_0) //0-�����

//Telephone/Modem _______________________________________________#
#define GSM_OFF      P1_7
#define TEL_BUTTON   P2_5
#define GSM_PWR_ON   P3_2=1
#define GSM_PWR_OFF  P3_2=0
#define GSM_DTR      P3_1
#define GSM_DTR_ON   P3_1=0
#define GSM_DTR_OFF  P3_1=1
#define GSM_DCD      P3_7
#define GSM_RTS      P3_3
#define GSM_CTS     (SFRPAGE=CPT1_PAGE,CPT1CN&0x40)
#define GSM_RI      (SFRPAGE=CPT0_PAGE,CPT0CN&0x40) //������ � ������� SMS

//Reset GSM/Prog iButton ________________________________________#
#define PROG_BUTTON (SFRPAGE=CONFIG_PAGE,P4_1)

//AT45DB041 Flash _______________________________________________#
#define CS_AT45  P3_6
#define RES_AT45 P3_5

//MAX3100 UART __________________________________________________#
#define CS_MAX31 P2_4

//Chip Select Outputs for SPI Devices
#define CSEL0 CS_MAX31 //"Chip Select" for SPI Device 0 (MAX3100 UART)
#define CSEL1 CS_AT45  //"Chip Select" for SPI Device 1 (AtFlash Logger)

//Software Reset uC
#define SOFT_RESET  SFRPAGE=LEGACY_PAGE;RSTSRC|=0x10

//GPS/GSM LED ___________________________________________________#
//#define LED  P2_6 //�������� SMS
//#define LED1 P2_7 //

//Indicator LEDs ________________________________________________#
#define LED      P2_6
#define LED1     P2_7
#define LEDSTAT  P4_0 //!MUST SET TO CONFIG_PAGE!

//MicroLAN i/o pin ______________________________________________#
#define LAN         P4_4//P4_5
#define LAN_0       SFRPAGE=CONFIG_PAGE;LAN=0
#define LAN_1       SFRPAGE=CONFIG_PAGE;LAN=1
#define IF_LAN_0    SFRPAGE=CONFIG_PAGE,!LAN
#define LAN_OUT     SFRPAGE=CONFIG_PAGE;P4MDOUT|=0x20//P4MDOUT|=0x10
#define LAN_IN      SFRPAGE=CONFIG_PAGE;P4MDOUT&=~0x20//P4MDOUT&=~0x10




#define reset_wdt WDTCN=0xA5

#define StartRA if(RA_Mode)Alarm_Enable=1 //������ �������������� �������������� ������

#define AlarmPointSize 122       //������������ ����� ����� ����� ��������� ���
#define lim_SIZE 50 //????       //����� ����� ������ LimitsData, ���������� � flash uC
#define AMOUNT_DATARAM_POINTS 2  //����� ���-�� ����� � ������ DataRAM[]



// M A I N   G l o b a l   V a r i a b l e s

typedef union u_t_int  {unsigned int  v_int;
                        struct       _v_char2  { unsigned char c[2]; } c2;
                       } _t_int;
typedef union u_t_long {unsigned long v_long;
                        struct       _v_char4l { unsigned char c[4]; } c4l;
                       } _t_long;
typedef union u_t_float{  float       v_float;
                         struct      _v_char4  { unsigned char c[4]; } c4;
                       } _t_float;

typedef enum _BOOL { FALSE = 0, TRUE } BOOL;

typedef union _WORD_VAL
{
    unsigned int Val;
    struct
    {
        unsigned char LSB;
        unsigned char MSB;
    } byte;
    unsigned char v[2];
} WORD_VAL;

typedef struct _struct_str {

    unsigned char*         str;
    unsigned char          len;
    unsigned char          tone;

                           } struct_str;


extern unsigned char xdata count1s;
extern unsigned char xdata time_wait_io;
extern unsigned char xdata time_wait_io2;
extern unsigned char xdata time_wait_io3;
extern unsigned char xdata time_wait_SMSPhone;
extern unsigned  int xdata time_wait_SMSPhone1;
extern unsigned char xdata time_wait_phone_receive;
extern unsigned char xdata time_wait_SMSPhone2;
extern unsigned char xdata time_wait_test;
extern unsigned int xdata Day_timer;
extern unsigned long xdata Rele_LCount;
extern unsigned char xdata time_wait_lan;
extern unsigned char xdata time_wait_txtsms;
extern unsigned char xdata time_nogprs;


extern unsigned char xdata DataLogger[128]; //�����, ���������� ��� ������� ���������� � ��������� �������

//extern unsigned char xdata S_Password[12]; //from Setup2 - Password
//extern unsigned char xdata S_PinCode[4];   //from Setup2 - Pin-code

extern unsigned char xdata count_restart_cpu;
extern unsigned char xdata count_restart_gsm;

extern unsigned long xdata Timer_inside; //���������� ����������� ������

extern unsigned char xdata cnt; //Counter ������ ����������

typedef struct SMS_HDR { //��������� ����� ������ SMS
    unsigned char id;
    unsigned char tail;
    unsigned char sn;
    unsigned char res;
    unsigned char fmt;
                       };

extern xdata struct {
          unsigned char In[140];
          struct SMS_HDR Header;
          unsigned char Out[SizeBufOut + AlarmPointSize+9];
          unsigned char OutAuto[SizeBufOut + AlarmPointSize+9];
                    } IOStreamBuf;
extern xdata struct {
                 unsigned int In;     //����� ��������� ������ �����
                 unsigned int Out;    //����� ��������� ������ ������
                 unsigned char ReqID; //��� ��������������� �������
                    } IOStreamState;

extern bit FlagTX1;
extern bit FlagTX0;

extern bit Timer_tick;
extern unsigned char xdata NTicks;

extern unsigned char xdata DataRAM[(AlarmPointSize+8)*AMOUNT_DATARAM_POINTS];
extern unsigned int xdata DataRAMPointer;

extern unsigned char xdata Char;
extern unsigned char xdata * pChar;
extern unsigned int xdata Int;
extern unsigned int xdata * pInt;
extern float xdata Float; //������ ����������
extern float xdata Float1; //������ ����������
//extern float xdata * pFloat; //������ ����������
//extern bit Bit; //������ ����������


typedef union EVENTS {
    struct {


                                        //      __<0 ����>________________________________
        char reset_cpu          : 1;    // [0] ������� �����������  
        char reset_gsm          : 1;    // [1] ������� ������

        char power_main_lo      : 1;    // [2] ��������� ���������� ��������� ������� < ������������

        char tem_critical_hi    : 1;    // [3] ���������� ����������� ���� �����������  > �����
        char tem_critical_lo    : 1;    // [4] ��������� ����������� ���� ����������� < �����
        char tem_microlan_hi    : 1;    // [5] ���������� ����������� �� MicroLan > �����
        char tem_microlan_lo    : 1;    // [6] ��������� ����������� �� MicroLan < �����

        char beacon_time        : 1;    // [7] �������������� �� ��������� ��������� ������� �����

                                        //      __<1 ����>________________________________
        char traffic_sensor_0   : 1;    // [8]  �������� ����� ������ � ������� I
        char traffic_sensor_1   : 1;    // [9]  �������� ����� ������ � ������� II
        char                    : 1;    // [10] ---
        char                    : 1;    // [11] ---

        char level_gsm_lo       : 1;    // [12] ������� ������� GSM = 0 ��� 1 (������)
        char level_gsm_norm     : 1;    // [13] ������� ������� GSM = 2, 3, 4 (����������)

        char reset_gprs         : 1;    // [14] ������� GPRS
        char error_gprs         : 1;    // [15] ������ GPRS

                                        //      __<2 ����>________________________________    
        char sensor_an1_hi      : 1;    // [16]
        char sensor_an1_lo      : 1;    // [17] 
        char sensor_an2_hi      : 1;    // [18]
        char sensor_an2_lo      : 1;    // [19] 
        char sensor_an3_hi      : 1;    // [20] 
        char sensor_an3_lo      : 1;    // [21] 
        char sensor_an4_hi      : 1;    // [22] 
        char sensor_an4_lo      : 1;    // [23] 

                                        //      __<3 ����>________________________________
        char sensor_dry1_c      : 1;    // [24] �.�.1
        char sensor_dry2_c      : 1;    // [25] �.�.2
        char sensor_dry3_c      : 1;    // [26] �.�.3
        char sensor_dry4_c      : 1;    // [27] �.�.4

        char set_guarding_on    : 1;    // [28] ��������� ������� ��� ������
        char set_guarding_off   : 1;    // [29] ������ ������� � ������

        char relay_switch       : 1;    // [30] ������������ �������������� ���������
        char                    : 1;    // [31] ---

                                        //      __<4 ����>________________________________    
        char                    : 1;    // [32]
        char                    : 1;    // [33] 
        char                    : 1;    // [34]
        char                    : 1;    // [35] 
        char                    : 1;    // [36] 
        char                    : 1;    // [37] 
        char                    : 1;    // [38] 
        char                    : 1;    // [39] 

                                        //      __<5 ����>________________________________    
        char                    : 1;    // [40]
        char                    : 1;    // [41] 
        char                    : 1;    // [42]
        char                    : 1;    // [43] 
        char                    : 1;    // [44] 
        char                    : 1;    // [45] 
        char                    : 1;    // [46] 
        char                    : 1;    // [47]

                                        //      __<6 ����>________________________________    
        char                    : 1;    // [48]
        char                    : 1;    // [49] 
        char                    : 1;    // [50]
        char                    : 1;    // [51] 
        char                    : 1;    // [52] 
        char                    : 1;    // [53] 
        char                    : 1;    // [54] 
        char                    : 1;    // [55] 

                                        //      __<7 ����>________________________________    
        char                    : 1;    // [56]
        char                    : 1;    // [57] 
        char                    : 1;    // [58]
        char                    : 1;    // [59] 
        char                    : 1;    // [60] 
        char                    : 1;    // [61] 
        char                    : 1;    // [62] 
        char                    : 1;    // [63]

            } b;

    struct {

        unsigned long l1;
        unsigned long l2;

           } l;

    unsigned char c[8];

                     };

extern union EVENTS xdata CurrentEvents; //������� ���������
extern union EVENTS xdata PreviousEvents; //���������� ���������
extern union EVENTS xdata mResult;


typedef union LIMITS { //��������� �������� �������
    struct {

        unsigned char power_main_lo_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char power_main_lo      ; // <�> ��������� ���������� ��������� ������� ����������� < ������������

        unsigned char tem_critical_hi_n  ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        signed char tem_critical_hi      ; // <�C> ���������� ����������� ���� ����������� > �����

        unsigned char tem_critical_lo_n  ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        signed char tem_critical_lo      ; // <�C> ��������� ����������� ���� ����������� < �����

        unsigned char tem_microlan_hi_n  ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        signed char tem_microlan_hi      ; // <�C> ���������� ����������� �� MicroLan > �����

        unsigned char tem_microlan_lo_n  ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        signed char tem_microlan_lo      ; // <�C> ��������� ����������� �� MicroLan < �����

        unsigned int beacon_time_base    ; // <���[0..1439]> ������� ����� (����� ���������) �����
        unsigned int beacon_time         ; // <���[0..1439]> �������� �����

        unsigned char sensor_an1_hi_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an1_hi      ; // <�.�.[0..127]>

        unsigned char sensor_an1_lo_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an1_lo      ; //

        unsigned char sensor_an2_hi_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an2_hi      ; //

        unsigned char sensor_an2_lo_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an2_lo      ; //

        unsigned char sensor_an3_hi_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an3_hi      ; //

        unsigned char sensor_an3_lo_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an3_lo      ; //

        unsigned char sensor_an4_hi_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an4_hi      ; //

        unsigned char sensor_an4_lo_n    ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_an4_lo      ; //

        unsigned char sensor_dry1_n      ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char sensor_dry1_x      ; // <�.�.[0..127]> �������������� �������� (�� ���.)

        unsigned char sensor_dry2_n      ; //
        unsigned char sensor_dry2_x      ; //

        unsigned char sensor_dry3_n      ; //
        unsigned char sensor_dry3_x      ; //

        unsigned char sensor_dry4_n      ; //
        unsigned char sensor_dry4_x      ; //

        unsigned char guarding_on_n      ; // <��� 01, ��� 02> ������������ �� ������ 1, 2
        unsigned char guarding_off_n     ; //
            
            } Each;

    unsigned char All[lim_SIZE];

                     };

extern union LIMITS xdata LimitsData;


extern unsigned char xdata TickCounter;

extern bit ForbiddenAll;
extern bit GetBack_m;

extern unsigned char xdata SMS_Tail[2];

extern unsigned char xdata FlashDump[1024];

extern unsigned char xdata reset_code;

extern bit RA_Mode;
extern bit Alarm_Enable;
extern unsigned char xdata StateSendSMS;

extern unsigned char xdata StateRequest;
extern bit SearchEnable;
extern unsigned char xdata StateSearch;
extern bit Auto_Enable;
extern unsigned int xdata CountAuto;
extern unsigned char xdata StateAlarm;

extern unsigned int xdata Beacon_timer;
extern unsigned char xdata TimeRA;

extern unsigned char xdata scount1s;

extern unsigned char xdata GSM_signal;

extern unsigned int code LED_Mode[];
extern unsigned int code LED1_Mode[];
extern unsigned int code LEDSTAT_Mode[];
extern unsigned char xdata StLED;
extern unsigned char xdata StLED1;
extern unsigned char xdata StLEDSTAT;
extern unsigned int xdata StSYSTEM;

extern unsigned char xdata mLogCS;

extern bit OutBufChange;
extern bit Beacon_Enable;

extern bit f_tab_mode;



// M A I N   F u n c t i o n s

void WriteFlash(unsigned char xdata * pwrite, unsigned int amount);
void ReadFlash(unsigned char code * pread, unsigned int amount);
void LockJTAG();
void ReadScratchPad1();
void WriteScratchPad1();

unsigned char HexASCII_to_Hex(unsigned char);

void call_program_reset();



#endif
