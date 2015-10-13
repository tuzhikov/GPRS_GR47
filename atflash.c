//Работа с модулем памяти Atmel DataFlash AT45DB041B

#include "atflash.h"
#include "main.h"
#include <c8051f120.h>



// A t m e l   D a t a F l a s h   V a r i a b l e s

unsigned char  data StateIntAF = 0x00;
unsigned char xdata * BufAF; //указатель на буфер записи/чтения
unsigned int  xdata BufEndAF;
unsigned char xdata BufTailAF;

union AFHEADER xdata WrHeader = {0x0}; //параметры записи в журнал
union AFHEADER xdata RdHeader = {0x0}; //параметры чтения из журнала

unsigned char xdata * HeaderAF; //указатель на текущий Header



// A t m e l   D a t a F l a s h   F u n c t i o n s

void WaitSPI()
{
    while (StateIntAF) reset_wdt;
}

void HoldSPI_AF() //ожидание освобождения и "захват" шины SPI (for AtFlash Logger)
{
    while (!CSEL1) reset_wdt;
    SFRPAGE = SPI0_PAGE;
    int_ext1_no;
    CSEL1 = 0;
    SPI0CFG = 0x70;
    SPI0CKR = 0x00;
    SPI0CN  = 0x0B;
    reset_wdt; //pause
}

void WaitReadyAF()
{
    HoldSPI_AF();

    StateIntAF = 0x11;
    
    SPI0DAT = 0x57;

    HoldSPI_AF();
}

void WriteAF() //записать блок в DataFlash
{
    WaitReadyAF(); //ожидание готовности SPI и DataFlash
    HeaderAF = &WrHeader;

    switch (WrHeader.t.Byte)
    {
        case 0x00: //new page:
            StateIntAF = 0x41;
            BufTailAF = 128; //остаток байт до конца страницы: 128 = 264 - 8 - 128
            break;

        case 0x80:
        //1) next page erase
            Int = WrHeader.t.Page;
            //jump to next page
            if (WrHeader.t.Page >= MaxPageAF) WrHeader.t.Page = 0x000;
            else WrHeader.t.Page++;
            WrHeader.t.Cmd = 0x81; //"PAGE ERASE"
            StateIntAF = 0x31;
            SPI0DAT = WrHeader.c[0];

            HoldSPI_AF();
            WrHeader.t.Page = Int; //return to current page
        //2) current page: a)buffer write, b)page program without erase
            StateIntAF = 0x01;
            break;
    }

    BufEndAF = BufAF + 128; //128 - длина блока записываемых данных
    WrHeader.t.Cmd  = 0x84; //"BUFFER WRITE"
    SPI0DAT = WrHeader.c[0];

    WaitReadyAF(); //ожидание готовности SPI
    WrHeader.t.Cmd = 0x88; //"BUFFER TO MAIN MEMORY PAGE PROGRAM WITHOUT BUILT-IN ERASE"
    StateIntAF = 0x31;
    SPI0DAT = WrHeader.c[0];

    WaitSPI();

    if (WrHeader.t.Byte == 0x00) WrHeader.t.Byte = 0x80;
    else
    {
        WrHeader.t.Byte = 0x00;
        if (WrHeader.t.Page >= MaxPageAF) WrHeader.t.Page = 0x000;
        else WrHeader.t.Page++;
    }
}

void ReadAF() //считать блок из DataFlash
{
    BufEndAF = BufAF + 128; //128 - длина блока считываемых данных

    WaitReadyAF(); //ожидание готовности SPI и DataFlash
    HeaderAF = &RdHeader;

    RdHeader.t.Cmd = 0x68; //"CONTINUOUS ARRAY READ"
    StateIntAF = 0x21;
    SPI0DAT = RdHeader.c[0];

    WaitSPI(); //ожидание готовности SPI
}

void InitAF()
{
    pChar = FlashDump + 127; //pointer to CheckSum

    while (1)
    {
        BufAF = FlashDump;
        ReadAF();

        reset_wdt;

        Char = 0;
        BufAF = FlashDump;
        while (BufAF < pChar) Char += *BufAF++; //вычисление контрольной суммы
        if ((Char ^ mLogCS) != *pChar) break; //если контрольная сумма некорректна, окончание поиска...

        if (RdHeader.t.Byte == 0x00) RdHeader.t.Byte = 0x80;
        else
        {
            RdHeader.t.Byte = 0x00;
            if (RdHeader.t.Page >= MaxPageAF) //весь объем журнала просмотрен...
            {
                RdHeader.t.Page = 0x000;
                break;
            }
            else RdHeader.t.Page++;
        }
    }

    WrHeader.l = RdHeader.l;
    reset_wdt;


    //***подготовка текущей страницы***

    HeaderAF = &WrHeader;
    HoldSPI_AF();

    if (WrHeader.t.Byte == 0x00) //только стирание "PAGE ERASE"
    {
        WrHeader.t.Cmd = 0x81;
        StateIntAF = 0x31;
        SPI0DAT = WrHeader.c[0];
    }
    else
    {
    //1)"MAIN MEMORY PAGE TO BUFFER TRANSFER"
        WrHeader.t.Cmd  = 0x53;
        StateIntAF = 0x31;
        SPI0DAT = WrHeader.c[0];
    //2)"BUFFER WRITE"
        WaitReadyAF(); //?
        BufAF = 0;
        BufTailAF = 256 - WrHeader.t.Byte;
        WrHeader.t.Cmd = 0x84;
        StateIntAF = 0x51;
        SPI0DAT = WrHeader.c[0];
    //3)"BUFFER TO MAIN MEMORY PAGE PROGRAM WITH BUILT-IN ERASE"
        HoldSPI_AF();
        WrHeader.t.Cmd = 0x83;
        StateIntAF = 0x31;
        SPI0DAT = WrHeader.c[0];
    }
} //void InitAF()
