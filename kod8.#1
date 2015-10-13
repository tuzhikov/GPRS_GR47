/*-------------------------------------------------------------------------------------------------*/
/*
* added a module with Iteris Tuzhikov Alexey at_alex@bk.ru
*/
/*-------------------------------------------------------------------------------------------------*/
#ifndef KOD8_H
#define KOD8_H

#define ITERIS_COD_MODE
//#define RTMS_COD_MODE

#define  SIZE_DATA_K8  54

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned char  xdata  BYTEX;
typedef unsigned short xdata  WORDX;
typedef unsigned long  xdata  DWORDX;
/*step machine */
typedef enum _Type_Step_Machine_{Null,One,Two,Three,For,Five,Sex,Seven,
								Eight,Nine,Ten}Type_Step_Machine;  
/*return value */
typedef enum _Type_Return_{retNull=0x0000,
						   retOk=0x0001,
						   retError=0x0002,
						   retDelay=0x0004}Type_Return;
/*return answer */
typedef enum _Type_Answer_{aswNull=0x0000,
						   aswOk=0x0001,
						   aswError=0x0002,
						   aswNoAnswer=0x0004,
						   aswTIME = 0x0008,
						   aswBIN_START = 0x0010,
						   aswBIN_STOP  = 0x0020,
						   aswBUFF_FULL = 0x0040
						   }Type_Answer;


extern unsigned char code K8_ID[2];
extern unsigned char code K8_TIME0;
extern unsigned char code K8_TIME1;

extern volatile unsigned char  xdata timer_hang_uart1;

extern unsigned char xdata timer0_k8_1;
extern unsigned char xdata timer1_k8_1;
extern unsigned char xdata timer_k8_uart1;
extern unsigned char xdata timer_k8_res_uart1;
extern unsigned char xdata timer_k8_gprs_delay;
extern unsigned char xdata timer_k8_01;
extern unsigned char  data timer_k8_ms;


extern unsigned long xdata LOutK8;
extern unsigned long xdata LInK8;

extern unsigned char xdata startK8;

typedef union CK8ST{ //operations' Status of K8 Routines
    unsigned char c;
    struct {
        unsigned char fTabMode  : 1; //flag: режим подключения табло
        unsigned char fTabStart : 1; //flag: принимается команда от ДЦ
        unsigned char fTabTx    : 1; //flag: режим передачи на табло OFF/ON
        unsigned char fDataCon  : 1; //flag: установлено прямое соединение
        }b;
	};
extern union CK8ST xdata StK8;

// сосотяние отправки данных по GPRS
extern Type_Return xdata sendStatusGPRS;
//
extern Type_Return xdata respondsIteris;
//
extern Type_Return xdata continueSession;
// структура буффера данных для ITERIS
typedef struct{
	char   *pBuff;
	WORD   leng;
}TDATA_ITERIS; 
/*--------------------------------------------------*/
void automat_k8();
void init_k8();
void cpyIterisData(const char *pBf);


#endif
