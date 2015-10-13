//MicroLan DS INTERFACE

#include <c8051f120.h>
#include "main.h"
#include "ds18b20.h"



#define LAN_DELAY 5 //[сек] - период измерения температуры датчиком MicroLAN



// ************************************************************************************************
//
// ************************************
// *            Переменные            *
// ************************************
//

bit                  bit_data_device;
bit                  bit_lsb_byte_device;
bit                  bit_priority_device;



bit _err_lan = 0; //сигнализирует об ошибке потока данных по микролан: "1" - ошибка
bit _lan_exist = 0; //присутствует ли устройство на шине микролан?: "1" - присутствует

bit                     bit_data_lan; // бит данных приема/передачи по шине микролан
unsigned char xdata     data_in_lan;  // байт данных принимаемый по шине микролан

unsigned char bdata     data_crc_8[1]; // Контр. сумма, расчитываемая по приеме данных по шине
                                       // Контр. сумма вычисляется по 8 битам (использ-ся data_crc_8[0])
sbit                    bit_data0_crc_8  = data_crc_8[0] ^ 0;
sbit                    bit_data1_crc_8  = data_crc_8[0] ^ 1;
sbit                    bit_data2_crc_8  = data_crc_8[0] ^ 2;
sbit                    bit_data3_crc_8  = data_crc_8[0] ^ 3;
sbit                    bit_data4_crc_8  = data_crc_8[0] ^ 4;
sbit                    bit_data5_crc_8  = data_crc_8[0] ^ 5;
sbit                    bit_data6_crc_8  = data_crc_8[0] ^ 6;
sbit                    bit_data7_crc_8  = data_crc_8[0] ^ 7;

bit                     bit_data0_temp;
bit                     bit_datax_temp;

unsigned char xdata     temperature_data_lsb_18b20; // мл. половина значения температуры с 1820
unsigned char xdata     temperature_data_msb_18b20; // ст. половина значения температуры с 1820

unsigned char xdata     Lan_err_count=0; // Счетчик ошибок
  signed char xdata     Lan_temper=0x7F; // Температура

bit VLAN = 0; //перключатель вызова automat_lan() - "0" / automat_vlan() - "1"
bit VLAN_INT = 0; //перключатель вызова automat_vlan() из main() - "0" / из прерывания - "1"
unsigned char xdata StateLAN = 0x01; //состояние automat_lan()
unsigned char data StateVLAN; //состояние automat_vlan()
unsigned char xdata MStateVLAN; //"Master StateVLAN" - исходное состояние
unsigned char xdata byte_out;

union CLANST xdata StLAN = {0x00};
unsigned char xdata context_LAN[8];
unsigned char xdata cnt_LAN;



//Начальная инициализация
void init_lan()
{
   _err_lan                       = no;       

   temperature_data_lsb_18b20     = 0xFF;
   temperature_data_msb_18b20     = 0xFF;
   Lan_temper                     = 0x7F;

   DataLogger[0x0C] = 0x07; //"unknown"
   DataLogger[0x0D] = 0xFF; //"unknown"

   LAN_1;
}

//Расчет 8-битовой контрольной суммы
void crc_8_lan(unsigned char in_crc)
//(перед началом вызова ф-ии установить data_crc_8[0]=0
//по окончании работы ф-ии результат - в data_crc_8[0])
{
unsigned char data cnt;

    for (cnt=0; cnt<8; cnt++)
    {
        bit_data0_temp = (in_crc & 0x01);

        bit_datax_temp  = bit_data0_temp ^ bit_data0_crc_8;
        bit_data0_crc_8 = bit_data1_crc_8;
        bit_data1_crc_8 = bit_data2_crc_8;
        bit_data2_crc_8 = bit_datax_temp ^ bit_data3_crc_8;
        bit_data3_crc_8 = bit_datax_temp ^ bit_data4_crc_8;
        bit_data4_crc_8 = bit_data5_crc_8;
        bit_data5_crc_8 = bit_data6_crc_8;
        bit_data6_crc_8 = bit_data7_crc_8;
        bit_data7_crc_8 = bit_datax_temp;

        in_crc >>= 1;
    }
}


void automat_vlan()
{
    if (VLAN_INT) return; //режим работы по прерыванию - выход...

    switch (StateVLAN)
    {
        //virtual reset_lan() - start
        case 0xA1:
            VLAN_INT = 1; //переключиться в режим работы по прерыванию
            _err_lan = no;
            _lan_exist = no;
            LAN_1;
            LAN_IN;
            StateVLAN = 0xA2;
            VDELAY15; //<по таймеру вызывается automat_vlan() при StateVLAN=0x02>
            return;
        //virtual reset_lan() - finish
        case 0xA5:
            StateVLAN = MStateVLAN; //вернуться в предыдущее состояние
            return;

        //virtual write_bit_lan() - start
        case 0xB1:
            VLAN_INT = 1; //переключиться в режим работы по прерыванию
            LAN_1;
            LAN_IN;
            StateVLAN = 0xB2;
            VDELAY5;
            return;
        //virtual write_bit_lan() - finish
        case 0xB3:
            StateVLAN = MStateVLAN; //вернуться в предыдущее состояние
            return;

        //virtual write_byte_lan()
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            if (bit_data_lan==1) {data_in_lan |= 0x80;} else data_in_lan &= 0x7F;
            data_in_lan >>= 1;
            byte_out >>= 1;
        case 0x00:
            if (byte_out & 0x01) bit_data_lan = 1; else bit_data_lan = 0;
            MStateVLAN = StateVLAN+1; //StateVLAN первоначально задается из automat_lan()
            StateVLAN = 0xB1;
            return;
        case 0x08:
            if (bit_data_lan==1) {data_in_lan |= 0x80;} else data_in_lan &= 0x7F;
            VLAN = 0; //переключиться на automat_lan()
            return;

        //run virtual reset_lan()
        case 0x21:
            MStateVLAN = 0x22;
            StateVLAN = 0xA1;
            return;
        case 0x22:
            VLAN = 0;
            return;
    }
}


void automat_lan()
{
    switch (StateLAN)
    {
        //CONFIG_DEVICE
        case 0x01: //RESET_LAN
            StateLAN = 0x02;
            StateVLAN = 0x21;
            VLAN = 1;
            break;
        case 0x02: //WRITE_BYTE_LAN <0xCC> - Skip ROM command
            StateLAN = 0x03;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xCC;
            break;
        case 0x03: //WRITE_BYTE_LAN <0x4E> - Write to scratchpad config = 3 bytes
            StateLAN = 0x04;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0x4E;
            break;
        case 0x04: //WRITE_BYTE_LAN <0xFF>,<0xFF>
        case 0x05:
            StateLAN++;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x06: //WRITE_BYTE_LAN <0x7F> - 12-bits (<0x1F> - 9-bits) data temperature
            StateLAN = 0x11;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0x7F;
            time_wait_lan = 0;
            break;

        case 0x11: //pause <sec>
            if (time_wait_lan >= LAN_DELAY) StateLAN = 0x21;
            break;

        //RESET_TEMPERATURE
        case 0x21: //RESET_LAN
            StateLAN = 0x22;
            StateVLAN = 0x21;
            VLAN = 1;
            break;
        case 0x22: //WRITE_BYTE_LAN <0xCC> - Skip ROM command
            StateLAN = 0x23;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xCC;
            break;
        case 0x23: //WRITE_BYTE_LAN <0x44> - Skip ROM command
            StateLAN = 0x31;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0x44;
            break;

        //WAIT_TEMPERATURE
        case 0x31: //WRITE_BYTE_LAN <0xFF>
            StateLAN = 0x32;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x32: //check for ready state
            if (data_in_lan == 0xFF) StateLAN = 0x41;
            else StateLAN = 0x31;
            break;

        //Считывание готовых данных температуры

        //READ_READY_DATA_TEMPERATURE
        case 0x41: //RESET_LAN
            _err_lan = no;
            StateLAN = 0x42;
            StateVLAN = 0x21;
            VLAN = 1;
            break;
        case 0x42: //WRITE_BYTE_LAN <0xCC> - Skip ROM command
            StateLAN = 0x43;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xCC;
            break;
        case 0x43: //WRITE_BYTE_LAN <0xBE>
            StateLAN = 0x44;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xBE;
            break;
        case 0x44: //WRITE_BYTE_LAN <0xFF>
            data_crc_8[0] = 0;
            StateLAN = 0x45;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x45: //calc CRC, WRITE_BYTE_LAN <0xFF>
            crc_8_lan(data_in_lan);
            temperature_data_lsb_18b20 = data_in_lan;
            StateLAN = 0x46;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x46: //calc CRC, WRITE_BYTE_LAN <0xFF>
            crc_8_lan(data_in_lan);
            temperature_data_msb_18b20 = data_in_lan;
            StateLAN = 0x47;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x47: //calc CRC, WRITE_BYTE_LAN <0xFF>
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
            crc_8_lan(data_in_lan);
            StateLAN++;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x4D: //check CRC
            if (data_crc_8[0] != data_in_lan) _err_lan = yes;
            StateLAN = 0x61;
            break;

        case 0x61: //get result
            if (_err_lan == yes)
            {
                Lan_err_count++;
                if (Lan_err_count >= 3)
                {
                    Lan_err_count=0;
                    Lan_temper = 0x7F;
                    DataLogger[0x0C] = 0x07; //"unknown"
                    DataLogger[0x0D] = 0xFF; //"unknown"
                }
                StateLAN = (StLAN.b.fGetSN ? 0x90 : 0x81); //"ERROR"
                break;
            }
            //if power on reset value (+85C), set result to "unknown"
            if ((temperature_data_msb_18b20==0x05) && (temperature_data_lsb_18b20==0x50))
            {
                Lan_temper = 0x7F;
                DataLogger[0x0C] = 0x07; //"unknown"
                DataLogger[0x0D] = 0xFF; //"unknown"
            }
            else
            {
                Lan_temper = (temperature_data_msb_18b20 << 4) | (temperature_data_lsb_18b20 >> 4);
                DataLogger[0x0C] = temperature_data_msb_18b20;
                DataLogger[0x0D] = temperature_data_lsb_18b20;
            }
            StateLAN = (StLAN.b.fGetSN ? 0x90 : 0x71); //"END"
            break;

        //END
        case 0x71: //RESET_LAN
            StateLAN = 0x01;
            StateVLAN = 0x21;
            VLAN = 1;
            break;

        //ERROR
        case 0x81: //RESET_LAN
            StateLAN = 0x01;
            StateVLAN = 0x21;
            VLAN = 1;
            break;

        //READ UNIQUE 64-BIT ROM CODE
        case 0x90: //START
            StLAN.b.fGetSN = 0;
            StLAN.b.fFailSN = 0;
            StLAN.b.cntErrSN = 0;
            StateLAN = 0x91;
            break;
        case 0x91: //RESET_LAN
            StateLAN = 0x92;
            StateVLAN = 0x21;
            VLAN = 1;
            break;
        case 0x92: //WRITE_BYTE_LAN <0x33> - Read ROM code
            StateLAN = 0x93;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0x33;
            break;
        case 0x93: //WRITE_BYTE_LAN <0xFF>
            data_crc_8[0] = 0;
            cnt_LAN = 0;
            StateLAN = 0x94;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x94: //calc CRC, WRITE_BYTE_LAN <0xFF>
            crc_8_lan(data_in_lan);
            context_LAN[cnt_LAN] = data_in_lan;
            if (++cnt_LAN >= 7) StateLAN = 0x95;
            StateVLAN = 0x00;
            VLAN = 1;
            byte_out = 0xFF;
            break;
        case 0x95: //check CRC
            context_LAN[cnt_LAN] = data_in_lan;
            if (data_crc_8[0] != data_in_lan)
            {
                if (++StLAN.b.cntErrSN >= 5)
                {
                    StLAN.b.fFailSN = 1;
                    StLAN.b.fReadySN = 1;
                    StateLAN = 0x81;
                    break;
                }
                StateLAN = 0x91;
                break;
            }
            StLAN.b.fReadySN = 1;
            StateLAN = 0x71;
            break;

        default: //?
            StateLAN = 0x01;
            break;
    }
}
