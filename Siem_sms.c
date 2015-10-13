//
// Модуль "siem_sms.c"
//
// Используется для передачи данных через SMS - сообщения с помощью телефона Siemens.
//
// 1. Для работы необходим счетчик "time_wait_SMSPhone"
//    который должен инкрементируется по таймеру (каждую секунду) в модуле "Main.c"
// 2. Необходимо настроить на работу с определенным UARTом (см. ниже << #define tx_data_sim tx_data_uart0>>).
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
// *   Настройка на работу с конкретным портом uart    *
// *****************************************************
#define  tx_data_sim      tx_data_uart0
#define  sizeBuf_rx_Sim   sizeBuf_rx_uart0
#define  ClearBuf_Sim     ClearBuf_uart0
#define  Read_byte_sim    Read_byte_uart0
#define  Send_char_sim    send_char_uart0


// ******************************
// *         Константы          *
// ******************************

// Комманды для перехода на загрузчик
#define Com_B_QueryVersion  0x4A    // 0xDA  // Запрос версии 
#define Com_T_Version       0x3A    // 0xCA  // Ответ версия
#define Com_B_NotReload     0x4B    // 0xDB  // Обновление не требуется
#define Com_B_Reload        0x4C    // 0xCC  // Требуется обновление
#define Com_T_Restart       0x3C    // 0xCC  // Терминал ушел на рестарт (в загрузчик)
#define Com_B_ReadyReload   0x4D    // 0xDD  // База готова к закачке
#define Com_T_QueryHeader   0x5A    // 0xEA  // Терминал запрашивает заголовок
#define Com_T_QueryData     0x5B    // 0xEB  // Терминал запрашивает блок данных
#define Com_T_NextBlock     0x5C    // 0xEC  // Блок данных закачан, переходим на следующий блок
#define Com_B_OkNextBlock1  'O'     // 'O'   // База OK, переходим на следующий блок
#define Com_B_OkNextBlock2  'K'     // 'K'   //
#define Com_T_EndReload     0x5E    // 0xEE  // Успешное окончание загрузки всех блоков
#define Com_T_HardError     0x5D    // 0xED  // Индикация ошибки железа
#define Com_T_Error         0x5F    // 0xEF  // Индикация ошибки

// ************************************************************************************************
//
// ************************************
// *            Переменные            *
// ************************************
//


// Modem
// ****************
//            bit   md_f_modem_conect = no; // Флаг "Наличие модемного соединения"
              bit   md_f_need_send    = no; // Флаг "Необходимость послать данные по модему"
unsigned char xdata md_who_call       = 0;  // Кто делал вызов (0-пока нет
                                            //                  1-удаленная загрузка
                                            //                  2-MD
                                            //                  3-сам терминал
                                            //                  4-VC)
unsigned char xdata md_mode           = 0;  // Режим (0-нормальная работа по sms
                                            //        1-нормальная работа по модему
                                            //        2-удаленная загрузка по модему
                                            //       -3-обрыв связи (вызов ДЦ) ждем 2 мин. убиваем все ответы на запросы (не посылаем по SMS)
                                            //       -4-обрыв связи (вызов терм) необходимо сделать вызов)
                                            //        5-голосовая связь по модему
                                            //        6-GPRS
unsigned char xdata md_npack_mod_inc  = 0;  // Номер текущего модемного пакета
unsigned char xdata md_CS; // Контрольная сумма

unsigned char xdata md_Len; // длина блока данных получ. по модему

unsigned char xdata md_resetGSM = 0; //   0 - ничего не надо
                                     //   1 - необходимо переинициализировать GSM-модем
                                     // >=2 - необходимо сделать сброс GSM-модема

//unsigned long xdata sTime_md_mode;
unsigned long xdata sTime_Net =0;

unsigned char xdata  count_atz=0; // счетчик попыток ATZ
//unsigned char xdata  count_cops=0; // счетчик для "AT+COPS=0"
unsigned char xdata  count_worksms=0; // счетчик для работы в командном режиме.
unsigned char xdata  count_p;   // счетчик попыток вообще
//unsigned char xdata  count_dcd=0; // счетчик попыток разорвать соединение при заклинившем DCD.
unsigned char xdata  count_error_no_reply_from_sms=0; // счетчик попыток передать сообщение
unsigned char xdata  count_try_tn=0; // счетчик попыток прочитать номера телефонов

unsigned  int xdata  count_uart_data; // счетчик получ. данных в строке от Simens
//unsigned char xdata  tx_data_sim; // передача и
unsigned char xdata  rx_data_sim; // прием по usart0


unsigned char xdata  current_sms_signal_level=0xFF; // уровень сигнала GSM
bit                 _flag_sms_signal_level_null=no; // флаг указыв. на 0 уровень GSM сигнала.

#define MAX_SIZE_BUF_READ_SIM 360
unsigned char xdata  buf_read_sim[MAX_SIZE_BUF_READ_SIM+1];

// Строка для передачи на Simens
#define MAX_SIZE_TRANSMIT_BUFFER 190
unsigned char xdata  sim_line_tx[MAX_SIZE_TRANSMIT_BUFFER+1];
unsigned char xdata  sim_line_tx_len=0; // Длина sms - сообщения для отсылки

unsigned char xdata _c_ans_kom_rx=0; // какая команда (ответ) пришла
bit                 _flag_new_line_sim=no; // флаг что пришла новая строка от Simens

unsigned char xdata  sms_receive_buffer[141]; // Здесь будет принятое SMS - сообщение
unsigned char xdata  sms_receive_buffer_len=0; // Длина принятого SMS - сообщения
unsigned char xdata  n_mem_sms=0; // номер принятого sms в памяти simens
unsigned char xdata  index_sms=0; // номер сохраненного sms в памяти simens

unsigned char xdata  c_flag_sms_send_out=0; // флаг что сообщение:
                                            // 1 - отправлено
                                            // 0 - не отправлено
                                            // 2 - в процессе отправки
                                            // 3 - отправлено, но не доставлено.
bit  _flag_new_sms=no;           // флаг "Считано новое SMS - сообщение".
bit  _flag_need_delete_sms=no;   // Необходимо удалить принятое SMS - сообщение.
bit  _flag_need_send_sms=no;     // Необходимо послать ответное SMS - сообщение.
bit  _flag_need_wait_SR;         // Необходимо ждать подтверждение доставки SMS.
bit  _f_SMS_accept_type_8bit=no; // флаг что SMS сообщение 8-битное.
bit  _f_SMS_noanswer=no;         // если sms предположительно с интернета,
                                 // то отвечать не надо.

bit  _flag_need_delete_badsms=no; // Необходимо удалить ненужное SMS - сообщение.
unsigned char xdata  n_mem_badsms=0; // номер ненужного сообщения в памяти simens


bit  _flag_tel_ready  =no; // флаг готовности телефона (номера SC и DC считаны).
bit  _flag_tel_ready_R=no; // флаг готовности телефона только к приему (номеров SC и DC нет).

unsigned char xdata need_write_telDC_ix=0;


//#define KOL_NT  21    //кол-во номеров телефонов
struct TEL_NUM xdata tel_num[KOL_NT]; // номер sms-центра и номера дисп. центров

unsigned char xdata who_send_sms=0;    // от кого пришло SMS - индекс номера (1-9)
                                       // индекс 10 - неизвестный номер.

unsigned char xdata code_access_sms=0; // от кого пришло SMS - код (группа) доступа
                                       // 0 - доступ запрещен
                                       // 1 - DC1-DC4
                                       // 5 - User1
unsigned char xdata sec_for_numtel;    // Защита по номерам телефонов.
                                       // Проверять (1) или не проверять (0) след. группы
                                       // бит 0 - SMS
                                       // бит 1 - Data call
                                       // бит 2 - Voice call

//unsigned char xdata tp_st_index =0; // TP-Status. Индекс сообщения "подтверждение доставки"
//unsigned char xdata tp_st_state =0; // TP-Status. Состояние получения сообщения "подтверждение доставки"
                                    // 0-пока нет
                                    // 1-пришел индекс
                                    // 2-подтверждение получено
unsigned char xdata tp_st_result=0; // TP-Status. Результат "доставлено/недоставлено"
                                    // 0-пока неизвестно
                                    // 1 - "доставлено"
                                    // 2 - "недоставлено"


// временные тел. номера для сравнений и преобразований.
unsigned char xdata tmp_telnum[kolBN];       // тел.номер абонента
unsigned char xdata len_telnum=0;            // длина тел.номера
unsigned char xdata tmp_telnum_ascii[kolCN]; // тел.номер в ascii
unsigned char xdata len_telnum_ascii=0;      // длина тел.номера в ascii

unsigned char xdata tmp_telnum_pos;

bit _flag_need_read_all_tels=yes; // необходимость прочитать все номера телефонов (есть/нет)

unsigned char xdata state_sim_rx=1; // Состояния автомата приемника Usart
unsigned char xdata state_sim_wk=75; // Состояния автомата работы c Simens
unsigned char xdata state_per_sim_wk=1; // Состояния автомата опросов Simens 
unsigned char xdata state_init_sim_wk; // Состояния автомата инициализации Simens 

unsigned char xdata Scounter;
unsigned char xdata strS[4];
//unsigned char xdata tmp_CCED[39];
/*
unsigned char xdata tmp_LAC[2]={0};
unsigned char xdata tmp_CI[2] ={0};
unsigned char xdata tmp_RxLev=0;
*/

//unsigned char xdata md_gprs_N=0;    // для настройки модема для gprs
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
void RestartToLoader(void); // Передача управления загрузчику.


unsigned char xdata D_date;
unsigned char xdata M_date;
unsigned  int xdata G_date;

unsigned char code Month[12][3] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

//
// ************************************
// *              Функции             *
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
        GetBack_m = 1; //для записи накопленных с момента включения точек в журнал
        Day_timer = Timer_inside % 86400 / 60; //86400 - секунд в сутках
    }
    Timer_inside += (unsigned long)NumWeek.v_int*604800L;
   int_timer2_yes;
}


// Инициализация модуля
//
void Init_sms_phone (void)
{
  unsigned char xdata i,j;

    for (j=0; j<KOL_NT; j++)
    {
        tel_num[j].len_SO = 0; // кол-во байт номера в формате SO.
        tel_num[j].sms_Al = 0; // текстовые sms-тревоги на эти номера не посылать.
        for (i=0; i<kolBN; i++) tel_num[j].num_SO[i] = 0xFF; // тел. номер в формате SO.
        tel_num[j].need_write = 0; // необходимость записать номер телефона в sim-карту.
        tel_num[j].accept     = 0; // принят пакет с номером телефона.
        tel_num[j].need_send_sms_Al=0; // нет необходимости посылать на этот номер текстовую sms-тревогу.
    }

    reset_wdt;

    ReadScratchPad1();
    if (FlashDump[8]=='S') sec_for_numtel = FlashDump[9];
    else                   sec_for_numtel = 0;
    reset_wdt;
}


// прием строки от Simens (из буфера)
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
            // Принимаем строку от Simens
            if ((rx_data_sim == 0x0D)||(rx_data_sim == 0x0A)) // конец строки
            {
                buf_read_sim[count_uart_data]=0;
               _flag_new_line_sim = yes; // получена новая строка от Simens
                state_sim_rx = 1; // -> goto state start
                return;
            }
            // Запись данных в буфер.
            buf_read_sim[count_uart_data] = rx_data_sim;
            count_uart_data++;
            if (count_uart_data > MAX_SIZE_BUF_READ_SIM)
            {
                state_sim_rx = 99; // -> goto error
                return;
            }

            if ((buf_read_sim[0]=='>')&&(buf_read_sim[1]==' ')) // это приглашение передавать sms - сообщение
            {
               _flag_new_line_sim = yes; // получена новая строка от Simens
                state_sim_rx = 1; // -> goto state start
            }
        break;

        // Прием данных по модему
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

        case 16: // Прием "NO CARRIER"
            buf_read_sim[count_uart_data++] = rx_data_sim;
            if (count_uart_data >= 10)
            {
                md_Len=10;
               _flag_new_line_sim = yes; // получена новая строка от Simens
                state_sim_rx = 1; // -> goto state start
            }
        break;

        case 17: // Прием заголовка
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

        case 20: // Прием данных по модему

            time_wait_SMSPhone2 = 0;

          //if (time_wait_phone_receive>05) state_sim_rx = 99;

            buf_read_sim[count_uart_data++] = rx_data_sim;
            if (count_uart_data>=md_Len)
            {
               _flag_new_line_sim = yes; // получена новая строка от Simens
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

// Преобразование 'F' в 0x0F
unsigned char HexASCII_to_Hex (unsigned char HA)
{
    if ((HA >= '0') && (HA <= '9')) return HA-0x30;
    if ((HA >= 'A') && (HA <= 'F')) return HA-0x37;
    if ((HA >= 'a') && (HA <= 'f')) return HA-0x57;
    return 0xf0;
}

// Преобразование 0xF0 в 'F'
unsigned char Hex_to_HexASCII_Hi (unsigned char Hx)
{
  unsigned char xdata HA;
    HA = (Hx&0xf0)>>4;
    if ((HA >= 0x00) && (HA <= 0x09)) return HA + 0x30;
    if ((HA >= 0x0A) && (HA <= 0x0F)) return HA + 0x37;
}

// Преобразование 0x0F в 'F'
unsigned char Hex_to_HexASCII_Lo (unsigned char Hx)
{
  unsigned char xdata HA;
    HA = (Hx&0x0f);
    if ((HA >= 0x00) && (HA <= 0x09)) return HA + 0x30;
    if ((HA >= 0x0A) && (HA <= 0x0F)) return HA + 0x37;
}

// ****************************************************************
// Получение номера телефона в формате SO из формата ASCII
// из формата ASCII: "+79027459999"
// в формат   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
// возвращает кол-во байт 7
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

// Получение номера телефона
// из формата: "919720479599F9"
//  в формат: 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
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
// Получение номера телефона
// из формата:      "919720479599F9"
//  в формат ASCII: "+79027459999"
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
// Получение номера телефона
// из формата SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
// в формат   ASCII: "+79027459999"
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

// Взять десятичное число из ASCII в hex : "10," = 10 или 0x0A
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

// Взять десятичное число из ASCII для номера ячейки памяти с sms: "10," = 0x10
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
* Сравнение двух телефонных номеров.
*/

bit Comp_telnum(unsigned char *ctelnum, unsigned char clen_telnum, unsigned char ID)
{
  unsigned char xdata i;

    if (tel_num[ID].len_SO<4) return (false);

    for (i=1; i<=clen_telnum-2; i++)
    {
        if (tel_num[ID].num_SO[tel_num[ID].len_SO-i] != ctelnum[clen_telnum-i])
        {
            // Не тот номер
            return (false);
        }
    }
    // Сравнение прошло успешно
    return (true);
}

// ***********************************************
// * Анализирование идущей от Siemens информации *
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

    // что пришло от Simens
   _c_ans_kom_rx = 0; // пока Unknown

    //  1 - OK
    //  2 - RING
    //  3 - NO CARRIER
    //  4 - ERROR
    //  5 - CONNECT
    //  6 - NO DIALTONE
    //  7 - BUSY
    // 10 - "+CLCC: "  определение номера звонящего абонента во время дозвона.
    // 11 - "+CSCA: "  чтение номера sms - центра
    // 12 - "+CPBR: "  чтение номера из телеф. книжки
    // 13 - "+CSQ: "   качество сигнала
    // 22 - "+CREG: "  в сети
    // 24 - "+CREG: "  не в сети
    // 22 - "+COPS: "  в сети
    // 24 - "+COPS: "  не в сети
    // 17 - "^SMONC: " RxLev indication
    // 28 - "+CCID: "  принят ID sim-карты.
    // 18 - "+CDSI: "  принят индекс сообщения "подтверждение о доставке sms"
    // 18 - "+CDS: "   принято сообщения "подтверждение о доставке sms"
    // 14 - "+CMGL: "  считать последнее sms - сообщение
    // 15 - "+CMGS: "  sms - сообщение передано успешно
    // 16 - "> "       приглашение передавать sms - сообщение
    // 19 - "07"       принято подтверждение о доставке sms
    // 20 - "07"       sms - сообщение
    // 21 - "07"       sms - uncknown

    // Com_B_QueryVersion (4A) - запрос версии
    // Com_B_NotReload    (4B) - обновление не требуется
    // Com_B_Reload       (4C) - перейти на загрузчик
    // 6d - запрос по модему

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
        _c_ans_kom_rx = 27; // sim-карта готова
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
/*  if (buf_read_sim[0] == 0x06) // пакет в режиме ПД доставлен.
    {
        _c_ans_kom_rx = 1; // OK
    }
    else
    if (buf_read_sim[0] == 0x19) // пакет в режиме ПД. Ошибка доставки.
    {
        _c_ans_kom_rx = 4; // ERROR
    }
    else*/
    if (buf_read_sim[0] == Com_B_QueryVersion)
    {
        _c_ans_kom_rx = Com_B_QueryVersion; // Com_B_QueryVersion (4A) - запрос версии
    }
    else
    if (buf_read_sim[0] == Com_B_NotReload)
    {
        _c_ans_kom_rx = Com_B_NotReload; // Com_B_NotReload (4B) - обновление не требуется
    }
    else
    if (buf_read_sim[0] == Com_B_Reload)
    {
        _c_ans_kom_rx = Com_B_Reload; // Com_B_Reload (4C) - перейти на загрузчик
    }
    else
    if ((buf_read_sim[0] == 'm')&&(buf_read_sim[1] == 'd')) // 6d 64   модем
    {
        _c_ans_kom_rx = 0x6d; // запрос по модему
        if ((buf_read_sim[2]==0x01)&&(buf_read_sim[8]==0x0F)) // команда сброс
        {
            if (buf_read_sim[9]==0x1F)
            {
                ReadScratchPad1();
                FlashDump[5] = 0xAA;
                WriteScratchPad1();
            }
            
            Command_3p(); // Переход в режим комманд
            time_wait_SMSPhone = 0;
            GSM_DTR_OFF;
            while (time_wait_SMSPhone < 2) reset_wdt; // Пауза
            GSM_DTR_ON;

            Command_ATH(); // Положить трубку
            time_wait_SMSPhone = 0;
            while (time_wait_SMSPhone < 2) reset_wdt; // Пауза

            reset_code = 0xB2;
            RSTSRC |= 0x10; // *** СБРОС ***
        }
    }
    else
    //
    // +CLIP: "89103010274",129,,,"",0
    //
    if (strncmp("+CLIP:", &buf_read_sim[0], 6) == 0) // определение номера звонящего абонента во время дозвона.
    {
        if (md_who_call==0) return; // сначала должен быть +CRING и тип звонка. Если еще нет, то ждем.

        if (buf_read_sim[7]=='"')
        {
            // Считывание номера телефона
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
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 11)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 12)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 13)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 14)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 15)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 16)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 17)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 18)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 19)) md_who_call=5; // +10 MD и Voice
                else
                if (Comp_telnum(&tmp_telnum[0], len_telnum, 20)) md_who_call=5; // +10 MD и Voice
                else
                if ((sec_for_numtel&0x02)==0) // Если не важно от кого поступил вызов, 
                {
                    md_who_call = 2; // то будем считать, что это MD1
                }
                else
                {   // Если важно от кого пришел вызов, а пришел неизвестно от кого,            
                    md_who_call = 0; // то доступ закрыт.
                }
                #ifndef MODE_5S
                    if (md_who_call==5) md_who_call=2; // MD (Нет режима по 5 секундам)
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
                if ((sec_for_numtel&0x04)==0) // Если не важно от кого поступил вызов, 
                {
                    md_who_call = 4; // то будем считать, что это VC
                }
                else*/
                {   // Если важно от кого пришел вызов, а пришел неизвестно от кого,            
                    md_who_call = 0; // то доступ закрыт.
                }
            }
        }
        else // Номер не определен
        {
            md_who_call = 0; // доступ пока закрыт.
            if (md_who_call==2) // 1-data call
            {
                if ((sec_for_numtel&0x02)==0) // Если не важно от кого поступил вызов, 
                {
                    md_who_call = 2; // то будем считать, что это MD1
            }   }
            else // 0-voice
            {
                if ((sec_for_numtel&0x04)==0) // Если не важно от кого поступил вызов, 
                {
                    md_who_call = 4; // то будем считать, что это VC
        }   }   }
       _c_ans_kom_rx = 10;
    }
    else
    if (strncmp("+CSCA:", &buf_read_sim[0], 6) == 0) // чтение номера sms - центра
    {
        // Получаем это:    +CSCA: "+79027459999"
        // Получение номера sms - центра в формате ASCII: "+79027459999"
        for (i=0; ;i++)
        {
            if (buf_read_sim[i+8]=='"') break;
            if (i>=kolCN) break;
            tmp_telnum_ascii[i] = buf_read_sim[i+8];
        }
        len_telnum_ascii = i;

        //_ERICSSON_
        if ((tmp_telnum_ascii[0]!='+') &&                 // если "плюса" нет,
            (strncmp("145", &buf_read_sim[8+i+2], 3) == 0)) // а он нужен,
        {
            for (i==len_telnum_ascii; i>0; i--) tmp_telnum_ascii[i] = tmp_telnum_ascii[i-1]; // то добавим
            len_telnum_ascii++;
            tmp_telnum_ascii[0]='+';
        }

        // Получение номера sms - центра
        // из формата ASCII: "+79027459999"
        // в формат   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
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
//                              чтение номера из телеф. книжки
// ****************************************************************************************
    if (strncmp("+CPBR:", &buf_read_sim[0], 6) == 0)
    {
        // Получаем это:    +CPBR: i,"+79027459999",typ,text

        // Определяем позицию в тел. книжке.
        iN = Get_byte_fr_ASCII(&buf_read_sim[7]);
        if (iN==19) iN=9; // BT - для удал. прошивки
        else
        if (iN>=9)  iN+=2; // были номера с9по18 стали с11по20
        if (iN>20) return;

        // Находим начальные кавычки
        for (i=0; ;i++)
        {
            if (buf_read_sim[i]=='"') { j = i+1; break; }
            if (i>10) break;
        }

        // Получение номера DC - центра в формате ASCII: "+79027459999"
        for (i=0; ;i++)
        {
            if (buf_read_sim[i+j]=='"') break;
            if (i>=kolCN) break;
            tmp_telnum_ascii[i] = buf_read_sim[i+j];
        }
        len_telnum_ascii = i; if (len_telnum_ascii==0) return;

        if ((tmp_telnum_ascii[0]!='+') &&                 // если "плюса" нет,
            (strncmp("145", &buf_read_sim[j+i+2], 3) == 0)) // а он нужен,
        {
            for (i==len_telnum_ascii; i>0; i--) tmp_telnum_ascii[i] = tmp_telnum_ascii[i-1]; // то добавим
            len_telnum_ascii++;
            tmp_telnum_ascii[0]='+';
        }

        // Получение номера DC - центра
        // из формата ASCII: "+79027459999"
        // в формат   SO:    0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        tel_num[iN].len_SO = GetTel_ASCII_to_SO (&tel_num[iN].num_SO[0],
                                                 &tmp_telnum_ascii[0],
                                                  len_telnum_ascii);
        if (tel_num[iN].len_SO==0) return;

      //tel_num[iN].need_read = no;
        tel_num[iN].accept    = yes; // номер телефона iN считан, больше читать не надо.

        _c_ans_kom_rx = 12; //
    }
    else
    if (strncmp("+CSQ:", &buf_read_sim[0], 5) == 0) // качество сигнала
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
    if (strncmp("+CREG:", &buf_read_sim[0], 6) == 0) // в сети или нет ?
    {
        i = buf_read_sim[9]-0x30;
        if ((i==1)||(i==5)) _c_ans_kom_rx = 22; //    в сети
                       else _c_ans_kom_rx = 24; // не в сети
    }
    else
// 0: automatic (default value)
// 1: manual
// 2: deregistration ; ME will be unregistered until <mode>=0 or 1 is selected.
// 3: set only <format> (for read command AT+COPS?)
// 4: manual / automatic (<oper> shall be present), if manual selection fails,
//    automatic mode is entered.
    if (strncmp("+COPS:", &buf_read_sim[0], 6) == 0) // в сети или нет ?
    {
        i = buf_read_sim[7]-0x30;
        /*
        if (i==2) _c_ans_kom_rx = 24; // не в сети
             else _c_ans_kom_rx = 22; //    в сети
        */
        //_ERICSSON_
        if ((i==0)&&(buf_read_sim[8]==0)) _c_ans_kom_rx = 24; // не в сети
                                     else _c_ans_kom_rx = 22; //    в сети
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
    if (strncmp("+CCID:", &buf_read_sim[0], 6) == 0) // ID sim-карты.
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
    if (strncmp("*E2SSN:", &buf_read_sim[0], 7) == 0) // ID sim-карты.
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
    if (strncmp("+CDS:", &buf_read_sim[0], 5) == 0) // идет подтверждение о доставке sms.
    {
//        _c_ans_kom_rx = 18;
        n_mem_sms=0;
    }
    else
    if (strncmp("+CMGL:", &buf_read_sim[0], 6) == 0) // считать последнее sms - сообщение
    {
       _c_ans_kom_rx = 14;
        if (_flag_new_sms==no)
        {
            // номер принятого сообщения в памяти simens
            n_mem_sms = Get_num_sms_fr_ASCII (&buf_read_sim[7]);
        }
    }
    else
    if (strncmp("+CMGS: ", &buf_read_sim[0], 7) == 0) // sms - сообщение передано успешно
    {
        _c_ans_kom_rx = 15;
    }
    else
    if (strncmp("> ", &buf_read_sim[0], 2) == 0) // приглашение передавать sms - сообщение
    {
        _c_ans_kom_rx = 16;
    }
    else
//  if (  (buf_read_sim[0] == '0')&& // sms - сообщение
//        (  (buf_read_sim[1] == '0')||
//           (  ((buf_read_sim[2] == '9')||(buf_read_sim[2] == '8'))&&
//              (buf_read_sim[3] == '1')
//           )
//        )
//     )
    if (buf_read_sim[0] == '0') // sms - сообщение
        //&&(n_mem_sms>0))
    {
       _c_ans_kom_rx = 21; // принято sms, тип пока uncknown

        if (buf_read_sim[1] == '0')
        {
            i=3;
            len_telnum=0;
        }
        else
        if (((buf_read_sim[2] == '9')||(buf_read_sim[2] == '8'))&&
             (buf_read_sim[3] == '1'))
        {
            // Получение номера sms - центра
            // из формата: "919720479599F9" в 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
            len_telnum = GetTel_ATSO_to_SO (&tmp_telnum[0], &buf_read_sim[2],
                                             HexASCII_to_Hex(buf_read_sim[1]));
            // Получение номера sms - центра
            // из формата: "919720479599F9" в ASCII: "+79027459999"
            //len_telnum_ascii = GetTel_ATSO_to_ASCII(&tmp_telnum_ascii[0], &buf_read_sim[2]);

            i = len_telnum*2+3;
        }
        else // Такое sms нам не надо. Удаляем.
        {
            if (n_mem_sms) _flag_need_delete_badsms=yes; return;
        }

        j = buf_read_sim[i] & 0x03; // message type (0-delivery, 1-submit, 2-status report)
        if ((j==3)||(j==1)) { if (n_mem_sms) _flag_need_delete_badsms=yes; return;}
        if (j==2) i+=5; // for Status report
             else i+=3; // for Delivery
        // Получение номера звонящего
        // из формата: "919720479599F9" в 0x91 0x97 0x20 0x47 0x95 0x99 0xF9
        len_telnum = GetTel_ATSO_to_SO (&tmp_telnum[0], &buf_read_sim[i], 
                                       (unsigned char)((HexASCII_to_Hex(buf_read_sim[i-1])+1)/2)+1);
        // Получение номера звонящего
        // из формата: "919720479599F9" в ASCII: "+79027459999"
        //len_telnum_ascii = GetTel_ATSO_to_ASCII(&tmp_telnum_ascii[0], &buf_read_sim[i]);

        i += len_telnum*2;

        if (j==2) // for Status report
        {
            // read Service-Centre-Time-Stamp
// случай <1>
//              00 0665 0B919701478881F3 3060302102450C 3060302102450C 00
//                                       030603122054   030603122054   ok
// случай <2>
//07919701479599F9 0665 0B919701478881F3 3060302102450C 3060302102450C 00
//                                       030603122054   030603122054   ok

            /*//if (ForbiddenAll) //время еще не было определено...
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
                    GetBack_m = 1; //для записи накопленных с момента включения точек в журнал
                    Day_timer = Timer_inside % 86400 / 60; //86400 - секунд в сутках
                }
                Timer_inside += (unsigned long)NumWeek.v_int*604800L;
              int_timer2_yes;
            //}*/

            i += 28;

            //tp_st_state=2; // сообщение "подтверждение доставки" получено.

            j = buf_read_sim[i];
            // '0', '0'     проверяем первый байт (в символах)
            //  000x xxxx - доставлено
            if ((j=='0')||(j=='1')) tp_st_result = 1; //   доставлено
                               else tp_st_result = 2; // недоставлено

//          if (buf_read_sim[1] != '0')
            if (n_mem_sms) _flag_need_delete_badsms = yes; // sms необходимо удалить
            _c_ans_kom_rx = 19; // принято подтверждение о доставке sms
        }
        else // for Delivery
        {
            if (_flag_new_sms) return;
            if (n_mem_sms==0) return;


/*          if ((buf_read_sim[i]  =='0') && (buf_read_sim[i+1]=='0') &&
                (buf_read_sim[i+2]=='F') && (buf_read_sim[i+3]=='6'))
            {
                // флаг что SMS сообщение 8-битное.
                _f_SMS_accept_type_8bit = yes;
            }
            else
            {
                // флаг что SMS сообщение текстовое (7-битное).
                _f_SMS_accept_type_8bit = no;
                 // если номер короткий, то наверняка с интернета,
                 // поэтому отвечать не надо.
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
                    // флаг что SMS сообщение 8-битное.
                    _f_SMS_accept_type_8bit = yes;
                }
                else
                if (((buf_read_sim[i+2]=='F')||(buf_read_sim[i+2]=='0'))
                      &&((j&0x0C)==0))
                {
                    // флаг что SMS сообщение текстовое (7-битное).
                   _f_SMS_accept_type_8bit = no;
                    // если номер короткий, то наверняка с интернета,
                    // поэтому отвечать не надо.
                    if (len_telnum<=4) _f_SMS_noanswer=1; else _f_SMS_noanswer=0;
                }
                else
                {
                    // Такое sms нам не надо. Удаляем.
                    if (n_mem_sms) _flag_need_delete_badsms=yes; return;
                }
            }

            i += 18;

reset_wdt;
            // Длина принятого sms - сообщения
            sms_receive_buffer_len  = ((HexASCII_to_Hex(buf_read_sim[i]))&0x0f)<<4;
            i++;
            sms_receive_buffer_len |=  (HexASCII_to_Hex(buf_read_sim[i]))&0x0f;
            i++; // 54 0x36
                 // sms_receive_buffer_len   25 0x19
            if (sms_receive_buffer_len>140) sms_receive_buffer_len=140;
            // Здесь будет принятое sms - сообщение
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

            // Проверка от кого пришло sms
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
            if (Comp_telnum(&tmp_telnum[0], len_telnum, 20)) { code_access_sms = 5; who_send_sms=20; } // Отвечать не надо
            else
            if ((sec_for_numtel&0x01)==0) // Если не важно от кого пришло sms. 
            {
                code_access_sms = 1; // Код доступа ОК.
                who_send_sms=10;    tel_num[10].len_SO = len_telnum; 
                for (i=0; i<kolBN; i++) tel_num[10].num_SO[i] = tmp_telnum[i]; // Запоминаем телефон
            }
            else
            {   // Если важно от кого пришло sms, а пришло неизвестно от кого,            
                code_access_sms = 0; // то код доступа закрыт.
            }
           _flag_new_sms = yes; // Флаг "получено новое sms-сообщение"
            index_sms = n_mem_sms;
           _c_ans_kom_rx = 20;  // принято sms
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
// отключить эхо комманд
void Command_ATE (void)
{
  unsigned char code AA[]={'A','T','E','0',13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<5; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
*/

// тел. номера из SIM карты AT+CPBS=SM  (old AT^SPBS=SM)
void Command_CPBS_SM (void)
{
  unsigned char code AA[]={'A','T','+','C','P','B','S','=','"','S','M','"',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<13; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// SMS-ки в SIM карте AT+CPMS="SM"
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

// Временные константы модема
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

// Команда на телефон считать номер DC-центра.
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

// Команда на телефон считать номер SMS-центра.
void Command_read_telSC_sim (void)
{
  unsigned char code AA[]={'A','T','+','C','S','C','A','?',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

/*
// Команда выключить телефон.
void Command_device_off (void)
{
  unsigned char code AA[]={'A','T','+','C','P','O','F',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/
/*
// Команда Изменить скорость.
void Command_speed (void)
{
  unsigned char code AA[]={'A','T','+','I','P','R','=','9','6','0','0',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<12; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// Команда на телефон считать кач-во сигнала.
void Command_get_quality_signal (void)
{
  unsigned char code AA[]={'A','T','+','C','S','Q',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<7; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Команда "модем в сети?".
void Command_test_network (void)
{
  unsigned char code AA[]={'A','T','+','C','R','E','G','?',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Команда "Регистрация в сети?".
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

// Команда на телефон считать RxLev indication
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

// Команда на телефон узнать есть ли принятые сообщения.
void Command_get_read_sms (void)
{
  unsigned char code AA[]={'A','T','+','C','M','G','L','=','4',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<10; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Команда на телефон удалить сообщение N.
void Command_del_sms (unsigned char N)
{
  unsigned char code AA[]={'A','T','+','C','M','G','D','='};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
    // Вывод номера sms (номер храниться уже в 10 виде)
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

// Команда AT+VGR
void Command_VGR (void)
{
  unsigned char code AA[]={'A','T','+','V','G','R','=','1','7','6',13};
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Команда AT+VGT
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
// 1-При переключении DTR командный режим
// 2-При переключении DTR разрыв связи
void Command_ATaD1 (void)
{
  unsigned char code AA[]="AT&D1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// управляет сигналом готовности, посылаемого модемом компьютеру (DSR).
// DSR выключен в командном режиме, DSR включен в режиме данных.
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
// разорвать текущее соединение, но gprs не трогать.
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

// Запрос ID sim-карты
/*void Command_ATCCID (void)
{
  unsigned char code AA[]="AT+CCID\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<8; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// Запрос ID sim-карты
void Command_IDSIM (void)
{
  unsigned char code AA[]="AT*E2SSN\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Установка RI для индикации вх. SMS
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

// Запрашиваем наш IP. Проверяем мы в GPRSe ?
/*void Command_testIP(void)
{
  unsigned char code AA[]="AT*E2IPI=0\x0d"; //_ERICSSON_
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/

// *** Статус ***
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

//at+crc=1  - Звонок по голосу или DATA
void Command_CRC (void)
{
  unsigned char code AA[]="AT+CRC=1\x0d";
  unsigned char i;
    ClearBuf_Sim ();
   _c_ans_kom_rx = 0;
    for (i=0; i<9; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
//at+clip=1 - Определение номера
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

// сервис, который будет использоваться MT при посылке MO SMS сообщения
// '2'- GPRS
// '3'- канал SMS
void Command_CGSMS (void)
{
  unsigned char code AA[]="AT+CGSMS=3\x0d";
  unsigned char i;
    ClearBuf_Sim (); _c_ans_kom_rx = 0;
    for (i=0; i<11; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}

// Ответ Com_T_Restart (3С) - Перехожу на загрузчик
/*void Command_1234 (void)
{
  unsigned char code AA[]={Com_T_Restart,'1','2','3','4',13};
  unsigned char i;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}
// Ответ Com_T_Restart (3С) - Перехожу на загрузчик
void Command_ver (void)
{
  unsigned char code AA[]={Com_T_Version,ID_TERM1,VERSION,ID_TERM2,BUILD,13};
  unsigned char i;
    ClearBuf_Sim();
   _c_ans_kom_rx = 0;
    for (i=0; i<6; i++) { tx_data_sim = AA[i]; Send_char_sim(); }
}*/


// Анализ считанных номеров
void Analise_tel_num (void)
{
  unsigned char xdata i,j;
  unsigned char xdata n;

    // Прописан ли хоть один номер ?
    for (i=1,n=0; i<=9; i++) if (tel_num[i].len_SO) { n=i; break; }

    if (n!=0)
    {
        // Хотя бы один номер есть. Забиваем им отсутствующие номера.
        for (i=1; i<=3; i++)
          if (tel_num[i].len_SO==0) // если нет номера, запишем.
          {
              tel_num[i].len_SO = tel_num[n].len_SO;
              for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
          }
        _flag_tel_ready_R = 0; // телефон готов и к передаче и к приему.
    }
}
/*
// Анализ считанных 10 дополнительных номеров
void Analise_10d_tel_num(void)
{
  unsigned char xdata i;

    for (i=20; i>=11; i--)
    {
        tel_num[i].need_send_sms_Al=0; // нет необходимости посылать на этот номер текстовую sms-тревогу.
        tel_num[i].sms_Al = 0;         // текстовые sms-тревоги на этот номер не посылать.
    }
    if (tel_num[8].len_SO!=0) tel_num[8].sms_Al = 1; // надо будет посылать текстовые sms-тревоги на этот номер.
    for (i=20; i>=11; i--)
    {
        if (tel_num[i].len_SO==0) break;
        else
        tel_num[i].sms_Al = 1; // надо будет посылать текстовые sms-тревоги на этот номер.
    }
}*/

// Есть необходимость отправить текстовые sms-тревоги.
/*void SetNeedSendSMSalert(void)
{
  unsigned char xdata i;

    if ((tel_num[8].len_SO!=0)&& // если такой номер есть и
        (tel_num[8].sms_Al==1))  // на него разрешено посылать текстовые sms-тревоги.
    {
        tel_num[8].need_send_sms_Al=1; // необходимо послать на этот номер текстовую sms-тревогу.
    }
    for (i=20; i>=11; i--)
    {
        if ((tel_num[i].len_SO!=0)&& // если такой номер есть и
            (tel_num[i].sms_Al==1))  // на него разрешено посылать текстовые sms-тревоги.
        {
            tel_num[i].need_send_sms_Al=1; // необходимо послать на этот номер текстовую sms-тревогу.
}   }   }
*/

// **************************
// * Инициализация телефона *
// **************************
bit Automat_InitSim(void)
{
  unsigned char xdata i;
  unsigned char xdata j;
  unsigned char xdata n;

    switch (state_init_sim_wk)
    {
        case 1: // начальное состояние алгоритма работы с телефоном
            count_atz = 0; // счетчик попыток команды ATZ
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
        case 3: // ждем 2 сек и посылаем ATZ
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

        case 5: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
            {
              //count_atz = 0;
                StSYSTEM &= ~0x0030; // МОДЕМ ОТВЕЧАЕТ - пока ошибок нет.
                StatePB = 0xFF;
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 10;
            }
            if ((time_wait_SMSPhone >= 3)||(_c_ans_kom_rx==4))
            {
                state_init_sim_wk = 3;
                count_atz++;
                if (count_atz >= 3)
                {
                    StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0010; // МОДЕМ НЕ ОТВЕЧАЕТ - ошибка.
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

        case 12: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
            {
//              count_atz = 0;
                ClearBuf_Sim ();
                StSYSTEM &= ~0x0080; // пока ошибок нет.
                time_wait_SMSPhone = 0; state_init_sim_wk = 20;
            }
            if ((time_wait_SMSPhone >= 5)||(_c_ans_kom_rx==4))
            {
                if (count_atz >= 3) { StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0080; } // ОШИБКА ИНИЦИАЛИЗАЦИИ МОДЕМА.
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
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==4)) // есть ответ
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
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==4)) // есть ответ
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

        case 26: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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

        case 28: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
            if (_c_ans_kom_rx==1) // Прочитан
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
        // *  AT+CNMI=2,0,0,1  или  AT+CNMI=3,0,0,1  *
        // *******************************************
        case 35:
            Command_CNMI(); time_wait_SMSPhone = 0; state_init_sim_wk = 36;
        break;

        case 36: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
                Send_str_sim("ATE0\r\0"); // отключить эхо комманд
                time_wait_SMSPhone = 0; state_init_sim_wk = 38;
            }
        break;

        case 38: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
        // *  at+crc=1  *  Звонок по голосу или DATA
        // **************
        case 39:
            //if (time_wait_SMSPhone >= 1)
            {
                Command_CRC();
                time_wait_SMSPhone = 0; state_init_sim_wk = 40;
            }
        break;
        case 40: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
        // *  at+clip=1  *  Определение номера
        // ***************
        case 41:
            //if (time_wait_SMSPhone >= 1)
            {
                Command_CLIP();
                time_wait_SMSPhone = 0; state_init_sim_wk = 42;
            }
        break;
        case 42: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
        case 47: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
        case 49: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // есть Ok
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
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // есть Ok
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
            if (_c_ans_kom_rx==1) // есть Ok
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
            if (_c_ans_kom_rx==1) // есть Ok
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
            if (_c_ans_kom_rx==1) // есть Ok
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
        // * Запрос ID sim-карты *
        // ***********************
        case 67:
            Command_IDSIM(); time_wait_SMSPhone = 0; state_init_sim_wk = 68;
        break;
        case 68:
            if (_c_ans_kom_rx==28) // есть ID
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
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone>=3)) // есть Ok
            {
                ClearBuf_Sim (); time_wait_SMSPhone = 0; state_init_sim_wk = 70;
            }
        break;
        // **************************************
        // * Установка RI для индикации вх. SMS *
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
        // * номер телефона sms центра *
        // *****************************
        case 80: // необходимо прочитать номер телефона sms центра. 
            if (_flag_need_read_all_tels==0) // Если нет необходимости читать номера телефонов
            {                                // (они уже считаны), то
                count_try_tn = 0;     // счетчик попыток прочитать номера телефонов
               _flag_tel_ready   = 1; // все готово к работе
               _flag_tel_ready_R = 0; //
                state_init_sim_wk = 150;
                return 0;
            }
            Command_read_telSC_sim();
            time_wait_SMSPhone = 0; state_init_sim_wk = 85;
        break;

        case 85:
            if (_c_ans_kom_rx==11) // Прочитан номер SMS - центра
            {
                tmp_telnum_pos = 1; // Начнем считывать номера телефонов с позиции 1;
                count_try_tn = 0; // счетчик попыток прочитать номера телефонов
                ClearBuf_Sim (); time_wait_SMSPhone1 = 0; state_init_sim_wk = 90; return 0;
            }
            if (_c_ans_kom_rx==1) // Не прописан номер sms - центра
            {
                if (++count_try_tn >= 3) // счетчик попыток прочитать номера телефонов
                {
                   _flag_tel_ready   = 1; // Телефон готов к работе, но
                   _flag_tel_ready_R = 1; // только к приему (номеров SC и DC нет).
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
        // * номера телефонов 1-9 *
        // ************************
        case 90:
            if (time_wait_SMSPhone1>=2)
            {
                // необходимо прочитать номера телефонов 1-9.
                tel_num[tmp_telnum_pos].accept = no;
                Command_read_telDC_sim(tmp_telnum_pos);
                time_wait_SMSPhone = 0; state_init_sim_wk = 95;
            }
        break;

        case 95:
            if (_c_ans_kom_rx==12) // Прочитан очередной номер
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
            if (_c_ans_kom_rx==1) // Не прописан очередной номер
            {
                count_try_tn = 0;
                tmp_telnum_pos++;
                if (tmp_telnum_pos>19) state_init_sim_wk = 100;
                                  else state_init_sim_wk = 90;
                time_wait_SMSPhone1 = 0; return 0;
            }
            if (_c_ans_kom_rx==4) // Error
            {
                if (++count_try_tn >= 30) // счетчик попыток прочитать номера телефонов
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

        // Анализ считанных номеров
        case 100:
            // Прописан ли хоть один номер ?
            for (i=1,n=0; i<=9; i++)
                if ((tel_num[i].accept)&&(tel_num[i].len_SO)) { n=i; break; }

            if (n!=0)
            {
                // Хотя бы один номер есть. Забиваем им отсутствующие номера.
                for (i=1; i<=3; i++)
                  if ((tel_num[i].accept==no)||(tel_num[i].len_SO==0)) // если нет номера, запишем.
                  {
                      tel_num[i].len_SO = tel_num[n].len_SO;
                      for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
                  }
/*              for (i=1; i<=9; i++)
                  if ((i!=4)&& // дублирующий номер по sms забивать не надо.
                      (i!=8)&& // номер пользователя US1 тоже не надо
                      ((tel_num[i].accept==no)||(tel_num[i].len_SO==0)))
                  {
                    //tel_num[i].accept = yes;
                      tel_num[i].len_SO = tel_num[n].len_SO;
                      for (j=0; j<kolBN; j++) tel_num[i].num_SO[j] = tel_num[n].num_SO[j];
                  }*/
                StSYSTEM &= ~0x00B0; // МОДЕМ ОТВЕЧАЕТ, НОМЕРА ЕСТЬ - пока ошибок нет.
            }
            else // Не прописано ни одного номера.
            {
                StSYSTEM &= ~0x00F0; StSYSTEM |= 0x0020; // НЕТ НОМЕРОВ - ошибка.
               _flag_tel_ready    = 1; // Телефон готов к работе, но
               _flag_tel_ready_R  = 1; // только к приему (номеров нет).
                state_init_sim_wk = 150;
                return 0;
            }

/*          if (((tel_num[5].accept)&&(tel_num[5].len_SO))&&       // есть MD1
                ((tel_num[6].accept==no)||(tel_num[6].len_SO==0))) // но нет MD2
            {
                // Забиваем MD2 - MD1.
                tel_num[6].len_SO = tel_num[5].len_SO;
                for (j=0; j<kolBN; j++) tel_num[6].num_SO[j] = tel_num[5].num_SO[j];
            }*/

            // Анализ 10 дополнительных номеров
            //Analise_10d_tel_num();

           _flag_tel_ready   = 1; // все готово к работе
           _flag_tel_ready_R = 0; //

            state_init_sim_wk = 150;
        break;

        // Все готово к работе
        case 150:
            //CurrentEvents.b.reset_cpu = 1; //произошел <пере>запуск программы
            time_wait_SMSPhone = 0; return 1;
        break;

        default:
            state_init_sim_wk = 1;
        break;
    }
    return 0;
}

// ***********************************************
// * Автомат постоянных опросов Siemens          *
// ***********************************************
void Automat_PerSimWork(void)
{
  unsigned char i;
  unsigned char code *pAddr = F_ADDR_SETTINGS+206;

    if ((md_resetGSM==1)&&(!_flag_need_send_sms)) //   1 - необходимо переинициализировать GSM-модем
    {
        if (GSM_DCD) // не в соединении, можно переинициализировать
        {
           _flag_need_read_all_tels=yes; // необходимо прочитать все номера телефонов.
            md_resetGSM=0;
            state_sim_wk = 1;
        }
        else // надо закрыть соединение
            state_sim_wk = 186;
        return;
    }
    else
    if (md_resetGSM>=2) // >=2 - необходимо сделать сброс GSM-модема
//     ||(gprs.b.rst_gsm))
    {
        md_resetGSM=0;
//      gprs.b.rst_gsm=0;
        state_sim_wk = 99;
        return;
    }

//  if (_c_ans_kom_rx==2) state_per_sim_wk = 15; // RING (Внезапно раздался звонок)
    if (_c_ans_kom_rx) _c_ans_kom_rx = 0;

    switch (state_per_sim_wk)
    {
        case 1:
            count_worksms=0;

        // Модем в сети или нет?.
        case 2:
            Command_network_registration();
            time_wait_SMSPhone = 0; state_sim_wk = 195;//190;
            state_per_sim_wk = 4;
        break;

        case 4: // Запрашиваем модем в сети или нет?
            if (time_wait_SMSPhone>=2)
            {
                Command_test_network(); //AT+CREG?   +CREG: 0,1
                time_wait_SMSPhone = 0; state_sim_wk = 62;
                state_per_sim_wk = 6;
            }
        break;

        case 6: // Запрашиваем качество сигнала
            if (time_wait_SMSPhone>=2)
            {
                Command_get_quality_signal ();
                time_wait_SMSPhone = 0; state_sim_wk = 60;
                state_per_sim_wk = 8;
            }
        break;

        case 8: // Необходимо послать SMS - сообщение.
            // если надо прошить номера телефонов, то пока сообщений не посылать.
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

        case 10: // Запрашиваем наличие новых SMS - сообщений в Simens.
            if (_flag_new_sms==no)
            {
                 state_sim_wk = 65;
            }
			
            time_wait_SMSPhone = 0; state_per_sim_wk = 11;
        break;

        case 11: // Смотрим RING
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

        // 1. не в gprs  dcd==1
        // 2. в ppp      dcd==1
        // 3. в tcp      dcd==0
        //
        case 15: // Смотрим GPRS
            // Проверка мы в соединении - выход из режима команд ("ATO").
            // Если нет - переход на дозвон и вход в GPRS.
            count_p = 0;
            //state_per_sim_wk = 13;

            if (time_nogprs) {state_per_sim_wk = 1; return;}

            // Если IP-адрес задан 127.0.0.1 , то в GPRS лезть не надо
            if ((pAddr[0]==127)&&(pAddr[1]==0)&&(pAddr[2]==0)&&(pAddr[3]==1))
            {
                state_per_sim_wk = 1;
                return;
            }

            state_sim_wk = 170; //ATO выход из командного режима
/*
            if (GSM_DCD)
            {
                //count_dcd=0;
                state_sim_wk = 174;//104; //    вход в gprs
            }
            else
            {
                //count_dcd++;
                //if (count_dcd>=3)
                //{
                //    count_dcd=0; md_resetGSM=2; // Сброс модема
                //}
                //else
                    state_sim_wk = 170;//100; //ATO выход из командного режима
            }
*/
            state_per_sim_wk = 17;
//          if (gprs.b.discnct) state_sim_wk     = 188;//118; //выход из GPRS.

            time_wait_SMSPhone = 0;
            
        break;

        case 17: // конец сеанса передачи данных по GPRS.
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
// * Автомат Siemens                             *
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
        // * Инициализация телефона *
        // **************************
        case 1:
            state_init_sim_wk = 1; // инициализация телефона
            state_per_sim_wk = 1;  // постоянные запросы
            state_sim_wk = 3;
        break;

        case 3:
            if (Automat_InitSim()==1) state_sim_wk = 10;
        break;

        // ******************************************************
        // *  Основной цикл                                     *
        // ******************************************************
        case 10:

            // ***********************************
            // *   Если телефон готов к работе   *
            // ***********************************
            if (_flag_tel_ready)
            {
                // Если необходимо удалить принятое SMS - сообщение.
                if ((_flag_need_delete_sms)||(_flag_need_delete_badsms))
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 12; return;
                }
                // Если необходимо записать номер телефона SMS центра
                if (tel_num[0].need_write)
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 30; return;
                }
                // Если необходимо записать номер телефона DC центров
                for (i=1; i<10; i++) if (tel_num[i].need_write)
                {
                    need_write_telDC_ix = i;time_wait_SMSPhone = 0;state_sim_wk = 39;return;
                }
                // Если необходимо записать номера доп. 10 телефонов MV центров
                for (i=11; i<21; i++) if (tel_num[i].need_write)
                {
                    need_write_telDC_ix = i;time_wait_SMSPhone = 0;state_sim_wk = 39;return;
                }
/*
                // Необходимо проверить пропадание/появление сигнала.
                if ((_flag_sms_signal_level_null==no) && // Если пропадания сигнала еще не было
                    (Get_quality_signal()<1))            // сигнал пропал
                {
                    sTime_Net  = Rele_LCount; // Засекаем время
                   _flag_sms_signal_level_null = yes; // Сигнал пропал.
                }
                else
                if ((Get_quality_signal()>=1)&&(Get_quality_signal()<=4) && // сигнал появился
                    (_flag_sms_signal_level_null==yes))                     // и было пропадание сигнала
                {
                   _flag_sms_signal_level_null = no;
                    if ((Rele_LCount-sTime_Net)>=180L) // сети не было 3 минуты
                    {
                        state_sim_wk = 99; // Необходимо сделать выкл/вкл телефона.
                        return;
                    }
                }*/
            }
            // **************************************
            // *   Если телефон не готов к работе   *
            // **************************************
            //else
            //{
            //}

            // *******************************************
            // *   Независимо от готовности телефона     *
            // *   периодически запрашиваем:             *
            // * 1. качество сигнала                     *
            // * 2. RING                                 *
            // * 3. наличие новых SMS-сообщений в Simens *
            // *******************************************
            Automat_PerSimWork();

        break;

        // ***************************************************
        // удалить принятое sms - сообщение из памяти телефона
        // ***************************************************
        case 12: 
            if (time_wait_SMSPhone >= 1)
            {
                Command_del_sms(n_mem_sms);
                time_wait_SMSPhone = 0; state_sim_wk = 14;
            }
        break;

        case 14: // ожидание удаления принятого sms - сообщения
            if (_c_ans_kom_rx==1) // есть удаление
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
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4)) // ошибка удаления
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
        // * Начало передачи sms сообщения *
        // *********************************
        case 15: // ATZ
            if (time_wait_SMSPhone >= 1)
            {
                Send_str_sim("ATE0\r\0"); // отключить эхо комманд
                //Send_str_sim("AT\r\0");
                time_wait_SMSPhone = 0; state_sim_wk = 16;
            }
        break;

        case 16:
            if (_c_ans_kom_rx==1) // есть OK
            {
                ClearBuf_Sim ();
              //count_atz = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 17; return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 5)
            {
                time_wait_SMSPhone = 0; state_sim_wk = 15; // ошибка
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
            if (_c_ans_kom_rx==16) // есть приглашение
            {
                count_atz=0;
                ClearBuf_Sim (); _c_ans_kom_rx = 0; count_p=0;
                /*time_wait_SMSPhone = 0;*/ state_sim_wk = 19;
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 20)
            {
                count_error_no_reply_from_sms++; // инкрементир-ие счетчика попыток передать сообщение
                time_wait_SMSPhone = 0; state_sim_wk = 15;
                if (count_error_no_reply_from_sms>=4)
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 0; // флаг показывает что произошла ошибка отправки сообщения.
                    count_error_no_reply_from_sms = 0;
                    state_sim_wk = 99; // ошибка
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
          //if ((_c_ans_kom_rx==15)||(_c_ans_kom_rx==1)) // sms - сообщение передано успешно
            if (_c_ans_kom_rx==15) // SMS - сообщение передано успешно
            {
                count_error_no_reply_from_sms = 0;
                if (_flag_need_wait_SR==no) // подтверждение не нужно.
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 1; // флаг что сообщение успешно отправлено.
                    time_wait_SMSPhone = 0; state_sim_wk = 10;
                    return;
                }
                time_wait_SMSPhone = 0; state_sim_wk = 22;
                StSYSTEM |= 0x0004; // Индикация ожидания подтверждения
                return;
            }
            if ((time_wait_SMSPhone >= 30)||(_c_ans_kom_rx==4))
            {
                count_error_no_reply_from_sms++; // инкрементир-ие счетчика попыток передать сообщение
                time_wait_SMSPhone = 0; state_sim_wk = 15;
                if (count_error_no_reply_from_sms>=3)
                {
                   _flag_need_send_sms = no;
                    c_flag_sms_send_out = 0; // флаг показывает что произошла ошибка отправки сообщения.
                    count_error_no_reply_from_sms = 0;
                    state_sim_wk = 99; // ошибка
                }
                return;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // Ждем сообщение "подтверждение о доставке"
        case 22:
            if (_c_ans_kom_rx==19)
            {
               _flag_need_send_sms = no;
                if (tp_st_result==1) c_flag_sms_send_out = 1; // сообщение отправлено и доставлено.
                else
                {
                    c_flag_sms_send_out = 3; // сообщение отправлено, но не доставлено.
                    // Если нет дублирующего номера, то посылка на него не нужна.
                    if (tel_num[4].len_SO==0) c_flag_sms_send_out = 1; // Будем считать что отправлено
                  //if (tel_num[4].len_SO==0) c_flag_sms_send_out = 0; // сообщение отправить не удалось.
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
                c_flag_sms_send_out = 1; // Будем считать что отправлено

/*              c_flag_sms_send_out = 3;
                // Если нет дублирующего номера, то посылка на него не нужна.
                if (tel_num[4].len_SO==0) c_flag_sms_send_out = 0; // сообщение отправить не удалось.
*/  
            }
        break;


        // ***********************************************
        // * Запись номера телефона sms центра в Siemens *
        // ***********************************************
        case 30:
          if (time_wait_SMSPhone >= 4)
          {
            // Записать номер sms центра в Siemens
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
            if (_c_ans_kom_rx==1) // Ok. Записан номер sms центра в Siemens
            {
                ClearBuf_Sim ();
                tel_num[0].need_write = no;
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4))  // ошибка
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
        // * Запись номера телефона DC центра в Siemens *
        // **********************************************
        case 39:
            if ((need_write_telDC_ix== 0)||
                (need_write_telDC_ix==10)||
                (need_write_telDC_ix >20))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            // Записать номер DC центра в Siemens
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

            if (tel_num[need_write_telDC_ix].len_SO==0) // нужно стереть номер из книжки
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
            if (_c_ans_kom_rx==1) // Ok. Записан номер DC центра в Siemens
            {
                tel_num[need_write_telDC_ix].need_write = no;
                if (need_write_telDC_ix==9)
                {
                    _flag_tel_ready_R = 0; // телефон готов и на прием и на передачу (номера SC и DC есть).
                    _flag_tel_ready   = 1;
                }
                need_write_telDC_ix = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((time_wait_SMSPhone >= 10)||(_c_ans_kom_rx==4))  // ошибка
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
        // *    ПАУЗА при сбросе    *
        // **************************
        case 50:
            if (time_wait_SMSPhone >= 10)
            {
                time_wait_SMSPhone = 0; state_sim_wk = 1;
            }
        break;

        // *********************************************
        // * Ожидание ответа при чтении кач-ва сигнала *
        // *********************************************
        case 60: 
            if (_c_ans_kom_rx==1) // Прочитан
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
            if (_c_ans_kom_rx==17) // Прочитан RxLev indication
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
        // * проверка модем в сети или нет ? *
        // ***********************************
        case 62: // модем в сети или нет?
            if (_c_ans_kom_rx==22) // модем в сети !!!
            {
                //StartRA; // Предпологаем наличие связи.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx==24)  // не в сети !!!
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
            if (_c_ans_kom_rx==22) // модем в сети !!!
            {
                //StartRA; // Предпологаем наличие связи.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if (_c_ans_kom_rx==24)  // не в сети !!!
            {
                //*------DEBUG------*
                send_c_out_max(0xBB); //"Не в сети (CREG)"
                send_c_out_max(0x00);
                //*------DEBUG------*
                state_sim_wk = 99; // сделать рестарт телефона
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
        // * Ожидание ответа при чтении/проверке "есть ли новые SMS - сообщения?" *
        // ************************************************************************
        case 65:
            if (time_wait_SMSPhone >= 1)
            {
                Command_get_read_sms(); state_sim_wk = 66;
            }
        break;

        case 66:
            if ((time_wait_SMSPhone >= 3)||(_flag_new_sms)|| // вышло время или уже считали 1ое sms
                (_c_ans_kom_rx==19))
            {
                Command_get_quality_signal(); // Прервать вывод списка sms
                time_wait_SMSPhone = 0; state_sim_wk = 67;
            }
            // 
			if ((_c_ans_kom_rx==1)&&(_flag_new_sms==0)) // OK уже все (sms в памяти нет)
            {
                ClearBuf_Sim ();
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
			if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 67:
            if ((time_wait_SMSPhone > 2)||(_c_ans_kom_rx==1))  // Подождать ответ на CSQ
            {
//              if (_flag_new_sms) // Если считали новое sms
//              {
//                  // Проверить, а вдруг это сброс ?
//              }
                ClearBuf_Sim ();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;

        //_ERICSSON_
        // *******************************-------------------------------------------
        // *   Выкл/вкл питания модема   *  Грубо, но наверняка
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
        // * Аппаратное выключение модема *
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
        // * Аппаратное включение модема *
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
                send_c_out_max(0xBD); //"Сброс модема"
                send_c_out_max(0x00);
                //*------DEBUG------*
                time_wait_SMSPhone1=0; state_sim_wk = 85;
                count_p=0; /*count_atz=0;*/
            }
        break;
        // *******************************--------------------------------------------
        // * Определение скорости модема *
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
        case 87: // ждем Ok
            if (_c_ans_kom_rx==1) // есть OK
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
                        state_sim_wk=75; // сброс модема еще раз
                    }
                    else
                    {
                        state_sim_wk=72; // выкл\вкл питание модема
                    }

                }
                else
                    state_sim_wk=85;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        // **********************************
        // * Установка скорости модема 9600 *
        // **********************************
        case 88:
            Send_str_sim("AT+IPR=9600\r\0"); time_wait_SMSPhone1 = 0; state_sim_wk = 89;
        break;
        case 89: // ждем Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=3)) // есть OK
            {
                set_com_speed(0);
                time_wait_SMSPhone = 0; time_wait_SMSPhone1 = 0;
              //if (count_atz==0) // первый раз опр.скорость и установки еще не сбрасывали.
              //    state_sim_wk = 90;
              //else
                if (count_p==0) // скорость нормальная - 9600.
                    state_sim_wk = 50;
                else            // скорость определилась не нормальная, надо сохранить.
                    state_sim_wk = 92;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;
        
        // ********************************
        // * Сброс установок на заводские *
        // ********************************
/*      case 90:
            if (time_wait_SMSPhone1>=5)
            {
                Send_str_sim("AT&F\r\0");
                count_atz=1;
                time_wait_SMSPhone1=0; state_sim_wk = 91;
            }
        break;
        case 91: // ждем Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=20))
            {
                //установки сбросили и снова на автоопределение скорости
                time_wait_SMSPhone1 = 0; state_sim_wk = 85;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;*/
        // ******************************
        // * Сохранение скорости модема *
        // ******************************
        case 92:
            if (time_wait_SMSPhone1>=5)
            {
                Send_str_sim("AT&W\r\0"); time_wait_SMSPhone1=0; state_sim_wk = 93;
            }
        break;
        case 93: // ждем Ok
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1>=20))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 50;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;



// ШШШШШШШШШШШШШШШШШШШШШШШШШШШ
// Ш                         Ш
// Ш     ПЕРЕДАЧА ДАННЫХ     Ш
// Ш                         Ш
// ШШШШШШШШШШШШШШШШШШШШШШШШШШШ
        // *****************************************
        // * Снятие трубки connect и прием комманд *
        // *****************************************
        case 100:
            Command_ATA(); // Снятие трубки
            time_wait_SMSPhone1 = 0; state_sim_wk = 105;
            time_wait_test = 0;
        break;
        case 105: // Ожидание CONNECTa
            /* ((md_who_call==4)&&(_c_ans_kom_rx==1)) // voice Ok
            {
                //c_gsmnet_cnt=0; // Сброс счетчика для таймаута рестартов модема
                //_need_sleep_gps_charge=0; // для выхода из спящего режима. Нужно включить GPS.
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
                //c_gsmnet_cnt=0; // Сброс счетчика для таймаута рестартов модема
                // режим работы
/*              if (md_who_call==1) md_mode = 2; // 2-удаленная загрузка по модему
                else
                if (md_who_call==4) md_mode = 5; // 5-голос
                else
                if (md_who_call>1)
                {
                    md_mode = 1; // 1-нормальная работа по модему
                    // если режим "5 секунд", то как будто пришел запрос 51.
                  #ifdef MODE_5S // при разрешенном режиме 5 секунд
                    if (md_who_call==5)
                    {
                        //sms_receive_buffer_len=2;
                        //sms_receive_buffer[0] = 0x51;
                        //sms_receive_buffer[1] = 0xFF;
                       _flag_new_sms = yes; // Флаг "получено новое сообщение"
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
                //md_f_modem_conect = yes; // Флаг "модемное соединение"
               _online=no; // пока нет режима online

                time_wait_SMSPhone2 = 0;
                time_wait_SMSPhone  = 0; state_sim_wk = 110; //return; // Ожидание комманд
*/
                time_wait_SMSPhone1 = 0; state_sim_wk = 110;
            }
            if (((md_who_call==5)&&(time_wait_SMSPhone1>21)) //нужно разрывать,а то не успеем за 5 секунд.
                ||
                (time_wait_SMSPhone1>600) // не дождались соединения
                ||
                (_c_ans_kom_rx==3)) // нет соединения "no carrier"
            {
                md_mode = 0; // 0-нормальная работа по sms
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // Разрыв соединения
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 110:
            if (GSM_DCD==0) // есть соединение
            {
                StK8.b.fDataCon=1;
                state_sim_wk = 112;
            }
            if (time_wait_SMSPhone1>=150) // не дождались
            {
                md_mode = 0; // 0-нормальная работа по sms
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // Разрыв соединения
            }
        break;

        case 112:
            if (StK8.b.fDataCon==0) // соединение разорвано
            {
                md_mode = 0; // 0-нормальная работа по sms
                c_flag_sms_send_out = 1; // пакет ушел
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
                md_mode = 0; // 0-нормальная работа по sms
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1 = 10; state_sim_wk = 120; // Разрыв соединения
            }
            if (_c_ans_kom_rx==3) // no carrier
            {
                md_mode = 0; // 0-нормальная работа по sms
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone = 0; state_sim_wk = 10; // Связь разорвана
            }
            _c_ans_kom_rx = 0;
        break;

        // Com_B_QueryVersion  0x4A    // Запрос версии 
        // Com_T_Version       0x3A    // Ответ версия
        // Com_B_NotReload     0x4B    // Обновление не требуется
        // Com_B_Reload        0x4C    // Требуется обновление
        //                     0x6D    // Запрос по модему
        // ***************************
        // *  Режим передачи данных  *
        // *     Ожидание команд     *
        // ***************************
        case 110:
            if (md_who_call==5) ; // в режиме 5 секунд не отвечаем на команды
            else
            if (_c_ans_kom_rx==Com_B_QueryVersion) // Com_B_QueryVersion (4A) - Запрос версии
            {
                // Ответ версия терминала и софта (Com_T_Version)
                Command_ver();
                md_mode=2; // чтобы сработала удаленная загрузка.
                time_wait_SMSPhone = 0; return;
            }
            else
            if (_c_ans_kom_rx==Com_B_NotReload) // Com_B_NotReload (4B) - Обновление не требуется
            {
                md_mode = 0; // 0-нормальная работа по sms
                break_gsm_req();
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; return; // Разрыв соединения
            }

            // ********************************
            // * удаленная загрузка по модему *
            // ********************************
            if (md_mode==2) // удал. загрузка сработает только при запросе версии.
            {
                if (_c_ans_kom_rx==Com_B_Reload) // Com_B_Reload (4C) - Перейти на загрузчик
                {
                    time_wait_SMSPhone = 0; state_sim_wk = 115; return;
                }
                else
                if (_c_ans_kom_rx==3) // no carier - разрыв связи
                {
                    md_mode = 0; // 0-нормальная работа по sms
                    ClearBuf_Sim();
                    c_flag_sms_send_out = 1; // пакет ушел
                    time_wait_SMSPhone = 0; state_sim_wk = 10; return;
                }
            }

            // *******************************
            // * нормальная работа по модему *
            // *******************************
            if ((md_mode==1)||(md_mode==5)) // передача данных или голос
            {
                if (_c_ans_kom_rx==3) // no carier - разрыв связи
                {
                    md_mode = 0;
                    break_gsm_req();
                    c_flag_sms_send_out = 1; // пакет ушел
                    state_per_sim_wk = 10;
                    ClearBuf_Sim();
                    time_wait_SMSPhone = 0; state_sim_wk = 10; return;
                }
                else
                // если не в режиме "5 секунд" и пришла команда от ДЦ
                if ((md_who_call!=5)&&(_c_ans_kom_rx==0x6D)) // "md"
                {
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 155; return;
                }
                else
                if (md_f_need_send) // Необходимо послать пакет на ДЦ
                {
                    //ClearBuf_Sim();
                   //_c_ans_kom_rx = 0;
                    // Если находится в спящем режиме по gps, но частота CPU в норме
                    // сбрасываем счетчик чтобы не заснул (не понизил частоту) пока
                    // идет работа по ПД.
                    //if (_need_sleep_gps_charge) time_sleep_gps=0;
                    md_f_need_send = 0;
                    md_npack_mod_inc++;
                    state_sim_wk = 160; return;
                }
            }

            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;

            if (
                // если передача данных в режиме "5сек" задерживается
                ((md_mode==1)&&(md_who_call==5)&&(time_wait_test>29)) ||
                // если в ПД нет активности в течении 1 минуты
                ((time_wait_SMSPhone >= 50)&&((md_mode==1)||(md_mode==2))) ||
                // если в голосовой связи прошло 3 минуты
                ((time_wait_SMSPhone >= 180)&&(md_mode==5)) // voice
                //(time_wait_SMSPhone2>=15)
               )
            {
                // Разрываем связь
                md_mode = 0; // 0-нормальная работа по sms
                md_f_need_send = 0;
                break_gsm_req();
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1 = 0; state_sim_wk = 120; // Разрыв соединения
            }
            if (GSM_DCD) //соединение уже разорвано
            {
                state_per_sim_wk = 10;
                ClearBuf_Sim();
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;

        case 115: // Посылка 3C "1234" 0D (Com_T_Restart) и переход на загрузчик
            if (time_wait_SMSPhone >= 1)
            {
                // Ответ Com_T_Restart (3С) - Перехожу на загрузчик
                Command_1234();
                // Необходимо перейти на загрузчик.
                time_wait_SMSPhone = 0; state_sim_wk = 117; // Разрыв соединения
            }
        break;

        case 117:
            //if (SM_BUSY==0)
            {
                // Необходимо перейти на загрузчик.
                RestartToLoader();
            }
        break;
*/
        case 120: // Разрыв соединения
            if (time_wait_SMSPhone1 >= 12)
            {
               _online=no; // пока нет режима online
                c_flag_sms_send_out = 1; // пакет ушел
                Command_3p(); // Переход в режим комманд        <<< +++ >>>
                time_wait_SMSPhone1 = 0; state_sim_wk = 125;
                GSM_DTR_OFF;
            }
        break;

        case 125:
            if ((_c_ans_kom_rx==1)||(time_wait_SMSPhone1 >= 12)) // OK
            {
                GSM_DTR_ON;
                //Command_ATH(); // Положить трубку
                Command_CHLD_1();
                time_wait_SMSPhone = 0; state_sim_wk = 126;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

        case 126:
            if ((_c_ans_kom_rx==1)||(_c_ans_kom_rx==3)||(time_wait_SMSPhone >= 2)) // NO CARRIER
            {
                md_mode=0;
                c_flag_sms_send_out = 1; // пакет ушел
                ClearBuf_Sim();
state_per_sim_wk = 1;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

        // ****************************
        // * Состояние ожидания RINGa *
        // ****************************
        case 140:
            if (_c_ans_kom_rx==10) // +CRING +CLIP
            {
                time_wait_SMSPhone = 0; state_sim_wk = 141;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
            if (time_wait_SMSPhone >= 10) // Не дождались
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
            if (time_wait_SMSPhone >= 8) // Не дождались
            {
                ClearBuf_Sim(); _c_ans_kom_rx = 0;
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
        break;
        // *********************************************************
        // * Определение номера звонящего абонента когда есть RING *
        // *********************************************************
        case 145:
            // Номера телефонов уже сравнены. Осталось проверить.
            if (md_who_call==0)
            {
                // Сравнение не прошло
                time_wait_SMSPhone1 = 5; state_sim_wk = 125; // Чужой номер. ATH.
                return;
            }
            else
            // Это удаленная загрузка или передача данных?
            {
                time_wait_SMSPhone = 0; state_sim_wk = 100; // Переход на снятие трубки
                return;
            }
        break;
/*
        // ***********************************************
        // *               запрос по модему              *
        // ***********************************************
        case 155:
           _c_ans_kom_rx = 0;
            time_wait_SMSPhone2 = 0;

            sms_receive_buffer_len = buf_read_sim[3];
            md_CS = 0;
            // Подсчет КС
            for (i=0; i<(sms_receive_buffer_len+8); i++) md_CS ^= buf_read_sim[i];
            // Проверка КС
            if (md_CS != buf_read_sim[sms_receive_buffer_len+8])
            { state_sim_wk = 156; return; } // КС не совпала, отправить ошибку
          //{ state_sim_wk = 110; return; } // ждем следующей команды
            // КС совпала, смотрим дальше
            if (buf_read_sim[2]==1) // запрос
            {
                //md_npack_mod_inc = 1;
                for (i=0; i<sms_receive_buffer_len; i++)
                    sms_receive_buffer[i] = buf_read_sim[i+8];
               _flag_new_sms = yes; // Флаг "получено новое сообщение"
                index_sms=0;
            }
            else
            if ((buf_read_sim[2]==0x06)||(buf_read_sim[2]==0x19))
            {
                time_wait_SMSPhone=0; state_sim_wk = 110; return; // ждем следующей команды
            }
            else // Неизвестная команда, отправить ошибку
            { state_sim_wk = 156; return; } // Отправить ERROR
          //{ state_sim_wk = 110; return; } // ждем следующей команды

            // ждем следующей команды
            time_wait_SMSPhone=0; state_sim_wk = 110; return;
        break;

        // Отправить ERROR
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
            state_sim_wk = 110; // ждем следующей команды
        break;

        // ***********************************************
        // *          послать данные по модему           *
        // ***********************************************
        case 160:
            if (!cBufTxW)
            {
            tx_data_sim = 'm'; Send_char_sim();
            tx_data_sim = 'd'; Send_char_sim();
            tx_data_sim =  2; Send_char_sim();
            tx_data_sim = sim_line_tx_len; Send_char_sim();
            tx_data_sim = md_npack_mod_inc; Send_char_sim();
            tx_data_sim =  1; Send_char_sim(); // терминал будет ждать подтверждения
            tx_data_sim =  0; Send_char_sim();
            tx_data_sim =  0; Send_char_sim();
            md_CS = 'm'^'d'^2^sim_line_tx_len^md_npack_mod_inc^1^0^0; // контр.сумма заголовка
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

         c_flag_sms_send_out = 1; // пакет ушел
         //DEBUG
         send_c_out_max(0xB8); //"send sms"
         send_c_out_max(0x0D);
         send_c_out_max(0x00);

            if ((sim_line_tx[0]==0x51)||(sim_line_tx[0]==0x67)
#ifdef MODE_5S // при разрешенном режиме 5 секунд
               ||(md_who_call==5)
#endif
               )
            {
                md_mode = 0; // 0-нормальная работа по sms
                break_gsm_req();
                md_f_need_send = 0;
                c_flag_sms_send_out = 1; // пакет ушел
                time_wait_SMSPhone1=0; state_sim_wk = 120; // Разрыв соединения
            }
            else
            {
                time_wait_SMSPhone=0;
                state_sim_wk = 110; // ждем следующей команды
              //state_sim_wk = 161; // пакет ушел, ждем подтверждения
            }
        break;
*/
// ШШШШШШШШШШШШШШШШШШШШШШ
// Ш                    Ш
// Ш      G P R S       Ш
// Ш                    Ш
// ШШШШШШШШШШШШШШШШШШШШШШ
        // ******************************************************
        // *                                                    *
        // *  Если были в командном режиме (в PPP),             *
        // *  то переход обратно в соединение TCP,              *
        // *  Иначе вход в PPP GPRS                             *
        // *                                                    *
        // ******************************************************
        case 170://100
            Command_ATO();
            time_wait_SMSPhone1=0; state_sim_wk = 171;
        break;
        case 171://101
            if (_c_ans_kom_rx==5) // Ok мы в соединении
            {
                //count_p = 0;
                //DEBUG
                send_c_out_max(0xB5); //"ATO"
                send_c_out_max(0x00);
                time_wait_SMSPhone = 0;
                state_sim_wk = 172;//102; // Будем ждать DCD
                return;
            }
            if ((_c_ans_kom_rx==4)||(_c_ans_kom_rx==3)) // Нет мы не в соединении
            {
                count_p = 0;
                md_mode = 0; // 1-нормальная работа по модему
                state_sim_wk = 174;//104; // Надо звонить
                //gprs.b.gprs_on = 0;
            }
            if (time_wait_SMSPhone1>20) // не отвечает
            {
                //DEBUG
                send_c_out_max(0xB6); //"ATO error"
                send_c_out_max(0x00);
                count_p++;
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                    return;
                }
                state_sim_wk = 170;//100; // попробуем ка еще разок.
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 172://102
            if (GSM_DCD==0) // есть connect
            {
                count_p = 0;
                //DEBUG
                send_c_out_max(0xBA); //"DCD=0 - connect"
                send_c_out_max(0x00);
                md_mode = 6; // 6-GPRS //1-нормальная работа по модему
                gprs.b.command = 0; // режим команд выключен
			//gprs.b.command = 1;
                state_sim_wk = 180;//110; // Ожидание конца сеанса передачи данных по GPRS
            }
            else
            if (time_wait_SMSPhone>=20)
            {
                count_p++;
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
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
        // *  Вход в GPRS  *
        // *               *
        // *****************
        case 174://104:
            StSYSTEM |= 0x0100; // Начало дозвона.
            Command_E2IPA(); // Вход в GPRS (PPP)
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
                send_c_out_max(0xB9); //"не входит в gprs"
                send_c_out_max(0x00);
                md_mode = 0; // 1-нормальная работа по модему
                count_p++;
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    time_nogprs=120; // GPRS нет засекаем 2 минуты паузу, потом будем пробовать еще.
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                   _flag_need_read_all_tels=0; // не надо читать все номера телефонов.
                    state_sim_wk = 1; // Error. Переинициализация.
                    return;
                }
                state_sim_wk = 174;//104; // Еще раз
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // **********************************
        // *                                *
        // *  Подключение к серверу в GPRS  *
        // *                                *
        // **********************************
        case 176://106
            if (time_wait_SMSPhone>=5)
            {
                Command_E2IPO(); // Подключение к серверу в GPRS (TCP)
                time_wait_SMSPhone=0; state_sim_wk = 177;//107;
            }
        break;

        case 177://107:
            if (_c_ans_kom_rx==5) // CONNECT
            {
                time_nogprs=0;
                count_p = 0;
                md_mode = 6; // 6-GPRS //1-нормальная работа по модему
                //DEBUG
                send_c_out_max(0xB1); //"connect (GPRS TCP)"
                send_c_out_max(0x00);

                time_wait_SMSPhone = 0;
                state_sim_wk = 172;//102; // Ожидание DCD и затем конца сеанса передачи данных по GPRS
                return;
            }
            if ((time_wait_SMSPhone>76) || // не дождались соединения
                (_c_ans_kom_rx==3) ||      // нет соединения "no carrier"
                (_c_ans_kom_rx==4))
            {
                //*------DEBUG------*
                send_c_out_max(0xBE); //"Error"
                send_c_out_max(0x00);
                //*------DEBUG------*
                    //gprs.b.gprs_on = 0;
                md_mode = 0; // 1-нормальная работа по модему
                count_p++;
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    time_nogprs=120; // GPRS нет засекаем 2 минуты паузу, потом будем пробовать еще.
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; //118; // Close gprs.
                    state_per_sim_wk = 1;
                    //gprs.b.f_cycle=1;
                    return;
                }
                time_wait_SMSPhone = 0;
                state_sim_wk = 176;//106; // Еще раз
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        // ***************************************************
        // *  Ожидание конца сеанса передачи данных по GPRS  *
        // ***************************************************
        case 180://110
            if (gprs.b.command)
            //if (time_wait_SMSPhone>=10)
            //if ((!GSM_RI)||(_flag_need_send_sms))
            {
                md_mode = 0; // 1-нормальная работа по модему
                count_p = 0;
                time_wait_SMSPhone1=0;
                state_sim_wk = 181;
            }
        break;
        case 181:
            if (time_wait_SMSPhone1>=5)
            {
                if (_flag_need_send_sms) //надо отправить по СМС
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
        // *   Переход в режим команд и       *
        // *   Возврат в режим опроса модема  *
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
            if (_c_ans_kom_rx==1) // OK есть режим команд !
            {
                count_p = 0;
                md_mode = 0; // 1-нормальная работа по модему
                //DEBUG
                send_c_out_max(0xB2); //"command mode ok"
                send_c_out_max(0x00);
                //if (gprs.b.gprs_er) state_sim_wk = 188;//118; // надо выйти из GPRS
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
                //if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    md_mode = 0; // 1-нормальная работа по модему
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; //188;//118; // надо выйти из GPRS
                }
            }
        break;

        // ****************************
        // *   Полный выход из GPRS   *
        // *                          *
        // ****************************
/*      case 186://116 // +++AT   (если есть CONNECT и не в командном режиме)
            if (time_wait_SMSPhone1 >= 12)
            {
                Command_3p(); // Переход в режим комманд        <<< +++AT >>>
                time_wait_SMSPhone = 0; state_sim_wk = 187;//117;
            }
        break;
        case 187://117
            if ((_c_ans_kom_rx==3)||(_c_ans_kom_rx==1)) // NO CARRIER или ok если был в командном режиме
            {
                state_sim_wk = 188://118;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
            if (time_wait_SMSPhone >= 5) // Не дождались
            {
                count_p++;
                state_sim_wk = 186;//116;
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    md_mode = 0; // 1-нормальная работа по модему
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                }
            }
        break;*/
/*
        case 188://118 // AT*E2PA=0,1 - выход из GPRS
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
                send_c_out_max(0xB4); //"error. не разрывается"
                send_c_out_max(0x02);
                send_c_out_max(0x00);
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    md_mode = 0; // 1-нормальная работа по модему
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                   _flag_need_read_all_tels=0; // не надо читать все номера телефонов.
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
                send_c_out_max(0xB4); //"error. не разрывается"
                send_c_out_max(0x01);
                send_c_out_max(0x00);
                if (count_p >= 3) // пробовали 3 раза, но нет, видимо не судьба.
                {
                    md_mode = 0; // 1-нормальная работа по модему
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                }
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;

        case 192://122 // Подождать DCD==1
            if (GSM_DCD==1) // no connect
            {
                count_p = 0;
                StSYSTEM &= ~0x0100; // Разрыв связи.
                //DEBUG
                send_c_out_max(0xB0); //"разъединение" "closing connect"
                send_c_out_max(0x00);
                md_mode = 0; // 1-нормальная работа по модему
                c_flag_sms_send_out = 1; // пакет ушел
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
                if (count_p >= 3) // пробовали 5 раз, но нет, видимо не судьба.
                {
                    md_mode = 0; // 1-нормальная работа по модему
                    count_p = 0;
                   _c_ans_kom_rx = 0;
                    state_sim_wk = 10; // Error
                    return;
                }
                time_wait_SMSPhone = 0; state_sim_wk = 190;//120; // попробуем ка еще разок.
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx=0;
        break;

*/


        // *****************
        // *   AT+COPS?    *  // Команда "Регистрация в сети?".
        // *****************
        case 195://190 // модем в сети или нет?
            if (_c_ans_kom_rx==22) // модем в сети !!!
            {
                StartRA; // Предпологаем наличие связи.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((_c_ans_kom_rx==24)||(_c_ans_kom_rx==4))  // не в сети !!!
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
            if (_c_ans_kom_rx==22) // модем в сети !!!
            {
                StartRA; // Предпологаем наличие связи.
                time_wait_SMSPhone = 0; state_sim_wk = 10; return;
            }
            if ((_c_ans_kom_rx==24)||(_c_ans_kom_rx==4))  // не в сети !!!
            {
                //*------DEBUG------*
                send_c_out_max(0xBC); //"Не в сети (COPS)"
                send_c_out_max(0x00);
                //*------DEBUG------*
                state_sim_wk = 99; // сделать рестарт телефона
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
        // * Ожидание ответа переключении с sms/gprs *
        // *******************************************
/*      case 200:
            if ((_c_ans_kom_rx)|| // ок (будем считать) переключен
                (time_wait_SMSPhone >= 5))
            {
                time_wait_SMSPhone = 0; state_sim_wk = 10;
            }
            if (_c_ans_kom_rx) _c_ans_kom_rx = 0;
        break;*/

        // ***************************
        // * Ошибка                  *
        // ***************************
        default:
            ClearBuf_Sim ();
            Restart_phone();
        break;
    }
}

// Полный сброс телефона (с выключением питания)
//
void Restart_phone (void)
{
    StSYSTEM &= ~0x0100; // Разрыв связи.
    // Нажимаем на кнопку телефона и переходим в сост. ожидания установления нормальной работы телефона
    count_restart_gsm++;
    mResult.b.reset_gsm = 1; //регистрация события по reset'у:

    c_flag_sms_send_out = 0;
    current_sms_signal_level = 0xff;

   _flag_tel_ready         = no;  // телефон пока не готов
   _flag_tel_ready_R       = no;  // будет готов только на прием

   _flag_need_read_all_tels=yes; // необходимо прочитать все номера телефонов.

    time_wait_SMSPhone = 0; state_sim_wk = 75;
}

// ******************************************************************
// Телефон готов к работе?
// Ret: 1-да, 0-нет
// 
/*bit Is_tel_ready (void)
{
    return _flag_tel_ready;
}

// ******************************************************************
// Телефон готов к приему?
// Ret: 1-да, 0-нет
// 
bit Is_tel_ready_R (void)
{
    return _flag_tel_ready_R;
}*/


// ******************************************************************
// Получить качество сигнала.
// Ret: байт кач-во сигнала
// 0 - нет сигнала или плохой сигнал
// 1 - переход от плохого сигнала к хорошему
// 2 - есть сигнал
// 3 - пока неизвестно
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
// Есть ли новое sms сообщение?
// Ret: 1-да, 0-нет
//
bit Is_new_sms (void)
{
    if ((_flag_new_sms)&&((index_sms>0)||(md_mode!=0))) return 1;
    return 0;
}

// Это 8 битное sms ?
bit Is_new_sms_8bit (void)
{
    return (_f_SMS_accept_type_8bit);
}

// Удалить принятое sms сообщение из памяти телефона.
//
void Delete_sms (void)
{
   _flag_new_sms = no;
    sms_receive_buffer_len = 0;
    // Необходимо удалить принятое sms - сообщение.
    if ((md_mode==0)&&(index_sms>0)) { _flag_need_delete_sms = yes; n_mem_sms = index_sms; }
}

// Есть неудаленное sms сообщение в памяти телефона?
// Ret: 1-да, 0-нет
//
bit IsNonDelete_sms (void)
{
    return (_flag_need_delete_sms);
}

// ******************************************************************
// Прочитать новое sms сообщение.
// Ret: длина sms - сообщения.
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
    c_flag_sms_send_out = 2; // пакет еще не отправлен.
    sim_line_tx_len = len_sms;
}

void do_pack_tel_8b_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Fmt)
{
  unsigned char xdata i;
  unsigned char xdata j;
  unsigned char xdata n=0;
  unsigned char xdata ix;

// Fmt - биты:
//  7   - 0-без подтверждения.   1-с подтверждением.
//  6   - 0-(8-битный протокол). 1-(7-битный протокол).
//  5   - ------
//  4-0 - индекс номера телефона.

    ix = Fmt & 0x1F; // индекс номера телефона.

//  sim_line_tx[n++] = 0x00;
    sim_line_tx[n++] = tel_num[0].len_SO;
    for (i=0; i<tel_num[0].len_SO; i++) sim_line_tx[n++] = tel_num[0].num_SO[i];

/*!V!*/    _flag_need_wait_SR = yes; sim_line_tx[n++] = 0x31;
//    if (Fmt&0x80) { _flag_need_wait_SR = yes; sim_line_tx[n++] = 0x31; } // 0x31  с  подтверждением
//             else { _flag_need_wait_SR =  no; sim_line_tx[n++] = 0x11; } // 0x11 без подтверждения
    sim_line_tx[n++] = 0x00;

    j = tel_num[ix].len_SO-1;
    if ((tel_num[ix].num_SO[j]&0xF0)==0xF0) j=j*2-1; else j=j*2;
    sim_line_tx[n++] = j;    //0x0B;

    for (i=0; i<tel_num[ix].len_SO; i++) sim_line_tx[n++] = tel_num[ix].num_SO[i];
    sim_line_tx[n++] = 0x00; //

    // ответ на "А"
    if (Fmt&0x40) sim_line_tx[n++] = 0x00; // 00 - для 7-битного протокола.
             else sim_line_tx[n++] = 0xF6; // F6 - для 8-битного протокола. 

    sim_line_tx[n++] = 0xA8;

    // ответ на "А"
    if (Fmt&0x40) sim_line_tx[n++] = (unsigned char)(((int)len_sms)*8/7); // 7-битный протокол.
             else sim_line_tx[n++] = len_sms;                             // 8-битный протокол.

    for (i=0; i<len_sms; i++)
    {
        sim_line_tx[n++] = buf_sms[i];
    }
    sim_line_tx_len = n;
}


// Послать sms сообщение.
// Buf - указатель на массив с sms - сообщением.
// Len - длина sms - сообщения.
// Num - номер Disp-центра
//
void Send_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Num)
{
    // Подготовка пакета для отправки
    do_pack_tel_8b_sms (&buf_sms[0], len_sms, Num);
    // Необходимо послать ответное sms - сообщение.
    _flag_need_send_sms = yes;
    c_flag_sms_send_out = 2; // пакет еще не отправлен.
}


// Есть подтверждение об отправке sms сообщения?
// Ret: 1-да, 0-нет, 2-пока неясно
//
unsigned char Is_sms_send_out (void)
{
    return c_flag_sms_send_out;
}

// Записать в телефон (в телефонную книжку) новый номер.
// Buf - указатель на массив с телефонным номером.
// ix  - индекс телефонного номера.
//
void Write_tel_to_phone (unsigned char ix, unsigned char *buf)
{
    unsigned char xdata i;

    if (buf[1]==0x0F) // пустой номер (нет номера)
    {
        tel_num[ix].len_SO = 0; for (i=0; i<kolBN; i++) tel_num[ix].num_SO[i] = 0xff;
        tel_num[ix].need_write = yes; // необходимость записать номер телефона (есть)
    }
    else
    {
        // Получение номера телефона из буфера buf
        // в формате SO:    0x0B 0x97 0x20 0x47 0x95 0x99 0xF9 ... 0x00 0x97 0x20 0x47 0x95 0x99 0xF9
        // "+" не передается.
        tmp_telnum[0] = 0x91; // '+'  подставляем автоматически. В любом случае.
        // При посылке "+79.." получиться 0x91 0x97 .. т.е. "+79.." - правильно
        // При посылке "+88.." получиться 0x91 0x88 .. т.е. "+88.." - правильно
        // При посылке "89..." получиться 0x91 0x88 .. т.е. "+89.." - не правильно !!!
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

        tel_num[ix].need_write = yes; // необходимость записать номер телефона (есть)

        need_write_telDC_ix = 0;
    }
}

void full_buf_num_tel(unsigned char *buf)
{
// ****************
// номера телефонов
// ****************
  unsigned char xdata i;
  unsigned char xdata j;

    // 9 номеров (108 байт)
    for(i=0;i<108;i++) buf[i] = 0;
    for(j=0; j<9; j++) for(i=0; i<tel_num[j].len_SO; i++) buf[i+j*12] = tel_num[j].num_SO[i];
}
void full_buf_num_tel2(unsigned char *buf)
{
// ****************
// номера телефонов
// ****************
  unsigned char xdata i;
  unsigned char xdata j;

    // 11 номеров (132 байт)
    for(i=0;i<132;i++) buf[i] = 0;

    // 10 номеров с 11 по 20
    for(j=0; j<10; j++) for(i=0; i<tel_num[j+11].len_SO; i++) buf[i+j*12] = tel_num[j+11].num_SO[i];
    //  1 номер   9 (BT)
    for(i=0; i<tel_num[9].len_SO; i++) buf[i+10*12] = tel_num[9].num_SO[i];
}


/**
* @function RestartToLoader
* Передача управления загрузчику
*/

//Передача управления загрузчику
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
    FlashDump[i++] = F_ADDR_SETTINGS >> 6; //bits |NNNNxxMM|: N-стр. Flash, xx-не исп., MM-start_mode

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
