// ******************************************************************
//
// IO.h
//
// Заголовки функций для работы с любым каналом связи
//
// ******************************************************************
//

#ifndef IO_H
#define IO_H


#define CHANNEL_SMS


unsigned char Hex_to_HexASCII_Hi (unsigned char Hx);
unsigned char Hex_to_HexASCII_Lo (unsigned char Hx);

extern unsigned char xdata * pBuf_read_data; // указатель на буфер для ввода информации

extern unsigned char xdata md_mode;
extern unsigned char xdata md_who_call;
//extern unsigned long xdata sTime_md_mode;
//extern unsigned char xdata tmp_LAC[2];
//extern unsigned char xdata tmp_CI[2];
//extern unsigned char xdata tmp_RxLev;

extern unsigned char xdata md_resetGSM;

extern unsigned char xdata code_access_sms; // от кого пришло SMS - код (группа) доступа
extern unsigned char xdata who_send_sms; // от кого пришло SMS - индекс номера (1-9)

extern unsigned char xdata sec_for_numtel; // Защита по номерам телефонов.
extern bit                _flag_need_send_sms; // Необходимо послать ответное SMS - сообщение.

extern bit                _online; // режим online

extern bit _flag_need_do_restart; // необходимо сделать рестарт

extern unsigned char xdata pasw_sms[8];  // пароль для sms пользователя
extern unsigned char xdata len_pasw_sms; // длина пароля для sms пользователя

extern bit _f_SMS_noanswer;   // если sms предположительно с интернета,
                              // то отвечать не надо.

//extern unsigned long xdata mAlert1_for_tsms; // Маска событий для текстовых sms-тревог.
//extern unsigned long xdata mAlert2_for_tsms; // 
//extern unsigned long xdata  alert1_for_tsms; // Произошедшие за сек. события для текстовых sms-тревог.
//extern unsigned long xdata  alert2_for_tsms; // 


#define kolCN 19 // кол-во байт для номера телефона   // ASCII
#define kolBN 10 // кол-во цифр для номера телефона   // SO
                 // round((16,17)/2)+1= 9
                 // round((18,19)/2)+1=10
                 // round((20,21)/2)+1=11
                 // round((22,23)/2)+1=12
                 // round((24,25)/2)+1=13
struct TEL_NUM
{
    unsigned char len_SO;       // кол-во байт номера в формате SO.
    unsigned char num_SO[kolBN];// тел. номер в формате SO.
    unsigned char need_write:1; // необходимость записать номер телефона в sim-карту.
    unsigned char accept    :1; // принят пакет с номером телефона.
    unsigned char sms_Al    :1; // посылать на этот номер текстовые sms-тревоги?.
    unsigned char need_send_sms_Al:1; // необходимо посылать на этот номер текстовую sms-тревогу.
};
#define KOL_NT  21    //кол-во номеров телефонов
extern struct TEL_NUM xdata tel_num[KOL_NT]; // номер sms-центра и номера дисп. центров

extern unsigned char xdata tmp_telnum_ascii[kolCN]; // тел.номер в ascii
extern unsigned char xdata len_telnum_ascii;      // длина тел.номера в ascii

extern bit  _flag_tel_ready;   // флаг готовности телефона (номера SC и DC считаны).
extern bit  _flag_tel_ready_R; // флаг готовности телефона только к приему (номеров SC и DC нет).

extern unsigned char xdata StatePB;

extern unsigned char xdata npack_31_inc;
extern unsigned char xdata npack_32_inc;
extern unsigned char xdata npack_33_inc;
extern unsigned char xdata npack_34_inc;
extern unsigned char xdata npack_35_inc;
extern unsigned char xdata npack_36_inc;
extern unsigned char xdata npack_37_inc;
extern unsigned char xdata npack_39_inc;
extern unsigned char xdata npack_3B_inc;
extern unsigned char xdata npack_3D_inc;
extern unsigned char xdata npack_3F_inc;
extern unsigned char xdata npack_43_inc;
extern unsigned char xdata npack_45_inc;
extern unsigned char xdata npack_47_inc;
extern unsigned char xdata npack_49_inc;
extern unsigned char xdata npack_4B_inc;

extern unsigned char xdata state_io_wk;

extern unsigned char xdata io_who_send_sms;

// Нажать на кнопку питания телефона.
void Turn_on_off_phone (void);

// Получить качество сигнала.
// Ret: байт кач-во сигнала
// 0 - нет сигнала или плохой сигнал
// 1 - переход от плохого сигнала к хорошему
// 2 - есть сигнал
// 3 - пока неизвестно
unsigned char Get_quality_signal (void);


// Инициализация модуля
void Init_io (void);

// Выбор канала связи ('S'-sms,'M'-modem, 'U'-UHF)
// Ret: 1-успешно, 0-нет.
//bit Choice_channel(unsigned char Ch);

// Свободен и готов к передаче
// Ret: 1-да, 0-нет
//bit Is_channel_ready(void);

// Есть принятые данные ?
// Ret: 1-да, 0-нет
//bit Is_read_data(void);

// Прочитать принятые данные.
// Ret: длина данных.
// Ret: Buf - указатель на массив с SMS - сообщением.
//unsigned char Read_new_data (unsigned char *Buf);

// Удалить принятые данные из памяти телефона если они есть.
//
//void Delete_data (void);

// Открыть канал
//void Open_channel(void);

// Открыт канал ?
// Ret: 1-да, 0-нет
//bit Is_channel_open(void);

// Послать данные
// Buf - буфер с данными
// Len - длина буфера
//void Send_data_to_channel(unsigned char *Buf, unsigned int Len);

// Есть подтверждение?
// Ret: 1-да, 0-нет, 2-пока неясно
//unsigned char Is_data_send(void);

// Закрыть канал
//void Close_channel(void);

void break_gsm_req(); //остановка обработки запроса, полученного по GSM

void full_buf_num_tel(unsigned char *buf);
void full_buf_num_tel2(unsigned char *buf);

void RestartToLoader(void); // Передача управления загрузчику.

// Автомат ввода/вывода
void Automat_io (void);

// Проверка - есть телефоны для прошивки?
bit Test_tel_to_phone(unsigned char xdata *pBufIO);

// Анализ считанных номеров
void Analise_tel_num (void);
// Анализ считанных 10 дополнительных номеров
void Analise_10d_tel_num(void);
// Есть необходимость отправить текстовые sms-тревоги.
void SetNeedSendSMSalert(void);


unsigned char Hex_to_DecASCII_5 (unsigned int HA, unsigned char *M);

unsigned char GetTel_ASCII_to_SO (unsigned char *tel_SO, unsigned char *tel_ascii, unsigned char Len);
unsigned char GetTel_SO_to_ASCII (unsigned char *tel_ascii, unsigned char *tel_SO, unsigned char len);

//функция формирования ответа на запрос 0x41 ('A'). Возвращает длину текстовой строки
unsigned char do_txtpack_S (unsigned char *pBuf, unsigned char G);
unsigned char do_txtpack_P (unsigned char *pBuf, unsigned char G);
unsigned char do_txtpack_N (unsigned char *pBuf, unsigned char G); // Основные номера
unsigned char do_txtpack_V (unsigned char *pBuf, unsigned char G); // Номер версии
unsigned char do_txtpack_T (unsigned char *pBuf, unsigned char G, unsigned char type); //Текст (опр. type)
//unsigned char do_txtpack_M(unsigned char *pBuf, unsigned char G);

//unsigned char do_txtpack8(unsigned char *pBuf);
unsigned char do_txtpack_alert (unsigned char *pBuf);
//Подготовка сообщения для отправки в ответ на запрос 0x03, 0x0B или 0x0F
unsigned char do_pack030F (unsigned char *pBuf);
//Подготовка сообщения для отправки в ответ на запрос 0x53, 0x5B
unsigned char do_pack53 (unsigned char *pBuf);

bit Test_read_text(unsigned char xdata * pBuf_read_data);
void Param_default(bit bPr2);

void NpackInc(unsigned char);
unsigned char Get_npack_inc(unsigned char);

void Get_HTTP_Time(unsigned char xdata *pInput);



#endif
