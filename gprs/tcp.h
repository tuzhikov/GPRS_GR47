#ifndef TCP_H
#define TCP_H


#include "..\main.h"
#include "ip.h"


typedef unsigned char 	TCP_SOCKET;
typedef unsigned int  	TCP_PORT;

/*
 * TCP Flags as defined by rfc793
 */
#define FIN     (0x01)
#define SYN     (0x02)
#define RST     (0x04)
#define PSH     (0x08)
#define ACKN 	(0x10)
#define URG     (0x20)
/*
 * TCP Options as defined by rfc 793
 */
#define TCP_OPTIONS_END_OF_LIST     (0x00)
#define TCP_OPTIONS_NO_OP           (0x01)
#define TCP_OPTIONS_MAX_SEG_SIZE    (0x02)
typedef struct _TCP_OPTIONS
{
    unsigned char        Kind;
    unsigned char        Length;
    WORD_VAL    MaxSegSize;
} TCP_OPTIONS;
/*
 * Pseudo header as defined by rfc 793.
 */
typedef struct _PSEUDO_HEADER
{
    IP_ADDR SourceAddress;
    IP_ADDR DestAddress;
    unsigned char Zero;
    unsigned char Protocol;
    unsigned int  TCPLength;
} PSEUDO_HEADER;
/*
 * TCP Header def. as per rfc 793.
 */
typedef struct _TCP_HEADER
{
    unsigned int    SourcePort;
    unsigned int    DestPort;
    unsigned long   SeqNumber;
    unsigned long   AckNumber;

	union
    {
    	struct
	    {
    	    unsigned char Reserved3      : 4;
        	unsigned char Val            : 4;
        } bits;
        unsigned char byte;
	} DataOffset;

    union
    {
        struct
        {
            unsigned char flagFIN    : 1;
            unsigned char flagSYN    : 1;
            unsigned char flagRST    : 1;
            unsigned char flagPSH    : 1;
            unsigned char flagACK    : 1;
            unsigned char flagURG    : 1;
            unsigned char Reserved2  : 2;
        } bits;
        unsigned char byte;
    } Flags;

    unsigned int    Window;
    unsigned int    Checksum;
    unsigned int    UrgentPointer;

} TCP_HEADER;
#define SIZE_TCP_HEADER		(20)

#define INVALID_SOCKET      (0xfe)
#define UNKNOWN_SOCKET      (0xff)

#define INVALID_BUFFER  	(unsigned char xdata*)(0xffff)
/*
 * TCP States as defined by rfc793
 */
typedef enum _TCP_STATE
{
    TCP_CLOSED = 0,
    TCP_LISTEN,
    TCP_SYN_RCVD,
    TCP_SYN_SENT,		// 3
    TCP_EST,			// 4
    TCP_FIN_WAIT_1,		// 5
	TCP_FIN_WAIT_2,
    TCP_CLOSING,
    TCP_TIMED_WAIT,
    TCP_SEND_RST,
    TCP_DATA_READY,
    TCP_LAST_ACK,		// 0x0A
    TCP_CLOSE,
    TCP_INVALID
} TCP_STATE;

typedef struct _SOCKET_INFO
{
    TCP_STATE 		smState;

    IP_ADDR  		remote;
    TCP_PORT 		localPort;
    TCP_PORT 		remotePort;

    unsigned char*	TxBuffer;
    unsigned int	TxCount;
    unsigned int	RxCount;

    unsigned long	SND_SEQ;
    unsigned long   SND_ACK;
	unsigned long   prevSND_ACK;

    unsigned char	RetryCount;

    struct
    {
        unsigned char Server       		: 1;
        unsigned char FirstRead    		: 1;
        unsigned char GetReady     		: 1;
    } Flags;

} SOCKET_INFO;

typedef struct _TCP_PACKET
{
	TCP_HEADER 		header;
	unsigned char  	Data[MAX_TCP_DATA];	 
} TCP_PACKET;

typedef struct {
    TCP_PACKET		packet;  
    unsigned int 	total_len;
} _tcp_context;
extern _tcp_context xdata 	tcp_context;

// for input data from PPP
typedef struct {
    unsigned int	i;  
    IP_ADDR 		local;
    IP_ADDR 		remote;
	unsigned char	protocol;
	unsigned int    len;
} _in_context;
extern _in_context xdata 	in_context;

typedef enum _state_REPLY_CODES {
	UNKNOW_REPLY = 0,
	_100_REPLY,
	_200_REPLY,
	_220_REPLY,
	_221_REPLY,
	_250_REPLY,
	_354_REPLY,
	_500_REPLY,
	_550_REPLY

} state_REPLY_CODES;

extern SOCKET_INFO xdata TCB[MAX_SOCKETS];

/*********************************************************************
 * Function:        void TCPInit(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          TCP is initialized.
 *
 * Side Effects:    None
 *
 * Overview:        Initialize all socket info.
 *
 * Note:            This function is called only one during lifetime
 *                  of the application.
 ********************************************************************/
void        		initTCP(void) large;

/*********************************************************************
 * Function:        TCP_SOCKET TCPListen(TCP_PORT port)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           port    - A TCP port to be opened.
 *
 * Output:          Given port is opened and returned on success
 *                  INVALID_SOCKET if no more sockets left.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
TCP_SOCKET  		TCPListen(TCP_PORT port) large;

/*********************************************************************
 * Function:        TCP_SOCKET TCPConnect(NODE_INFO* remote,
 *                                      TCP_PORT remotePort)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           remote      - Remote node address info
 *                  remotePort  - remote port to be connected.
 *
 * Output:          A new socket is created, connection request is
 *                  sent and socket handle is returned.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            By default this function is not included in
 *                  source.  You must define STACK_CLIENT_MODE to
 *                  be able to use this function.
 ********************************************************************/
TCP_SOCKET  		TCPConnect(IP_ADDR *remote, TCP_PORT port) large;

/*********************************************************************
 * Function:        BOOL TCPIsConnected(TCP_SOCKET s)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           s       - Socket to be checked for connection.
 *
 * Output:          TRUE    if given socket is connected
 *                  FALSE   if given socket is not connected.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            A socket is said to be connected if it is not
 *                  in LISTEN and CLOSED mode.  Socket may be in
 *                  SYN_RCVD or FIN_WAIT_1 and may contain socket
 *                  data.
 ********************************************************************/
BOOL        		TCPIsConnected(TCP_SOCKET s) large;

BOOL 				TCPIsDisconnected(TCP_SOCKET s) large;
/*********************************************************************
 *
 * PreCondition:    TCPInit() is already called     AND
 *
 * Input:           remote      - Remote node info
 *                  localPort   - Source port number
 *                  remotePort  - Destination port number
 *                  seq         - Segment sequence number
 *                  ack         - Segment acknowledge number
 *                  flags       - Segment flags
 *                  buffer      - Buffer to which this segment
 *                                is to be transmitted
 *                  len         - Total data length for this segment.
 *
 * Output:          A TCP segment is assembled and put to transmit.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void 				TransmitTCP(IP_ADDR *remote,
                        TCP_PORT localPort,
                        TCP_PORT remotePort,
                        unsigned long  tseq,
                        unsigned long  tack,
                        unsigned char  flags,
                        unsigned char* buffer,
                        unsigned int   len) large;

/*********************************************************************
 * Function:        void TCPDisconnect(TCP_SOCKET s)
 *
 * PreCondition:    TCPInit() is already called     AND
 *
 * Input:           s       - Socket to be disconnected.
 *
 * Output:          A disconnect request is sent for given socket.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
unsigned char  		  	TCPDisconnect(TCP_SOCKET s) large;

/*********************************************************************
 * Function:        BOOL TCPRequest(TCP_SOCKET s, unsigned char* buffer,
 *                      unsigned int   len)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           s       - Socket whose data is to be transmitted.
 *
 * Output:          All and any data associated with this socket
 *                  is marked as ready for transmission.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL        		TCPRequest(TCP_SOCKET socket, unsigned char* buffer, unsigned int len) large;

/*********************************************************************
 * Function:        BOOL TCPDiscard(TCP_SOCKET s)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           s       - socket
 *
 * Output:          TRUE if socket received data was discarded
 *                  FALSE if socket received data was already
 *                          discarded.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
BOOL        		TCPDiscard(TCP_SOCKET socket) large;

/*********************************************************************
 *
 ********************************************************************/
BOOL 				rcvTCP(void ) large;

/*********************************************************************
 *
 ********************************************************************/
unsigned char		timeoutSendTCP(TCP_SOCKET socket) large;

/*********************************************************************
 * Function:        void TCPTick(void)
 *
 * PreCondition:    TCPInit() is already called.
 *
 * Input:           None
 *
 * Output:          Each socket FSM is executed for any timeout
 *                  situation.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void        TCPTick(signed char* ) large;

void 		CloseSocket(SOCKET_INFO* ps) large;


#endif
