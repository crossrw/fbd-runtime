#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fbdrt.h"

// #define SILENT
#define CICLCOUNT 1000000

// date: 19-01-2022 17:52:44
// file name: C:\Users\al\Documents\fbd2Projects\examples\Светофор.fbdbin
// title: Управление светофором
// required RTL version: 7
// HMI: used
// netvar: not used
// elements count: 24
// sizeof(tSignal) = 4
// sizeof(tElemIndex) = 2
const unsigned char description[] = {0x17, 0x17, 0x17, 0x03, 0x03, 0x03, 0x0F, 0x20, 0x00, 0x04, 0x44, 0x04, 0x04, 0x44, 0x00, 0x06, 0x06, 0x06, 0x01, 0x0C, 0x0C, 0x0C, 0x00, 0x01, 0x94, 0x06, 0x00, 0x0F, 0x00, 0x06, 0x00, 
0x11, 0x00, 0x06, 0x00, 0x10, 0x00, 0x17, 0x00, 0x12, 0x00, 0x04, 0x00, 0x10, 0x00, 0x0F, 0x00, 0x09, 0x00, 0x11, 0x00, 0x0D, 0x00, 0x05, 0x00, 0x15, 0x00, 0x0A, 0x00, 0x07, 0x00, 0x06, 0x00, 
0x03, 0x00, 0x11, 0x00, 0x13, 0x00, 0x0F, 0x00, 0x14, 0x00, 0x10, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x02, 0x00, 0x11, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x00, 0x0B, 0x00, 0xD0, 0x07, 0x00, 0x00, 
0x98, 0x3A, 0x00, 0x00, 0x40, 0x1F, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x98, 0x3A, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
0xE8, 0x03, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0xA0, 0x0F, 0x00, 0x00, 0xD0, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE5, 0x0C, 0x00, 
0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC8, 0x40, 0x00, 0x80, 0xC2, 0xF0, 0xE5, 0xEC, 0xFF, 0x20, 0xE7, 0xE5, 0xEB, 0xE5, 0xED, 
0xEE, 0xE3, 0xEE, 0x00, 0xC2, 0xF0, 0xE5, 0xEC, 0xFF, 0x20, 0xEA, 0xF0, 0xE0, 0xF1, 0xED, 0xEE, 0xE3, 0xEE, 0x00, 0xC2, 0xF0, 0xE5, 0xEC, 0xFF, 0x20, 0xE6, 0xE5, 0xEB, 0xF2, 0xEE, 0xE3, 0xEE, 
0x00, 0xD3, 0xEF, 0xF0, 0xE0, 0xE2, 0xEB, 0xE5, 0xED, 0xE8, 0xE5, 0x20, 0xF1, 0xE2, 0xE5, 0xF2, 0xEE, 0xF4, 0xEE, 0xF0, 0xEE, 0xEC, 0x00, 0x31, 0x00, 0x31, 0x39, 0x2D, 0x30, 0x31, 0x2D, 0x32, 
0x32, 0x20, 0x31, 0x37, 0x3A, 0x35, 0x32, 0x00, 0xFF, 0x94, 0x01, 0x00, 0x00, 0x64, 0x00, 0x0F, 0x00, 0x24, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 0x05, 
0x00, 0xE0, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD1, 0xE2, 0xE5, 0xF2, 0xEE, 0xF4, 0xEE, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x7D, 0x00, 0x23, 0x00, 0xC2, 0x00, 0xD1, 0x00, 0x86, 0x31, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x96, 0x00, 0xB8, 0x00, 0xC7, 
0x00, 0x49, 0x4A, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x5F, 0x00, 0xB8, 0x00, 0x90, 0x00, 0x49, 0x4A, 0x00, 0x00, 0x18, 0x00, 0x05, 
0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x28, 0x00, 0xB8, 0x00, 0x59, 0x00, 0x49, 0x4A, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x87, 0x00, 0x9B, 0x00, 0xB8, 0x00, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x2D, 0x00, 0xB8, 0x00, 0x5E, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x87, 0x00, 0x2D, 0x00, 0xB8, 0x00, 0x5E, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x18, 0x00, 0x05, 
0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x87, 0x00, 0x9B, 0x00, 0xB8, 0x00, 0xCC, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 
0x00, 0x87, 0x00, 0x64, 0x00, 0xB8, 0x00, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x05, 0x00, 0x02, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x64, 0x00, 0xB8, 0x00, 0x95, 
0x00, 0xE0, 0xFF, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0xDC, 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD0, 0xE5, 0xE6, 
0xE8, 0xEC, 0x20, 0x22, 0xC4, 0xE5, 0xED, 0xFC, 0x22, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x02, 0x00, 0x02, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0xDC, 0x00, 0x00, 0xFC, 0x00, 
0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD0, 0xE5, 0xE6, 0xE8, 0xEC, 0x20, 0x22, 0xCD, 0xEE, 0xF7, 0xFC, 0x22, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x03, 0x00, 0x02, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 
0x00, 0x0A, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x00, 0x03, 0x00, 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA1, 0x9F, 0xBC, 
0x9B};

#define MEM_SIZE 4096

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
        printf(" request InputPin(%d)\n", index);
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
        printf(" request NVRAM(%d)\n", index);
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
        printf(" set OutputPin(%d) to value %d\n", index, *value);
        break;
    case 1:
        printf(" set NVRAM(%d) to value %d\n", index, *value);
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
#ifndef SILENT
        printf("step %d\n", i);
#endif

        fbdDoStepEx(100, 0);
        msleep(10);

    }

    // end = GetTickCount();
    // printf("%lu ms\n", end-start);
    // printf("cicle duration %f ms\n", 1.0*(end-start)/CICLCOUNT);



    return 0;
}
