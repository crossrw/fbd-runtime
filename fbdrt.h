#ifndef FBDRT_H
#define	FBDRT_H

#include <stdbool.h>       // For true/false definition

// =========================================================================
// = начало настроек =======================================================
//
// определение порядка байт, по умолчанию используется LOW_ENDIAN
//#define BIG_ENDIAN
//
// размер памяти для сигнала схемы (1, 2 или 4)
#define SIGNAL_SIZE 4
//
// размер пямяти для индекса элемента (1 или 2)
#define INDEX_SIZE 2
//
// префикс и суфикс используемые для определения памяти в ROM или FLASH
#define ROM_CONST const
#define ROM_CONST_SUFX
//
// префикс и суфикс используемые для определения памяти в которой хранится описание схемы
#define DESCR_MEM const
#define DESCR_MEM_SUFX
//
// нобходимо определить символ USE_HMI если планируете использовать функции HMI
#define USE_HMI
//
// определив символ SPEED_OPT вы увеличите скорость выполнения схемы за счет увеличения размера необходимого RAM
// #define SPEED_OPT
//
// максимальный размер стека, используемый при расчете схемы
// один элемент стека занимает (sizeof(tElemIndex)+1) байт
#define FBDSTACKSIZE 128
//
// data type for stack pointer
typedef unsigned char tFBDStackPnt;
//
typedef long tLongSignal;
//
// = конец настроек ========================================================
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

// -------------------------------------------------------------------------------------------------------
// Инициализация схемы
// -------------------------------------------------------------------------------------------------------
// Функция должна быть вызвана один раз в самом начале работы.
// Результат: количество ОЗУ, необходимое для выполнения схемы (значение больше 0) или (в случае ошибки) отрицательное значение:
// -1 - неверный код элемента в описании схемы
// -2 - неверный размер tSignal или tElementIndex
int fbdInit(DESCR_MEM unsigned char *descr);
//
//
// need call after fbdInit(), set memory buf for calculating
void fbdSetMemory(char *buf);
//
// -------------------------------------------------------------------------------------------------------
// Calculating function
// -------------------------------------------------------------------------------------------------------
// executing one step scheme calculating, period - time from the previous call fbdDoStep() in milliseconds
void fbdDoStep(tSignal period);

// -------------------------------------------------------------------------------------------------------
// Network support
// -------------------------------------------------------------------------------------------------------
// set net variable value
void fbdSetNetVar(tSignal index, tSignal value);
// get net variable value
bool fbdGetNetVar(tSignal index, tSignal *value);

#ifdef USE_HMI
// HMI
// -------------------------------------------------------------------------------------------------------
// структура описания точки контроля или регулирования
typedef struct {
    tSignal value;              // текущее значение точки
    tSignal lowlimit;           // нижний предел значения (только для точек регулирования)
    tSignal upperLimit;         // верхний предел значения (только для точек регулирования)
    tSignal defValue;           // значение "по умолчанию" (только для точек регулирования)
    tSignal divider;            // делитель для отображения на индикаторе
    tSignal step;               // шаг регулирования (только для точек регулирования)
    DESCR_MEM char *caption;    // текстовая надпись
} tHMIdata;
//
// получить значение точки регулирования
bool fbdHMIgetSP(tSignal index, tHMIdata *pnt);
//
// установить значение точки регулирования
void fbdHMIsetSP(tSignal index, tSignal value);
//
// получить значение точки контроля
bool fbdHMIgetWP(tSignal index, tHMIdata *pnt);
//
// структура описания проекта
typedef struct {
    DESCR_MEM char *name;       // наименование проекта
    DESCR_MEM char *version;    // версия проекта
    DESCR_MEM char *btime;      // дата и время компиляции
} tHMIdescription;
//
// получить структуру с описанием проекта
void fbdHMIgetDescription(tHMIdescription *pnt);
//
#endif // USE_HMI

#endif	// FBDRT_H
