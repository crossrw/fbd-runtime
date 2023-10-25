#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fbdrt.h"

// #define SILENT
#define CICLCOUNT 1000000

// date: 25-10-2023 20:04:30
// file name: C:\Users\al\Documents\fbd2Projects\event.fbdbin
// title: без имени
// required RTL version: 10
// HMI: used
// netvar: not used
// elements count: 8
// sizeof(tSignal) = 4
// sizeof(tElemIndex) = 2
const unsigned char description[] = {0x25, 0x25, 0x25, 0x25, 0x20, 0x00, 0x01, 0x01, 0x94, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x07, 0x00, 0x06, 0x00, 0x04, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00, 
0xC8, 0x00, 0x00, 0x00, 0x01, 0x25, 0x00, 0x00, 0x90, 0x01, 0x00, 0x00, 0x02, 0x26, 0x00, 0x00, 0x58, 0x02, 0x00, 0x00, 0x03, 0x27, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xE8, 0x03, 0x00, 0x00, 0xB8, 0x0B, 0x00, 0x00, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE5, 0x0C, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x00, 0x00, 
0x00, 0x04, 0x00, 0x00, 0x00, 0xC8, 0x40, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xE1, 0xE5, 0xE7, 0x20, 0xE8, 0xEC, 0xE5, 0xED, 0xE8, 0x00, 0x31, 0x00, 0x32, 0x35, 0x2D, 
0x31, 0x30, 0x2D, 0x32, 0x33, 0x20, 0x32, 0x30, 0x3A, 0x30, 0x34, 0x00, 0xFF, 0x02, 0x00, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x20, 0x32, 0x00, 0x02, 0x01, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x20, 0x33, 
0x00, 0x02, 0x02, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x20, 0x34, 0x00, 0x02, 0x03, 0x45, 0x76, 0x65, 0x6E, 0x74, 0x20, 0x35, 0x00, 0x8C, 0xE3, 0xE5, 0xD4};

#define MEM_SIZE 4096

char memory[MEM_SIZE];
tSignal batram[1024];


void msleep(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

tSignal FBDgetProc(char type, tSignal index)
{
    time_t my_time;
    struct tm * timeinfo;
    //
    time(&my_time);
    timeinfo = localtime (&my_time);
    switch(type) {
    case FBD_PIN:
        // printf(" request InputPin(%d)\n", index);
        // имитация RTC
        switch(index) {
            case GP_RTC_HOUR:
                return timeinfo->tm_hour;
            case GP_RTC_MINUTE:
                return timeinfo->tm_min;
            case GP_RTC_SECOND:
                return timeinfo->tm_sec;
            case GP_RTC_DAY:
                return timeinfo->tm_mday;
            case GP_RTC_MONTH:
                return timeinfo->tm_mon+1;
            case GP_RTC_YEAR:
                return timeinfo->tm_year+1900;
            default:
                return 0;
        }
    case FBD_NVRAM:
        // printf(" request NVRAM(%d)\n", index);
        return batram[index];
    case FBD_HRDW:
        // printf(" request HRDW(%d)\n", index);
        switch(index) {
            case FBD_HRDW_ETH:
                // статус Ethernet
                return 1;
            case FBD_HRDW_NTP:
                // статус NTP
                return 1;
            case FBD_HRDW_TZO:
                // смещение GMT в минутах
                return 3*60;
            case FBD_HRDW_BAT:
                // напряжение батареи в 0.01
                return 360;
            default:
                return 0;
        }
        return 0;
    }
    return 0;
}

void FBDsetProc(char type, tSignal index, tSignal *value)
{
#ifndef SILENT
    switch(type)
    {
    case FBD_PIN:
        // printf(" set OutputPin(%d) to value %d\n", index, *value);
        break;
    case FBD_NVRAM:
        // printf(" set NVRAM(%d) to value %d\n", index, *value);
        batram[index] = *value;
        break;
    }
#endif
}

#ifdef USE_HMI
// рисование графических примитивов
//
// рисование залитого прямоугольника
void FBDdrawRectangle(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color)
{
    printf("draw rectangle: x1:%d y1:%d x2:%d y2:%d color:%d\n", x1, y1, x2, y2, color);
}
// рисование текста
void FBDdrawText(tScreenDim x1, tScreenDim y1, unsigned char font, tColor color, tColor bkcolor, bool transparent, char *text)
{
    printf("draw text: x1:%d y1:%d font:%d color:%d bkcolor:%d text:'%s'\n", x1, y1, font, color, bkcolor, text);
}
// рисование линии
void FBDdrawLine(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color)
{
    printf("draw line: x1:%d y1:%d x2:%d y2:%d color:%d\n", x1, y1, x2, y2, color);
}
// рисование залитого эллипса
void FBDdrawEllipse(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color)
{
    printf("draw ellipse: x1:%d y1:%d x2:%d y2:%d color:%d\n", x1, y1, x2, y2, color);
}
// рисование картинки
void FBDdrawImage(tScreenDim x1, tScreenDim y1, tScreenDim image)
{
    printf("draw image: x1:%d y1:%d index:%d\n", x1, y1, image);
}
// завершение рисования экрана (копирование видеообласти)
void FBDdrawEnd(void)
{
    printf("draw end -------------------------------------\n");
}
#endif

int main(void)
{
    int res,i;

    // DWORD start, end;
    //
    res = fbdInit(description);
    if(res <= 0) {
        printf("result = %d\n", res);
        return 0;
    }
    if(res > MEM_SIZE) {
        printf("not enough memory\n");
        return 0;
    }
    //
    fbdSetMemory(memory, true);
    printf("memory request size = %d\n", res);
    //
    printf("FBD_OPT_REQ_VERSION: %d\n",    FBD_REQ_VERSION);
    printf("FBD_NETVAR_USE: %d\n",         FBD_NETVAR_USE);

    printf("FBD_NETVAR_PORT: %d\n",        FBD_NETVAR_PORT);
    printf("FBD_NETVAR_GROUP: %d\n",       FBD_NETVAR_GROUP);
    printf("FBD_SCREEN_COUNT: %d\n",       FBD_SCREEN_COUNT);
    printf("FBD_SCHEMA_SIZE: %d\n",        FBD_SCHEMA_SIZE);
    printf("FBD_HINTS_COUNT: %d\n",        FBD_HINTS_COUNT);
    printf("FBD_EVENTS_COUNT: %d\n",       FBD_EVENTS_COUNT);

    // main loop
    // start = GetTickCount();

    // printf("IO hints:\n");
    // for(i=0; i<16; i++) {
    //     printf(" type=0 index=%d text:'%s'\n", i, fbdHMIgetIOhint(0, i));
    //     printf(" type=1 index=%d text:'%s'\n", i, fbdHMIgetIOhint(1, i));
    //     printf("\n");
    // }

    i=0;
    while(i < CICLCOUNT) {
        i++;

        fbdDoStepEx(10, 0);
        msleep(10);


        tEventLogItem event;
        int ei = 0;
        //

        // printf("\n\nEVENTS LOG:\n");
        //
        // while(1) {
        //     if(!fbdGetLogEvent(ei, &event)) {
        //         printf("total %d\n", ei);
        //         //
        //         if((ei > 0) && ((ei % 20) == 0)) {
        //             printf("pause\n");
        //         }
        //         break;
        //     }
        //     printf("idx:%3d se:%u  %2u-%2u %.2u:%.2u:%.2u '%s'\n", ei, event.flags.started, event.flags.day, event.flags.month, event.flags.hours, event.flags.minutes, event.flags.seconds, event.message);
        //     ei++;
        // }
        //

        printf("\n\nCURRENT EVENTS LOG:\n");
        for(ei=0; ei<fbdTotalEventsCount(); ei++) {
            if(fbdGetCurrentEvent(ei, &event)) {
                printf("idx:%3d '%s'\n", ei, event.message);
            }
        }


    }

    // end = GetTickCount();
    // printf("%lu ms\n", end-start);
    // printf("cicle duration %f ms\n", 1.0*(end-start)/CICLCOUNT);



    return 0;
}
