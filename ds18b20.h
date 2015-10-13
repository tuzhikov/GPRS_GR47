#ifndef LAN_H
#define LAN_H



sfr16 TMR_VLAN = 0xCC; //(0xCC = Timer3 [TMR3H:TMR3L])

/* for SYSCLK=22MHz
#define VDELAY5   SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFE8F;TR3=1 //T ~ 20 mksec
#define VDELAY15  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFC66;TR3=1 //T ~ 50 mksec
#define VDELAY17  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFC54;TR3=1 //T ~ 51 mksec
#define VDELAY20  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFBAE;TR3=1 //T ~ 60 mksec
#define VDELAY25  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFAF6;TR3=1 //T ~ 70 mksec
#define VDELAY40  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF700;TR3=1 //T ~ 125 mksec
#define VDELAY250 SFRPAGE=TMR3_PAGE;TMR_VLAN=0xCA00;TR3=1 //T ~ 750 mksec
#define VDELAY500 SFRPAGE=TMR3_PAGE;TMR_VLAN=0x9400;TR3=1 //T ~ 1500 mksec
*/
//for SYSCLK=36MHz
#define VDELAY5   SFRPAGE=TMR3_PAGE;TMR_VLAN=0xFD99;TR3=1 //T ~ 20 mksec
#define VDELAY15  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF9FF;TR3=1 //T ~ 50 mksec
#define VDELAY17  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF9E1;TR3=1 //T ~ 51 mksec
#define VDELAY20  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF8CC;TR3=1 //T ~ 60 mksec
#define VDELAY25  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF79A;TR3=1 //T ~ 70 mksec
#define VDELAY40  SFRPAGE=TMR3_PAGE;TMR_VLAN=0xF100;TR3=1 //T ~ 125 mksec
#define VDELAY250 SFRPAGE=TMR3_PAGE;TMR_VLAN=0xA600;TR3=1 //T ~ 750 mksec
#define VDELAY500 SFRPAGE=TMR3_PAGE;TMR_VLAN=0x4C00;TR3=1 //T ~ 1500 mksec



// D S 1 8 B 2 0   F u n c t i o n s

void init_lan();
void automat_lan();
void automat_vlan();
void crc_8_lan(unsigned char in_crc);


// D S 1 8 B 2 0   V a r i a b l e s

extern bit _err_lan;
extern bit _lan_exist;
extern bit bit_data_lan;
extern signed char xdata Lan_temper;

extern bit VLAN;
extern bit VLAN_INT;

extern unsigned char data StateVLAN;

extern unsigned char bdata data_crc_8[1];
extern unsigned char xdata StateLAN;
extern unsigned char xdata context_LAN[8];
typedef union CLANST { //operations' Status of LAN Routines
    unsigned char c;
    struct {
        unsigned char cntErrSN : 3; //счетчик ошибок потока данных при запросе серийного кода
        unsigned char fGetSN   : 1; //flag: поступил запрос на чтение серийного кода
        unsigned char fReadySN : 1; //flag: получен результат запроса серийного кода
        unsigned char fFailSN  : 1; //flag: не удалось считать серийный код
           } b;
                     };
extern union CLANST xdata StLAN;



#endif
