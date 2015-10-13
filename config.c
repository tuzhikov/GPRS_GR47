//---------------------------------------------------------------
// CYGNAL Integrated Products 
//
// C Code Configuration Tool: F12X INITIALIZATION/CONFIGURATION CODE
//----------------------------------------------------------------
// This file is read only. To insert the code into your  
// application, simply cut and paste or use the "Save As" 
// command in the file menu to save the file in your project 
// directory. 
//----------------------------------------------------------------

//----------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------

#include <C8051F120.h>  // Register definition file.
#include "config.h"
#include "main.h"

//------------------------------------------------------------------------------------
// Global CONSTANTS
//------------------------------------------------------------------------------------

//скорости перечислены в порядке перебора
unsigned int code com_speed[NSPEEDS] = {S9600,S115200,S19200,S4800,S57600,S38400,S2400,S1200,S230400,S460800};

//------------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------------

void set_com_speed(unsigned char N) //set Timer4 clock for UART0
{
  int_uart0_no; //disable UART0 interrupt
    SFRPAGE = UART0_PAGE;
    REN0 = 0; //UART0 reception disabled
    SFRPAGE = TMR4_PAGE;
    TR4 = 0; //stop Timer4
    RCAP4 = com_speed[N]; //set clock value for Timer4
    TR4 = 1; //run Timer4
    SFRPAGE = UART0_PAGE;
    REN0 = 1; //UART0 reception enabled
    RI0 = 0; //clear UART0 interrupt pending flag
  int_uart0_yes; //enable UART0 interrupt
}

//------------------------------------------------------------------------------------
// Config Routine
//------------------------------------------------------------------------------------

void ConfigOsc()
{
  int b = 0;

// ! FOR INTERNAL OSC + PLL (36.864 MHz) !
    SFRPAGE    = CONFIG_PAGE;
    CCH0CN    &= ~0x20;
    SFRPAGE    = LEGACY_PAGE;
    FLSCL      = 0x90;            // FLASH Memory Control
    SFRPAGE    = CONFIG_PAGE;
    CCH0CN    |= 0x20;
    PLL0CN     = 0x01;            // PLL Control Register
    PLL0DIV    = 0x01;            // PLL pre-divide Register 
    PLL0FLT    = 0x37;            // PLL Filter Register
    PLL0MUL    = 0x03;            // PLL Clock scaler Register
    for (b=0; b<15; b++);         // wait at least 5us
    PLL0CN    |= 0x02;            // enable PLL
    while ((PLL0CN & 0x10) == 0); // wait for PLL to lock
    CLKSEL     = 0x02;            // Oscillator Clock Selector
	OSCICN     = 0x82;            // Internal Oscillator Control Register
}
/*-----------------------------------------------------------------------------*/
void config (void) {

//----------------------------------------------------------------
// Watchdog Timer Configuration
//
// WDTCN.[7:0]: WDT Control
//   Writing 0xA5 enables and reloads the WDT.
//   Writing 0xDE followed within 4 clocks by 0xAD disables the WDT
//   Writing 0xFF locks out disable feature.
//
// WDTCN.[2:0]: WDT timer interval bits
//   NOTE! When writing interval bits, bit 7 must be a 0.
//
//  Bit 2 | Bit 1 | Bit 0
//------------------------     
//    1   |   1   |   1      Timeout interval = 1048576 x Tsysclk
//    1   |   1   |   0      Timeout interval =  262144 x Tsysclk
//    1   |   0   |   1      Timeout interval =   65636 x Tsysclk
//    1   |   0   |   0      Timeout interval =   16384 x Tsysclk
//    0   |   1   |   1      Timeout interval =    4096 x Tsysclk
//    0   |   1   |   0      Timeout interval =    1024 x Tsysclk
//    0   |   0   |   1      Timeout interval =     256 x Tsysclk
//    0   |   0   |   0      Timeout interval =      64 x Tsysclk
// 
//------------------------

    WDTCN = 0x07;   // Watchdog Timer Control Register
    WDTCN = 0xA5;

//----------------------------------------------------------------
// CROSSBAR REGISTER CONFIGURATION
//
// NOTE: The crossbar register should be configured before any  
// of the digital peripherals are enabled. The pinout of the 
// device is dependent on the crossbar configuration so caution 
// must be exercised when modifying the contents of the XBR0, 
// XBR1, and XBR2 registers. For detailed information on 
// Crossbar Decoder Configuration, refer to Application Note 
// AN001, "Configuring the Port I/O Crossbar Decoder". 
//----------------------------------------------------------------

// Configure the XBRn Registers

    SFRPAGE = 0x0F;
    XBR0 = 0x0F;    // XBAR0: Initial Reset Value
    XBR1 = 0x14;    // XBAR1: Initial Reset Value
    XBR2 = 0x44;    // XBAR2: Initial Reset Value
// Select Pin I/0

// NOTE: Some peripheral I/O pins can function as either inputs or 
// outputs, depending on the configuration of the peripheral. By default,
// the configuration utility will configure these I/O pins as push-pull 
// outputs.
                    // Port configuration (1 = Push Pull Output)
    SFRPAGE = 0x0F;
    P0MDOUT = 0x35; // Output configuration for P0
    P1MDOUT = 0x85; // Output configuration for P1
    P2MDOUT = 0x1F; // Output configuration for P2
    P3MDOUT = 0x7A; // Output configuration for P3

    P4MDOUT = 0x00; // Output configuration for P4
    P5MDOUT = 0x00; // Output configuration for P5
    P6MDOUT = 0x00; // Output configuration for P6
    P7MDOUT = 0x00; // Output configuration for P7

    P1MDIN = 0xFF;  // Input configuration for P1

    SFRPAGE = 0x00;
    EMI0CF = 0x03;    // External Memory Configuration Register

//----------------------------------------------------------------
// Comparators Register Configuration
//
// Bit 7  | Bit 6  | Bit 5  | Bit 4  | Bit 3 | Bit 2 | Bit 1 | Bit 0
//------------------------------------------------------------------     
//  R/W   |    R   |  R/W   |  R/W   |  R/W  |  R/W  |  R/W  |  R/W
//------------------------------------------------------------------
// Enable | Output | Rising | Falling|  Positive     |  Negative    
//        | State  | Edge   | Edge   |  Hysterisis   |  Hysterisis    
//        | Flag   | Int.   | Int.   |  00: Disable  |  00: Disable
//        |        | Flag   | Flag   |  01:  5mV     |  01:  5mV  
//        |        |        |        |  10: 10mV     |  10: 10mV
//        |        |        |        |  11: 20mV     |  11: 20mV 
// ----------------------------------------------------------------

    SFRPAGE = 0x01;
    CPT0MD = 0x03;   // Comparator 0 Configuration Register
    CPT0CN = 0x85;   // Comparator 0 Control Register
    
    SFRPAGE = 0x02;
    CPT1MD = 0x03;   // Comparator 1 Configuration Register
    CPT1CN = 0x85;   // Comparator 1 Control Register
                    
//----------------------------------------------------------------
// Oscillator Configuration
//----------------------------------------------------------------

    ConfigOsc();

//----------------------------------------------------------------
// Reference Control Register Configuration
//----------------------------------------------------------------

    SFRPAGE = 0x00;
    REF0CN = 0x07;  // Reference Control Register

//----------------------------------------------------------------
// ADC0 Configuration
//----------------------------------------------------------------

    SFRPAGE = 0x00;
    AMX0CF = 0x60;  // AMUX Configuration Register
    AMX0SL = 0x00;  // AMUX Channel Select Register
    ADC0CF = 0x68;  // ADC Configuration Register
    ADC0CN = 0xC1;  // ADC Control Register
    
    ADC0LTH = 0x00; // ADC Less-Than High Byte Register
    ADC0LTL = 0x00; // ADC Less-Than Low Byte Register
    ADC0GTH = 0xFF; // ADC Greater-Than High Byte Register
    ADC0GTL = 0xFF; // ADC Greater-Than Low Byte Register

//----------------------------------------------------------------
// ADC2 Configuration
//----------------------------------------------------------------   
/*
    SFRPAGE = 0x01;
    AMX2SL = 0x00;  // AMUX Channel Select Register
    AMX2CF = 0x00;  // AMUX Configuration Register
    ADC2CF = 0xF8;  // ADC Configuration Register
    ADC2LT = 0xFF;  // ADC Less-Than Register
    ADC2GT = 0xFF;  // ADC Greater-Than Register
    ADC2CN = 0x00;  // ADC Control Register
*/
//----------------------------------------------------------------
// DAC Configuration
//----------------------------------------------------------------
/*
    SFRPAGE = 0x00;
    DAC0CN = 0x00;  // DAC0 Control Register
    DAC0L = 0x00;   // DAC0 Low Byte Register
    DAC0H = 0x00;   // DAC0 High Byte Register

    SFRPAGE = 0x01;
    DAC1CN = 0x00;  // DAC1 Control Register
    DAC1L = 0x00;   // DAC1 Low Byte Register
    DAC1H = 0x00;   // DAC1 High Byte Register
*/
//----------------------------------------------------------------
// SPI Configuration
//----------------------------------------------------------------

    SFRPAGE  = 0x00;
    SPI0CFG = 0x70; // SPI Configuration Register
    SPI0CN  = 0x0A; // SPI Control Register
    SPI0CKR = 0x01; // SPI Clock Rate Register

//----------------------------------------------------------------
// UART0 Configuration
//----------------------------------------------------------------

    SFRPAGE = 0x00;
    SADEN0 = 0x00;      // Serial 0 Slave Address Enable
    SADDR0 = 0x00;      // Serial 0 Slave Address Register
    SSTA0 = 0x1F;       // UART0 Status and Clock Selection Register
    SCON0 = 0x72;       // Serial Port Control Register
    RI0 = 0;

    PCON = 0x00;        // Power Control Register

//----------------------------------------------------------------
// UART1 Configuration
//----------------------------------------------------------------
    SFRPAGE = 0x01;    
    SCON1 = 0x12;       // Serial Port 1 Control Register
    RI1 = 0;

//----------------------------------------------------------------
// SMBus Configuration
//----------------------------------------------------------------  
    
    SFRPAGE = 0x00; 
    SMB0CN = 0x00;  // SMBus Control Register
    SMB0ADR = 0x00; // SMBus Address Register
    SMB0CR = 0x00;  // SMBus Clock Rate Register


//----------------------------------------------------------------
// PCA Configuration
//----------------------------------------------------------------

    SFRPAGE = 0x00;
    PCA0MD = 0x08;      // PCA Mode Register
    PCA0CN = 0x40;      // PCA Control Register
    PCA0L = 0x00;       // PCA Counter/Timer Low Byte
    PCA0H = 0x00;       // PCA Counter/Timer High Byte  
    

    //Module 0
    PCA0CPM0 = 0x46;    // PCA Capture/Compare Register 0
    PCA0CPL0 = 0x00;    // PCA Counter/Timer Low Byte
    PCA0CPH0 = 0x0A;    // PCA Counter/Timer High Byte

    //Module 1
    PCA0CPM1 = 0x00;    // PCA Capture/Compare Register 1
    PCA0CPL1 = 0x00;    // PCA Counter/Timer Low Byte
    PCA0CPH1 = 0x00;    // PCA Counter/Timer High Byte

    //Module 2
    PCA0CPM2 = 0x00;    // PCA Capture/Compare Register 2
    PCA0CPL2 = 0x00;    // PCA Counter/Timer Low Byte
    PCA0CPH2 = 0x00;    // PCA Counter/Timer High Byte
    
//----------------------------------------------------------------
// Timers Configuration
//----------------------------------------------------------------

    SFRPAGE = 0x00;
    CKCON = 0x01;   // Clock Control Register "было CKCON = 0x02;" сейчас SYSCLK\4
    // Timer 0
	TL0 = 0xFF;     // Timer 0 Low Byte
    TH0 = 0x00;     // Timer 0 High Byte
	// Timer 1
	TL1 = 0xFF;     // Timer 1 Low Byte
  #ifdef _ITERIS_38400
	CKCON = 0x01;
	TH1 = 0x88;     // Timer 1 High 
	#endif	
  #ifdef _GPS_9600
    CKCON = 0x02;
	TH1 = 0xD8;     // Timer 1 High Byte <at 36 MHz>
  #endif
  #ifdef _GPS_4800
    CKCON = 0x02;
	TH1 = 0xB0;     // Timer 1 High Byte <at 36 MHz>
  #endif
    TMOD = 0x20;    // Timer Mode Register
    TCON = 0x44;    // Timer Control Register 

    //!200Hz!
    TMR2CF = 0x00;  // Timer 2 Configuration
    RCAP2L = 0x00;  // Timer 2 Reload Register Low Byte
    RCAP2H = 0xC4;  // Timer 2 Reload Register High Byte
    TMR2L = 0xFF;   // Timer 2 Low Byte 
    TMR2H = 0xFF;   // Timer 2 High Byte    
    TMR2CN = 0x04;  // Timer 2 CONTROL  
        
    SFRPAGE = 0x01;
    TMR3CF = 0x08;  // Timer 3 Configuration
    RCAP3L = 0x00;  // Timer 3 Reload Register Low Byte
    RCAP3H = 0x00;  // Timer 3 Reload Register High Byte
    TMR3H = 0x00;   // Timer 3 High Byte
    TMR3L = 0x00;   // Timer 3 Low Byte
    TMR3CN = 0x00;  // Timer 3 Control Register

    SFRPAGE = 0x02;
    TMR4CF = 0x08;  // Timer 4 Configuration
    RCAP4L = 0x10;  // Timer 4 Reload Register Low Byte
    RCAP4H = 0xFF;  // Timer 4 Reload Register High Byte
    TMR4H = 0xFF;   // Timer 4 High Byte
    TMR4L = 0xFF;   // Timer 4 Low Byte
    TMR4CN = 0x04;  // Timer 4 Control Register

//----------------------------------------------------------------
// Reset Source Configuration
//
// Bit 7  | Bit 6  | Bit 5  | Bit 4  | Bit 3 | Bit 2 | Bit 1 | Bit 0
//------------------------------------------------------------------     
//    R  |   R/W  |  R/W   |  R/W   |   R   |   R   |  R/W  |  R
//------------------------------------------------------------------
//  JTAG  |Convert | Comp.0 | S/W    | WDT   | Miss. | POR   | HW
// Reset  |Start   | Reset/ | Reset  | Reset | Clock | Force | Pin
// Flag   |Reset/  | Enable | Force  | Flag  | Detect| &     | Reset
//        |Enable  | Flag   | &      |       | Flag  | Flag  | Flag
//        |Flag    |        | Flag   |       |       |       |
//------------------------------------------------------------------
// NOTE! : Comparator 0 must be enabled before it is enabled as a 
// reset source.
//
// NOTE! : External CNVSTR must be enalbed through the crossbar, and
// the crossbar enabled prior to enabling CNVSTR as a reset source 
//------------------------------------------------------------------

    SFRPAGE = 0x00;
    RSTSRC |= 0x04; // Reset Source Register (set bit04-Missing Clock Detector enabled)


//----------------------------------------------------------------
// Interrupt Configuration
//----------------------------------------------------------------

    IE = 0xB4;          //Interrupt Enable
    // Global 0x80
    // /INT0  0x01
    // /INT1  0x04
    // Timer2 0x20
    // UART0  0x10
    IP = 0x00;          //Interrupt Priority
    EIE1 = 0x01;        //Extended Interrupt Enable 1
    // SPI
    EIE2 = 0x41;        //Extended Interrupt Enable 2
    // UART1
    // Timer3
    EIP1 = 0x00;        //Extended Interrupt Priority 1
    EIP2 = 0x01;        //Extended Interrupt Priority 2
    // Timer3
    

// other initialization code here...



}   //End of config


//------------------------------------------------------------------------------------
// Sleep Mode Routines
//------------------------------------------------------------------------------------

//bit _slow_mode = 0;

void SetOscSlowMode()
{
  reset_wdt;
  global_int_no;

    //Step 1. Switch the system clock to the internal oscillator that is running and stable
    SFRPAGE    = CONFIG_PAGE;
    CLKSEL     = 0x00;
    OSCICN     = 0x80;        //f=~3MHz (24.5/8)
    while (!(OSCICN & 0x40)); //wait for osc to run

    //Step 2. Configure flash speed
    CCH0CN    &= ~0xE0;
    SFRPAGE    = LEGACY_PAGE;
    FLSCL      = 0x80;        // FLASH Memory Control
    SFRPAGE    = CONFIG_PAGE;
    CCH0CN    |= 0xE0;

    //Step 3. Disable & power off the PLL. Disable external osc
    PLL0CN     = 0x00;

    //_slow_mode = 1;

  global_int_yes;

    //Step 4. Recalculate timers
    //Timer2
    SFRPAGE = 0x00;
    TR2 = 0; //stop Timer2
    RCAP2L = 0x00;  // Timer 2 Reload Register Low Byte
    RCAP2H = 0xFB;  // Timer 2 Reload Register High Byte
    TR2 = 1; //run Timer2
    //Timer4 - GSM (UART0) clock source
    set_com_speed(0); //0-9600b/s
    //PCA & Receiver
    SFRPAGE = 0x00;
    CR = 0; //stop PCA0 Timer
    PCA0CPH1 = 0x01; //set MAX31XX clock = ~1.5MHz
    //Receiver timing - by '_slow_mode' flag
    CR = 1; //run PCA0 Timer
    //Timer3 - MicroLan DS18B20 timing - by '_slow_mode' flag
}
/*
void SetOscFastMode()
{
  reset_wdt;
  global_int_no;

    //Step 1. Switch the system clock to full speed
    ConfigOsc();

    _slow_mode = 0;

  global_int_yes;

    //Step 2. Recalculate timers
    //Timer2
    SFRPAGE = 0x00;
    TR2 = 0; //stop Timer2
    RCAP2L = 0x00;  // Timer 2 Reload Register Low Byte
    RCAP2H = 0xC4;  // Timer 2 Reload Register High Byte  
    TR2 = 1; //run Timer2
    //Timer4 - GSM (UART0) clock source
    set_com_speed(0); //0-9600b/s
    //PCA & Receiver
    SFRPAGE = 0x00;
    CR = 0; //stop PCA0 Timer
    PCA0CPH1 = 0x0A; //set MAX31XX clock = 1.8432MHz
    //Receiver timing - by '_slow_mode' flag
    CR = 1; //run PCA0 Timer
    //Timer3 - MicroLan DS18B20 timing - by '_slow_mode' flag
}
*/
