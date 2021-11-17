#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fbdrt.h"

// #define SILENT
#define CICLCOUNT 1000000

const unsigned char description[] = {0x16, 0x0F, 0x94, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE5, 0x0C, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 
 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0xE1, 0xE5, 0xE7, 0x20, 0xE8, 0xEC, 0xE5, 0xED, 0xE8, 0x00, 0x31, 0x00, 0x32, 0x39, 0x2D, 0x30, 0x33, 0x2D, 0x31, 0x39, 
 0x20, 0x32, 0x31, 0x3A, 0x30, 0x30, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x69, 0x6E, 0x70, 0x75, 0x74, 0x31, 0x00, 0x00, 0x02, 0x69, 0x6E, 0x70, 0x75, 0x74, 0x33, 0x00, 0x00, 0x0A, 0x74, 0x68, 0x69, 
 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x65, 0x6D, 0x70, 0x65, 0x72, 0x61, 0x74, 0x75, 0x72, 0x65, 0x20, 0x31, 0x00, 0x01, 0x01, 0x6F, 0x75, 0x74, 0x70, 0x75, 0x74, 0x32, 0x00, 0x01, 0x02, 0x6F, 
 0x75, 0x74, 0x70, 0x75, 0x74, 0x33, 0x00, 0x01, 0x0B, 0x61, 0x6F, 0x31, 0x00, 0x4C, 0x92, 0x49, 0x9B};

#define MEM_SIZE 2048

char memory[MEM_SIZE];

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
#ifndef SILENT
    switch(type) {
    case 0:
        printf(" request InputPin(%ld)\n", index);
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
    case 1:
        printf(" request NVRAM(%ld)\n", index);
        return 0;
    }
#endif
    return 0;
}

void FBDsetProc(char type, tSignal index, tSignal *value)
{
#ifndef SILENT
    switch(type)
    {
    case 0:
        printf(" set OutputPin(%ld) to value %ld\n", index, *value);
        break;
    case 1:
        printf(" set NVRAM(%ld) to value %ld\n", index, *value);
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
    printf("FBD_OPT_REQ_VERSION: %ld\n",    FBD_REQ_VERSION);
    printf("FBD_NETVAR_USE: %ld\n",         FBD_NETVAR_USE);
    printf("FBD_NETVAR_PORT: %ld\n",        FBD_NETVAR_PORT);
    printf("FBD_NETVAR_GROUP: %ld\n",       FBD_NETVAR_GROUP);
    printf("FBD_SCREEN_COUNT: %ld\n",       FBD_SCREEN_COUNT);
    printf("FBD_SCHEMA_SIZE: %ld\n",        FBD_SCHEMA_SIZE);
    printf("FBD_HINTS_COUNT: %ld\n",        FBD_HINTS_COUNT);
    //
    printf("fbdModbusUsage(void): %d\n",        fbdModbusUsage());

    // main loop

    printf("IO hints:\n");
    for(i=0; i<16; i++) {
        printf(" type=0 index=%d text:'%s'\n", i, fbdHMIgetIOhint(0, i));
        printf(" type=1 index=%d text:'%s'\n", i, fbdHMIgetIOhint(1, i));
        printf("\n");
    }

    i=0;
    while(i < CICLCOUNT) {
        i++;
#ifndef SILENT
        printf("step %d\n", i);
#endif
        fbdDoStepEx(100, 0);
        msleep(10);
    }
    //
    return 0;
}
