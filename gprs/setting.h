#ifndef _SET_H
#define _SET_H


#define LOCAL_PORT_START_NUMBER 	(1024)
#define LOCAL_PORT_END_NUMBER   	(5000)
#define HTTP_SERVER_PORT            (80)//(84 = NTCNVG.RU) (83 = NTCNVG.RU -> JRun)
#define SMTP_SERVER_PORT            (25)


// TOTAL NUMBER OF SOCKET IN CPU 
#define MAX_SOCKETS					(1)
// DATA BUFFER PPP
#define MAX_SIZE_BUFFER_PPP 		0x0180//0x0220
// DATA BUFFER IP
#define MAX_IP_DATA		            0x0160//0x0200
// DATA BUFFER TCP
#define MAX_TCP_DATA				0x0140//0x01E0
// Maximum number of times a connection be retried before closing it down.
#define MAX_RETRY_COUNTS    		(3)
// TIMEOUTS FOR SOCKETS
#define RTT							_5_sec_01
// ------------	HTTP TIMEOUT -------------
#define TIME_BETWEEN_CONNECTING		_5_sec_01
#define TIME_BETWEEN_CONNECT_POST	_1_sec_001
#define TIME_HTTP_DISCONNECT		_3_min_1
// ------------	SMTP TIMEOUT -------------
#define TIME_SMTP_DISCONNECT		_3_min_1


#define MAX_IP_OPTIONS_LEN     		(20)            // As per RFC 791.


typedef union _IP_ADDR
{
	unsigned char       v[4];
    unsigned long       Val;
} IP_ADDR;

typedef union _HTTP_PORT
{
	unsigned char       ch[2];
    unsigned int        Val;
} HTTP_PORT;

extern signed int code 	timeoutState[];

extern IP_ADDR code	remoteIP;
extern HTTP_PORT code remotePORT;

#endif
