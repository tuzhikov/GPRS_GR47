// ******************************************************************
//
// IO.h
//
// ��������� ������� ��� ������ � ����� ������� �����
//
// ******************************************************************
//

#ifndef IO_H
#define IO_H


#define CHANNEL_SMS


unsigned char Hex_to_HexASCII_Hi (unsigned char Hx);
unsigned char Hex_to_HexASCII_Lo (unsigned char Hx);

extern unsigned char xdata * pBuf_read_data; // ��������� �� ����� ��� ����� ����������

extern unsigned char xdata md_mode;
extern unsigned char xdata md_who_call;
//extern unsigned long xdata sTime_md_mode;
//extern unsigned char xdata tmp_LAC[2];
//extern unsigned char xdata tmp_CI[2];
//extern unsigned char xdata tmp_RxLev;

extern unsigned char xdata md_resetGSM;

extern unsigned char xdata code_access_sms; // �� ���� ������ SMS - ��� (������) �������
extern unsigned char xdata who_send_sms; // �� ���� ������ SMS - ������ ������ (1-9)

extern unsigned char xdata sec_for_numtel; // ������ �� ������� ���������.
extern bit                _flag_need_send_sms; // ���������� ������� �������� SMS - ���������.

extern bit                _online; // ����� online

extern bit _flag_need_do_restart; // ���������� ������� �������

extern unsigned char xdata pasw_sms[8];  // ������ ��� sms ������������
extern unsigned char xdata len_pasw_sms; // ����� ������ ��� sms ������������

extern bit _f_SMS_noanswer;   // ���� sms ���������������� � ���������,
                              // �� �������� �� ����.

//extern unsigned long xdata mAlert1_for_tsms; // ����� ������� ��� ��������� sms-������.
//extern unsigned long xdata mAlert2_for_tsms; // 
//extern unsigned long xdata  alert1_for_tsms; // ������������ �� ���. ������� ��� ��������� sms-������.
//extern unsigned long xdata  alert2_for_tsms; // 


#define kolCN 19 // ���-�� ���� ��� ������ ��������   // ASCII
#define kolBN 10 // ���-�� ���� ��� ������ ��������   // SO
                 // round((16,17)/2)+1= 9
                 // round((18,19)/2)+1=10
                 // round((20,21)/2)+1=11
                 // round((22,23)/2)+1=12
                 // round((24,25)/2)+1=13
struct TEL_NUM
{
    unsigned char len_SO;       // ���-�� ���� ������ � ������� SO.
    unsigned char num_SO[kolBN];// ���. ����� � ������� SO.
    unsigned char need_write:1; // ������������� �������� ����� �������� � sim-�����.
    unsigned char accept    :1; // ������ ����� � ������� ��������.
    unsigned char sms_Al    :1; // �������� �� ���� ����� ��������� sms-�������?.
    unsigned char need_send_sms_Al:1; // ���������� �������� �� ���� ����� ��������� sms-�������.
};
#define KOL_NT  21    //���-�� ������� ���������
extern struct TEL_NUM xdata tel_num[KOL_NT]; // ����� sms-������ � ������ ����. �������

extern unsigned char xdata tmp_telnum_ascii[kolCN]; // ���.����� � ascii
extern unsigned char xdata len_telnum_ascii;      // ����� ���.������ � ascii

extern bit  _flag_tel_ready;   // ���� ���������� �������� (������ SC � DC �������).
extern bit  _flag_tel_ready_R; // ���� ���������� �������� ������ � ������ (������� SC � DC ���).

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

// ������ �� ������ ������� ��������.
void Turn_on_off_phone (void);

// �������� �������� �������.
// Ret: ���� ���-�� �������
// 0 - ��� ������� ��� ������ ������
// 1 - ������� �� ������� ������� � ��������
// 2 - ���� ������
// 3 - ���� ����������
unsigned char Get_quality_signal (void);


// ������������� ������
void Init_io (void);

// ����� ������ ����� ('S'-sms,'M'-modem, 'U'-UHF)
// Ret: 1-�������, 0-���.
//bit Choice_channel(unsigned char Ch);

// �������� � ����� � ��������
// Ret: 1-��, 0-���
//bit Is_channel_ready(void);

// ���� �������� ������ ?
// Ret: 1-��, 0-���
//bit Is_read_data(void);

// ��������� �������� ������.
// Ret: ����� ������.
// Ret: Buf - ��������� �� ������ � SMS - ����������.
//unsigned char Read_new_data (unsigned char *Buf);

// ������� �������� ������ �� ������ �������� ���� ��� ����.
//
//void Delete_data (void);

// ������� �����
//void Open_channel(void);

// ������ ����� ?
// Ret: 1-��, 0-���
//bit Is_channel_open(void);

// ������� ������
// Buf - ����� � �������
// Len - ����� ������
//void Send_data_to_channel(unsigned char *Buf, unsigned int Len);

// ���� �������������?
// Ret: 1-��, 0-���, 2-���� ������
//unsigned char Is_data_send(void);

// ������� �����
//void Close_channel(void);

void break_gsm_req(); //��������� ��������� �������, ����������� �� GSM

void full_buf_num_tel(unsigned char *buf);
void full_buf_num_tel2(unsigned char *buf);

void RestartToLoader(void); // �������� ���������� ����������.

// ������� �����/������
void Automat_io (void);

// �������� - ���� �������� ��� ��������?
bit Test_tel_to_phone(unsigned char xdata *pBufIO);

// ������ ��������� �������
void Analise_tel_num (void);
// ������ ��������� 10 �������������� �������
void Analise_10d_tel_num(void);
// ���� ������������� ��������� ��������� sms-�������.
void SetNeedSendSMSalert(void);


unsigned char Hex_to_DecASCII_5 (unsigned int HA, unsigned char *M);

unsigned char GetTel_ASCII_to_SO (unsigned char *tel_SO, unsigned char *tel_ascii, unsigned char Len);
unsigned char GetTel_SO_to_ASCII (unsigned char *tel_ascii, unsigned char *tel_SO, unsigned char len);

//������� ������������ ������ �� ������ 0x41 ('A'). ���������� ����� ��������� ������
unsigned char do_txtpack_S (unsigned char *pBuf, unsigned char G);
unsigned char do_txtpack_P (unsigned char *pBuf, unsigned char G);
unsigned char do_txtpack_N (unsigned char *pBuf, unsigned char G); // �������� ������
unsigned char do_txtpack_V (unsigned char *pBuf, unsigned char G); // ����� ������
unsigned char do_txtpack_T (unsigned char *pBuf, unsigned char G, unsigned char type); //����� (���. type)
//unsigned char do_txtpack_M(unsigned char *pBuf, unsigned char G);

//unsigned char do_txtpack8(unsigned char *pBuf);
unsigned char do_txtpack_alert (unsigned char *pBuf);
//���������� ��������� ��� �������� � ����� �� ������ 0x03, 0x0B ��� 0x0F
unsigned char do_pack030F (unsigned char *pBuf);
//���������� ��������� ��� �������� � ����� �� ������ 0x53, 0x5B
unsigned char do_pack53 (unsigned char *pBuf);

bit Test_read_text(unsigned char xdata * pBuf_read_data);
void Param_default(bit bPr2);

void NpackInc(unsigned char);
unsigned char Get_npack_inc(unsigned char);

void Get_HTTP_Time(unsigned char xdata *pInput);



#endif
