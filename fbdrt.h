#ifndef FBDRT_H
#define	FBDRT_H

#include <stdbool.h>       // For true/false definition

// =========================================================================
// = begin tuning ==========================================================
// byte order
//#define BIG_ENDIAN
// size of FBD signal
#define SIGNAL_SIZE 2
// size of element index
#define INDEX_SIZE 1
//
// data in ROM/FLASH
#define ROM_CONST const
#define ROM_CONST_SUFX
// schema description
#define DESCR_MEM const
#define DESCR_MEM_SUFX
//
// needed if you use HMI functions
#define USE_HMI
//
// speed optimization reduces the calculation time, but increases
// the size of memory (RAM) required
#define SPEED_OPT
// stack size fo calculating, one stack element size =(sizeof(tElemIndex)+1) байт
#define FBDSTACKSIZE 32
// data type for stack pointer
typedef unsigned char tFBDStackPnt;
//
typedef long tLongSignal;
//
// = end tuning ============================================================
// =========================================================================
//
typedef unsigned short tOffset;
//
#if defined(BIG_ENDIAN) && (SIGNAL_SIZE > 1)
#define SIGNAL_BYTE_ORDER(x) lotobigsign(x)
#else
#define SIGNAL_BYTE_ORDER(x) (x)
#endif // defined

#if defined(BIG_ENDIAN) && (INDEX_SIZE > 1)
#define ELEMINDEX_BYTE_ORDER(x) lotobigidx(x)
#else
#define ELEMINDEX_BYTE_ORDER(x) (x)
#endif // defined

#if (SIGNAL_SIZE == 1)
typedef signed char tSignal;
#define MAX_SIGNAL 127
#define MIN_SIGNAL (-128)
//
#elif (SIGNAL_SIZE == 2)
typedef signed short tSignal;
#define MAX_SIGNAL 32767
#define MIN_SIGNAL (-32768)
//
#elif (SIGNAL_SIZE == 4)
typedef signed long int tSignal;
#define MAX_SIGNAL 2147483647L
#define MIN_SIGNAL (-2147483648L)
#else
#error Invalid value of SIGNAL_SIZE
#endif // SIGNAL_SIZE
//
#if INDEX_SIZE == 1
typedef unsigned char tElemIndex;
#elif INDEX_SIZE == 2
typedef unsigned short tElemIndex;
#else
#error Invalid value of INDEX_SIZE
#endif // INDEX_SIZE
//
// end element description flag
#define END_MARK (unsigned char)((sizeof(tSignal)|(sizeof(tElemIndex)<<3))|0x80)
// END_MARK:
// bit 0-2: sizeof(tSignal)
// bit 3-4: sizeof(tElemIndex)
// bit 5:   reserved
// bit 6:   reserved
// bit 7:   1

// Initialization functions
// -------------------------------------------------------------------------------------------------------
// need call first, return amount of memory (RAM) required for calculating or (if error) negative value:
// -1 - invalid element code in description
// -2 - wrong sizeof tSignal or tElementIndex
int fbdInit(DESCR_MEM unsigned char *descr);
// need call after fbdInit(), set memory buf for calculating
void fbdSetMemory(char *buf);

// Calculating function
// -------------------------------------------------------------------------------------------------------
// executing one step scheme calculating, period - time from the previous call fbdDoStep() in milliseconds
void fbdDoStep(tSignal period);

#ifdef USE_HMI
// HMI
// -------------------------------------------------------------------------------------------------------
// stack calculating item
typedef struct {
    tSignal value;              // current point value
    tSignal lowlimit;           // low limit for value (only for setpoints)
    tSignal upperLimit;         // upper limit for value (only for setpoints)
    DESCR_MEM char *caption;    // text caption
} tHMIdata;
// get Setting Point
bool fbdHMIgetSP(tSignal index, tHMIdata *pnt);
// set Setting Point
void fbdHMIsetSP(tSignal index, tSignal value);
// get Watch Point
bool fbdHMIgetWP(tSignal index, tHMIdata *pnt);
#endif // USE_HMI

#endif	// FBDRT_H
