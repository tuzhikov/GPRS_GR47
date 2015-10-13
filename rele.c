//
//  RELE    Исполнительные устройства
//

#include <c8051f120.h>
#include "main.h"
#include "rele.h"
#include "sensor.h"
//#include "adc.h"

// -------------------------------------------------------------
//    Переменные:
// -------------------------------------------------------------

// Для Реле
// Запоминаем сколько было на счетчике (засекаем время)
// Время через которое включится реле
unsigned long xdata sTime_rele_on[N_RELE]={0};
// Время на которое включится реле
unsigned long xdata sTime_rele_work[N_RELE]={0};
// Время для замера пауз при работе ОС
unsigned long xdata sTime_work_OS=0;

// Время с которым сравниваем
// Время через которое включится реле
unsigned long xdata time_rele_on[N_RELE]={0};
// Время на которое включится реле
unsigned long xdata time_rele_work[N_RELE]={0};

// флаг/переменная что нужно сделать с одним из реле 'N' - ничего; '1' - включить; '0' - выключить;
unsigned char xdata _c_rele_do[N_RELE]={0};

unsigned char c_Rele; // Здесь будет храниться информация о вкл/выкл реле

// -------------------------------------------------------------
//    Функции:
// -------------------------------------------------------------

// иниц.
void initialize_rele(void)
{
    P_r1 = 0;
    P_r2 = 0;
    P_r3 = 0;
    P_r4 = 0;
    c_Rele = 0; // Выключили все реле

    ReadScratchPad1();
    if (FlashDump[2]=='R')
    {
        c_Rele = FlashDump[3] & (port_rele_mask);
        // восстанавливаются состояния реле, сост. внешнего охранного устр-ва, кроме динамика(сирены) и блокировки.
        port_rele = (port_rele & (~port_rele_mask))|(c_Rele);

       _flag_secbut_on = (FlashDump[0] == 0x55);
    }
    reset_wdt;
}

void SaveToFlash(void)
{
    ReadScratchPad1();
    FlashDump[0] = _flag_secbut_on ? 0x55 : 0xAA;   // сост. установки под охрану
    FlashDump[2] = 'R'; FlashDump[3] = c_Rele;      // сост. всех реле
    WriteScratchPad1();
    reset_wdt;
}

unsigned char get_sost_rele(void)
{
    return (c_Rele<<4);
//  return ((port_rele & port_rele_mask)<<4);
}

void automat_rele()
{
  unsigned char xdata Ri;

    // Управление ИУ.
    // Проверка времени.
    // Включение/выключение ИУ.
    for (Ri=0; Ri<N_RELE; Ri++)
    {
        if ((_c_rele_do[Ri] == '0') && ((Rele_LCount-sTime_rele_work[Ri]) >= time_rele_work[Ri]))
        {
            // Прошло заданное время - выключаем реле
           _c_rele_do[Ri] = 'N';

            if (Ri == 0) { P_r1 = 0; c_Rele &=(~1); mResult.b.relay_switch = 1; }
            else
            if (Ri == 1) { P_r2 = 0; c_Rele &=(~2); mResult.b.relay_switch = 1; }
            else
            if (Ri == 2) { P_r3 = 0; c_Rele &=(~4); mResult.b.relay_switch = 1; }
            else
            if (Ri == 3) { P_r4 = 0; c_Rele &=(~8); mResult.b.relay_switch = 1; }
            SaveToFlash();
        }
        if ((_c_rele_do[Ri] == '1') && ((Rele_LCount-sTime_rele_on[Ri]) >= time_rele_on[Ri]))
        {
            // Прошло заданное время - включаем реле
           _c_rele_do[Ri] = 'N';
            if (Ri == 0) { P_r1 = 1; c_Rele |=1; mResult.b.relay_switch = 1; }
            else
            if (Ri == 1) { P_r2 = 1; c_Rele |=2; mResult.b.relay_switch = 1; }
            else
            if (Ri == 2) { P_r3 = 1; c_Rele |=4; mResult.b.relay_switch = 1; }
            else
            if (Ri == 3) { P_r4 = 1; c_Rele |=8; mResult.b.relay_switch = 1; }
            SaveToFlash();

            // Если задано время через которое надо выключить, то готовимся к выключению
            if (time_rele_work[Ri] > 0)
            {
                _c_rele_do[Ri] = '0';
                 sTime_rele_work[Ri] = Rele_LCount;
            }
        }
        reset_wdt;
    }
}

/**
* @function UprRele.
* Управление исполнительными устройствами.
* Задание времени включения/выключения ИУ.
*/

//11.00 00.00 00.3C
//21 00 00 00 00 3C
//31 00 00 00 01 3C
//41 00 00 00 01 3C
//0F
// Управление исполнительными устройствами (Реле)
// Задание времени включения/выключения ИУ.
// |Type| N_Rele / Do |Zad_On |TimeWrk|Sc |ND| | | | | | |ND| | | | | | |ND| | | | | | | Sec |
// |    ¦             |   |   |   |   | | |              |              |              |     |
//             0        1   2   3   4  5 6 7              14             21              28
//
// Type   - код команды (4D)
// N_Rele - ст.тетрада - номер ИУ
// Do     - мл.тетрада - 1111 - ничего не делать с ИУ
//                       0000 - выключить ИУ
//                       0001 - включить ИУ
// Zad_On - Задержка включения (в сек)
// TimeWrk- Время работы (в сек)
// Scale  - Масшт.коефф. для времен.
//
// 4 таких блока после кода команды для каждого ИУ 
//
void UprRele(unsigned char *buf)
{
  unsigned char xdata NRele;
  unsigned char xdata i;
  unsigned  int xdata Scale;

    for (i=0; i<N_RELE; i++)
    {
        NRele = ((buf[7*i]&0xf0)>>4)-1;
        if ((NRele>=0)&&(NRele<=3))
        {
            Scale = ((((unsigned int)buf[7*i+5])<<8)&0xff00) + buf[7*i+6];
            if ((buf[7*i]&0x0f) == 0) // Если команда выключить реле, то готовимся к выключению
            {
                // Через какое время выключить реле (сколько реле будет работать)
                time_rele_work [NRele] = ((unsigned long)(((((unsigned int)buf[7*i+3])<<8)&0xff00) + buf[7*i+4])) * Scale;
                // Запоминаем счетчик
                sTime_rele_on  [NRele] = Rele_LCount;
                sTime_rele_work[NRele] = Rele_LCount;
               _c_rele_do[NRele] = '0';
            }
            else
            if ((buf[7*i]&0x0f) == 1) // Если команда включить реле, то готовимся к включению
            {
                // Через какое время включить реле
                time_rele_on   [NRele] = ((unsigned long)(((((unsigned int)buf[7*i+1])<<8)&0xff00) + buf[7*i+2])) * Scale;
                // Через какое время выключить реле (сколько реле будет работать)
                time_rele_work [NRele] = ((unsigned long)(((((unsigned int)buf[7*i+3])<<8)&0xff00) + buf[7*i+4])) * Scale;
                // Запоминаем счетчик
                sTime_rele_on  [NRele] = Rele_LCount;
                sTime_rele_work[NRele] = Rele_LCount;
               _c_rele_do[NRele] = '1';
        }   }
    }

    if ((buf[28]&0x0f) == 1) // флаг постановки на охрану
    {
        if (_flag_secbut_on==no) mResult.b.set_guarding_on = 1;
        //time_sleep_gps=13; // Вход в спящий режим быстрее.

       _flag_secbut_on   = yes; // флаг постановка на охрану
//        state_sensor[N_SB_ON] = 10; // Включить звук
    }
    else if ((buf[28]&0x0f) == 0)  // флаг снятия с охраны
    {
        if (_flag_secbut_on==yes)
            mResult.b.set_guarding_off = 1; // событие снятие с охраны

       _flag_secbut_on   = no; // флаг снятие с охраны
//        state_sensor[N_SB_OFF] = 15; // Включить звук
    }
    else return;

    //записать состояние установки под охрану в Scratchpad FLASH Memory
    if (mResult.b.set_guarding_on || mResult.b.set_guarding_off)
        SaveToFlash();
}
