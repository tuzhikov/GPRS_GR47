#ifndef ADC_H
#define ADC_H



// A D C   F u n c t i o n s

void StartConvertADC();
bit FinishConvertADC();



// A D C   G l o b a l   V a r i a b l e s

extern unsigned int xdata ADCBuffer[8]; //� ����� ���������� ����� 7-������ �������� � ��� (������� 7 ���)
extern signed char xdata Therm; //�������� �������� �����������

extern bit BusyConverter; //=1 �����. ������� ����� ������� ������. � ������� � �����



#endif
