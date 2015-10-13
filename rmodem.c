#include "rmodem.h"
#include "main.h"
#include "uart.h"
#include <c8051f120.h>
#include "io.h"



#define TIME_SWITCH_RM_OUT  5//2 //[сек/10] - время переключения модема на передачу
#define TIME_SWITCH_RM_IN   2//2 //[сек/10] - время перед переключением на прием (для завершения передачи)
#define TIME_START_DATA_RM  2 //[сек/10] - время перед началом ответа на запрос
#define TIMEOUT_REQUEST_RM 50 //[сек/10] - время ожидания данных от automat_request()
#define TIMEOUT_REPLY_RM   15//10 //[сек/10] - время ожидания подтверждения от ЦДП
#define TIME_SKIP_REPLY_RM  4//3 //[сек/10] - время для пропуска входящего пакета
#define TIME_GAP_STREAM_RM  3 //[сек/10] - время, определяющее разрыв входного потока
#define MAX_LEN_PAC_RM    140 //[байт] - максимальная длина поля Command/Data
#define MAX_FAILS_RM_IN     6//15 //[пакетов] - максимальное число непринятых пакетов (сброс сеанса связи)

#define REQ 'C'
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x19



unsigned char code ID_RM[8] _at_ (F_ADDR_SETTINGS+0x12);

struct { //служебная часть пакета RM
    unsigned char hdr; //тип пакета
    unsigned char sn;  //порядковый номер пакета
    unsigned char csn;
    unsigned char id[8];
    unsigned char len; //длина поля Command/Data
    unsigned char rc; //Response-Code
    union {
        unsigned int  i;
        unsigned char c[2];
          } crc; //к. сумма пакета
       } xdata context_RM;
unsigned char xdata * pcontext = &context_RM;

unsigned char xdata sn_RM_inc; //счетчик пакетов RM одного потока

struct { //счетчики однотипных пакетов SMS
    unsigned char npack_31_inc;
    unsigned char npack_33_inc;
    unsigned char npack_35_inc;
    unsigned char npack_37_inc;
    unsigned char npack_39_inc;
    unsigned char npack_3D_inc;
    unsigned char npack_43_inc;
    unsigned char npack_45_inc;
    unsigned char npack_47_inc;
    unsigned char npack_49_inc;
    unsigned char npack_4D_inc;
    unsigned char npack_51_inc;
       } xdata sn_SMS = {0};

unsigned char get_sn_SMS(unsigned char N)
{
    switch (N)
    {
        case 0x31: return sn_SMS.npack_31_inc++;
        case 0x33: return sn_SMS.npack_33_inc++;
        case 0x35: return sn_SMS.npack_35_inc++;
        case 0x37: return sn_SMS.npack_37_inc++;
        case 0x39: return sn_SMS.npack_39_inc++;
        case 0x3D: return sn_SMS.npack_3D_inc++;
        case 0x43: return sn_SMS.npack_43_inc++;
        case 0x45: return sn_SMS.npack_45_inc++;
        case 0x47: return sn_SMS.npack_47_inc++;
        case 0x49: return sn_SMS.npack_49_inc++;
        case 0x4D: return sn_SMS.npack_4D_inc++;
        case 0x51: return sn_SMS.npack_51_inc++;
        default  : return 0; //нет счетчика
    }
}

unsigned char xdata StateRM = 0x01;
unsigned char xdata time_wait_RM = 0;
unsigned char xdata cnt_RM;
unsigned char xdata fail_cnt_RM;
unsigned int  xdata acc_crc_RM; //CRC of Data Block Accumulator

bit FastMode_RM = 0; //"1" - "быстрый" режим (радиоканал не используется)
bit RM_Enable = 0; //исп. для переключения между [SMS/MODEM - Req/Alarm] : [RM - Req]

union CRMST xdata StRM = {0x00}; //доп. параметры состояния работы с модемом

unsigned char xdata M[6] = {':',ID_TERM1,VERSION,ID_TERM2,BUILD,13};



free_input_stream() //освободить буфер ввода
{
    if (!(IOStreamState.In & 0xC000)) return; //буфер ужЕ свободен
    if (StateRequest > 0x01) //обработка запроса
    {
        StateRequest = 0xFF;
        SearchEnable = 0;
        StateSearch = 0x00;
    }
    else
        StateRequest = 0x00; //запрос по каналу GSM удален...
}

bit free_output_stream() //попытка освободить буфер вывода
{
    if (IOStreamState.Out & 0x4000) return 0; //буфер используется при выводе и не может быть освобожден
    if (Auto_Enable) return 1;
    if ((StateAlarm == 0x23) || (StateAlarm == 0xFF)) //тревога в буфере
    {
        if ((IOStreamState.Out & 0x9B00) == 0x9000) //обычная тревога записана в буфер
        {
            IOStreamState.Out = 0x1000;
            StateSendSMS &= ~0x40;
            StateSendSMS |= 0x04;
        }
        else
        if (((IOStreamState.Out & 0x9B00) == 0x9800) || (StateSendSMS & 0x10)) //авто-тревога
        {
            IOStreamState.Out = 0x1800;
            StateSendSMS &= ~0x10;
            StateSendSMS |= 0x01;
        }
        return 0; //буфер еще не освобожден
    }
    if (StateRequest > 0x01) //обработка запроса
    {
        StateRequest = 0xFF;
        IOStreamState.Out = 0x2000; //?
        SearchEnable = 0;
        StateSearch = 0x00;
    }
    return 0; //буфер еще не освобожден
}

void fail_reset_rmodem() //.return to Initial State. /break off automat_request()
{
    if (RM_Enable) //запрос по радиомодему
    {
        IOStreamState.Out = 0x0000;
        StateRequest = 0xFF;
        SearchEnable = 0;
        StateSearch = 0x00;
    }
    if (FastMode_RM)
    {
        Init_max(0x0D); //set 1200 b/s
        pcontext = &context_RM;
        max_receive();
    }
    ClearBuf_max();
    StateRM = 0x01; //starting request
}

void check_gap_rmodem() //.check for gap of incoming data.
{
    if (time_wait_RM < TIME_GAP_STREAM_RM) return;

    StateRM &= 0x10;
    StateRM |= 0x0E; //"Receive Error"/"Receive Reply Error"
}

void automat_rmodem()
{
unsigned char data c;
unsigned int data i;

    switch (StateRM)
    {
        case 0x00: //.dead.
        break;


        //Receive Mode

        case 0x01: //.starting request.get Preamble [___'C'___'C'].
            if (!receive_char_max()) break;
            if (char_max == REQ)
            {
                FastMode_RM = 0;
                StateRM = 0x02;
                break;
            }
            if (char_max == 'J') //загрузчик
            {
                pcontext = M;
                OutMAXPointer2 = 6;
                time_wait_RM = 0;
                StateRM = 0x91;
            }
        break;

        case 0x91: //загрузчик
            if (time_wait_RM >= 30) StateRM = 0x01;
            if (OutMAXPointer2 || !receive_char_max()) break;
            if (char_max == 'L')
            {
                pcontext[0] = '<';
                OutMAXPointer2 = 1;
                send_max();
                RestartToLoader();
            }
            if (char_max == 'J')
            {
                pcontext = M;
                OutMAXPointer2 = 6;
                time_wait_RM = 0;
            }
        break;

        case 0x11: //.processing request.get Preamble [___'C'___'C'].
            if (receive_char_max() && (char_max == REQ))
            {
                time_wait_RM = 0;
                StateRM = 0x12;
                break;
            }
            if (time_wait_RM >= TIMEOUT_REPLY_RM)
            {
                fail_cnt_RM++; //.force "Fail Breakdown".
                StateRM = 0x1E; //"Receive Reply Error"
            }
        break;

        case 0x02: //.starting request.check Header [SOH].
            if (!receive_char_max()) break;
            switch (char_max)
            {
                case REQ: //.still Preamble.
                break;

                case SOH: //.got request.
                    context_RM.hdr = SOH;
                    context_RM.rc = 0x00; //"Ok"
                    free_input_stream();
                    IOStreamState.In = 0x4200; //информация о режиме; "буфер занят"
                    time_wait_RM = cnt_RM = 0;
                    StateRM = 0x03;
                break;

                default:
                    StateRM = 0x0E; //"Receive Error"
                break;
            }
        break;

        case 0x12: //.processing request.get Header [ACK/NAK/EOT].
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            switch (char_max)
            {
                case REQ: //.still Preamble.
                break;

                case ACK: //.got "ack".
                case NAK: //.got "nak".
                case EOT: //.got "end".
                    context_RM.hdr = char_max;
                    context_RM.rc = 0x00; //"Ok"
                    cnt_RM = 0;
                    StateRM = 0x13;
                break;

                default:
                    StateRM = 0x1E; //"Receive Reply Error"
                break;
            }
        break;

        case 0x03: //.starting request.check Sequence Number.
        case 0x13: //.processing request.check Sequence Number.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            if (cnt_RM)
            {
                if (char_max != ~context_RM.sn)
                {
                    StateRM += 0xB; //"Receive Error"/"Receive Reply Error"
                    break;
                }
                context_RM.csn = char_max;
                cnt_RM = 0;
                StateRM++;
                break;
            }
            context_RM.sn = char_max;
            cnt_RM = 1;
        break;

        case 0x04: //.starting request.check Address.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            //адреса при передаче по модему не должны содержать FF в первых 4-х байтах
            //широковещательный адрес "быстрого режима" - [FF FF FF FF XX XX XX XX]
            switch (cnt_RM)
            {
                case 0x00: //.set probable mode (modem/direct).
                    if (char_max == 0xFF) {FastMode_RM = 1; cnt_RM = 0x10;} //check broadcasting address
                    if (char_max == ID_RM[0]) {cnt_RM = 0x00; break;} //check modem address
                    if (!FastMode_RM) {StateRM = 0x0E; break;} //.wrong address. /"Receive Error"
                break;

                case 0x01: //.check modem/broadcasting address.
                case 0x02: //.check modem/broadcasting address.
                case 0x03: //.check modem/broadcasting address.
                    if (char_max != 0xFF) FastMode_RM = 0;
                    if ((char_max != ID_RM[cnt_RM]) && !FastMode_RM) StateRM = 0x0E; //.wrong address. /"Receive Error"
                break;

                case 0x04: //.check modem/broadcasting address.
                    if (FastMode_RM) //.broadcasting address detected.
                    {
                        context_RM.id[4] = char_max;
                        cnt_RM = 0x14;
                        break;
                    }
                case 0x05: //.check modem address.
                case 0x06: //.check modem address.
                    if (char_max != ID_RM[cnt_RM]) StateRM = 0x0E; //.wrong address. /"Receive Error"
                break;

                case 0x07: //.modem address detected?. /skip checking last address byte
                    for (c=0; c<7; c++) context_RM.id[c] = ID_RM[c];
                    context_RM.id[7] = char_max;
                    StateRM = 0x05;
                break;

                case 0x11: //.check broadcasting address.
                case 0x12: //.check broadcasting address.
                case 0x13: //.check broadcasting address.
                    if (char_max != 0xFF) StateRM = 0x0E; //.wrong address. /"Receive Error"
                break;

                case 0x14: //.get broadcasting address.
                case 0x15: //.get broadcasting address.
                case 0x16: //.get broadcasting address.
                    context_RM.id[cnt_RM & 0x0F] = char_max;
                break;

                case 0x17: //."fast" mode enabled.
                    for (c=0; c<4; c++) context_RM.id[c] = 0xFF;
                    context_RM.id[7] = char_max;
                    StateRM = 0x05;
                break;
            }
            cnt_RM++;
        break;

        case 0x14: //.processing request.check Address.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            if (cnt_RM >= 7)
            {
                context_RM.id[7] = char_max;
                StateRM = 0x15;
                break;
            }
            if (char_max != context_RM.id[cnt_RM++]) StateRM = 0x1E; //"Receive Reply Error"
        break;

        case 0x05: //.starting request.get Length.
        case 0x15: //.processing request.get Length.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            if (char_max > MAX_LEN_PAC_RM)
            {
                StateRM += 0x9; //"Receive Error"/"Receive Reply Error"
                break;
            }
            acc_crc_RM = cnt_RM = 0;
            StateRM++;
            if (context_RM.len = char_max) break;
            StateRM++; //if no data block, jump to .check CRC.
        break;

        case 0x06: //.starting request.get Command Data.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            if (context_RM.rc==0) IOStreamBuf.In[cnt_RM] = char_max;
            acc_crc_RM += char_max;
            if (++cnt_RM >= context_RM.len)
            {
                cnt_RM = 0;
                StateRM = 0x07;
            }
        break;

        case 0x16: //.processing request.get Command Data. /skip Data :)
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            acc_crc_RM += char_max;
            if (++cnt_RM >= context_RM.len)
            {
                cnt_RM = 0;
                StateRM = 0x17;
            }
        break;

        case 0x07: //.starting request.check CRC.
        case 0x17: //.processing request.check CRC.
            if (!receive_char_max()) {check_gap_rmodem(); break;}
            time_wait_RM = 0;
            context_RM.crc.c[cnt_RM] = char_max;
            if (cnt_RM++)
            {
                cnt_RM = 0;
                time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
                StateRM++;
                pcontext = &context_RM;
                for (i=acc_crc_RM,c=0; c<12; c++) i += pcontext[c];
                if (i != context_RM.crc.i) StateRM++;
            }
        break;

        case 0x08: //.starting request.CRC Ok.
            if (time_wait_RM < TIME_SWITCH_RM_OUT) break;
            time_wait_RM = 0;
            max_transmit(); //.send Preamble.
            if (free_output_stream()) RM_Enable = 1;
            if (++cnt_RM >= 3) //.Preamble sent.
            {
                if (RM_Enable) //обрабатывается ТОЛЬКО запрос по радиомодему
                    IOStreamState.In = 0x8200 | context_RM.len; //.pass to automat_request.
                else
                    if (cnt_RM >= 10) //unsuccessfully
                    {
                        //Input/Output Stream inaccessible or Req/Alarm BUSY
                        context_RM.rc = 0x10; //"Program I/O/R/A BUSY"
                        IOStreamState.In = 0x0000;
                    }
                if (RM_Enable || context_RM.rc)
                {
                    if ((context_RM.id[0] == 0xFF) && (context_RM.id[1] == 0xFF) &&
                        (context_RM.id[2] == 0xFF) && (context_RM.id[3] == 0xFF)) //установить ID
                        for (c=0; c<8; c++) context_RM.id[c] = ID_RM[c];
                    StateRM = 0x21; //.send "ack".
                }
            }
        break;

        case 0x09: //.starting request.CRC Err.
            if (time_wait_RM < TIME_SWITCH_RM_OUT) break;
            time_wait_RM = 0;
            max_transmit(); //.send Preamble.
            if (++cnt_RM >= 3) //.Preamble sent.
            {
                context_RM.rc = 0x0C; //"CRC Error"
                if ((context_RM.id[0] == 0xFF) && (context_RM.id[1] == 0xFF) &&
                    (context_RM.id[2] == 0xFF) && (context_RM.id[3] == 0xFF)) //установить ID
                    for (c=0; c<8; c++) context_RM.id[c] = ID_RM[c];
                StateRM = 0x31; //.send "nak".
            }
        break;

        case 0x0E: //.Receive Error. /stay on Receive Mode
            if (IOStreamState.In & 0x0200) IOStreamState.In = 0x0000;
            StateRM = 0x01;
        break;

        case 0x1E: //.Receive Reply Error. /repeat last outsending packet
            if (time_wait_RM < TIME_SKIP_REPLY_RM) break; //skip incoming packet
            cnt_RM = 0;
            time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
            ClearBuf_max();
            if (++fail_cnt_RM >= MAX_FAILS_RM_IN)
            {
                if (RM_Enable) //запрос по радиомодему
                {
                    IOStreamState.Out = 0x0000;
                    StateRequest = 0xFF;
                    SearchEnable = 0;
                    StateSearch = 0x00;
                }
                context_RM.rc = 0x0F; //"Fail Breakdown"
                StateRM = 0x1B; //.fail breakdown. /switch to Transmit Mode
            }
            else StateRM = 0x19; //.repeat last packet. /switch to Transmit Mode
        break;

        case 0x18: //.processing request.CRC Ok.
            fail_cnt_RM = 0;
            switch (context_RM.hdr)
            {
                case ACK: //.send new packet.
                    i = SMS_Tail[0] = SMS_Tail[1];
                    SMS_Tail[1] = 0;
                    sn_RM_inc++;
                    if (IOStreamState.Out & 0x0400) //обработка запроса в automat_request() окончена
                    {
                        if (i==0) //все пакеты ушли
                        {
                            IOStreamState.Out = 0x0000;
                            cnt_RM = 0;
                            time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
                            context_RM.rc = 0x00; //"Ok"
                            StateRM = 0x1B; //switch to Transmit Mode
                            break;
                        }
                        IOStreamState.Out &= 0xFF00;
                    }
                    else //получить новые данные в automat_request()
                    {
                        IOStreamState.Out &= 0x3F00;
                        time_wait_RM = 0;
                    }
                    //сдвиг хвоста в начало буфера
                    for (c=0; c<i; c++) IOStreamBuf.Out[c] = IOStreamBuf.Out[c+SizeBufOut];
                    IOStreamState.Out |= i;
                    cnt_RM = 0;
                    time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
                    StateRM = 0x1A; //switch to Transmit Mode
                break;

                case NAK: //.send last transmitted packet.
                    cnt_RM = 0;
                    time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
                    StateRM = 0x19; //switch to Transmit Mode
                break;

                case EOT: //jump to .starting request.
                    fail_reset_rmodem();
                break;
            }
        break;

        case 0x19: //.processing request.got "nak"/CRC Err. /switch to Transmit Mode
        case 0x1A: //.processing request.got "ack"/timeout expired. /switch to Transmit Mode
        case 0x1B: //.processing request.got last "ack"/fail breakdown. /switch to Transmit Mode
            if (time_wait_RM < TIME_SWITCH_RM_OUT) break;
            time_wait_RM = 0;
            max_transmit(); //.send Preamble.
            if (FastMode_RM || (++cnt_RM >= 3)) //.Preamble sent.
                switch (StateRM)
                {
                    case 0x19: StateRM = 0x44; break; //.send last transmitted packet.
                    case 0x1A: StateRM = 0x43; break; //.send new packet.
                      default: StateRM = 0x51; break; //.send "eot".
                }
        break;


        //Transmit Mode

        case 0x21: //.starting request.send "ack".
            if (OutMAXPointer2) break;
            context_RM.hdr = ACK;
            context_RM.len = 1;
            pcontext = &context_RM;
            for (context_RM.crc.i=c=0; c<13; c++) context_RM.crc.i += pcontext[c];
            OutMAXPointer2 = 15;
            if (context_RM.rc==0) StateRM = 0x41; //.start processing request.
            else StateRM = 0x32; //.end.
        break;

        case 0x31: //.starting request.send "nak".
            if (OutMAXPointer2) break;
            context_RM.hdr = NAK;
            context_RM.len = 1;
            pcontext = &context_RM;
            for (context_RM.crc.i=c=0; c<13; c++) context_RM.crc.i += pcontext[c];
            OutMAXPointer2 = 15;
            StateRM = 0x32;
         break;

        case 0x32: //.starting request.wait while outsending packet.
        case 0x52: //.processing request.wait while outsending packet.
        case 0x41: //.prep to send Terminal Data.wait while outsending packet.
        case 0x47: //.send Terminal Data.wait while outsending packet.
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            time_wait_RM = 0;
            StateRM++;
        break;

        case 0x33: //.starting request.switch to Receive Mode. /jump to Initial State
            if (time_wait_RM < TIME_SWITCH_RM_IN) break;
            max_receive();
            ClearBuf_max();
            StateRM = 0x01; //starting request
        break;

        case 0x42: //.prep to send Terminal Data.pause.
            if (time_wait_RM < TIME_START_DATA_RM) break;
            if (FastMode_RM) Init_max(0x00); //set 115.2k b/s
            fail_cnt_RM = sn_RM_inc = 0; //уст. порядковый номер пакета "0", счетчик непринятых пакетов "0"
            cnt_RM = 1; //?
            time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
            StateRM = 0x1A; //.send Preamble.
        break;

        case 0x43: //.send Terminal Data.send RM header. /build all packet
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            if (!(IOStreamState.Out & 0x8000)) //нет данных для отправки
            {
                if (!RM_Enable) //?/обработка запроса окончена, ответа нет/?
                {
                    context_RM.rc = 0x00; //"Ok"
                    StateRM = 0x51; //send "end"
                    break;
                }
                if (!FastMode_RM && (time_wait_RM >= TIMEOUT_REQUEST_RM))
                {
                    //время передачи истекло - требуется переключить RTS
                    pcontext = &context_RM;
                    max_receive();
                    cnt_RM = 1; //!
                    time_wait_RM = TIME_SWITCH_RM_OUT; //no delay
                    StateRM = 0x1A; //.back to transmit.
                }
                break;
            }
            c = IOStreamState.Out & 0x00FF;
            if (c==0) //буфер пустой
            {
                if (IOStreamState.Out & 0x0400) //последний пакет
                {
                    context_RM.rc = 0x00; //"Ok"
                    StateRM = 0x51; //send "end"
                }
                SMS_Tail[0] = SMS_Tail[1] = 0;
                IOStreamState.Out = 0x0000;
                break;
            }
            IOStreamState.Out |= 0x4000; //"буфер IOStream используется"
            //form SMS packet header
            IOStreamBuf.Header.id = IOStreamState.ReqID;
            IOStreamBuf.Header.tail = 5 + SMS_Tail[0];
            IOStreamBuf.Header.sn = get_sn_SMS(IOStreamState.ReqID);
            IOStreamBuf.Header.res = 0x00;
            IOStreamBuf.Header.fmt = ((IOStreamState.ReqID==0x31)||(IOStreamState.ReqID==0x3D)) ? 0x80 : 0x00;
            if (c > SizeBufOut) c = SizeBufOut; //хвост есть - не последний пакет
            else if (IOStreamState.Out & 0x0400) //последний пакет
                    IOStreamBuf.Header.fmt |= 0x01;
            //form RM packet header
            context_RM.hdr = SOH;
            context_RM.sn = sn_RM_inc;
            context_RM.csn = ~context_RM.sn;
            context_RM.len = 5 + c;
            pcontext = &IOStreamBuf.Header;
            for (i=c=0; c<context_RM.len; c++) i += pcontext[c];
            pcontext = &context_RM;
            for (c=0; c<12; c++) i+= pcontext[c];
            context_RM.crc.i = i;
            OutMAXPointer2 = 12;
            StateRM = 0x45;
        break;

        case 0x44: //.send Terminal Data.send RM header <again>. /send last transmitted packet
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            c = IOStreamState.Out & 0x00FF;
            if (c > SizeBufOut) c = SizeBufOut;
            //form RM packet header <again>
            context_RM.hdr = SOH;
            context_RM.sn = sn_RM_inc;
            context_RM.csn = ~context_RM.sn;
            context_RM.len = 5 + c;
            pcontext = &IOStreamBuf.Header;
            for (i=c=0; c<context_RM.len; c++) i += pcontext[c];
            pcontext = &context_RM;
            for (c=0; c<12; c++) i+= pcontext[c];
            context_RM.crc.i = i;
            OutMAXPointer2 = 12;
            StateRM = 0x45;
        break;

        case 0x45: //.send Terminal Data.send SMS data.
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            pcontext = &IOStreamBuf.Header;
            OutMAXPointer2 = context_RM.len;
            StateRM = 0x46;
        break;

        case 0x46: //.send Terminal Data.send RM CRC.
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            pcontext = &context_RM.crc;
            OutMAXPointer2 = 2;
            StateRM = 0x47;
        break;

        case 0x48: //.send Terminal Data.switch to Receive Mode.
            if (!FastMode_RM)
            {
                if (time_wait_RM < TIME_SWITCH_RM_IN) break;
                pcontext = &context_RM;
                max_receive();
            }
            ClearBuf_max();
            time_wait_RM = 0;
            StateRM = 0x11; //receive "ack"/"nak"/"end"
        break;

        case 0x51: //.processing request.send "end".
            if (OutMAXPointer2) break; //отправка пакета в MAX...
            context_RM.hdr = EOT;
            context_RM.sn = sn_RM_inc;
            context_RM.csn = ~context_RM.sn;
            context_RM.len = 1;
            pcontext = &context_RM;
            for (context_RM.crc.i=c=0; c<13; c++) context_RM.crc.i += pcontext[c];
            OutMAXPointer2 = 15;
            StateRM = 0x52;
        break;

        case 0x53: //.processing request.switch to Receive Mode. /jump to Initial State
            if (FastMode_RM) Init_max(0x0D); //set 1200 b/s
            else
                if (time_wait_RM < TIME_SWITCH_RM_IN) break;
            max_receive();
            ClearBuf_max();
            StateRM = 0x01; //starting request
            if (StRM.b.fNeedRst) //запрошен рестарт процессора
            {
                reset_code = 0xC1;
                SOFT_RESET; //Software Reset uC
            }
        break;
    }
}
