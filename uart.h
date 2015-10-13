#ifndef UART_H
#define UART_H



#define SPIBSY SPI0CFG&0x80

#define SIZE_BUF_RX_UART0  700
#define SIZE_BUF_RX_UART1  128
#define SIZE_BUF_RX_MAX    10

#define SIZE_BUF_TX_UART0  100  //0x180//28+kolCN
#define SIZE_BUF_TX_UART1  500



extern unsigned char xdata buf_rx_uart0[];
extern unsigned  int xdata ukBuf_rx_r_uart0;
extern unsigned  int xdata ukBuf_rx_w_uart0;
extern unsigned  int xdata sizeBuf_rx_uart0;
extern unsigned char xdata buf_tx_uart0[];
extern unsigned  int xdata cBufTxR;
extern unsigned  int xdata cBufTxW;

extern unsigned char xdata buf_rx_uart1[];
extern unsigned char xdata ukBuf_rx_r_uart1;
extern unsigned char xdata ukBuf_rx_w_uart1;
extern unsigned char xdata sizeBuf_rx_uart1;
extern unsigned char xdata buf_tx_uart1[];
extern unsigned  int xdata cBufTxR1;
extern unsigned  int xdata cBufTxW1;

extern unsigned char xdata buf_rx_max[];
extern unsigned char xdata ukBuf_rx_r_max;
extern unsigned char xdata ukBuf_rx_w_max;
extern unsigned char xdata sizeBuf_rx_max;


extern unsigned char xdata tx_data_uart0;
extern unsigned char xdata tx_data_uart1;


extern unsigned char xdata char_max;
extern bit _flag_tangenta; // 1 - передача, 0 - прием
extern unsigned char xdata OutMAXPointer2;



void Init_uart0 (void);
void Init_uart1 (void);
void Init_max(unsigned char clk_md);

// посылка очередного байта информ. на Usart0.
void send_char_uart0 (void);
// отправка строки на UART0
void sendStringUART0(unsigned char *str);
// посылка очередного байта информ. на Usart1.
void send_char_uart1 (void);
// отправка строки на UART1
void sendStringUART1(unsigned char *str);
// посылка очередного байта информ. на Max.
void send_max();

// Прочитать байт из приемного буфера от uart0
unsigned char Read_byte_uart0 (void);
// Прочитать байт из приемного буфера от uart0 (для GPRS)
bit Read_byte_uart0_GPRS (void);
// Прочитать байт из приемного буфера от uart1
unsigned char Read_byte_uart1 (void);
// Прочитать байт из приемного буфера от max
bit receive_char_max (void);

// Очистить приемный буфер от uart0
void ClearBuf_uart0 (void);
// Очистить приемный буфер от uart1
void ClearBuf_uart1 (void);
// Очистить приемный буфер от max
void ClearBuf_max (void);

// Переключить тангенту на передачу
void max_transmit (void);
// Переключить тангенту на прием
void max_receive (void);



#endif
