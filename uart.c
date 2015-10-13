#include <c8051F120.h>
#include <String.h>
#include "main.h"
#include "rmodem.h"
#include "uart.h"
#include "io.h"
#include "gprs\gprs.h"



unsigned char xdata buf_rx_uart0[SIZE_BUF_RX_UART0];
unsigned  int xdata ukBuf_rx_r_uart0=0;
unsigned  int xdata ukBuf_rx_w_uart0=0;
unsigned  int xdata sizeBuf_rx_uart0=0;
unsigned char xdata buf_tx_uart0[SIZE_BUF_TX_UART0];
unsigned  int xdata cBufTxR=0; //read from buffer (to write to SBUF reg)
unsigned  int xdata cBufTxW=0; //write to buffer from automat

unsigned char xdata buf_rx_uart1[SIZE_BUF_RX_UART1];
unsigned char xdata ukBuf_rx_r_uart1=0;
unsigned char xdata ukBuf_rx_w_uart1=0;
unsigned char xdata sizeBuf_rx_uart1=0;
unsigned char xdata buf_tx_uart1[SIZE_BUF_TX_UART1];
unsigned  int xdata cBufTxR1=0; //read from buffer (to write to SBUF reg)
unsigned  int xdata cBufTxW1=0; //write to buffer from automat

unsigned char xdata buf_rx_max[SIZE_BUF_RX_MAX];
unsigned char xdata ukBuf_rx_r_max=0;
unsigned char xdata ukBuf_rx_w_max=0;
unsigned char xdata sizeBuf_rx_max=0;
/*
unsigned char xdata buf_rx_dtmf[SIZE_BUF_RX_DTMF];
unsigned char xdata ukBuf_rx_r_dtmf=0;
unsigned char xdata ukBuf_rx_w_dtmf=0;
unsigned char xdata sizeBuf_rx_dtmf=0;
*/


unsigned char xdata tx_data_uart0;
unsigned char xdata tx_data_uart1;


unsigned char xdata char_max;
unsigned char xdata OutMAXPointer1 = 0; //��. �� �������� �� ������ ����
unsigned char xdata OutMAXPointer2 = 0; //��. �� ������������ � ����� ����
bit _flag_tangenta = 0; //"0" - ��������, "1" - �����


/*------------------------------- Instal UART0, UARt1 ------------------------------*/
void Init_uart0 (void){FlagTX0 = yes;}
void Init_uart1 (void){FlagTX1 = yes;}
/*---------------------------- �������� UART0, UARt1--------------------------------*/
// ������� ���������� ����� ������. �� Usart0.
void send_char_uart0()
{
  reset_wdt;
  while (cBufTxW >= SIZE_BUF_TX_UART0)reset_wdt; //dead

  int_uart0_no;

    if (FlagTX0) //idle transmission
    {
        FlagTX0 = no;
        SFRPAGE = UART0_PAGE;
        SBUF0 = tx_data_uart0;
    }
    else         //busy transmission
        buf_tx_uart0[cBufTxW++] = tx_data_uart0;

  int_uart0_yes;
}
// send string to UART0
void sendStringUART0(unsigned char *str)//reentrant
{
unsigned short  StrLength = strlen(str); //������ ������ � ������ ��� ������������� ����
unsigned short index = 0;

while(index < StrLength)
	{ 
	tx_data_uart0 = str[index];
	send_char_uart0();
	index++;
	reset_wdt;							  
	}	
}
// ������� ���������� ����� ������. �� Usart1.
void send_char_uart1()
{
  reset_wdt;
  while (cBufTxW1 >= SIZE_BUF_TX_UART1); //dead

  int_uart1_no;

    if (FlagTX1) //idle transmission
    {
        FlagTX1 = no;
        SFRPAGE = UART1_PAGE;
        SBUF1 = tx_data_uart1;
    }
    else         //busy transmission
        buf_tx_uart1[cBufTxW1++] = tx_data_uart1;

  int_uart1_yes;
}
// send string to UART1
void sendStringUART1(unsigned char *str)//reentrant
{
unsigned char  StrLength = strlen(str); //������ ������ � ������ ��� ������������� ����
unsigned short index = 0;

while(index < StrLength)
	{ 
	tx_data_uart1 = str[index];
	send_char_uart1();
	index++;
	reset_wdt;							  
	}	
}
/*------------------- ������ ����� UART0, UART1 ------------------------------------- */
// ��������� ���� �� ��������� ������ �� uart0
unsigned char Read_byte_uart0 (void)
{
  unsigned char B=0;
  int_uart0_no;
    if (sizeBuf_rx_uart0)
    {
        B = buf_rx_uart0[ukBuf_rx_r_uart0];
        ukBuf_rx_r_uart0++;
        sizeBuf_rx_uart0--;
        if (ukBuf_rx_r_uart0 >= SIZE_BUF_RX_UART0) ukBuf_rx_r_uart0 = 0;
    }
  int_uart0_yes;
    return B;
}
// ��������� ���� �� ��������� ������ �� uart0 (��� GPRS)
bit Read_byte_uart0_GPRS (void)
{
  int_uart0_no;
    if(sizeBuf_rx_uart0)
    {
    in_data = buf_rx_uart0[ukBuf_rx_r_uart0];
    sizeBuf_rx_uart0--;
    int_uart0_yes;
        if (++ukBuf_rx_r_uart0 >= SIZE_BUF_RX_UART0) ukBuf_rx_r_uart0 = 0;
    return 1;
    }
  int_uart0_yes;
    return 0;
}
// ��������� ���� �� ��������� ������ �� uart1
unsigned char Read_byte_uart1 (void)
{
  unsigned char B=0;
  int_uart1_no;
    if (sizeBuf_rx_uart1)
    {
        B = buf_rx_uart1[ukBuf_rx_r_uart1];
        ukBuf_rx_r_uart1++;
        sizeBuf_rx_uart1--;
        if (ukBuf_rx_r_uart1 >= SIZE_BUF_RX_UART1) ukBuf_rx_r_uart1 = 0;
    }
  int_uart1_yes;
    return B;
}
/*------------------ �������� ������ ��� UART 0? UARt1 ---------------------------- */
// �������� �������� ����� �� uart0
void ClearBuf_uart0 (void)
{
    if (sizeBuf_rx_uart0==0) return;
  int_uart0_no;
    sizeBuf_rx_uart0=0;
    ukBuf_rx_r_uart0=0;
    ukBuf_rx_w_uart0=0;
  int_uart0_yes;
}
// �������� �������� ����� �� uart1
void ClearBuf_uart1 (void)
{
    //if (sizeBuf_rx_uart1==0) return;
  int_uart1_no;
    memset(buf_rx_uart1,0,sizeof(buf_rx_uart1));
	sizeBuf_rx_uart1=0;
    ukBuf_rx_r_uart1=0;
    ukBuf_rx_w_uart1=0;
  int_uart1_yes;
}
// -----------------------------------------------------------------------
//          M A X
// -----------------------------------------------------------------------

void HoldSPI_MAX() //�������� ������������ � "������" ���� SPI (for MAX3100 UART)
{
    while (!CSEL1) reset_wdt;
    int_SPI0_no;
    int_ext1_no;
    CSEL0 = 0;
    SFRPAGE = SPI0_PAGE;
    SPIEN = 0; //? disable SPI
    SPI0CFG = 0x40;
    SPI0CKR = 0x05;
    SPI0CN  = 0x0B;
    reset_wdt; //pause
}

void FreeSPI_MAX() //���������� SPI � ������������ ��������� (for MAX3100 UART)
{
    CSEL0 = 1;
    SFRPAGE = SPI0_PAGE;
    SPI0CFG = 0x70; //? set Phase/Polarity for AtFlash
    SPI0CN = 0x0A; //? clear SPI interrupt flags, disable SPI
    int_ext1_yes;
    int_SPI0_yes;
}

void Init_max(unsigned char clk_md)
{
    HoldSPI_MAX();
    // ---Baud-Rates at MAX3100 Ext Clock 1.8 MHz---
    // 115.2k: clk_md = 0x00
    //  57.6k: clk_md = 0x01
    //  19.2k: clk_md = 0x09
    //   9600: clk_md = 0x0A
    //   4800: clk_md = 0x0B
    //   1200: clk_md = 0x0D
    SPI0DAT = 0xC4;
    while (!TXBMT) reset_wdt;
    SPI0DAT = clk_md;
    while (!TXBMT) reset_wdt;
    while (SPIBSY) reset_wdt;
    FreeSPI_MAX();
}

void send_max() //automat
{
  unsigned char data RX_SPI_1;
  if (!OutMAXPointer2) return;
    HoldSPI_MAX();
  start1:
    SPI0DAT = 0x40;
    while (!SPIF) reset_wdt; SPIF = 0;
    RX_SPI_1 = SPI0DAT;
    SPI0DAT = 0x00;
    while (!SPIF) reset_wdt; SPIF = 0;
    CSEL0 = 1;
    reset_wdt;
    CSEL0 = 0;
    if (RX_SPI_1 & 0x40) SPI0DAT = (_flag_tangenta ? 0x80 : 0x82);
    else goto start1;
    while (!SPIF) reset_wdt; SPIF = 0;
    SPI0DAT = pcontext[OutMAXPointer1++];
    while (!SPIF) reset_wdt; SPIF = 0;
    FreeSPI_MAX();
  if (OutMAXPointer1 >= OutMAXPointer2) OutMAXPointer1 = OutMAXPointer2 = 0;
}

// ��������� ���� �� ��������� ������ �� max
bit receive_char_max (void)
{
    bit B=0;
  int_SPI0_no;
  int_ext1_no;
    if (sizeBuf_rx_max)
    {
        char_max = buf_rx_max[ukBuf_rx_r_max];
        ukBuf_rx_r_max++;
        sizeBuf_rx_max--;
        if (ukBuf_rx_r_max >= SIZE_BUF_RX_MAX) ukBuf_rx_r_max = 0;
        B=1;
    }
  int_ext1_yes;
  int_SPI0_yes;
    return B;
}

// �������� �������� ����� �� max
void ClearBuf_max (void)
{
    if (sizeBuf_rx_max==0) return;
  int_SPI0_no;
  int_ext1_no;
    sizeBuf_rx_max=0;
    ukBuf_rx_r_max=0;
    ukBuf_rx_w_max=0;
  int_ext1_yes;
  int_SPI0_yes;
}

// ����������� �������� �� ��������
void max_transmit (void)
{
   _flag_tangenta=0;
    pcontext[0] = 0x43; // �������� 'C' � ������������� RTS � 1 (�������� ������).
    OutMAXPointer2 = 1;
}

// ����������� �������� �� �����
void max_receive (void)
{
   _flag_tangenta=1;
    pcontext[0] = 0x43; // �������� 'C' � ������������� RTS � 0 (����� ������).
    OutMAXPointer2 = 1;
}
