#include <string.h>

#include "..\main.h"

#include "setting.h"
#include "ppp.h"
#include "ip.h"
#include "tcp.h"
#include "http.h"



_contextIP   xdata 		contextIP;
unsigned int xdata 		id_ip;
IP_ADDR xdata 			local_ip    	= {0}; 
//
//
//
void initIP()
{
	id_ip = 0x0000;
}

void rcvIP(unsigned char protocol) large
{
	switch(protocol) {
		case IP_PROT_ICMP:
		break;

		case IP_PROT_TCP:
			rcvTCP();	
		break;

		case IP_PROT_UDP:
		break;
	}
}

void sendIP(unsigned char protocol, IP_ADDR *remote) large
{
	switch(protocol) {
		case IP_PROT_ICMP:
			//IP_Header(remote, IP_PROT_ICMP, (unsigned char*)&icmp_context.packet, icmp_context.total_len);
		break;

		case IP_PROT_TCP:
			IP_Header(remote, IP_PROT_TCP, (unsigned char*)&tcp_context.packet, tcp_context.total_len);
		break;

		case IP_PROT_UDP:
		break;
	}
}

/*********************************************************************
 *
 ********************************************************************/
BOOL getHeaderIP()
{
IP_HEADER xdata  	header;
unsigned char xdata optionsLen;

	in_context.i = 5;
    // Read IP header.
    memcpy((unsigned char*)&header, &context_InPacketPPP[pcip].buffer[in_context.i], sizeof(header));
	in_context.i += sizeof(header);
    // Make sure that this IPv4 packet.
    if((header.VersionIHL & 0xf0) != IP_VERSION) return FALSE;
    //
    // Calculate options length in this header, if there is any.
    // IHL is in terms of numbers of 32-bit DWORDs; i.e. actual
    // length is 4 times IHL.
    //
    optionsLen = ((header.VersionIHL & 0x0f) << 2) - sizeof(header);
    //
    // If there is any option(s)
    //
    if(optionsLen > MAX_IP_OPTIONS_LEN) return FALSE; 
    if(optionsLen > 0) in_context.i += optionsLen;
	//
    // If caller is intrested, return destination IP address
    // as seen in this IP header.
    //
	in_context.local = header.DestAddress;
    in_context.remote = header.SourceAddress;
	in_context.protocol = header.Protocol;
	in_context.len = (header.TotalLength - optionsLen - sizeof(header));

    return TRUE;
}

/*********************************************************************
 *
 ********************************************************************/
unsigned char IP_Header(IP_ADDR* remote, 
						unsigned char protocol, 
						unsigned char* _data,
						unsigned int data_len)
{
    contextIP.packet.header.VersionIHL        	= IP_VERSION | IP_IHL;
    contextIP.packet.header.TypeOfService     	= IP_SERVICE;
    contextIP.packet.header.TotalLength       	= sizeof(contextIP.packet.header) + data_len;
    contextIP.packet.header.Identification    	= ++id_ip;
    contextIP.packet.header.FragmentInfo      	= 0x4000;
    contextIP.packet.header.TimeToLive        	= MY_IP_TTL;
    contextIP.packet.header.Protocol          	= protocol;
    contextIP.packet.header.HeaderChecksum    	= 0;
    contextIP.packet.header.SourceAddress.v[0]	= local_ip.v[0];
    contextIP.packet.header.SourceAddress.v[1]	= local_ip.v[1];
    contextIP.packet.header.SourceAddress.v[2]	= local_ip.v[2];
    contextIP.packet.header.SourceAddress.v[3]	= local_ip.v[3];
    contextIP.packet.header.DestAddress.v[0]  	= remote->v[0];
	contextIP.packet.header.DestAddress.v[1]  	= remote->v[1];
	contextIP.packet.header.DestAddress.v[2]  	= remote->v[2];
	contextIP.packet.header.DestAddress.v[3]  	= remote->v[3];
    contextIP.packet.header.HeaderChecksum  	= CalcIPChecksum((unsigned char*)&contextIP.packet.header, 
																 sizeof(contextIP.packet.header));
	memcpy((void*)contextIP.packet.Data, (void*)_data, data_len);
	contextIP.total_len = IP_HEADER_SIZE + data_len;

	sendPacketPPP();

    return(1);
}

unsigned int CalcIPChecksum(unsigned char* buffer, unsigned int count)
{
    unsigned int xdata  i;
    unsigned int xdata  *val;
	union {
        unsigned long Val;
        struct {
            WORD_VAL MSB;
            WORD_VAL LSB;
        } words;
    } tempSum, sum;

    sum.Val = 0;
    i = count >> 1;
    val = (unsigned int *)buffer;
    reset_wdt;
    while(i--) sum.Val += *val++;
    reset_wdt;
    if(count & 1) sum.Val += *(unsigned char *)val;
    tempSum.Val = sum.Val;

    while((i = tempSum.words.MSB.Val) != 0)
    {
        sum.words.MSB.Val = 0;
        sum.Val = (unsigned long)sum.words.LSB.Val + (unsigned long)i;
        tempSum.Val = sum.Val;
    }

    return (~sum.words.LSB.Val);
}

