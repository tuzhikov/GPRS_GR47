//
//  SENSOR
//

#include <c8051f120.h>
#include "main.h"
#include "sensor.h"
//#include "rele.h"
//#include "adc.h"

// -------------------------------------------------------------
//    Переменные:
// -------------------------------------------------------------

bit _flag_secbut_on;    // флаг "постановка на охрану"

unsigned char xdata  count_measure_sensor[8]; // счетчик проверки уровня сигнала на датчике

// для обслуживания автомата датчиков
unsigned char xdata  state_sensor[8];

// переменные хранящие считанные, тек. и предыдущие значения на датчиках.
unsigned char xdata port_sensor[8];
unsigned char xdata port_sensor_p[8];
//unsigned char xdata port_sensor_z[8];

//unsigned int xdata port_sensor_previous;
unsigned char xdata port_sensor_current;
unsigned char xdata port_sensor_latch; // Защелка на каждый датчик

unsigned int xdata time_wait_sensor[8];
unsigned int xdata *Setup_a;
unsigned int xdata *Setup_t;

#define Setup_s1   (&(LimitsData.Each.sensor_dry1_n))-1
#define Setup_s1_t (&(LimitsData.Each.sensor_dry1_x))-1

// заданы в h-файле
//#define Setup_G_on  (LimitsData.Each.guarding_on_n)
//#define Setup_G_off (LimitsData.Each.guarding_off_n)

unsigned char xdata i;

void automat_sensor_N(unsigned char Ns);


void read_port_sensor_cur(void)
{
    port_sensor_current  = SENSOR1 & port_sensor1_mask;
}

void read_port_sensor(unsigned char n)
{
    port_sensor[n] = SENSOR1 & port_sensor1_mask & (0x01<<n);
}

// иниц. по датчикам
void initialize_sensor(void)
{
    read_port_sensor_cur(); // начальное снятие значений с датчиков

    DataLogger[0x05] = port_sensor_current & port_sensor1_mask;

    port_sensor_latch    = 0;

    for (i=0; i<4; i++)
    {
        read_port_sensor(i);
        port_sensor_p[i]        = port_sensor[i];
        count_measure_sensor[i] = 0;
        time_wait_sensor[i]     = 0;
        state_sensor[i]         = 1;
    }

    Setup_a = Setup_s1;
    Setup_t = Setup_s1_t;

    reset_wdt;
}

void reset_sensor(unsigned char n)
{
    Setup_a = Setup_s1;
    Setup_t = Setup_s1_t;

    count_measure_sensor[n] = 0;
    time_wait_sensor[n] = 0;
    state_sensor[n] = 1; // привед автомата в нач. состояние

    reset_wdt;
}

bit AskBit(unsigned char I, unsigned char n)
{
    reset_wdt;
    return ((bit)((I>>n)&1));
}

void SetBit(unsigned char *I, unsigned char n,  bit b)
{
    reset_wdt;
    *I &= ~((0x01)<<n); // Очистка бита
    *I |=  (((unsigned char)b)<<n); // Установка бита
}

bit IsSensChange(unsigned char n)  // значение контакта изменилось ?
{
//    if ((AskBit(port_sensor_p[n], n)) != (AskBit(port_sensor[n], n))) return 1;
    if (port_sensor_p[n] != (port_sensor[n])) return 1;

    return 0;
}

//void setsob_lo(unsigned char n)
void SetSob(unsigned char n)
{
         if (n==0)  mResult.b.sensor_dry1_c = 1;
    else if (n==1)  mResult.b.sensor_dry2_c = 1;
    else if (n==2)  mResult.b.sensor_dry3_c = 1;
    else if (n==3)  mResult.b.sensor_dry4_c = 1;
}

/*void SetSob(unsigned char n)
{
    setsob_lo(n); // устанавливаем бит события для контакта n.

}*/

// Проверка совпадает ли фронт на датчике Ns с заданным в профиле?
bit SensFrontTest(unsigned char Ns)
{
    // По любому фронту
    if ((Setup_a[Ns]&3)==3) return 1;

    // По фронту 01
    if (((Setup_a[Ns]&3)==1) && (port_sensor[Ns]>=1)) return 1;

     // По фронту 10
    if (((Setup_a[Ns]&3)==2) && (port_sensor[Ns]==0)) return 1;

    return 0;
}

// __-- и --__      Setup_s.=3
// только __-- 01   Setup_s.=1
// только --__ 10   Setup_s.=2
// Действия по изменению состояния датчиков
void SensAction(unsigned char Ns)
{
    // Если защелка для бита Ns снята
    if (AskBit(port_sensor_latch, Ns)==0)
    {
        // Сохранить текущее (новое) состояние бита
        SetBit(&port_sensor_current, Ns, AskBit(port_sensor[Ns], Ns));

        if (SensFrontTest(Ns)==1) // Определяем нужный фронт или нет
        {
            SetSob(Ns); // Установить событие
            SetBit(&port_sensor_latch, Ns, 1); // Защелкнуть бит
        }

        // Приравниваем пред бит к текущ
        port_sensor_p[Ns] = port_sensor[Ns];
        //SetBit(&port_sensor_p, i, AskBit(port_sensor, i));
    }
}

void automat_sensor()
{
    for (i=0; i<4; i++)
    {
        automat_sensor_N(i);
        reset_wdt;
    }
}

void automat_sensor_N(unsigned char Ns)
{
    switch(state_sensor[Ns])
    {
        case 1:
            read_port_sensor(Ns); // опрос состояния контакта N
            if (IsSensChange(Ns)) // Если значение контакта N изменилось
            {
                state_sensor[Ns] = 2;         // переход в состояние ожидания-повторных замеров
                count_measure_sensor[Ns] = 0; // кол-во повторных замеров обнуляется

                // Проверка совпадает ли фронт на датчике Ns с заданным в профиле?
                if (SensFrontTest(Ns)==0)
                {
                    state_sensor[Ns] = 4; // Нет не совпадает
                }
                time_wait_sensor[Ns] = 0; // таймер повторных замеров обнуляется
            }
        break;

        case 2:
            if(time_wait_sensor[Ns] >= ((Setup_t[Ns]&0x00FF)*10)) // пауза t
            {
                time_wait_sensor[Ns] = 1;     // таймер повторных замеров обнуляется
                state_sensor[Ns] = 4;         // переход в состояние ожидания-повторных замеров
            }
        break;

        case 4:
            if (time_wait_sensor[Ns]>=1) // опрос через каждые 0.1 сек (всего 3 раза)
            {
                time_wait_sensor[Ns] = 0;
                read_port_sensor(Ns); // опрос состояния контакта N датчиков
                if (IsSensChange(Ns)) // Если значение контакта N еще раз изменилось
                {
                    count_measure_sensor[Ns]++;
                    if (count_measure_sensor[Ns] >= 3)
                    {
                        state_sensor[Ns] = 1;
                        SensAction(Ns);
                        time_wait_sensor[Ns] = 0;
                    }
                }
                else state_sensor[Ns] = 99; // Ложное срабатывание датчиков (случайно)
            }
        break;

        default: // state_sensor_error
            reset_sensor(Ns);
        break;
    }
}
