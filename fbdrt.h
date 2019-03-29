#ifndef FBDRT_H
#define	FBDRT_H

#include <stdbool.h>       // For true/false definition

// 7 - базовая
// 8 - поддержка экранов
#define FBD_LIB_VERSION 8

// описание упакованной структуры
#if defined ( __CC_ARM   )
  #define __packed_struct           __packed struct
#elif defined   (  __GNUC__  )
  #define __packed_struct           struct __attribute__ ((__packed__))
#elif defined   (  __TASKING__  )
  #define __packed_struct           struct __packed
#endif

//
//
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
// нобходимо определить символ USE_MATH если планируете использовать генератор sin
#define USE_MATH
//
// SPEED_OPT - оптимизация скорости выполнения за счет увеличения размера необходимого RAM
// Чем больше схема, тем больше увеличение скорости.
// На примере waterheating.fbd:
// - уменьшение времени цикла в 4.9 раза;
// - увеличение размера RAM c 374 до 870 байт (2.3 раза)
#define SPEED_OPT
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
#ifdef USE_MATH
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI
#endif // USE_MATH

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
// -3 - несовпадает версия программы и библиотеки
// -4 - неверная контрольная сумма программы
int fbdInit(DESCR_MEM unsigned char *descr);
//
//
// Функция должна быть вызвана после fbdInit()
// Параметры:
//  buf       - указатель на буфер памяти (размер возвращается fbdInit()), которая будет использована при расчетах
//  needReset - признак необходимости сбросить nvram после загрузки новой (ранее не выполнявщейся) схемы
void fbdSetMemory(char *buf, bool needReset);
//
// -------------------------------------------------------------------------------------------------------
// Функция вычисления
// -------------------------------------------------------------------------------------------------------
// Функция выполняет один шаг вычисления схемы
//  period - время с момента предыдущего вызова fbdDoStep(), например в милисекундах
void fbdDoStep(tSignal period);


// -------------------------------------------------------------------------------------------------------
// Экраны
// -------------------------------------------------------------------------------------------------------
#ifdef USE_HMI

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

typedef unsigned short tScreenDim;
typedef unsigned short tColor;

// описание структуры экрана
typedef __packed_struct Screen_t {
    unsigned short len;                                         // размер экрана                    2
    tColor bkcolor;                                             // цвет фона                        2
    unsigned short period;                                      // период обновления                2
    unsigned short elemCount;                                   // количество элементов экрана      2
    // тут элементы экрана
} tScreen;

// описание элементов экрана
// базовый элемент с видимостью
typedef __packed_struct ScrElemBase_t {
    unsigned short len;                                         // размер структуры                 2
    unsigned short type;                                        // тип элемента                     2
    //
    unsigned short visibleCond;                                 // условие видимости                2
    tElemIndex visibleElem;                                     // индекс элемента                  2
    tSignal visibleValue;                                       // константа - значение сигнала     4
    //
    tScreenDim x1;                                              // координата x                     2
    tScreenDim y1;                                              // координата y                     2
} tScrElemBase;

// элемент прямоугольник
typedef __packed_struct ScrElemRect_t {
    tScrElemBase parent;                                        //                                  16
    //
    tScreenDim x2;                                              // координата x2                    2
    tScreenDim y2;                                              // координата y2                    2
    tColor color;                                               // цвет                             2
    unsigned short reserve;                                     //                                  2
} tScrElemRect;

// элемент эллипс
typedef __packed_struct ScrElemCircle_t {
    tScrElemBase parent;                                        //                                  16
    //
    tScreenDim x2;                                              // координата x2                    2
    tScreenDim y2;                                              // координата y2                    2
    tColor color;                                               // цвет                             2
    unsigned short reserve;                                     //                                  2
} tScrElemCircle;

// элемент линия
typedef __packed_struct ScrElemLine_t {
    tScrElemBase parent;                                        //                                  16
    //
    tScreenDim x2;                                              // координата x2                    2
    tScreenDim y2;                                              // координата y2                    2
    tColor color;                                               // цвет                             2
    tScreenDim width;                                           // толщина линии                    2
    float sine;                                                 //                                  4
    float cosinus;                                              //                                  4
} tScrElemLine;

// элемент картинка
typedef __packed_struct ScrElemImage_t {
    tScrElemBase parent;                                        //                                  16
    //
    unsigned short index;                                       // индекс картинки                  2
    unsigned short reserve;                                     //                                  2 !!!
} tScrElemImage;

// элемент текст
typedef __packed_struct ScrElemText_t {
    tScrElemBase parent;                                        //                                  16
    //
    tColor color;                                               // цвет                             2
    tColor bkcolor;                                             // цвет фона                        2
    //
    tElemIndex valueElem;                                       // индекс элемента                  2
    unsigned char font;                                         // индекс шрифта, старший бит прозрачность 1
    unsigned char divider;                                      // делитель                         1
    char text[];                                                // сам текст, заканчивается 0       длинна должны быть кратна 4 !
} tScrElemText;

// элемент шкала
typedef __packed_struct ScrElemGauge_t {
    tScrElemBase parent;                                        //                                  16
    //
    tScreenDim x2;                                              // координата x2                    2
    tScreenDim y2;                                              // координата y2                    2
    //
    tColor color;                                               // цвет                             2
    tColor bkcolor;                                             // цвет фона                        2
    //
    tSignal maxvalue;                                           // максимальное значение шкалы      4
    tElemIndex valueElem;                                       // индекс элемента                  2
    unsigned short orientation;                                 // ориентация: 0 - гор, 1 - вер     2
} tScrElemGauge;


// один шаг вычисления схемы с последующим рисованием (при необходимости) экрана screen
// если screen < 0, то не рисовать экран
void fbdDoStepEx(tSignal period, short screenIndex);

#define GP_RTC_HOUR     20
#define GP_RTC_MINUTE   21
#define GP_RTC_SECOND   22
#define GP_RTC_DAY      23
#define GP_RTC_MONTH    24
#define GP_RTC_YEAR     25

#endif

// -------------------------------------------------------------------------------------------------------
// Сетевые переменные
// -------------------------------------------------------------------------------------------------------
//
// структура описания сетевой переменной
typedef struct netvar_t {
    tSignal index;              // номер сетевой переменной
    tSignal value;              // значение сетевой переменной
} tNetVar;
//

// Установить значение переменной, принятое по сети
// параметр - заполненная структура с принятой сетевой переменной
void fbdSetNetVar(tNetVar *netvar);
//
// Получить значение переменной для отправки по сети
// Результат выполнения:
//  false - переменных для отправки больше нет
//  true  - переменная для отправки есть, она помещена в структуру netvar
// Функцию необходимо вызывать до тех пор, пока она не вернет false
bool fbdGetNetVar(tNetVar *netvar);
//
// установить для всех выходных сетевых переменных признак изменения
// функцию можно вызывать периодически для принудительной отправки всех переменных
void fbdChangeAllNetVars(void);
//
// получение значений глобальных настроек схемы
// параметр:
#define FBD_OPT_REQ_VERSION  0
#define FBD_OPT_NETVAR_USE   1
#define FBD_OPT_NETVAR_PORT  2
#define FBD_OPT_NETVAR_GROUP 3
#define FBD_OPT_SCREEN_COUNT 4
#define FBD_OPT_SCHEMA_SIZE  5
#define FBD_OPT_HINTS_COUNT  6

extern DESCR_MEM unsigned char DESCR_MEM_SUFX *fbdGlobalOptionsCount;
extern DESCR_MEM tSignal DESCR_MEM_SUFX *fbdGlobalOptions;

#define FBD_REQ_VERSION  fbdGlobalOptions[FBD_OPT_REQ_VERSION]
#define FBD_NETVAR_USE   fbdGlobalOptions[FBD_OPT_NETVAR_USE]
#define FBD_NETVAR_PORT  fbdGlobalOptions[FBD_OPT_NETVAR_PORT]
#define FBD_NETVAR_GROUP fbdGlobalOptions[FBD_OPT_NETVAR_GROUP]
#define FBD_SCREEN_COUNT ((*fbdGlobalOptionsCount>FBD_OPT_SCREEN_COUNT)?fbdGlobalOptions[FBD_OPT_SCREEN_COUNT]:0)
#define FBD_SCHEMA_SIZE ((*fbdGlobalOptionsCount>FBD_OPT_SCHEMA_SIZE)?fbdGlobalOptions[FBD_OPT_SCHEMA_SIZE]:0)
#define FBD_HINTS_COUNT ((*fbdGlobalOptionsCount>FBD_OPT_HINTS_COUNT)?fbdGlobalOptions[FBD_OPT_HINTS_COUNT]:0)

//
#ifdef USE_HMI
// HMI
// -------------------------------------------------------------------------------------------------------
// структура описания точки контроля или регулирования
typedef struct hmidata_t {
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
typedef struct hmidescription_t {
    DESCR_MEM char *name;       // наименование проекта
    DESCR_MEM char *version;    // версия проекта
    DESCR_MEM char *btime;      // дата и время компиляции
} tHMIdescription;
//
// получить структуру с описанием проекта
void fbdHMIgetDescription(tHMIdescription *pnt);
//
// возвращает указатель на текстовое описание (хинт) входа или выхода,
// если такого описание не найдено, то возвращает NULL
DESCR_MEM char DESCR_MEM_SUFX *fbdHMIgetIOhint(char type, char index);

//
#endif // USE_HMI

#endif	// FBDRT_H
