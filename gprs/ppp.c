//  --------------------------------------------------------------------------------
// | ------------------------------------------------------------------------------ |
// ||                                                                              ||
// ||     G P R S    C O M M U N I C A T I O N    L E V E L       (REMASTERED)     ||
// ||                                                                              ||
// | ------------------------------------------------------------------------------ |
//  --------------------------------------------------------------------------------

#include "c8051f120.h"
#include <stdio.h>
#include <stdlib.h>                                                   
#include <string.h>

#include "..\main.h"
#include "..\uart.h"
#include "gprs.h"

#include "setting.h"
#include "ppp.h"
#include "ip.h"
#include "tcp.h"
#include "http.h"
//#include "smtp.h"



#define  put_char(x)  tx_data_uart0=x;send_char_uart0()


unsigned char code PPP_LOGIN[16] _at_ (F_ADDR_SETTINGS+174);
unsigned char code PPP_PASSW[16] _at_ (F_ADDR_SETTINGS+190);


_PPP_MODE xdata 					PPP_MODE;
_sPPP xdata 						sPPP = termPPP;
unsigned char xdata					cRetryPPP = 0;
_sParsePPP xdata 					sParsePPP = startFlagPPP;

unsigned char xdata					acip = 0;	// active  context_inpacket_ppp
unsigned char xdata					pcip;		// passive context_inpacket_ppp
struct _context_InPacketPPP xdata 	context_InPacketPPP[2]; 
unsigned char xdata 				txBufferPPP[MAX_SIZE_BUFFER_PPP];
unsigned int  xdata 				checksum;
unsigned char xdata 				idPPP;

unsigned int  xdata 				rxIndex;
unsigned int  xdata 				txIndex, txEnd;

bit 								rxESC;
bit 								txESC;


//кол. повторений пакетов к ISP
unsigned char code Lim_cR_PPP[LEN_ENUM_sPPP] = {3,20,3,3,3,3,3,3};


void initPPP(bit src)
{
	cRetryPPP	= 0;
	sParsePPP 	= startFlagPPP;
	rxESC 		= FALSE;		
	txESC   	= FALSE;
    if (!src)
    {
        local_ip.Val = 0L;
    	PPP_MODE	 = WAIT_REQUEST_FROM_ISP;
	    nextStatePPP(estConnectionLCP);
    	idPPP 	     = 0x01;
    }
    ppp.b.no_resp = 0;
    ppp.b.out_ppp = 0;
    ppp.b.end_rcv = 0;
    ppp.b.end_snd = 0;
    ppp.b.connect = 0;
}

void resetGSMRx() {sParsePPP = startFlagPPP;}


void parsePPP() large
{
static unsigned char xdata c;

	if(in_data == 0x7D) {
		rxESC = TRUE;		
	} 
	else 
	{
		if(rxESC) { in_data ^= 0x20; }; 
		switch(sParsePPP) {
			case startFlagPPP:
				rxESC = FALSE;
				rxIndex = 0;
				context_InPacketPPP[acip].protocol = UNCERTAIN_PACKET;
        		if(in_data == 0x7E) { 
					context_InPacketPPP[acip].buffer[rxIndex] = in_data;
					rxIndex++;
					sParsePPP = addressPPP;
				} else {
					resetGSMRx(); 
				}
			break;

	        case addressPPP:
    	      	if(in_data == 0xFF) {
					context_InPacketPPP[acip].buffer[rxIndex] = in_data;
					rxIndex++;
					sParsePPP = controlPPP;
				} else 
				if(in_data == 0x7E) {
					sParsePPP = addressPPP;
				} else {
					resetGSMRx(); 
				}
			break;

			case controlPPP:
    	      	if(in_data == 0x03) { 
					context_InPacketPPP[acip].buffer[rxIndex] = in_data;
					rxIndex++;
					c = 2;
					sParsePPP = packetPPP;
				} else {
					resetGSMRx(); 	
				}
			break;

			case packetPPP:
          		context_InPacketPPP[acip].buffer[rxIndex] = in_data;
				rxIndex++;
				c--; 
				if(c == 0) {
					if((context_InPacketPPP[acip].buffer[rxIndex-2] == 0xC0) && 
					   (context_InPacketPPP[acip].buffer[rxIndex-1] == 0x21)) 
						context_InPacketPPP[acip].protocol = LCP;
	   	      		if((context_InPacketPPP[acip].buffer[rxIndex-2] == 0xC0) && 
					   (context_InPacketPPP[acip].buffer[rxIndex-1] == 0x23)) 
						context_InPacketPPP[acip].protocol = PAP;
    	   	  		if((context_InPacketPPP[acip].buffer[rxIndex-2] == 0x80) && 
					   (context_InPacketPPP[acip].buffer[rxIndex-1] == 0x21)) 
						context_InPacketPPP[acip].protocol = IPCP;
       				if((context_InPacketPPP[acip].buffer[rxIndex-2] == 0x00) && 
					   (context_InPacketPPP[acip].buffer[rxIndex-1] == 0x21)) 
						context_InPacketPPP[acip].protocol = IP_PACKET_PPP;
       				if((context_InPacketPPP[acip].buffer[rxIndex-2] == 0x80) && 
					   (context_InPacketPPP[acip].buffer[rxIndex-1] == 0xFD)) 
						context_InPacketPPP[acip].protocol = CCP;
					if(context_InPacketPPP[acip].protocol > UNCERTAIN_PACKET) sParsePPP = dataPacketPPP; 
						else {
							resetGSMRx(); 
						}
				}
			break;

			case dataPacketPPP:
				if(((in_data == 0x7E) && (rxESC == FALSE)) || (rxIndex >= MAX_SIZE_BUFFER_PPP-1)) {
					context_InPacketPPP[acip].buffer[rxIndex] = in_data;
					rxIndex++;
					resetGSMRx();
					pcip  = acip;	// switch context_inpacket_ppp
					acip ^= 0x01;
					cRetryPPP = 0;
					//* cancelWait(T_PPP_NoResponseFromISP);
                    timer_no_resp_isp = ppp.b.no_resp = 0; //(таймаут отменен)
					//* sendSignal(S_PPP_EndReceivingPacket, TASK_GSM_SEND);
                    ppp.b.end_rcv = 1;
					break;
				} 
				context_InPacketPPP[acip].buffer[rxIndex] = in_data;
				rxIndex++;
			break;

			case endPacketPPP:
			break;
 
			default:
				resetGSMRx(); 
			break;
		}
		if(rxESC) { rxESC = FALSE; };
	}
}

void rcvPacketPPP() large
{
unsigned char xdata type;

    //*------DEBUG------*
    //send_c_out_max(0xEE); //"INCOMING PACKET"
    //send_c_out_max(context_InPacketPPP[pcip].protocol);
    //send_c_out_max(context_InPacketPPP[pcip].buffer[5]);
    //send_c_out_max(0x00);
    //*------DEBUG------*

	switch(context_InPacketPPP[pcip].protocol) {
		case LCP:															// PROTOCOL
			if(context_InPacketPPP[pcip].buffer[5] == TERM_REQ) {			// if CODE = TERM, then to end PPP
                //*------DEBUG------*
                send_c_out_max(0xAE); //"TERM REQ LCP from ISP"
                send_c_out_max(0x00);
                //*------DEBUG------*
				PPP_Header(LCP, TERM_ACK);
					add(idPPP);												// id 
					add(0x00);
					add(0x04);												// length 
				PPP_End();		
				nextStatePPP(termPPP);
				//* sendSignal(S_PPP_TerminatePPP, TASK_GSM_SEND);
                ppp.b.out_ppp = 1;
				PPP_MODE = REQUEST_FROM_MODEM;
				break;
			};
			switch(sPPP) {
				case estConnectionLCP:
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case REQ:
							type = testOption(0x03);
							if(type == 0x03)  {
                            //*------DEBUG------*
                            send_c_out_max(0xA3); //"TEST OPTION = 0x03"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							PPP_Header(LCP, ACK); 
								add(context_InPacketPPP[pcip].buffer[6]);	// id
								add(0x00);
								add(0x08);									// length
								add(type);									// OPTION - type
								add(0x04);
								add(0xC0);
								add(0x23);
                                ppp.b.end_snd = 1;
							} else {
                            //*------DEBUG------*
                            send_c_out_max(0xA0); //"TEST OPTION != 0x03"
                            send_c_out_max(type);
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							PPP_Header(LCP, REJ);
								add(context_InPacketPPP[pcip].buffer[6]);	// id
								add(0x00);
								add(0x06);									// length
								add(type);									// type
								add(0x02);
							}
							PPP_End();
							//* wait(T_PPP_NoResponseFromISP, _3_sec);
                            timer_no_resp_isp = 30;
                            ppp.b.no_resp = 0;
							if(type == 0x03) {
								PPP_MODE = REQUEST_FROM_MODEM;
							}
						break;
						case ACK:
                            //*------DEBUG------*
                            send_c_out_max(0xAA); //"ACK to LSP from ISP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							nextStatePPP(authModemPAP);
							PPP_MODE = REQUEST_FROM_MODEM;
						break;
					}
				break;
				
				case closingLCP:
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case TERM_ACK:
                            //*------DEBUG------*
                            send_c_out_max(0xAC); //"CLOSING LCP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							nextStatePPP(termPPP); 
							//* sendSignal(S_PPP_TerminatePPP, TASK_GSM_SEND);
                            ppp.b.out_ppp = 1;
							PPP_MODE = REQUEST_FROM_MODEM;
						break;
					}
				break;
			}
		break; // case LCP:	

		case PAP:
			switch(sPPP) {
				case authModemPAP:
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case ACK:
                            //*------DEBUG------*
                            send_c_out_max(0xA5); //"ACK to PAP from ISP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							nextStatePPP(configIPCP);
							PPP_MODE = WAIT_REQUEST_FROM_ISP;
						break;
					}
				break;
			}
		break;

		case IPCP:				
			switch(sPPP) {
				case configIPCP:									
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case REQ:
							type = testOption(0x03);
                            //*------DEBUG------*
                            send_c_out_max(0x81); //"IPCP - REQ from ISP"
                            send_c_out_max(type);
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							PPP_Header(IPCP, ACK);
							    add(context_InPacketPPP[pcip].buffer[6]);	// id
								add(0x00);
								add(0x0A);									// length
								add(type);									// type
								add(0x06);
								add(context_InPacketPPP[pcip].buffer[11]);
								add(context_InPacketPPP[pcip].buffer[12]);
								add(context_InPacketPPP[pcip].buffer[13]);
							    add(context_InPacketPPP[pcip].buffer[14]);
							PPP_End();
							//* wait(T_PPP_NoResponseFromISP, _3_sec);
                            timer_no_resp_isp = 30;
                            ppp.b.no_resp = 0;
							nextStatePPP(getIP_IPCP);
							PPP_MODE = REQUEST_FROM_MODEM;
                            ppp.b.end_snd = 1;
						break;
					}
				break;

				case getIP_IPCP:
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case ACK:
                            //*------DEBUG------*
                            send_c_out_max(0x8A); //"ACK to get IP (IPCP) from ISP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							nextStatePPP(readyPPP);
							PPP_MODE = WAIT_REPLY_FROM_ISP;
							initTCP();
                            STATE_HTTP = START_HTTP;
                            http.b.discnct = 0;
							cRetryPPP = 0;
                            ppp.b.connect = 1;
							//* cancelWait(T_PPP_NoResponseFromISP);
                            timer_no_resp_isp = ppp.b.no_resp = 0; //(таймаут отменен)
						break;
						case NAK:
                            //*------DEBUG------*
                            send_c_out_max(0x82); //"NAK to get IP (IPCP) from ISP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							local_ip.v[0] = context_InPacketPPP[pcip].buffer[11]; 
							local_ip.v[1] = context_InPacketPPP[pcip].buffer[12]; 
							local_ip.v[2] = context_InPacketPPP[pcip].buffer[13]; 
							local_ip.v[3] = context_InPacketPPP[pcip].buffer[14]; 
							nextStatePPP(getIP_IPCP);
							PPP_MODE = REQUEST_FROM_MODEM;
						break;
					}
				break;

				case closingIPCP:
					switch(context_InPacketPPP[pcip].buffer[5]) {			// CODE
						case TERM_ACK:
                            //*------DEBUG------*
                            send_c_out_max(0x8C); //"closing IPCP"
                            send_c_out_max(0x00);
                            //*------DEBUG------*
							idPPP++;
							nextStatePPP(closingLCP);
							PPP_MODE = REQUEST_FROM_MODEM;
						break;
					}
				break;
			}
		break;	

		case IP_PACKET_PPP:
			switch(sPPP) {
				case readyPPP:
					if(getHeaderIP()) {
                        //*------DEBUG------*
                        //send_c_out_max(0x89); //"ready PPP, received IP packet"
                        //send_c_out_max(0x00);
                        //*------DEBUG------*
					    rcvIP(in_context.protocol);	
					};
				break;
			}
		break;
	}
}

void sendPacketPPP() large
{
  unsigned int  data  i;
  unsigned char xdata c1, c2;

	if(cRetryPPP >= Lim_cR_PPP[sPPP]) {
        //*------DEBUG------*
        send_c_out_max(0xC3); //"cRetryPPP >= 3; termPPP"
        send_c_out_max(0x00);
        //*------DEBUG------*
 		cRetryPPP = 0;
		nextStatePPP(termPPP); 
		//* sendSignal(S_PPP_TerminatePPP, TASK_GSM_SEND);
        ppp.b.out_ppp = 1;
		return;
	}

	switch(sPPP) {
		case estConnectionLCP:
			PPP_Header(LCP, REQ);
				add(idPPP);								// id
				add(0x00);
				add(0x06);								// length
				add(0x02);								// type Authentication 
				add(0x02);
			PPP_End();
            //*------DEBUG------*
            send_c_out_max(0xC1); //"estConnectionLCP; sending"
            send_c_out_max(0x00);
            //*------DEBUG------*
			//* wait(T_PPP_NoResponseFromISP, _5_sec);
            timer_no_resp_isp = 50;
            ppp.b.no_resp = 0;
			PPP_MODE = WAIT_REPLY_FROM_ISP;
			break;

		case authModemPAP:
			PPP_Header(PAP, REQ);
				add(idPPP);					            // id
                c1 = strlen(PPP_LOGIN);
                c2 = strlen(PPP_PASSW);
				add(0x00);
				add(6+c1+c2);				            // length
                add(c1);				                // len 1 str
                for (i=0; PPP_LOGIN[i]; i++) add(PPP_LOGIN[i]);
				add(c2);    				            // len 1 str
                for (i=0; PPP_PASSW[i]; i++) add(PPP_PASSW[i]);
			PPP_End();
            //*------DEBUG------*
            send_c_out_max(0xCB); //"sending 'beeline'"
            send_c_out_max(0x00);
            //*------DEBUG------*
			//* wait(T_PPP_NoResponseFromISP, _30_sec);
            timer_no_resp_isp = 11; //?25сек?
            ppp.b.no_resp = 0;
			PPP_MODE = WAIT_REPLY_FROM_ISP;
		break;

		case getIP_IPCP:
			PPP_Header(IPCP, REQ);
				add(idPPP);					// id
				add(0x00);
				add(0x0A);					// length
				add(0x03);					// type
				add(0x06);
				add(local_ip.v[0]);
				add(local_ip.v[1]);
				add(local_ip.v[2]);
				add(local_ip.v[3]);
			PPP_End();
            //*------DEBUG------*
            send_c_out_max(0x91); //"get IP (IPCP) - REQ MTA"
            send_c_out_max(0x00);
            //*------DEBUG------*
			//* wait(T_PPP_NoResponseFromISP, _5_sec);
            timer_no_resp_isp = 50;
            ppp.b.no_resp = 0;
			PPP_MODE = WAIT_REPLY_FROM_ISP;
		break;

		case readyPPP:
			PPP_Header(IP_PACKET_PPP, NULL);
				for(i=0;i<contextIP.total_len;i++) {
				    add(((unsigned char*)&contextIP.packet)[i]);							
				}
			PPP_End();
            //*------DEBUG------*
            //send_c_out_max(0x99); //"ready PPP, sending packet"
            //send_c_out_max(0x00);
            //*------DEBUG------*
		break;

		case closingIPCP:
			PPP_Header(IPCP, TERM_REQ);
				add(idPPP);												// id 
				add(0x00);
				add(0x04);												// length 
			PPP_End();
            //*------DEBUG------*
            send_c_out_max(0x9C); //"closing IPCP from MTA"
            send_c_out_max(0x00);
            //*------DEBUG------*
			//* wait(T_PPP_NoResponseFromISP, _5_sec);
            timer_no_resp_isp = 50;
            ppp.b.no_resp = 0;
			PPP_MODE = WAIT_REPLY_FROM_ISP;
		break;

		case closingLCP:
			PPP_Header(LCP, TERM_REQ);
				add(idPPP);												// id 
				add(0x00);
				add(0x04);												// length 
			PPP_End();
            //*------DEBUG------*
            send_c_out_max(0xCC); //"sending 'closingLCP'"
            send_c_out_max(0x00);
            //*------DEBUG------*
			//* wait(T_PPP_NoResponseFromISP, _5_sec);
            timer_no_resp_isp = 50;
            ppp.b.no_resp = 0;
			PPP_MODE = WAIT_REPLY_FROM_ISP;
		break;

		case termPPP:
		break;
	}
}


unsigned char readyStackPPP() {return (sPPP == readyPPP);}

void nextStatePPP(_sPPP state) {sPPP = state;}

void closePPP()
{
    ppp.b.connect = 0;
	nextStatePPP(closingIPCP);
	sendPacketPPP();
}

void PPP_Header(PROTOCOL_PPP_PACKET protocol, unsigned char _code) large
{
	checksum = 0xFFFF;
	txIndex = 1;
	txBufferPPP[0] = ' ';
	add(0xFF);
	add(0x03);
	// PROTOCOL
	if(protocol == LCP) 			{ add(0xC0); add(0x21); }
	if(protocol == PAP)				{ add(0xC0); add(0x23); }
	if(protocol == IPCP) 			{ add(0x80); add(0x21); }
	if(protocol == IP_PACKET_PPP)	{ add(0x00); add(0x21); }
	if(protocol == CCP) 			{ add(0x80); add(0xFD); }
	// CODE
	if(_code) add(_code);
}

void PPP_End() large
{
unsigned int xdata cs; 

	cs = ~checksum;
	add((unsigned char)(cs & 0xff));
	add((unsigned char)(cs / 256));
	txEnd   = txIndex;
	txIndex = 0;
	txESC   = FALSE;

	funcPPP();	
}

void funcPPP()
{
unsigned char data c;
	while(1) {
        reset_wdt;
		c = txBufferPPP[txIndex];
		if(txIndex == txEnd) {		// end
			txEnd = 0; 
			c = 0x7E;
			put_char(c);
			break;
		} else if(txESC) {
			c ^= 0x20;
			txESC = FALSE;
			txIndex++;	
		} else if((c < 0x20) || (c == 0x7D) || (c == 0x7E)) {	
			txESC = TRUE;
			c = 0x7D;
		} else {						// begin
			if(!txIndex) c = 0x7E;
			txIndex++;	
		}
		put_char(c);
	}
}

unsigned char testOption(unsigned char option)
// return first option
{
signed int    xdata size_option;
unsigned char xdata first_option = 0x00;
unsigned char xdata i;

	size_option = *((signed int*)(&context_InPacketPPP[pcip].buffer[7])) - 4;	
	i = 9;
	//if(size_payload > MAX_SIZE_BUFFER_PPP) size_payload - MAX_SIZE_BUFFER_PPP;
	while(size_option > 0) {
		first_option = context_InPacketPPP[pcip].buffer[i];
		if(first_option != option) break;
		i += context_InPacketPPP[pcip].buffer[i+1]; 
		size_option -= context_InPacketPPP[pcip].buffer[i+1];	
	}
	return(first_option);
}

// Add next character to the CRC checksum for PPP packets
unsigned int calc(unsigned int c) 
{
unsigned char data i;				// Just a loop index

   c &= 0xFF;				// Only calculate CRC on low byte
   for(i=0;i<8;i++) {		// Loop eight times, once for each bit
      if (c&1) {			// Is bit high?
         c /= 2;			// Position for next bit      
         c ^= 0x8408;		// Toggle the feedback bits
      } else c /= 2;		// Just position for next bit
   }						// This routine would be best optimized in assembly
   return c;				// Return the 16 bit checksum
}

// Add character to the new packet
void add(unsigned char c) 
{
   checksum = calc(c^checksum) ^ (checksum/256); 	// Add CRC from this char to running total
   txBufferPPP[txIndex] = c;						// Store character in the transmit buffer
   txIndex++;										// Point to next empty spot in buffer
}
