//
// IO_SMS.h
//

#ifndef IO_SMS_H
#define IO_SMS_H

// ******************************************************************
//
// Ввод/вывод с использованием сотовых телефонов через SMS сообщения.
//
// ******************************************************************

extern unsigned char xdata D_date;
extern unsigned char xdata M_date;
extern unsigned  int xdata G_date;


#define LENGTH_ID_SIM_CARD	20
typedef struct {
    //unsigned char	manufacturer[16];     		// from CGMI
	//unsigned char 	imei[15]; 		    	// from CGSN
	//unsigned char 	imsi[15];		        // from CIMI (MCC-3 digit, MNC-2,3 digit)
	unsigned char 	sim[LENGTH_ID_SIM_CARD];    // from CCID
    //unsigned char 	ready;                	// from CPAS
    //unsigned char 	pin_ready;			   	// from CPIN
 
	//network_state 	net_state;		    	// from CREG
	//unsigned int  	LAC;			        // from CREG - location area code 
	//unsigned int  	CID;		            // from CREG - cell ID

	//unsigned char 	operator[5]; 	        // from COPS - in numeric format
	//unsigned int  	signal_level; 	    	// from CSQ
	//unsigned char 	status_sim;	        	// from SCID

    //unsigned char	call_mode;
    //unsigned char	enter_number[2*SMS_ORIGINATING_ADDRESS_LENGTH];

    unsigned char 	exist_simid : 1;
    unsigned char 	simid_len   : 5;
    //unsigned char 	exist_smsc;
	//unsigned char 	exist_clients;

	//unsigned char   battery_supply;		
	//unsigned char   battery_charge;

	//struct gsm_peer smsc;
} _context_gsm;
extern _context_gsm xdata	context_gsm;


// Инициализация модуля
//
void Init_sms_phone (void);


// Телефон готов к работе?
// Ret: 1-да, 0-нет
//
bit Is_tel_ready (void);

// ******************************************************************
// Телефон готов к приему?
// Ret: 1-да, 0-нет
// 
bit Is_tel_ready_R (void);


// Автомат Siemens
void Automat_sms_phone (void);


// Получить качество сигнала.
// Ret: байт кач-во сигнала
// 0 - нет сигнала или плохой сигнал
// 1 - переход от плохого сигнала к хорошему
// 2 - есть сигнал
// 3 - пока неизвестно
//
//unsigned char Get_quality_signal (void);


// Есть ли новое SMS сообщение?
// Ret: 1-да, 0-нет
//
bit Is_new_sms (void);


// Это 8 битное sms ?
// Ret: 1-да, 0-нет
//
bit Is_new_sms_8bit (void);


// Прочитать новое SMS сообщение.
// Ret: длина SMS - сообщения.
// Ret: buf_sms - указатель на массив с SMS - сообщением.
//
//unsigned char Read_new_sms (unsigned char *buf_sms);
unsigned char Read_new_sms (void);


// Удалить принятое sms сообщение из памяти телефона.
//
void Delete_sms (void);

// Есть неудаленное sms сообщение в памяти телефона?
// Ret: 1-да, 0-нет
//
bit IsNonDelete_sms (void);


// Послать SMS сообщение.
// Buf - указатель на массив с SMS - сообщением.
// Len - длина SMS - сообщения.
// Num - номер Disp-центра
//
void Send_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Num);

void Send_modem (unsigned char *buf_sms, unsigned char len_sms);

// Есть подтверждение об отправке SMS сообщения?
// Ret: 1-да, 0-нет, 2-пока неясно
//
unsigned char Is_sms_send_out (void);


// Нажать на кнопку питания телефона.
//
void Turn_on_off_phone (void);


// Полный сброс телефона (с выключением питания)
//
void Restart_phone (void);


// Записать в телефон (в телефонную книжку) новый номер.
// Buf - указатель на массив с телефонным номером.
// ix  - индекс телефонного номера.
//
void Write_tel_to_phone (unsigned char ix, unsigned char *buf);




#endif
