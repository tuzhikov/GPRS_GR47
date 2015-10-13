#ifndef ATFLASH_H
#define ATFLASH_H



#define MaxPageAF 4095



// A t m e l   D a t a F l a s h   G l o b a l   V a r i a b l e s

typedef union AFHEADER {
    struct {
        unsigned char Cmd;       //Opcode
        unsigned int  Ctrl : 1;  //Access to Control Area (inside Page, above 256)
        unsigned int  Page : 12; //Page Address Bits
        unsigned int  Res  : 3;  //Reserved Bits
        unsigned char Byte;      //Byte/Buffer Address Bits
           } t;         //"type"
    unsigned char c[4]; //"char"
    unsigned long l;    //"long"
                       };

extern unsigned char xdata * BufAF;
extern unsigned int  xdata BufEndAF;
extern unsigned char xdata BufTailAF;
extern union AFHEADER xdata WrHeader;
extern union AFHEADER xdata RdHeader;
extern unsigned char xdata * HeaderAF;

extern unsigned char data StateIntAF;



// A t m e l   D a t a F l a s h   F u n c t i o n s

void WriteAF();
void ReadAF();
void WaitReadyAF();
void InitAF();



#endif
