#ifndef ADC_H
#define ADC_H



// A D C   F u n c t i o n s

void StartConvertADC();
bit FinishConvertADC();



// A D C   G l o b a l   V a r i a b l e s

extern unsigned int xdata ADCBuffer[8]; //в буфер помещаются суммы 7-битных значений с АЦП (старшие 7 бит)
extern signed char xdata Therm; //выходное значение температуры

extern bit BusyConverter; //=1 соотв. моменту между началом преобр. и записью в буфер



#endif
