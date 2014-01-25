#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "fbdrt.h"

const unsigned char description[] = {0x002, 0x000, 0x00C, 0x001, 0x08A, 0x002, 0x000, 0x000, 0x003, 0x000, 0x000, 0x0E8, 0x003};

unsigned char memory[100];

tSignal FBDgetProc(char type, tSignal index)
{
    switch(type) {
    case 0:
        printf(" request InputPin(%d)\n", index);
        return 0;
    case 1:
        printf(" request Variable(%d)\n", index);
        return 0;
    case 2:
        printf(" request EEPROM(%d)\n", index);
        return 0;
    }
}

void FBDsetProc(char type, tSignal index, tSignal *value)
{
    switch(type)
    {
    case 0:
        printf(" set OutputPin(%d) to value %d\n", index, *value);
        break;
    case 1:
        printf(" set Variable(%d) to value %d\n", index, *value);
        break;
    case 2:
        printf(" set EEPROM(%d) to value %d\n", index, *value);
        break;
    }
}

int main(void)
{
    int res,i;
    res = fbdInit(description);
    if(res<=0) {
     printf("result = %d\n", res);
     return 0;
    }
    fbdSetMemory(memory);
    printf("memory request size = %d\n", res);
    // main loop
    i=0;
    while(i++ < 1000) {
        printf("step %d\n", i);
        Sleep(100);
        fbdDoStep(100);
    }
    return 0;
}
