#include "setting.h"
#include "gprs.h"



unsigned char code PPP_LOGIN[16] _at_ (F_ADDR_SETTINGS+174);
unsigned char code PPP_PASSW[16] _at_ (F_ADDR_SETTINGS+190);

IP_ADDR code 	remoteIP   _at_ (F_ADDR_SETTINGS+206);
HTTP_PORT code remotePORT  _at_ (F_ADDR_SETTINGS+218);

//IP_ADDR code 	remoteIP2  _at_ (F_ADDR_SETTINGS+210);
//IP_ADDR code 	remoteIP   		= { 213,  59,  64, 229 }; 
								// www.ntcnvg.ru  -> { 213,  59,  64, 228 };
//IP_ADDR code 	remoteIP_MAIL	= { 194,  67,  23, 111 };
								// E-Mail ntcnvg.ru    	-> { 213,  59,  64, 225 };
								// E-Mail smtp.mail.ru 	-> { 194,  67,  23, 111 };
								// www.yandex.ru 		-> { 213, 180, 216, 200 };
								// www.google.ru 		-> {  66, 102,  11, 104 }; 
								// www.ntcnvg.ru 		-> { 213,  59,  64, 228 }; 
								// www.ntcnvg.ru 2		-> { 213,  59,  64, 229 }; 
/*
signed int code timeoutState[] = { 
    (RTT + _2_sec_01),	// TCP_CLOSED,
    (RTT + _4_sec_01),	// TCP_LISTEN,
    (RTT + _6_sec_01),  // TCP_SYN_RCVD,
    (RTT + _6_sec_01),  // TCP_SYN_SENT,		
    (RTT + _6_sec_01),  // TCP_EST,			
    (RTT + _2_sec_01),	// TCP_FIN_WAIT_1,		
	(RTT + _2_sec_01),	// TCP_FIN_WAIT_2,
    (RTT + _2_sec_01),	// TCP_CLOSING,
    (RTT + _2_sec_01),	// TCP_TIMED_WAIT,
    (RTT + _4_sec_01),	// TCP_SEND_RST,
    (RTT + _4_sec_01),	// TCP_DATA_READY,
    (RTT + _2_sec_01),	// TCP_LAST_ACK,		
    (RTT + _2_sec_01),	// TCP_CLOSE,
    (RTT + _2_sec_01),	// TCP_INVALID
};
*/