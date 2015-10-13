#ifndef IP_H
#define IP_H


#include "..\main.h"
#include "setting.h"


// This is left shifted by 4.  Actual value is 0x04.
#define IPv4                (0x40)
#define IP_VERSION          IPv4

/*
 * IHL (Internet Header Length) is # of DWORDs in a header.
 * Since, we do not support options, our IP header length will be
 * minimum i.e. 20 bytes : IHL = 20 / 4 = 5.
 */
#define IP_IHL              (0x05)

#define IP_SERVICE_NW_CTRL  (0x07)
#define IP_SERVICE_IN_CTRL  (0x06)
#define IP_SERVICE_ECP      (0x05)
#define IP_SERVICE_OVR      (0x04)
#define IP_SERVICE_FLASH    (0x03)
#define IP_SERVICE_IMM      (0x02)
#define IP_SERVICE_PRIOR    (0x01)
#define IP_SERVICE_ROUTINE  (0x00)

#define IP_SERVICE_N_DELAY  (0x00)
#define IP_SERCICE_L_DELAY  (0x08)
#define IP_SERVICE_N_THRPT  (0x00)
#define IP_SERVICE_H_THRPT  (0x10)
#define IP_SERVICE_N_RELIB  (0x00)
#define IP_SERVICE_H_RELIB  (0x20)

#define IP_SERVICE          (IP_SERVICE_ROUTINE | IP_SERVICE_N_DELAY)

#define MY_IP_TTL           (100)   /* Time-To-Live in Seconds */

//
#define IP_PROT_ICMP    (1)
#define IP_PROT_TCP     (6)
#define IP_PROT_UDP     (17)




typedef union _IP_ADDR
{
    unsigned char       v[4];
    unsigned long       Val;
} IP_ADDR;
/*
 * IP packet header definition
 */
typedef struct _IP_HEADER
{
    unsigned char    VersionIHL;
    unsigned char    TypeOfService;
    unsigned int     TotalLength;
    unsigned int     Identification;
    unsigned int     FragmentInfo;
    unsigned char    TimeToLive;
    unsigned char    Protocol;
    unsigned int     HeaderChecksum;

    IP_ADDR SourceAddress;
    IP_ADDR DestAddress;

} IP_HEADER;
#define IP_HEADER_SIZE    (sizeof(IP_HEADER))

typedef struct _IP_PACKET
{
	IP_HEADER 	header;
	unsigned char    	Data[MAX_IP_DATA];	 
} IP_PACKET;

typedef struct {
    IP_PACKET		packet;  
    unsigned int 	total_len;
} _contextIP;
extern _contextIP xdata 	contextIP;

extern unsigned int xdata	id_ip;
extern IP_ADDR xdata 		local_ip; 
extern IP_ADDR code			remoteIP; 
extern IP_ADDR code			remoteIP_MAIL;

//
void 			initIP(void );
void 			rcvIP(unsigned char protocol) large;
void 			sendIP(unsigned char protocol, IP_ADDR *remote) large;

/*********************************************************************
 *
 ********************************************************************/
unsigned char IP_Header(IP_ADDR* remote, 
						unsigned char protocol, 
						unsigned char* _data,
						unsigned int data_len);


/*********************************************************************
 *
 ********************************************************************/
BOOL getHeaderIP(void );

unsigned int CalcIPChecksum(unsigned char* buffer, unsigned int count);

#endif



