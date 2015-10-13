#ifndef SENSOR_H
#define SENSOR_H


#define SENSOR1  (SFRPAGE=CONFIG_PAGE,P5) // Контакты 
#define SENSOR1WR  SFRPAGE=CONFIG_PAGE;P5

#define port_sensor1_mask 0x0f // битовая маска контактов


//#define Setup_G_on  (LimitsData.Each.guarding_on_n)
//#define Setup_G_off (LimitsData.Each.guarding_off_n)


// Functions
// ---------------------------------------
void initialize_sensor (void);
void automat_sensor (void);
void SaveSecToFlash(void);

// ---------------------------------------
// переменные хранящие тек. и предыдущие значения на датчиках.
extern unsigned char xdata port_sensor_latch; // Защелка на каждый датчик
extern unsigned char xdata port_sensor_current;

extern bit _flag_secbut_on; // флаг "постановка на охрану"
//extern bit _flag_alarm_sec; // флаг тревоги "нарушение постановки на охрану"

extern unsigned int xdata time_wait_sensor[8];

extern unsigned char xdata state_sensor[8];



#endif
