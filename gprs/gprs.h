#ifndef GPRS_H
#define GPRS_H



#include "..\main.h"


/***************\
*               * \
*   D E B U G   *   >----------------------------------->
*               * /
\***************/

#ifdef DEBUG_GPRS_MAX

 void debug_gprs_init();
 void send_c_out_max(unsigned char C_out);

#else

 #define send_c_out_max(x)

#endif

//------------------------------------------------------<




#define MAX_SEND_ERRORS_GPRS   10
#define SIZE_BUF_IN_GPRS      141
#define SIZE_BUF_HDR_GPRS      30     //����. ����� ��������� HTTP (����., "HTTP/1.1 100 Continue")
#define TIMEOUT_HTTP_STREAM     8     //[*1s]
#define TIMEOUT_HTTP_REQ      120     //[*1s]

#define INPAC_HEADER_LEN        5
#define INPAC_HEADER_STR   "KOD8#"    //��������� ������ Navigator DC � ������ HTTP


#define          _1_sec_001     100   //[*0.01s]
#define          _2_sec_001     200   //[*0.01s]
#define          _2_sec_01       20   //[*0.1s]
#define          _3_sec_01       30   //[*0.1s]
#define          _4_sec_01       40   //[*0.1s]
#define          _5_sec_01       50   //[*0.1s]
#define          _6_sec_01       60   //[*0.1s]
#define          _7_sec_01       70   //[*0.1s]
#define          _8_sec_01       80   //[*0.1s]
#define          _9_sec_01       90   //[*0.1s]
#define          _10_sec_01     100   //[*0.1s]
#define          _15_sec_01     150   //[*0.1s]
#define          _20_sec_01     200   //[*0.1s]
#define          _10_sec_1       10   //[*1s]
#define          _15_sec_1       15   //[*1s]
#define          _30_sec_1       30   //[*1s]
#define          _40_sec_1       40   //[*1s]
#define          _50_sec_1       50   //[*1s]
#define          _1_min_1        60   //[*1s]
#define          _2_min_1        120  //[*1s]
#define          _3_min_1        180  //[*1s]
#define          _4_min_1        240  //[*1s]



typedef union GPRSGL {

    unsigned int all;

    struct {                               //Flags:
        unsigned int abs_off /*0001*/ : 1; // "GPRS �� ������������ (������ ������ �� SMS)" (v)
        unsigned int f_cycle /*0002*/ : 1; // "������ ���� ������ ������" (m)
        unsigned int gprs_md /*0004*/ : 1; // "����� GPRS �������" (m)
        unsigned int command /*0008*/ : 1; // "����/����� � ����� AT-������" (v/m)
        unsigned int ret_cmd /*0010*/ : 1; // "������� � ����� AT-������" (m)
        unsigned int t_input /*0020*/ : 1; // "������ �������� ������" (m)
        unsigned int gprs_er /*0040*/ : 1; //
        unsigned int new_req /*0080*/ : 1; // "������� ����� ������ �� ���" (m)
        unsigned int         /*0100*/ : 1; //
        unsigned int sendbuf /*0200*/ : 1; //* Need send IOStreamBuf.Out data to server
        unsigned int sending /*0400*/ : 1; //* sending HTTP buffer
        unsigned int ackhttp /*0800*/ : 1; //* received "HTTP/1.1 X00"
        unsigned int         /*1000*/ : 1; //*
           } b;      //"bit"

                     };
extern union GPRSGL xdata gprs;

typedef struct _TIME_HTTP{
	unsigned char Hour;
	unsigned char Min;
	unsigned char Sec;
	unsigned char Day;
	unsigned char Mon;
	unsigned int  Year;
}TIME_HTTP;

extern TIME_HTTP xdata TimeHttp;

extern unsigned char xdata in_data;

extern unsigned char data tick_gprs;
extern unsigned  int xdata timer_gprs_glob;
extern unsigned char xdata timer_no_resp_isp;
extern unsigned char xdata timer_tcp_timeout;
extern unsigned char xdata timer_http_delay;
extern unsigned char xdata timer_http_wait;

extern unsigned char xdata StateGlobGPRS;

extern unsigned int xdata SizeOutGPRS;

void automat_gprs_glob();

#endif
