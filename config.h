#ifndef CONFIG
#define CONFIG

#define NSPEEDS 10

/*
//Fast Mode (22.1184MHz)
#define S460800 0xFFFD
#define S230400 0xFFFA
#define S115200 0xFFF4
#define S57600  0xFFE8
//#define S56000  0xFFE7
#define S38400  0xFFDC
#define S19200  0xFFB8
//#define S14400  0xFFA0
#define S9600   0xFF70
#define S4800   0xFEE0
#define S2400   0xFDC0
#define S1200   0xFB80
*/

//Fast Mode (36.864MHz)
#define S460800 0xFFFB
#define S230400 0xFFF6
#define S115200 0xFFEC
#define S57600  0xFFD8
//#define S56000  0xFFD7
#define S38400  0xFFC4
#define S19200  0xFF88
//#define S14400  0xFF60
#define S9600   0xFF10
#define S4800   0xFE20
#define S2400   0xFC40
#define S1200   0xF880

//Slow Mode (~3MHz)
#define S460800_SM 0xFFFF   //incorrect (really 192000)
#define S230400_SM 0xFFFF   //incorrect (really 192000), dev=-16.67%
#define S115200_SM 0xFFFE   //incorrect (really  96000), dev=-16.67%
#define S57600_SM  0xFFFD   //incorrect (really  64000), dev=+11.11%
//#define S56000_SM  0xFFFC //incorrect (really  48000), dev=-14.29%
#define S38400_SM  0xFFFB
#define S19200_SM  0xFFF6
//#define S14400_SM  0xFFF3 //dev=+2.56%
#define S9600_SM   0xFFEC
#define S4800_SM   0xFFD8
#define S2400_SM   0xFFB0
#define S1200_SM   0xFF60


// C O N F I G   F u n c t i o n s/
void config(void);
void SetOscSlowMode();
//void SetOscFastMode();
void set_com_speed(unsigned char N); //set Timer4 clock for UART0

#endif
