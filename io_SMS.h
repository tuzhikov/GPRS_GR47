//
// IO_SMS.h
//

#ifndef IO_SMS_H
#define IO_SMS_H

// ******************************************************************
//
// ����/����� � �������������� ������� ��������� ����� SMS ���������.
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


// ������������� ������
//
void Init_sms_phone (void);


// ������� ����� � ������?
// Ret: 1-��, 0-���
//
bit Is_tel_ready (void);

// ******************************************************************
// ������� ����� � ������?
// Ret: 1-��, 0-���
// 
bit Is_tel_ready_R (void);


// ������� Siemens
void Automat_sms_phone (void);


// �������� �������� �������.
// Ret: ���� ���-�� �������
// 0 - ��� ������� ��� ������ ������
// 1 - ������� �� ������� ������� � ��������
// 2 - ���� ������
// 3 - ���� ����������
//
//unsigned char Get_quality_signal (void);


// ���� �� ����� SMS ���������?
// Ret: 1-��, 0-���
//
bit Is_new_sms (void);


// ��� 8 ������ sms ?
// Ret: 1-��, 0-���
//
bit Is_new_sms_8bit (void);


// ��������� ����� SMS ���������.
// Ret: ����� SMS - ���������.
// Ret: buf_sms - ��������� �� ������ � SMS - ����������.
//
//unsigned char Read_new_sms (unsigned char *buf_sms);
unsigned char Read_new_sms (void);


// ������� �������� sms ��������� �� ������ ��������.
//
void Delete_sms (void);

// ���� ����������� sms ��������� � ������ ��������?
// Ret: 1-��, 0-���
//
bit IsNonDelete_sms (void);


// ������� SMS ���������.
// Buf - ��������� �� ������ � SMS - ����������.
// Len - ����� SMS - ���������.
// Num - ����� Disp-������
//
void Send_sms (unsigned char *buf_sms, unsigned char len_sms, unsigned char Num);

void Send_modem (unsigned char *buf_sms, unsigned char len_sms);

// ���� ������������� �� �������� SMS ���������?
// Ret: 1-��, 0-���, 2-���� ������
//
unsigned char Is_sms_send_out (void);


// ������ �� ������ ������� ��������.
//
void Turn_on_off_phone (void);


// ������ ����� �������� (� ����������� �������)
//
void Restart_phone (void);


// �������� � ������� (� ���������� ������) ����� �����.
// Buf - ��������� �� ������ � ���������� �������.
// ix  - ������ ����������� ������.
//
void Write_tel_to_phone (unsigned char ix, unsigned char *buf);




#endif
