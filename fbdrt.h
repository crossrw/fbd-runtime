#ifndef FBDRT_H
#define	FBDRT_H

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

// #define FBD_DEBUG

// = begin tuning ==========================================================
// stack size fo calculating, one stack element size =(sizeof(tElemIndex)+1) байт
#define FBDSTACKSIZE 32
// data type for stack pointer
typedef unsigned char tFBDStackPnt;
// data type for FBD signal
typedef int16_t tSignal;
typedef long tLongSignal;
#define MAX_SIGNAL INT16_MAX
#define MIN_SIGNAL INT16_MIN
// data type for element index
typedef uint8_t tElemIndex;
//
// data in ROM/FLASH
#define ROM_CONST const
// schema description
#define DESCR_MEM const
// = end tuning ===========================================================
//
// end element description flag
#define END_MARK (unsigned char)((sizeof(tSignal)|(sizeof(tElemIndex)<<3))|0x80)
// END_MARK:
// bit 0-2: sizeof(tSignal)
// bit 3-4: sizeof(tElemIndex)
// bit 5:   reserved
// bit 6:   reserved
// bit 7:   1

// need call first, return amount of memory required for calculating
// if error:
// -1 - invalid element code in description
// -2 - wrong sizeof tSignal or tElementIndex
int fbdInit(DESCR_MEM unsigned char *descr);
// need call after fbdInit(), set memory buf for calculating
void fbdSetMemory(char *buf);
// executing one step scheme calculating, period - time from the previous call fbdDoStep() in milliseconds
void fbdDoStep(tSignal period);

#endif	/* FBDRT_H */
