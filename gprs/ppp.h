#ifndef _PPP_H
#define _PPP_H


#include "setting.h"


#define PPP_PROT_IP	    	(1)
//
// Context of the PPP PACKET
//
#define	REQ			1 	// Request options list for PPP negotiations
#define	ACK			2	// Acknowledge options list for PPP negotiations
#define	NAK			3	// Not acknowledged options list for PPP negotiations
#define	REJ			4	// Reject options list for PPP negotiations
#define	TERM_REQ	5	// Termination packet for LCP to close connection
#define	TERM_ACK	6	// Termination packet for LCP to close connection

typedef enum  {
		UNCERTAIN_PACKET = 0,
		LCP,
		PAP,
		IPCP,
		IP_PACKET_PPP,
		CCP
} PROTOCOL_PPP_PACKET;

typedef struct _context_InPacketPPP {

	PROTOCOL_PPP_PACKET protocol;
	unsigned char 		buffer[MAX_SIZE_BUFFER_PPP];

};

// GSM MODE
typedef enum {
		WAIT_REQUEST_FROM_ISP = 0,
		REPLY_FROM_MODEM,
		REQUEST_FROM_MODEM,
		WAIT_REPLY_FROM_ISP

} _PPP_MODE;
extern _PPP_MODE xdata PPP_MODE;

// PARSE GPRS
typedef enum  {
	startFlagPPP = 0,
    addressPPP,
	controlPPP,
	packetPPP,
	dataPacketPPP,
	endPacketPPP	

} _sParsePPP;
extern _sParsePPP xdata sParsePPP;


//*--------------------*
#define LEN_ENUM_sPPP 8
typedef enum  {
		estConnectionLCP = 0,
		authModemPAP,
		configIPCP,
		getIP_IPCP,
		readyPPP, //4			
		closingIPCP,
		closingLCP,
		termPPP					
	} _sPPP;
extern _sPPP xdata sPPP;
//*--------------------*

//
extern unsigned char xdata					acip;	// active context_inpacket_ppp
extern unsigned char xdata					pcip;	// passive context_inpacket_ppp
extern struct _context_InPacketPPP xdata 	context_InPacketPPP[];  
extern unsigned char xdata 					txBufferPPP[MAX_SIZE_BUFFER_PPP];

extern unsigned char xdata					cRetryPPP;
extern unsigned int  xdata 					checksum;
extern unsigned char xdata 					idPPP;
extern unsigned int  xdata 					rxIndex;
extern unsigned int  xdata 					txIndex;
extern unsigned int  xdata 					txEnd;

extern bit 									rxESC;
extern bit 									txESC;


void 			initPPP(bit );
void 			nextStatePPP(_sPPP state);
void 			closePPP(void );
unsigned char 	readyStackPPP(void );
void 			rcvPacketPPP(void ) large;
void 			sendPacketPPP(void ) large;
void			parsePPP(void ) large;
void 			funcPPP(void );

void 			PPP_Header(PROTOCOL_PPP_PACKET , unsigned char _code) large;
void 			PPP_End(void ) large;
unsigned char 	testOption(unsigned char option);
unsigned int 	calc(unsigned int c);
void 			add(unsigned char c);


#endif
