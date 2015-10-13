//  --------------------------------------------------------------------------------
// | ------------------------------------------------------------------------------ |
// ||                                                                              ||
// ||     T R A F F I C   D E T E C T O R   I N T E R F A C E     (          )     ||
// ||                                                                              ||
// | ------------------------------------------------------------------------------ |
//  --------------------------------------------------------------------------------
/*-------------------------------------------------------------------------------------------------*/
/*
* added a module with Iteris Tuzhikov Alexey at_alex@bk.ru
*/
/*-------------------------------------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "uart.h"
#include "kod8.h"

#define DEBUG_ITERIS

unsigned char code K8_ID[2] _at_ (F_ADDR_SETTINGS+214);
unsigned char code K8_TIME0 _at_ (F_ADDR_SETTINGS+216);
unsigned char code K8_TIME1 _at_ (F_ADDR_SETTINGS+217);


#define K8_GSM_ENDCMD       120 //[*0.005s] интервал отсутствия данных (окончание приема команды от ДЦ)
//?? #define K8_TAB_SWITCH   ?? //[*0.005s] интервал переключения на передачу/ на прием
//?? #define K8_GSM_TIMEOUT2 30 //[*1s]    интервал отсутствия данных (закрытие соединения с ДЦ)
//?? #define K8_TAB_TIMEOUT1 20 //[*0.1s]  интервал отсутствия данных ответа от табло


unsigned char code K8_START_PAC[3] = {0xFF, 0x1B, 0x09};

unsigned char xdata StateK8 = 0x01;
unsigned char xdata idnK8; //опрашиваемый детектор (0/1)
unsigned char xdata bufK8[SIZE_DATA_K8];
unsigned char xdata cntK8;
unsigned char xdata in_data_K8;

unsigned char xdata timer0_k8_1=0, 
					timer1_k8_1=0, 
					timer_k8_uart1 = 0,
					timer_k8_res_uart1 = 0,
					timer_k8_gprs_delay = 0;  //[*1s]
volatile unsigned char  xdata timer_hang_uart1;
unsigned char xdata timer_k8_01;                   //[*0.1s]
unsigned char  data timer_k8_ms;                   //[*0.005s]

unsigned long xdata LOutK8 = 0; //число переданных байт на порт KOD8
unsigned long xdata LInK8 = 0;  //число принятых байт с порта KOD8

unsigned char xdata startK8 = 0x00;

union CK8ST xdata StK8 = {0x0};

unsigned char code NOC[] = "NO CARRIER\r";

#define MAX_RES_BUFFER_ITERIS 200
BYTEX BuffIteris[MAX_RES_BUFFER_ITERIS];
TDATA_ITERIS xdata IterisData;
Type_Return xdata sendStatusGPRS = retNull;
Type_Return xdata respondsIteris = retNull;
Type_Return xdata continueSession = retNull;
// результат ответа iteris
static WORDX IterisAnswer = retNull;
#define MAX_TO_SEND_BUFFER_ITERIS 500
BYTEX BufferToSendData[MAX_TO_SEND_BUFFER_ITERIS]={0};

/*-----------------------------------------------------------------------------------*/
static Type_Return machineIteris(void);
/*-----------------------------------------------------------------------------------*/
void init_k8()
{
    //переключение порта табло на прием
    TANG_SWITCH = 0;
    //StK8.b.fTabTx = 1;
}

#ifdef RTMS_COD_MODE
	bit test_start_k8()
	{
    return (in_data_K8 == K8_START_PAC[cntK8]);
	}
#endif
/*----------------------------------------------------------------------------------*/
void automat_k8(void)
{
unsigned char data c1, c2;

Type_Return xdata ret;

switch (StateK8)
    {
        case 0x01: //wait GSM connection; send req
            //запрошено прямое соединение по DATA..
            if (StK8.b.fDataCon)
            {
                tx_data_uart0 = '>'; send_char_uart0();
                ClearBuf_uart0();
                ClearBuf_uart1();
                cntK8 = 0;
                StateK8 = StK8.b.fTabMode ? 0x30 : 0x20;
                break;
            }
            if (StK8.b.fTabMode) break;
	// ------------- автомат работы по ITERIS  ------------------------------//
	#ifdef ITERIS_COD_MODE	
			if(timer0_k8_1)break; // задержка опроса Iteris 
			// запуск машины Iteris
			ret=machineIteris();
			if(ret == retDelay){
				timer0_k8_1 = K8_TIME0; // установить таймер 
				}
			break;
	#endif
	//-------------- end ITERIS----------------------------------------------//
	// ------------- работа с RTMS-------------------------------------------//
	#ifdef RTMS_COD_MODE    
		    //(синхронизация на 2`сек период |**|**|**|...)
			if ((startK8 & 0x01) && !timer0_k8_1 && K8_TIME0)
            {
                startK8 &= ~0x01;
                idnK8 = 0;
				StateK8 = 0x02; // выходим по времени
            }
            else
            if ((startK8 & 0x02) && !timer1_k8_1 && K8_TIME1)
            {
                startK8 &= ~0x02;
                idnK8 = 1;
            }
            else break;
    		//*-----------------*
            ClearBuf_uart1();
            tx_data_uart1 = 0xFF;         send_char_uart1();
            tx_data_uart1 = 0x8F;         send_char_uart1();
            tx_data_uart1 = 0x01;         send_char_uart1();
            tx_data_uart1 = K8_ID[idnK8]; send_char_uart1(); send_char_uart1();
            //*-----------------*
            LOutK8 += 5;
            //*-----------------*
            cntK8 = 0;
            timer_k8_01 = 10;
            StateK8 = 0x02;
		break; 

	    case 0x02: //get start bytes
            if (sizeBuf_rx_uart1)
            {
                in_data_K8 = Read_byte_uart1();
                TEST1_K8:
                if (test_start_k8())
                {
                    bufK8[cntK8] = in_data_K8;
                    if (++cntK8 >= 3)
                    {
                        StateK8 = 0x05;
                        timer_k8_01 = 10;
                    }
                }
                else
                if (cntK8)
                {
                    cntK8 = 0;
                    goto TEST1_K8;
                }
                break;
            }
            if (!timer_k8_01) StateK8 = 0x01;
        break;

        case 0x05: //get context data
            while (sizeBuf_rx_uart1)
            {
                bufK8[cntK8] = Read_byte_uart1();
                if (++cntK8 >= SIZE_DATA_K8)
                {
                    //запись в DataLogger[]
                    c2 = idnK8 ? 0x44 : 0x0E;
                    for(c1=0; c1<SIZE_DATA_K8; )
                        DataLogger[c2++] = bufK8[c1++];
                    //set event
                    if (idnK8)
                    {
                        mResult.b.traffic_sensor_1 = 1;
                        timer1_k8_1 = K8_TIME1;
                    }
                    else
                    {
                        mResult.b.traffic_sensor_0 = 1;
                        timer0_k8_1 = K8_TIME0;
                    }
                    StateK8 = 0x10;
                }
                timer_k8_01 = 10;
                break;
            }
			if (!timer_k8_01) StateK8 = 0x01;
        break;
	#endif
	//------------------------- END RTMS -----------------------------------//
        case 0x10: //pause
            if (!timer_k8_01) StateK8 = 0x01;
        break;
        case 0x20: //Direct Data connection to GSM port
            while (sizeBuf_rx_uart0)
            {
                if ((tx_data_uart1 = Read_byte_uart0()) == NOC[cntK8])
                {
                    if (++cntK8 >= 11) {StateK8 = 0x22; return;} //"NO CARRIER" received
                }
                else
                    if (tx_data_uart1 == NOC[cntK8=0]) cntK8 = 1;
                send_char_uart1();
            }
            reset_wdt;
            while (sizeBuf_rx_uart1)
            {
                tx_data_uart0 = Read_byte_uart1(); send_char_uart0();
            }
            if (GSM_DCD) {StateK8 = 0x22; return;} //соединение закрыто
        break;
        case 0x22:
            //переключение порта табло на прием
            TANG_SWITCH = 0;
            StK8.b.fTabStart = 0;
            StK8.b.fTabTx = 0;
            StK8.b.fDataCon = 0;
            ClearBuf_uart0();
            timer0_k8_1 = K8_TIME0;
            timer1_k8_1 = K8_TIME1;
            timer_k8_01 = 10;
            StateK8 = 0x10;
        break;
        //Tab - GSM connection
        case 0x30:
            while (sizeBuf_rx_uart0)
            {
                if (!StK8.b.fTabTx) //еще не включен на передачу
                {
                    //переключение порта табло на передачу
                    TANG_SWITCH = 1;
                    cBufTxR1 = cBufTxW1 = 0;
                    cntK8 = 0;
                    StateK8 = 0x31;
                    return;
                }
                timer_k8_ms = K8_GSM_ENDCMD;
                if ((c1 = Read_byte_uart0()) == NOC[cntK8])
                {
                    if (++cntK8 >= 11) {StateK8 = 0x22; return;} //"NO CARRIER" received
                }
                else
                    if (c1 == NOC[cntK8=0]) cntK8 = 1;
                if (cBufTxW1 < SIZE_BUF_TX_UART1-1) buf_tx_uart1[cBufTxW1++] = c1;
                StK8.b.fTabStart = 1;
                break;
            }
            if (GSM_DCD) {StateK8 = 0x22; return;} //соединение закрыто
            if (StK8.b.fTabStart)
            {
                if (!timer_k8_ms)
                {
                    cBufTxR1 = 1; //! [0] sended by send_char_uart1(), [1],[2],.. sended by Uart1_intr()
                    tx_data_uart1 = buf_tx_uart1[0]; send_char_uart1(); //START TRANSFER DATA TO K8T
                    StateK8 = 0x32;
                    break;
                }
            }
            else
            {
                while (sizeBuf_rx_uart1)
                {
                    tx_data_uart0 = Read_byte_uart1(); send_char_uart0();
                }
            }
        break;
        case 0x31: //небольшая задержка на переключение прием->передача
            if (FlagTX1) //отправка в порт K8T завершена
            {
                StK8.b.fTabTx = 1;
                StateK8 = 0x30;
            }
        break;
        case 0x32: //transferring data to K8T
            if (GSM_DCD) {StateK8 = 0x22; return;} //соединение закрыто
            if (FlagTX1) //отправка в порт K8T завершена
            {
                //переключение порта табло на прием
                TANG_SWITCH = 0;
                StK8.b.fTabStart = 0;
                StK8.b.fTabTx = 0;
                ClearBuf_uart1();
                StateK8 = 0x30;
            }
        break;
		default:StateK8 = 0x01;
			break;
    }
}
/*-----------------------------------------------------------------------------------------------------*/
/*
Фнкциии работы с Iteris
*/
/*-----------------------------------------------------------------------------------------------------*/
// парсинг принятых строк
static Type_Return AnaliseStream(char *str)
{
static bit flNewData = false;
static WORD    inxBegin = 0;
static BYTE    CurrentCountPskg= 0; // счетчик пакетов одной выборки
static DWORDX  SaveCountPskg= 0;	// общее количество пакетов для отправки
WORD strLen;

// time "Time 09-25-2015,10:55:43,DS"
if(!strncmp("Time",str,4)){		
			flNewData = false;
			return aswOk;
			}
// ответ bin all
if(!strncmp("Bin Start",str,7)){
			flNewData = true;
			return aswBIN_START; // поймали start
			}
// проверка на завершение команды bin all
if(!strncmp("Bin End",str,7)){
		inxBegin=SaveCountPskg=CurrentCountPskg=0;
		flNewData = false;
		return aswBIN_STOP; // поймали end
		}
// собираем данные в массив и ловим конец массива
if(flNewData){
	strLen = strlen(str);
	if((inxBegin+strLen)>=MAX_TO_SEND_BUFFER_ITERIS){ // буффер полон
		SaveCountPskg = CurrentCountPskg;
		CurrentCountPskg = 0;
		inxBegin = 0;
		return aswBUFF_FULL; // это не конец нужно отправвить его по GPRS
		}
	//остановились здесь начинаем заполнять буфер
	if(++CurrentCountPskg>SaveCountPskg){ 
		strcpy((BufferToSendData+inxBegin),str);
		inxBegin+=strLen;
		BufferToSendData[inxBegin] = 0x0D;// разделитель
		inxBegin++;
		}
	}
return aswNull;
}
// прием строки от модема (из буфера)												             
static bit ReceiveStrIteris(unsigned char Byte)
{
static BYTEX Step = Null;
//BYTE i;

switch(Step)
	{
	// Что-то пришло проверяем
	case Null:
    	if ((Byte == 0x0D)||(Byte == 0x0A)) return Null;
		//for(i=0;i<sizeof(BuffIteris);i++)BuffIteris[i] = 0;
		IterisData.pBuff = BuffIteris;
		IterisData.pBuff[Null] = Byte;
		IterisData.leng = 1; // первый пришел байт 
        Step = One; // на прием строки
		return false;
	// Собираем строку
	case One: 
    	// ищем конец строки
		if ((Byte == 0x0D)||(Byte == 0x0A)){
        	IterisData.pBuff[IterisData.leng]=0;
        	Step = Null; // в начало
             return true;
			}
        // Запись данных в буфер.
		IterisData.pBuff[IterisData.leng] = Byte;
        if (++IterisData.leng >=MAX_RES_BUFFER_ITERIS){
        	Step = Two; // go to Error
            }
		return false;        	
	// ERROR ждем символов 0x0D 0x0A 
    case Two:
    	if ((Byte == 0x0D)||(Byte == 0x0A))Step = Null; // если конец строки 
		return false;
	default:Step = Null; //в начало
    	return false;
    }
}
/*-----------------------------------------------------------------------------------------------------*/
// Ожиданеи ответа от модуля
static Type_Answer ReceiverIteris( WORD Answer,
						     	   BYTE AnswerTime)
{
static BYTEX  rStep = Null;

switch(rStep)
	{
	case Null:
		timer_k8_res_uart1 = AnswerTime; // ставим время
		IterisAnswer = aswNull;
		rStep++;
	case One:
		if(!timer_k8_res_uart1){ // нет ответа выходим
			IterisAnswer = aswNoAnswer;
			}
		if(Answer&IterisAnswer){ // от вет есть?
			rStep = Null; 
			return IterisAnswer;
			} 
		if(IterisAnswer)IterisAnswer=Null;
		return IterisAnswer;
	default:rStep = Null;
		return aswNull;
	}
}
/*-----------------------------------------------------------------------------------------------------*/
//Передаем строку и принимаем ответ. Answer - крмбинация ответов AnswerTime - время ожидания ответа, CommandRepeatCount - колличество повторов, TimeOutSend - время ожидания перед отправкой
static Type_Answer TransmitCommand(BYTE *pCmd,
								   WORD Answer, 
								   BYTE DelayTime, 
								   BYTE CommandRepeatCount)
{   
static BYTEX  CountCommand;  //счетчик команд
static BYTEX  step = Null;

switch(step)
	{
    case Null:
		CountCommand = Null;    // счетчик команд
		IterisAnswer  = aswNull;// ответ
		step = One;
	case One: // начало 
		ClearBuf_uart1(); 
        sendStringUART1("\r"); // отправляем, так надо для iteris
		sendStringUART1(pCmd);
		step = Two;
		timer_k8_uart1 = DelayTime; //  dec sec
		return IterisAnswer;
    case Two: // принимаем и разбираем строки, надо здесть ждать время еще
		// ждем время
		if(!timer_k8_uart1){ // сечтчик посчитал
			if(++CountCommand < CommandRepeatCount)step = One;
							else IterisAnswer = aswNoAnswer;
			}
		// номер ответа
		if(Answer&IterisAnswer){// получили ожидаемый ответ?
        	step = Null;
		   	return IterisAnswer;
		   	} 
		if(IterisAnswer)IterisAnswer = aswNull;
		return IterisAnswer;
	default:
		IterisAnswer=aswNull;// ответ
		step = Null;
		return IterisAnswer; // ошибка нет ответа выходим
	}
}
/*------------ автомат работы с ITERIS Vantage V2 -----------------------------------------------*/
static Type_Return machineIteris(void)
{
static BYTEX step = Null;
static WORDX xdata Answer = aswNull;
WORDX i = 0;
// чтение буффера приема
for(i = 0;i<sizeBuf_rx_uart1;i++)
	{
	const BYTE byte = Read_byte_uart1();
	if(ReceiveStrIteris(byte)){ // есть строка надо проверить
		if(IterisAnswer==aswNull)IterisAnswer=AnaliseStream(IterisData.pBuff);
		break;
		}
	reset_wdt;
	}
// start automat Iteris
switch(step)
	{
	case Null:
		Answer = TransmitCommand("0000\r",aswNoAnswer,2,2);
		if(Answer&aswNoAnswer){
			memset(BufferToSendData,0,sizeof(BufferToSendData)); // чистка буффера
			step = One;}
		return retNull;
	case One: // запрос времени
		Answer = TransmitCommand("time\r",aswNoAnswer|aswOk,2,2);
		if(Answer&aswOk)      {respondsIteris = retOk;step = Two;}
		if(Answer&aswNoAnswer){respondsIteris = retError;step = Null;}
		return retNull;
	case Two:// установить время под вопросом?
		step = Three;
		return retNull;
	case Three: // запросить данные
		Answer = TransmitCommand("bin all\r",aswNoAnswer|aswBIN_START|aswBIN_STOP,2,5);
		if(Answer&aswBIN_START)             step = For;  
		if(Answer&(aswBIN_STOP|aswNoAnswer))step = Five;  
		timer_hang_uart1 = 0;
		return retNull;
	case For: // работаем с данными
		Answer = ReceiverIteris((aswBIN_STOP|aswBUFF_FULL),200);
		if(Answer&(aswBUFF_FULL|aswBIN_STOP))step = Five;
		// таймер для выхода по времени
		if(timer_hang_uart1>20)
				step = Eight; // нет данных  по UART1
		return retNull;
	// проверяем есть надо отправлять?
	case Five:
		{
		const BYTEX len = strlen(BufferToSendData);
		if(len){timer_k8_gprs_delay= 255;step=Sex;}   // надо отправить по GPRS
		  else {step = Eight;} // нет инф для отправки по GPRS выходим в ожидание
		// установить флаги 
		if(Answer&(aswBUFF_FULL|aswBIN_STOP|aswNoAnswer))continueSession = retOk;
		}
		return retNull;
	// ждем отправки пакета данных по GPRS
	case Sex:
		if(sendStatusGPRS==retOk){
			sendStatusGPRS = retNull;
			continueSession = retNull;
			memset(BufferToSendData,0,sizeof(BufferToSendData));
			if(Answer&(aswBIN_STOP)) step = Nine;  // чистим и выходим
			if(Answer&(aswBUFF_FULL))step = Three; // много данных, новый запрос
			if(Answer&(aswNoAnswer)) step = Eight; // в ожидание
			}
		if((sendStatusGPRS==retError)||
			(!timer_k8_gprs_delay)){step = Eight;} // в ожидание
		return retOk;
	// нет информации, все отправили, надо подождать.
	case Eight:step = Null;
		return retDelay;
	// очистить iteris
	case Nine:
		Answer = TransmitCommand("bin reset\r",aswNoAnswer|aswOk,2,2);
		if(Answer&aswNoAnswer){step = Null;return retDelay;}
		return retNull;
	default:step = Null;
		return retError;
	}
}
// делаем выборку одной строки
void cpyIterisData(const char *pBf)
{
xdata char *pSymStart = 0;
xdata char *pSymEnd = 0;
BYTEX leng;
WORDX allLeng = 0;
do{
	pSymStart = &BufferToSendData[allLeng];
	pSymEnd = strchr(pSymStart,0x0D);
	if(!pSymEnd)break; // выходим
	leng=(pSymEnd - pSymStart);
	if(leng){// есть еще данные
		strcat(pBf,"Bin=");
		strncat(pBf,pSymStart,leng);
		strcat(pBf,"&");
		}else break;
	allLeng += leng;
	allLeng++; // следующий символ
	reset_wdt;
	}while(pSymEnd); 
}
//------------------------------------------ END ------------------------------------------------//