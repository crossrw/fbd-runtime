#ifndef FBDRT_H
#define	FBDRT_H

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

// #define FBD_DEBUG

// = Начало настройки ==========================================================
// глубина стека для расчетов, один элемент занимает (sizeof(tElemIndex)+1) байт
#define FBDSTACKSIZE 32
// тип данных, используемый для хранения указателя стека
typedef unsigned char tFBDStackPnt;
// тип, используемый для хранения сигнала схемы
typedef int16_t tSignal;
#define MAX_SIGNAL INT16_MAX
#define MIN_SIGNAL INT16_MIN
// тип, используемый для хранения индексов элементов
typedef uint8_t tElemIndex;
//
// таблицы в ROM
#define ROM_CONST const
// размещение описания схемы
#define DESCR_MEM const
// = Конец настройки ===========================================================
//
// признак окончани описания элементов в схеме
#define END_MARK (unsigned char)((sizeof(tSignal)|(sizeof(tElemIndex)<<3))|0x80)

// надо вызвать первой, возвращает размер памяти необходимой для работы
// в случае ошибки возвращает:
// -1 - неверный элемент в описании
// -2 - несовпадает размер сигнала или индекса
int fbdInit(DESCR_MEM unsigned char *descr);
// надо вызвать после fbdInit() и передать указатели на память
void fbdSetMemory(char *buf);
// выполнить шаг расчета схемы, надо вызывать периодически, параметр - период вызова в мс.
void fbdDoStep(tSignal period);

#endif	/* FBDRT_H */
