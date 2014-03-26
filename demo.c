#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "fbdrt.h"

const unsigned char description[] = {0x002, 0x000, 0x00C, 0x001, 0x08A, 0x002, 0x000, 0x000, 0x003, 0x000, 0x000, 0x0E8, 0x003};

#define MEM_SIZE 100

unsigned char memory[MEM_SIZE];

tSignal FBDgetProc(char type, tSignal index)
{
    switch(type) {
    case 0:
        printf(" request InputPin(%d)\n", index);
        return 0;
    case 1:
        printf(" request NVRAM(%d)\n", index);
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
        printf(" set NVRAM(%d) to value %d\n", index, *value);
        break;
    }
}

int main(void)
{
    int res,i;
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
    fbdSetMemory(memory);
    printf("memory request size = %d\n", res);
    // main loop
    i=0;
    while(1) {
        printf("step %d\n", i++);
        Sleep(100);
        fbdDoStep(100);
    }
    return 0;
}
