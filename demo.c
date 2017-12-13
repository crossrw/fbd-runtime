#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "fbdrt.h"

const unsigned char description[] = {0x4C, 0x4C, 0x00, 0x47, 0x01, 0x00, 0x94, 0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x04, 0x00, 0x03, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0xE1, 0xE5, 0xE7, 0x20, 0xE8, 0xEC, 0xE5, 0xED, 0xE8, 0x00, 0x31, 0x00, 0x31, 0x31, 0x2D, 0x31, 0x32, 0x2D, 0x32, 0x30, 0x31, 0x37, 0x20, 0x31, 0x38, 0x3A, 0x32, 0x36, 
0x3A, 0x30, 0x33, 0x00};

#define MEM_SIZE 100

char memory[MEM_SIZE];

tSignal FBDgetProc(char type, tSignal index)
{
    switch(type) {
    case 0:
        printf(" request InputPin(%ld)\n", index);
        return 0;
    case 1:
        printf(" request NVRAM(%ld)\n", index);
        return 0;
    }
    return 0;
}

void FBDsetProc(char type, tSignal index, tSignal *value)
{
    switch(type)
    {
    case 0:
        printf(" set OutputPin(%ld) to value %ld\n", index, *value);
        break;
    case 1:
        printf(" set NVRAM(%ld) to value %ld\n", index, *value);
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
