#define THIS_IS_TCP

#include "c8051f120.h"	
#include <string.h>
#include <math.h>

#include "..\main.h"

#include "setting.h"
#include "ppp.h"
#include "ip.h"
#include "tcp.h"
#include "gprs.h"
#include "http.h"
//#include "smtp.h"



_tcp_context xdata 		tcp_context;
_in_context  xdata 		in_context;

/*
 * These are all sockets supported by this TCP.
 */
SOCKET_INFO xdata 	TCB[MAX_SOCKETS] = {0};

/*
 * Local temp port numbers.
 */
static unsigned int xdata _NextPort;

//
static  void    	HandleTCPSeg(TCP_SOCKET 	s,
                                 IP_ADDR*		remote,
                                 TCP_HEADER*	h,
                                 unsigned int	len) large;
static TCP_SOCKET 	FindMatchingSocket(TCP_HEADER *h, IP_ADDR *remote) large;
static void    		SwapTCPHeader(TCP_HEADER* header) large;
static unsigned int	CalcTCPChecksum(unsigned int len) large;
void 		        CloseSocket(SOCKET_INFO* ps) large;
#define 			_SendTCP(remote, localPort, remotePort, seq, ack, flags)     \
        			TransmitTCP(remote, localPort, remotePort, seq, ack, flags, INVALID_BUFFER, 0)


//struct_str code	    tcpPREV = { "\r\n<RECEIVE PREVIOUS PACKET!!!\r\n" ,	 31, 0 };


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
void initTCP(void) large
{
TCP_SOCKET xdata 	s;
SOCKET_INFO xdata* 	ps;

	//* cancelSignal(T_TCP_Timeout0);
	//* cancelSignal(T_TCP_Timeout1);
    timer_tcp_timeout = tcp.b.timeout = 0;

    // Initialize all sockets.
    for(s = 0;s < MAX_SOCKETS;s++) {
        ps = &TCB[s];
        ps->smState        		= TCP_CLOSED;
        ps->Flags.Server   		= FALSE;
        ps->Flags.FirstRead 	= TRUE;
        ps->Flags.GetReady      = FALSE;
        ps->TxBuffer        	= INVALID_BUFFER;
		ps->RetryCount		    = 0;
    };

    _NextPort = LOCAL_PORT_START_NUMBER;
}

/*********************************************************************
 * Function:        void sendTCP(void)
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
unsigned char xdata t_TCP_SYN_SENT   = 0;
unsigned char xdata t_TCP_SYN_RCVD   = 0;
unsigned char xdata t_TCP_EST        = 0;
unsigned char xdata t_TCP_LAST_ACK   = 0;
unsigned char xdata t_TCP_TIMED_WAIT = 0;
/*
unsigned char* code sm_state[] = {
	"CLOSED__",
    "LISTEN__",
    "SYN_RCVD",
    "SYN_SENT",		// 3
    "EST_____",		// 4
    "FIN_WT_1",		// 5
	"FIN_WT_2",
    "CLOSING_",
    "TIMED_WT",
    "SEND_RST",
    "DATA_RDY",
    "LAST_ACK",		// 0x0A
    "CLOSE___",
    "INVALID_"
};

unsigned char* code f[] = {
	"RST",
    "ACK",
    "FIN",
    "SYN",		
    "PSH",		
    "URG"
};

void DEBUG_timeoutSendTCP(TCP_SOCKET s, unsigned char flags)
{
unsigned char xdata i;
unsigned char xdata c = 0;
SOCKET_INFO xdata*	ps;

    ps = &TCB[s];			// берем текущий сокет
		putGPS('[');
		charToHex2(s);
		putGPS(hex2[1]); putGPS(hex2[0]);
		putGPS(']');
	putGPS('-'); putGPS('>');
	for(i=0;i<8;i++) putGPS(sm_state[ps->smState][i]);
	putGPS(':');
	putGPS('{');
		if(flags & RST) { for(i=0;i<3;i++) putGPS(f[0][i]); putGPS(' '); }
		if(flags & ACKN){ for(i=0;i<3;i++) putGPS(f[1][i]); putGPS(' '); }
		if(flags & FIN) { for(i=0;i<3;i++) putGPS(f[2][i]); putGPS(' '); }
		if(flags & SYN) { for(i=0;i<3;i++) putGPS(f[3][i]); putGPS(' '); }
		if(flags & PSH) { for(i=0;i<3;i++) putGPS(f[4][i]); putGPS(' '); }
		if(flags & URG) { for(i=0;i<3;i++) putGPS(f[5][i]); putGPS(' '); }
	putGPS('S'); putGPS('=');
		charToHex2((unsigned char)((ps->SND_SEQ >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS(',');
	putGPS('A'); putGPS('=');
		charToHex2((unsigned char)((ps->SND_ACK >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS(',');
		charToHex2(ps->RetryCount);
		putGPS(hex2[0]);
	putGPS('}');
	putGPS(0x0D); putGPS(0x0A);
}
*/
unsigned char timeoutSendTCP(TCP_SOCKET socket) large
{
SOCKET_INFO xdata* 	ps;
unsigned long xdata seq;
unsigned char xdata flags;

	if(sendingActive()) return(FALSE);
	//
    flags = 0x00;
    ps = &TCB[socket];
    /*
     * Closed or Passively Listening socket do not care
     * about timeout conditions.
     */
    if((ps->smState == TCP_CLOSED) || (ps->smState == TCP_LISTEN && ps->Flags.Server == TRUE)) return(FALSE);
    ps->RetryCount++;
    /*
     * A timeout has occured.  Respond to this timeout condition
   	 * depending on what state this socket is in.
     */
    switch(ps->smState) {
   	    case TCP_SYN_SENT:
       	    /*
        	 * Keep sending SYN until we hear from remote node.
       	     * This may be for infinite time, in that case
        	 * caller must detect it and do something.
  	         * Bug Fix: 11/1/02
    	     */
			if(ps->RetryCount < MAX_RETRY_COUNTS) { flags = SYN; seq = ps->SND_SEQ++; } 
				else CloseSocket(ps);
t_TCP_SYN_SENT++;
	    break;
	
        case TCP_SYN_RCVD:
   	        /*
    	     * We must receive ACK before timeout expires.
   	    	 * If not, resend SYN+ACK.
          	 * Abort, if maximum attempts counts are reached.
             */
	        if(ps->RetryCount < MAX_RETRY_COUNTS) { flags = SYN | ACKN; seq = ps->SND_SEQ++;} 
				else CloseSocket(ps);
t_TCP_SYN_RCVD++;
        break;

        case TCP_EST:
	        /*
   		     * Don't let this connection idle for very long time.
       		 * If we did not receive or send any message before timeout
          	 * expires, close this connection.
  	         */
    	    if(ps->RetryCount <= MAX_RETRY_COUNTS) { flags = ACKN; seq = ps->SND_SEQ; } 
				else {
					ps->RetryCount = 0;
                	flags = FIN | ACKN; seq = ps->SND_SEQ++;	
                	ps->smState = TCP_FIN_WAIT_1;
   	        	}
t_TCP_EST++;
    	break;
	
        case TCP_FIN_WAIT_1:
		case TCP_FIN_WAIT_2:
	    case TCP_LAST_ACK:
   	        /*
    	     * If we do not receive any response to out close request,
   	    	 * close it outselves.
           	 */	
            if(ps->RetryCount <= MAX_RETRY_COUNTS) { flags = FIN | ACKN; seq = ps->SND_SEQ++; } 
				else CloseSocket(ps);
t_TCP_LAST_ACK++;
   	    break;
	
        case TCP_CLOSING:
	    case TCP_TIMED_WAIT:
       	    /*
        	 * If we do not receive any response to out close request,
       	     * close it outselves.
        	 */
   	        if(ps->RetryCount <= MAX_RETRY_COUNTS) { flags = ACKN; seq = ps->SND_SEQ; } 
				else CloseSocket(ps);
t_TCP_TIMED_WAIT++;
        break;
	}
	//* wait((T_TCP_Timeout0 + socket), timeoutState[ps->smState]);
    //* !SOCKET0 ONLY!
    timer_tcp_timeout = timeoutState[ps->smState];
    tcp.b.timeout = 0;
	
    if(flags > 0x00) {
//#if(DEBUG_IO==1)
//DEBUG_timeoutSendTCP(socket, flags);
//#endif
        _SendTCP(&ps->remote,
                  ps->localPort,
                  ps->remotePort,
                  seq,
                  ps->SND_ACK,
                  flags);
    };
	return(TRUE);
}

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
TCP_SOCKET TCPConnect(IP_ADDR *remote, TCP_PORT remotePort) large
{
TCP_SOCKET xdata 	s;
SOCKET_INFO xdata* 	ps;
BOOL xdata 			lbFound;

	if(sendingActive()) return(INVALID_SOCKET);
    lbFound = FALSE;
    /*
     * Find an available socket
     */
    for(s = 0; s < MAX_SOCKETS; s++) {
        ps = &TCB[s];
        if (ps->smState == TCP_CLOSED) {
            lbFound = TRUE;
            break;
        }
    }
    /*
     * If there is no socket available, return error.
     */
    if(lbFound == FALSE) return(INVALID_SOCKET);
	memcpy((unsigned char*)&ps->remote, (const void*)remote, sizeof(ps->remote));
    /*
     * Each new socket that is opened by this node, gets
     * next sequential port number.
     */
    ps->localPort = ++_NextPort;
    if(_NextPort > LOCAL_PORT_END_NUMBER) _NextPort = LOCAL_PORT_START_NUMBER;
    /*
     * This is the port, we are trying to connect to.
     */
    ps->remotePort = remotePort;
    /*
     * Each new socket that is opened by this node, will
     * start with next segment seqeuence number.
     */
    ps->SND_SEQ = 0;
    ps->SND_ACK = 0;
	/*
     * This is a client socket.
     */
    ps->Flags.Server = FALSE; 

    /*
     * Send SYN message.
     */
    _SendTCP(&ps->remote,
             ps->localPort,
             ps->remotePort,
             ps->SND_SEQ,
             ps->SND_ACK,
             SYN);
//#if(DEBUG_IO==1)
//DEBUG_timeoutSendTCP(s, SYN);
//#endif
    ps->smState = TCP_SYN_SENT;
	ps->RetryCount = 0;
    ps->SND_SEQ++;
	// Init timeout for this socket
	//* wait((T_TCP_Timeout0 + s), timeoutState[ps->smState]);	
    //* !SOCKET0 ONLY!
    timer_tcp_timeout = timeoutState[ps->smState];
    tcp.b.timeout = 0;

    return(s);
}

/*********************************************************************
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
void TransmitTCP(IP_ADDR *remote,
                 TCP_PORT localPort,
                 TCP_PORT remotePort,
                 unsigned long 	tseq,
                 unsigned long 	tack,
                 unsigned char 	flags,
                 unsigned char* buffer,
                 unsigned int 	len) 		large
{
WORD_VAL xdata        checkSum;
TCP_OPTIONS xdata     options;
unsigned char xdata   optionsLen = 0;
unsigned int  xdata   buffer_len;
PSEUDO_HEADER xdata   pseudoHeader;

	buffer_len = len;
    tcp_context.packet.header.SourcePort           = localPort;
    tcp_context.packet.header.DestPort             = remotePort;
    tcp_context.packet.header.SeqNumber            = tseq;
    tcp_context.packet.header.AckNumber            = tack;
    tcp_context.packet.header.Flags.bits.Reserved2 = 0;
    tcp_context.packet.header.DataOffset.bits.Reserved3 = 0;
    tcp_context.packet.header.Flags.byte           = flags;
    //* Window=126 for MAX_TCP_DATA=0x0140; Window=286 for MAX_TCP_DATA=0x01E0
    tcp_context.packet.header.Window               = (MAX_TCP_DATA);
    //*------DEBUG------*
    send_c_out_max(0xE8); //"send TCP packet"
    send_c_out_max(tcp_context.packet.header.Flags.byte);
    send_c_out_max(0x00);
    //*------DEBUG------*
    /*
     * Limit one segment at a time from remote host.
     * This limit increases overall throughput as remote host does not
     * flood us with packets and later retry with significant delay.
     */
    if(tcp_context.packet.header.Window >= MAX_TCP_DATA) tcp_context.packet.header.Window = MAX_TCP_DATA;
	    else if(tcp_context.packet.header.Window > 54) { tcp_context.packet.header.Window -= 54; } 
			else tcp_context.packet.header.Window = 0;
    tcp_context.packet.header.Checksum          = 0;
    tcp_context.packet.header.UrgentPointer     = 0;
    len += sizeof(tcp_context.packet.header);

	// OPTIONS
    if(flags & SYN) {
        len += sizeof(options);
        options.Kind = TCP_OPTIONS_MAX_SEG_SIZE;
        options.Length = optionsLen = 0x04;
        options.MaxSegSize.v[0] = (MAX_TCP_DATA >> 8);   // 0x05;
        options.MaxSegSize.v[1] = (MAX_TCP_DATA & 0xff); // 0xb4;
        tcp_context.packet.header.DataOffset.bits.Val  = (sizeof(tcp_context.packet.header) + sizeof(options)) >> 2;
		memcpy((void*)&tcp_context.packet.Data[0], (void*)&options, sizeof(TCP_OPTIONS));
    } else tcp_context.packet.header.DataOffset.bits.Val = sizeof(tcp_context.packet.header) >> 2;

    // Calculate IP pseudoheader checksum.
    pseudoHeader.SourceAddress.v[0] = local_ip.v[0];
    pseudoHeader.SourceAddress.v[1] = local_ip.v[1];
    pseudoHeader.SourceAddress.v[2] = local_ip.v[2];
    pseudoHeader.SourceAddress.v[3] = local_ip.v[3];
    pseudoHeader.DestAddress    	= *remote;
    pseudoHeader.Zero           	= 0x0;
    pseudoHeader.Protocol       	= IP_PROT_TCP;
    pseudoHeader.TCPLength      	= len;
    tcp_context.packet.header.Checksum = ~CalcIPChecksum((unsigned char*)&pseudoHeader, sizeof(pseudoHeader));
    checkSum.Val = tcp_context.packet.header.Checksum;

	// Put data in TCP packet
	if(buffer_len) memcpy((void*)&tcp_context.packet.Data[optionsLen], (void*)&buffer[0], buffer_len);
    checkSum.Val = CalcTCPChecksum(len);
	tcp_context.packet.header.Checksum = checkSum.Val;
	tcp_context.total_len = len;

	sendIP(IP_PROT_TCP, remote);
}

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
BOOL TCPIsConnected(TCP_SOCKET s) large
{
    return (TCB[s].smState == TCP_EST);
}

BOOL TCPIsDisconnected(TCP_SOCKET s) large
{
    return (TCB[s].smState == TCP_CLOSED);
}

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
unsigned char TCPDisconnect(TCP_SOCKET s) large
{
SOCKET_INFO xdata *ps;

	if(sendingActive()) return(FALSE);
    ps = &TCB[s];
    /*
     * If socket is not connected, may be it is already closed
     * or in process of closing.  Since user has called this
     * explicitly, close it forcefully.
     */
    if(ps->smState != TCP_EST)
    {
        CloseSocket(ps);
        return(TRUE);
    }
    /*
     * Discard any outstand data that is to be read.
     */
    TCPDiscard(s);
    /*
     * Send FIN message.
     */
    _SendTCP(&ps->remote,
            ps->localPort,
            ps->remotePort,
            ps->SND_SEQ,
            ps->SND_ACK,
            (FIN | ACKN));
//#if(DEBUG_IO==1)
//DEBUG_timeoutSendTCP(s, (FIN | ACKN));
//#endif
    ps->SND_SEQ++;
    ps->smState = TCP_FIN_WAIT_1;

    return(TRUE);
}

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
BOOL TCPRequest(TCP_SOCKET s, unsigned char* buffer, unsigned int len) large
{
SOCKET_INFO xdata *ps;

    if(sendingActive()) return(FALSE);
    ps = &TCB[s];
	ps->TxBuffer = buffer;
	ps->TxCount  = len;
    TransmitTCP(&ps->remote,
                 ps->localPort,
                 ps->remotePort,
                 ps->SND_SEQ,
                 ps->SND_ACK,
                 ACKN | PSH,
                 &buffer[0],
                 len);
//#if(DEBUG_IO==1)
//DEBUG_timeoutSendTCP(s, ACKN);
//#endif
    ps->SND_SEQ += (unsigned long)len;

    return(TRUE);
}

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
BOOL TCPDiscard(TCP_SOCKET s) large
{
SOCKET_INFO xdata* ps;

    ps = &TCB[s];

    /*
     * This socket must contain data for it to be discarded.
     */
    if(!ps->Flags.GetReady )  return FALSE;
    ps->Flags.GetReady = FALSE;

    return(TRUE);
}

/*********************************************************************
 * Function:        BOOL rcvTCP(NODE_INFO* remote,
 *                                  WORD len)
 *
 * PreCondition:    TCPInit() is already called     AND
 *                  TCP segment is ready in MAC buffer
 *
 * Input:           remote      - Remote node info
 *                  len         - Total length of TCP semgent.
 *
 * Output:          TRUE if this function has completed its task
 *                  FALSE otherwise
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
static unsigned char xdata c = 0;
/*void DEBUG_TCP_from_server(TCP_SOCKET s, TCP_HEADER* h)
{
unsigned char xdata i;
SOCKET_INFO xdata*	ps;

	putGPS('<'); putGPS('-');
    ps = &TCB[s];			// берем текущий сокет
		putGPS('[');
		charToHex2(s);
		putGPS(hex2[1]); putGPS(hex2[0]);
		putGPS(']');
	for(i=0;i<8;i++) putGPS(sm_state[ps->smState][i]);
	putGPS(':');
	putGPS('{');
	if(h->Flags.bits.flagRST) { for(i=0;i<3;i++) putGPS(f[0][i]); putGPS(' '); }
	if(h->Flags.bits.flagACK) { for(i=0;i<3;i++) putGPS(f[1][i]); putGPS(' '); }
	if(h->Flags.bits.flagFIN) { for(i=0;i<3;i++) putGPS(f[2][i]); putGPS(' '); }
	if(h->Flags.bits.flagSYN) { for(i=0;i<3;i++) putGPS(f[3][i]); putGPS(' '); }
	if(h->Flags.bits.flagPSH) { for(i=0;i<3;i++) putGPS(f[4][i]); putGPS(' '); }
	if(h->Flags.bits.flagURG) { for(i=0;i<3;i++) putGPS(f[5][i]); putGPS(' '); }
	putGPS('S'); putGPS('=');
		charToHex2((unsigned char)((h->SeqNumber >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->SeqNumber >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->SeqNumber >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->SeqNumber)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS(',');
	putGPS('A'); putGPS('=');
		charToHex2((unsigned char)((h->AckNumber >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->AckNumber >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->AckNumber >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((h->AckNumber)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS('}'); 
}
*/
BOOL rcvTCP() large
{
TCP_HEADER    xdata   TCPHeader;
TCP_SOCKET    xdata   socket;
unsigned int  xdata   optionsSize;
unsigned int  xdata   data_offset;

    // Retrieve TCP header.
	memcpy((unsigned char*)&TCPHeader, &context_InPacketPPP[pcip].buffer[in_context.i], SIZE_TCP_HEADER);
	//
    // Skip over options and retrieve all data bytes.
	data_offset = ((unsigned int)(TCPHeader.DataOffset.bits.Val) << 2);
    if(data_offset >= SIZE_TCP_HEADER) optionsSize = abs(data_offset - SIZE_TCP_HEADER);
    in_context.len = abs(in_context.len - data_offset);
	//
    // Position packet read pointer to start of data area.
	in_context.i += data_offset;
    //
    // Find matching socket.
    socket = FindMatchingSocket(&TCPHeader, &in_context.remote);
    if(socket < INVALID_SOCKET) {
//#if(DEBUG_IO==1)
//DEBUG_TCP_from_server(socket, &TCPHeader);
//#endif
    //*------DEBUG------*
    send_c_out_max(0xE9); //"received TCP packet"
    send_c_out_max(TCPHeader.Flags.byte);
    send_c_out_max(0x00);
    //*------DEBUG------*
        HandleTCPSeg(socket, &in_context.remote, &TCPHeader, in_context.len);

    reset_wdt;

		if(socket == HTTP_socket)
            parse_incoming_cmd(&context_InPacketPPP[pcip].buffer[in_context.i], in_context.len);

	reset_wdt;

    }
    else
    {
        /*
         * If this is an unknown socket, discard it and
         * send RESET to remote node.
         */
    }

    return(TRUE);
}

/*********************************************************************
 * Function:        static WORD CalcTCPChecksum(WORD len)
 *
 * PreCondition:    TCPInit() is already called     AND
 *                  MAC buffer pointer set to starting of buffer
 *
 * Input:           len     - Total number of bytes to calculate
 *                          checksum for.
 *
 * Output:          16-bit checksum as defined by rfc 793.
 *
 * Side Effects:    None
 *
 * Overview:        This function performs checksum calculation in
 *                  MAC buffer itself.
 *
 * Note:            None
 ********************************************************************/
unsigned int CalcTCPChecksum(unsigned int len) large
{
BOOL xdata 			lbMSB;
WORD_VAL xdata 		checkSum;
unsigned char xdata Checkbyte;
unsigned int  xdata l;

	l = len;
    lbMSB = TRUE;
    checkSum.Val = 0;

    while(len)
    {
        reset_wdt;

        Checkbyte = ((unsigned char xdata*)(&tcp_context.packet))[l - len];

        if(lbMSB) {
            if((checkSum.v[0] = Checkbyte+checkSum.v[0]) < Checkbyte) {
                if(++checkSum.v[1] == 0) checkSum.v[0]++;
            }
        } else {
            if((checkSum.v[1] = Checkbyte+checkSum.v[1]) < Checkbyte) {
                if(++checkSum.v[0] == 0) checkSum.v[1]++;
            }
        };
        lbMSB = !lbMSB;
		len--;
    }

	checkSum.v[0] = ~checkSum.v[0];
    checkSum.v[1] = ~checkSum.v[1];
    
    return checkSum.Val;
}

/*********************************************************************
 * Function:        static TCP_SOCKET FindMatchingSocket(TCP_HEADER *h,
 *                                      NODE_INFO* remote)
 *
 * PreCondition:    TCPInit() is already called
 *
 * Input:           h           - TCP Header to be matched against.
 *                  remote      - Node who sent this header.
 *
 * Output:          A socket that matches with given header and remote
 *                  node is searched.
 *                  If such socket is found, its index is returned
 *                  else INVALID_SOCKET is returned.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
static TCP_SOCKET FindMatchingSocket(TCP_HEADER *h, IP_ADDR *remote) large
{
SOCKET_INFO xdata*	ps;
TCP_SOCKET  xdata 	s;
TCP_SOCKET  xdata 	partialMatch;

    partialMatch = INVALID_SOCKET;
    for(s = 0;s < MAX_SOCKETS;s++) {
        ps = &TCB[s];
        if(ps->smState != TCP_CLOSED) {
            if(ps->localPort == h->DestPort) {
                if(ps->smState == TCP_LISTEN) partialMatch = s;
                if((ps->remotePort == h->SourcePort) && (ps->remote.Val == remote->Val)) return(s);
            }
        }
    };
    ps = &TCB[partialMatch];
    if(partialMatch != INVALID_SOCKET && ps->smState == TCP_LISTEN) {
        memcpy((void*)&ps->remote, (void*)remote, sizeof(*remote));
        ps->localPort       = h->DestPort;
        ps->remotePort      = h->SourcePort;
        ps->Flags.GetReady  = FALSE;
        ps->TxBuffer        = INVALID_BUFFER;
        return partialMatch;
    }
    if(partialMatch == INVALID_SOCKET) return UNKNOWN_SOCKET;
    	else return INVALID_SOCKET;
}

/*********************************************************************
 * Function:        static void CloseSocket(SOCKET_INFO* ps)
 *
 * PreCondition:    TCPInit() is already called
 *
 * Input:           ps  - Pointer to a socket info that is to be
 *                          closed.
 *
 * Output:          Given socket information is reset and any
 *                  buffer held by this socket is discarded.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void CloseSocket(SOCKET_INFO* ps) large
{
    if(ps->TxBuffer != INVALID_BUFFER) {
        ps->TxBuffer = INVALID_BUFFER;
    }
    ps->remote.Val = 0x00;
    ps->remotePort = 0x00;
    if(ps->Flags.GetReady) {

    };
    ps->Flags.GetReady       = FALSE;
    if(ps->Flags.Server) ps->smState = TCP_LISTEN;
    	else ps->smState = TCP_CLOSED;

    return;
}

/*********************************************************************
 * Function:        static void HandleTCPSeg(TCP_SOCKET s,
 *                                      NODE_INFO *remote,
 *                                      TCP_HEADER* h,
 *                                      WORD len)
 *
 *
 * Input:           s           - Socket that owns this segment
 *                  remote      - Remote node info
 *                  h           - TCP Header
 *                  len         - Total buffer length.
 *
 * Output:          TCP FSM is executed on given socket with
 *                  given TCP segment.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
typedef enum _TCP_ERROR
{
    TCP_NO_ERROR = 0,
    TCP_ERROR_00,
    TCP_ERROR_01,
    TCP_ERROR_02,
    TCP_ERROR_03,
    TCP_ERROR_04,
    TCP_ERROR_05,
    TCP_ERROR_06,
    TCP_ERROR_07,
    TCP_ERROR_08,
    TCP_ERROR_09
} TCP_ERROR;
unsigned char xdata errorTCP;

/*
void DEBUG_TCP_from_modem(TCP_SOCKET s, unsigned char flags, unsigned char RetryCount)
{
unsigned char xdata i;
SOCKET_INFO xdata*	ps;

    ps = &TCB[s];			// берем текущий сокет
		putGPS('[');
		charToHex2(s);
		putGPS(hex2[1]); putGPS(hex2[0]);
		putGPS(']');
	putGPS('-'); putGPS('>');
	for(i=0;i<8;i++) putGPS(sm_state[ps->smState][i]);
	putGPS(':');
	putGPS('{');
		if(flags & RST) { for(i=0;i<3;i++) putGPS(f[0][i]); putGPS(' '); }
		if(flags & ACKN){ for(i=0;i<3;i++) putGPS(f[1][i]); putGPS(' '); }
		if(flags & FIN) { for(i=0;i<3;i++) putGPS(f[2][i]); putGPS(' '); }
		if(flags & SYN) { for(i=0;i<3;i++) putGPS(f[3][i]); putGPS(' '); }
		if(flags & PSH) { for(i=0;i<3;i++) putGPS(f[4][i]); putGPS(' '); }
		if(flags & URG) { for(i=0;i<3;i++) putGPS(f[5][i]); putGPS(' '); }
	putGPS('S'); putGPS('=');
		charToHex2((unsigned char)((ps->SND_SEQ >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_SEQ)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS(',');
	putGPS('A'); putGPS('=');
		charToHex2((unsigned char)((ps->SND_ACK >> 24) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK >> 16) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK >>  8) & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
		charToHex2((unsigned char)((ps->SND_ACK)       & 0x000000ff));
		putGPS(hex2[1]); putGPS(hex2[0]);
	putGPS(',');
		charToHex2(RetryCount);
		putGPS(hex2[0]);
	putGPS('}');
	putGPS(0x0D); putGPS(0x0A);
	c = 0;
}
*/
static void HandleTCPSeg(TCP_SOCKET 	s,
                         IP_ADDR*		remote,
                         TCP_HEADER*	h,
                         unsigned int	len) large
{
unsigned long xdata 	ack;
unsigned long xdata 	seq;
SOCKET_INFO   xdata*	ps;
unsigned char xdata 	flags;
unsigned long xdata 	prevAck, prevSeq;

	// ini error state
	errorTCP = TCP_NO_ERROR;
	//
    flags = 0x00;
    ps = &TCB[s];			// берем текущий сокет и запоминаем 
    prevAck = ps->SND_ACK;	// current seq and ack for our connection so that if
    prevSeq = ps->SND_SEQ;	// we have to silently discard this packet, we can go back to
							// previous ack and seq numbers.
    ack = h->SeqNumber;		
    ack += (unsigned long)len;// ack делаем равным сумме SeqNumber из принятого пакета и длине данных из него 
    seq = ps->SND_SEQ;		// seq берем из текущего сокета

	// смотрим в каком состоянии находится текущий сокет
    if(ps->smState == TCP_LISTEN) {	// если сокет используется как сервер
		/*
        //MACDiscardRx();
        ps->SND_SEQ     = ++ISS;
        ps->SND_ACK     = ++ack;
        seq             = ps->SND_SEQ;
        ++ps->SND_SEQ;
        if(h->Flags.bits.flagSYN) {
            // This socket has received connection request.
            // Remember calling node, assign next segment seq. number
            // for this potential connection.
            memcpy((void*)&ps->remote, (const void*)remote, sizeof(*remote));
            ps->remotePort  = h->SourcePort;
            // Grant connection request.
            flags           = SYN | ACKN;
            ps->smState     = TCP_SYN_RCVD;
        } else {
            // Anything other than connection request is ignored in
            // LISTEN state.
            flags               = RST;
            seq                 = ack;
            ack                 = h->SeqNumber;
            ps->remote.Val 		= 0x00;
        }
		*/
    } else {//if(ps->smState == TCP_LISTEN) 
	    if(h->Flags.bits.flagRST) {	// Reset FSM, if RST is received.
            CloseSocket(ps);
            return;
		// УСТАНОВКА СОЕДИНЕНИЯ СО СТОРОНЫ СЕРВЕРА	
        } else if(seq == h->AckNumber) {						// если SND_SEQ из сокета равен принятому AckNumber
        	if(ps->smState == TCP_SYN_RCVD) {					// из текущего принятого пакета
            	if(h->Flags.bits.flagACK) {
                    ps->SND_ACK = ack;
                    ps->RetryCount = 0;
                    ps->smState = TCP_EST;						// CONNECTION ESTABLISHED !!!
                    if(len > 0) {
                        ps->Flags.GetReady  = TRUE;
                        ps->RxCount         = len;
                        ps->Flags.FirstRead = TRUE;
                    } else errorTCP = TCP_ERROR_00;
                } else {
                    errorTCP = TCP_ERROR_01;
                }
			// УСТАНОВКА СОЕДИНЕНИЯ СО СТОРОНЫ СОКЕТА
            } else if(ps->smState == TCP_SYN_SENT) {			// TCP_SYN_SENT
                if(h->Flags.bits.flagSYN) {
                    ps->SND_ACK = ++ack;						// SND_ACK текущего сокета присваивается полученное от сервера (ack+1)
                    if(h->Flags.bits.flagACK) {					// если со стороны сервера пришло SYN | ACK, то соединение
                        flags = ACKN;							// считаем успешно завершившимся
						ps->RetryCount = 0;
                        ps->smState = TCP_EST;					// CONNECTION ESTABLISHED !!!
                    } else {
                        flags = SYN | ACKN;						// если со сторны сервера пришло только SYN , то соединение
                        ps->smState = TCP_SYN_RCVD;				// берет на себя сервер (set TCP_SYN_RCVD), сокет передает
                        ps->SND_SEQ = ++seq;					// со своей стороны SYN | ACK
                    };
                    if(len > 0) {								// если длина принятого пакета не равна 0
                        ps->Flags.GetReady  = TRUE;
                        ps->RxCount         = len;
                        ps->Flags.FirstRead = TRUE;
                    } else errorTCP = TCP_ERROR_02;
                } else {										// не пришло SYN от сервера -> значит он находится в состоянии EST 
                	flags = RST;								// передать серевру команду на ABORT его состояния 
					//ps->smState = TCP_SYN_SENT;
					CloseSocket(ps);
                }
            } else {
				// СОЕДИНЕНИЕ УСТАНОВЛЕНО И ОТ СЕРВЕРА ИДУТ ПАКЕТЫ ПО ЗАПРОСУ 
                if(h->SeqNumber != ps->SND_ACK) {	// если SeqNumber принятого пакета не равен ожидаемому сокетом SND_ACK,
                    // Discard buffer.				// то не отбросывать даный пакет, возможно: 
                    //								// потеря пакета ACK, переданного со стороны сокета, подтверждающего 
	                if(ps->smState == TCP_EST) {	// ранее принятый пакет. Необходимо передать пакет ACK с ранее 
						errorTCP = TCP_ERROR_04;	// сохраненным SND_ACK.  
						// если пришел тот же самый пакет, что и предыдущий
						// то, подтверждаем его		
//#if(DEBUG_DATA==1)	
//sendBufferGPS(&tcpPREV.str[0], tcpPREV.len);
//#endif
        	        };
				    return;
				};
				//
				ps->SND_ACK = ack;					
                if(ps->smState == TCP_EST) {		
                    if(h->Flags.bits.flagACK) {		// если SeqNumber принятого пакета равен ожидаемому сокетом SND_ACK,
						// принят пакет				// то следующее ожидаемое SND_ACK увеличить на длину принятого пакета
                    };								// в этом случае, если теперь переданный пакет (flags=ACK) не дойдет 
					if(h->Flags.bits.flagPSH) {		// до сервера, то сервер начнет повторять предыдущий пакет, но
						//*****if(s == SMTP_socket) {		// ожидаемый сокетом SND_ACK не будет совпадать с принятым !!!
	                    	//*****sendSignal(S_SMTP_EndString, TASK_SMTP);
						//*****}
                    };
                    if(h->Flags.bits.flagFIN) {
                        flags = (FIN | ACKN);
                        seq = ps->SND_SEQ++;
                        ack = ++ps->SND_ACK;
                        ps->smState = TCP_LAST_ACK;
                    };
                    if(len > 0) {
//#if(DEBUG_DATA==1)
//sendBufferGPS(&context_InPacketPPP[pcip].buffer[in_context.i], len);
//#endif
                    	if(!ps->Flags.GetReady) {// флаг готовности приема на стороне сокета
                        	//ps->Flags.GetReady = TRUE;
                            ps->RxCount         = len;
                            ps->Flags.FirstRead = TRUE;
                            if(h->Flags.bits.flagACK && 
                               h->Flags.bits.flagPSH) {
	                            http.b.ackflag = 1;
                            }
                            // 4/1/02
                            flags |= ACKN;
                       	} else {
                            flags = 0x00;			// Since we cannot accept this packet, restore to previous seq and ack.
                            ps->SND_SEQ = prevSeq;	// and do not send anything back. Host has to resend this packet 
                            ps->SND_ACK = prevAck;	// when we are ready.
							errorTCP = TCP_ERROR_05;
                        }
                    } else {
						errorTCP = TCP_ERROR_06;
                    }
                } else if(ps->smState == TCP_LAST_ACK) {
                    if(h->Flags.bits.flagACK) {
                        CloseSocket(ps);
                    }
                } else if(ps->smState == TCP_FIN_WAIT_1) {
					if(h->Flags.bits.flagACK) {
						ps->smState = TCP_FIN_WAIT_2;							
                    }
				    if(h->Flags.bits.flagFIN) {
                        flags = ACKN;
                        ack = ++ps->SND_ACK;
                        if(h->Flags.bits.flagACK) {
                            CloseSocket(ps);
                        } else {
                            ps->smState = TCP_CLOSING;
                        }
                    }
				} else if(ps->smState == TCP_FIN_WAIT_2) {
					if(h->Flags.bits.flagACK) {
                        CloseSocket(ps);
                    }
					if(h->Flags.bits.flagFIN) {
    	            	flags = ACKN;
        	            ack = ++ps->SND_ACK;
            	       	CloseSocket(ps);
					}
    	        } else if(ps->smState == TCP_CLOSING) {
                    if(h->Flags.bits.flagACK) {
                        CloseSocket(ps);
                    }
                }
            }
        } else {
			if(ps->smState == TCP_EST) {		
	            if(h->Flags.bits.flagACK) {		// если SeqNumber принятого пакета равен ожидаемому сокетом SND_ACK,
						// принят пакет			// то следующее ожидаемое SND_ACK увеличить на длину принятого пакета
                };								// в этом случае, если теперь переданный пакет (flags=ACK) не дойдет 
				if(h->Flags.bits.flagPSH) {		// до сервера, то сервер начнет повторять предыдущий пакет, но
												// ожидаемый сокетом SND_ACK не будет совпадать с принятым !!!
	                    
                };
                if(h->Flags.bits.flagFIN) {
    	            flags = (FIN | ACKN);
                    seq = ps->SND_SEQ++;
                    ack = ++ps->SND_ACK;
                    ps->smState = TCP_LAST_ACK;
                };
                if(len > 0) {
//#if(DEBUG_DATA==1)
//sendBufferGPS(&context_InPacketPPP[pcip].buffer[in_context.i], len);
//#endif
			       	if(!ps->Flags.GetReady) {	// флаг готовности приема на стороне сокета
    		           	//ps->Flags.GetReady = TRUE;
        	            ps->RxCount         = len;
            	        ps->Flags.FirstRead = TRUE;
                        if(h->Flags.bits.flagACK && 
                           h->Flags.bits.flagPSH) {
	                        http.b.ackflag = 1;
                        }
 	                    // 4/1/02
    	                flags |= ACKN;
	               	} else {
		                flags = 0x00;			// Since we cannot accept this packet, restore to previous seq and ack.
        	            ps->SND_SEQ = prevSeq;	// and do not send anything back. Host has to resend this packet 
            	        ps->SND_ACK = prevAck;	// when we are ready.
						errorTCP = TCP_ERROR_05;
	                }
            	} else {
					errorTCP = TCP_ERROR_06;
                }
            } else if(ps->smState == TCP_LAST_ACK) {
	            if(h->Flags.bits.flagACK) {
    	            CloseSocket(ps);
                }
            } else if(ps->smState == TCP_FIN_WAIT_1) {
				if(h->Flags.bits.flagACK) {
					ps->smState = TCP_FIN_WAIT_2;							
                }
			    if(h->Flags.bits.flagFIN) {
             		flags = ACKN;
                   	ack = ++ps->SND_ACK;
                   	if(h->Flags.bits.flagACK) {
                   		CloseSocket(ps);
                    } else {
                    	ps->smState = TCP_CLOSING;
                    }
                }
			} else if(ps->smState == TCP_FIN_WAIT_2) {
				if(h->Flags.bits.flagACK) {
                	CloseSocket(ps);
                }
				if(h->Flags.bits.flagFIN) {
                	flags = ACKN;
                    ack = ++ps->SND_ACK;
    	            CloseSocket(ps);
				}
    	    } else if(ps->smState == TCP_CLOSING) {
	            if(h->Flags.bits.flagACK) {
    	            CloseSocket(ps);
                }
            } else {
	           	flags = RST;								// передать серверу команду на ABORT его состояния 
	            CloseSocket(ps);
    	        if(ps->smState == TCP_EST) {
					errorTCP = TCP_ERROR_07;
				}
        	}
		}
    }
    /*
     * Clear retry counts and timeout tick counter.
     */
	if(errorTCP != TCP_ERROR_04) {	// если пришел не ожидаемый нами пакет, 
    	ps->RetryCount = 0;			// то он не должен нам сбить установки 
    	//* wait((T_TCP_Timeout0 + s), timeoutState[ps->smState]);
        //* !SOCKET0 ONLY!
        timer_tcp_timeout = timeoutState[ps->smState];
        tcp.b.timeout = 0;
	}
	//
	ps->prevSND_ACK = prevAck;
    if(flags > 0x00) {
//#if(DEBUG_IO==1)
//DEBUG_TCP_from_modem(s, flags, ps->RetryCount);			
//#endif
	   	_SendTCP(remote, 
				 h->DestPort, 
				 h->SourcePort, 
				 seq, 
				 ack, 
				 flags);
    }
}

