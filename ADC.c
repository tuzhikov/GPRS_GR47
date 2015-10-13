#include "ADC.h"
#include "main.h"
#include <c8051f120.h>


// A D C   V a r i a b l e s

sfr16 ADC0 = 0xBE; // ADC0 data

unsigned int xdata ADCBuffer[8] = {0x0000};
signed char xdata Therm = 0;

bit BusyConverter = 0;

unsigned char xdata ADCInput = 0;

#define AccLim 42 //должно быть >1
unsigned char xdata AccCounter = 0; //счетчик накапливаемых значений по каналам АЦП

signed long xdata Thermometer = 0;



// A D C   F u n c t i o n s

void StartConvertADC()
{
    if (BusyConverter) return;
    SFRPAGE = ADC0_PAGE;
    AMX0SL = ADCInput; //Figure 5.5, page 32
    AD0BUSY = 1;       //starts ADC conversion
    BusyConverter = 1;
}

bit FinishConvertADC()
{
  unsigned char data i;

    if (BusyConverter)
    {
        SFRPAGE = ADC0_PAGE;
        if (AD0BUSY) return(0);
        BusyConverter = 0;

        if (ADCInput != 8)
        {
            ADCBuffer[ADCInput++] += ADC0H >> 1; //старшие 7 бит
            //if (ADCInput == 4) ADCInput = 5; //пропустить неиспользуемые
        }
        else
        {
            Thermometer += ADC0>>4;

            ADCInput = 0;

            if (++AccCounter >= AccLim) //накоплено необходимое кол. последовательных значений
            {
                for (i=0; i<8; i++) ADCBuffer[i] /= AccLim;

                Thermometer /= AccLim;
                Therm = (signed char)((Thermometer*59-77600)/286);

                AccCounter = 1; //уже одно значение есть
                return(1);    
            } //if (++AccCounter >= AccLim)
        } //if (ADCInput < 8)...else
    } //if (BusyConverter)
    return(0);
}
