#ifndef RMODEM_H
#define RMODEM_H



extern unsigned char xdata time_wait_RM;
extern unsigned char xdata * pcontext;
extern bit RM_Enable;
extern unsigned char code ID_RM[8];
typedef union CRMST { //operations' Status of RM Routines
    unsigned char c;
    struct {
        unsigned char fNeedRst  : 1; //flag: требуется перезапуск процессора
           } b;
                    };
extern union CRMST xdata StRM;



void automat_rmodem();



#endif
