/**
 * @file fbdrt.h
 * @author crossrw1@gmail.com
 * @brief FBD-Runtime Library Headers
 * @version 9.0
 * @date 2021-11-17
 */

#ifndef FBDRT_H
#define	FBDRT_H

#include <stdbool.h>        // for true/false definition
#include <stdint.h>         // type definition

// 7 - базовая
// 8 - поддержка экранов
// 9 - Modbus
#define FBD_LIB_VERSION 9

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
typedef uint8_t tFBDStackPnt;
//
typedef int32_t tLongSignal;
//
// = конец настроек ========================================================
// =========================================================================
//
// типы элементов:
#define ELEMMASK    0x3F
#define INVERTFLAG  0x40
//
typedef enum {
    ELEM_OUT_PIN =  0,
    ELEM_CONST   =  1,
    ELEM_NOT     =  2,
    ELEM_AND     =  3,
    ELEM_OR      =  4,
    ELEM_XOR     =  5,
    ELEM_RSTRG   =  6,
    ELEM_DTRG    =  7,
    ELEM_ADD     =  8,
    ELEM_SUB     =  9,
    ELEM_MUL     =  10,
    ELEM_DIV     =  11,
    ELEM_TON     =  12,
    ELEM_CMP     =  13,
    ELEM_OUT_VAR =  14,
    ELEM_INP_PIN =  15,
    ELEM_INP_VAR =  16,
    ELEM_PID     =  17,
    ELEM_SUM     =  18,
    ELEM_COUNTER =  19,
    ELEM_MUX     =  20,
    ELEM_ABS     =  21,
    ELEM_WP      =  22,
    ELEM_SP      =  23,
    ELEM_TP      =  24,
    ELEM_MIN     =  25,
    ELEM_MAX     =  26,
    ELEM_LIM     =  27,
    ELEM_EQ      =  28,
    ELEM_BAND    =  29,
    ELEM_BOR     =  30,
    ELEM_BXOR    =  31,
    ELEM_GEN     =  32,
    ELEM_INP_MDBS=  33,
    ELEM_OUT_MDBS=  34,
    //
    ELEM_TYPE_COUNT
} tFBD_ELEMENT_TYPE;
//
#ifdef USE_MATH
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI
#endif // USE_MATH

typedef uint16_t tOffset;
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
typedef int8_t tSignal;
#define MAX_SIGNAL INT8_MAX
#define MIN_SIGNAL INT8_MIN
//
#elif (SIGNAL_SIZE == 2)
typedef int16_t tSignal;
#define MAX_SIGNAL INT16_MAX
#define MIN_SIGNAL INT16_MIN
//
#elif (SIGNAL_SIZE == 4)
typedef int32_t tSignal;
#define MAX_SIGNAL INT32_MAX
#define MIN_SIGNAL INT32_MIN
#else
#error Invalid value of SIGNAL_SIZE
#endif // SIGNAL_SIZE
//
#if INDEX_SIZE == 1
typedef uint8_t tElemIndex;
#define MAX_INDEX UINT8_MAX
#elif INDEX_SIZE == 2
typedef uint16_t tElemIndex;
#define MAX_INDEX UINT16_MAX
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

// коды ошибок, возвращаемые fbdInit()
typedef enum {
    ERR_INVALID_ELEMENT_TYPE    =   -1,
    ERR_INVALID_SIZE_TYPE       =   -2,
    ERR_INVALID_LIB_VERSION     =   -3,
    ERR_INVALID_CHECK_SUM       =   -4
} FBD_INIT_RESULT;

// -------------------------------------------------------------------------------------------------------
// Инициализация схемы
// -------------------------------------------------------------------------------------------------------

int fbdInit(DESCR_MEM unsigned char *descr);

void fbdSetMemory(char *buf, bool needReset);

void fbdDoStep(tSignal period);

// -------------------------------------------------------------------------------------------------------
// Экраны
// -------------------------------------------------------------------------------------------------------
#ifdef USE_HMI

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

typedef uint16_t tScreenDim;
typedef uint16_t tColor;

// описание структуры экрана
typedef __packed_struct Screen_t {
    uint16_t len;                                               // размер экрана                    2
    tColor bkcolor;                                             // цвет фона                        2
    uint16_t period;                                            // период обновления                2
    uint16_t elemCount;                                         // количество элементов экрана      2
    // тут элементы экрана
} tScreen;

// описание элементов экрана
// базовый элемент с видимостью
typedef __packed_struct ScrElemBase_t {
    uint16_t len;                                               // размер структуры                 2
    uint16_t type;                                              // тип элемента                     2
    //
    uint16_t visibleCond;                                       // условие видимости                2
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
    uint16_t reserve;                                           //                                  2
} tScrElemRect;

// элемент эллипс
typedef __packed_struct ScrElemCircle_t {
    tScrElemBase parent;                                        //                                  16
    //
    tScreenDim x2;                                              // координата x2                    2
    tScreenDim y2;                                              // координата y2                    2
    tColor color;                                               // цвет                             2
    uint16_t reserve;                                           //                                  2
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
    uint16_t index;                                             // индекс картинки                  2
    uint16_t reserve;                                           //                                  2 !!!
} tScrElemImage;

// элемент текст
typedef __packed_struct ScrElemText_t {
    tScrElemBase parent;                                        //                                  16
    //
    tColor color;                                               // цвет                             2
    tColor bkcolor;                                             // цвет фона                        2
    //
    tElemIndex valueElem;                                       // индекс элемента                  2
    uint8_t font;                                               // индекс шрифта, старший бит прозрачность 1
    uint8_t divider;                                            // делитель                         1
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
    uint16_t orientation;                                       // ориентация: 0 - гор, 1 - вер     2
} tScrElemGauge;

// один шаг вычисления схемы с последующим рисованием (при необходимости) экрана screen
void fbdDoStepEx(tSignal period, short screenIndex);

// перечисление используется в процедуре отрисовки тектовых сообщений на экране для получения текущего времени
enum GP_RTC_PARAMS {
    GP_RTC_HOUR     =  20,
    GP_RTC_MINUTE   =  21,
    GP_RTC_SECOND   =  22,
    GP_RTC_DAY      =  23,
    GP_RTC_MONTH    =  24,
    GP_RTC_YEAR     =  25
};
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
void fbdSetNetVar(tNetVar *netvar);
//
bool fbdGetNetVar(tNetVar *netvar);
//
void fbdChangeAllNetVars(void);

// -------------------------------------------------------------------------------------------------------
// MODBUS
// -------------------------------------------------------------------------------------------------------
//
// настройки последовательного порта для Modbus RTU
//
// скорость обмена:
typedef enum {
    FBD_BR_1200                 = 0,
    FBD_BR_2400                 = 1,
    FBD_BR_4800                 = 2,
    FBD_BR_9600                 = 3,
    FBD_BR_19200                = 4,
    FBD_BR_38400                = 5,
    FBD_BR_57600                = 6,
    FBD_BR_115200               = 7
} tFBD_BAUDRATE;
//
// контроль чётности:
typedef enum {
    FBD_PAR_NONE                = 0,
    FBD_PAR_ODD                 = 1,
    FBD_PAR_EVEN                = 2
} tFBD_PARITY;
//
// стоп-биты:
typedef enum {
    FBD_SB_1                    = 0,
    FBD_SB_2                    = 1
} tFBD_STOPB;
//
// статус использования Modbus
typedef enum {
    FBD_MODBUS_NONE             = 0,
    FBD_MODBUS_RTU              = 1,
    FBD_MODBUS_TCP              = 2,
    FBD_MODBUS_BOTH             = 3
} tFBD_MODBUS_USAGE;
//
// функция Modbus
typedef enum {
    FBD_MODBUS_READ_COILS               = 1,
    FBD_MODBUS_READ_DISCRETE_INPUTS     = 2,
    FBD_MODBUS_READ_HOLDING_REGISTERS   = 3,
    FBD_MODBUS_READ_INPUT_REGISTERS     = 4,
    //
    FBD_MODBUS_WRITE_SINGLE_COIL        = 5,
    FBD_MODBUS_WRITE_SINGLE_REGISTER    = 6,
    FBD_MODBUS_WRITE_MULTIPLE_COILS     = 15,
    FBD_MODBUS_WRITE_MULTIPLE_REGISTERS = 16
} tFBD_MODBUS_FUNCTION;

// структура описания настроек последовательного порта Modbus RTU
typedef struct modbusrtusettings_t {
    unsigned int timeout;                   // время одидания ответа в мс (0..4095)
    tFBD_BAUDRATE baudRate;                 // скорость обмена
    tFBD_PARITY parity;                     // контроль чётности: 0-None, 1-Odd, 2-Even
    tFBD_STOPB stopBits;                    // количество стоп-бит: 0-1, 1-2
} tModbusRTUsettings;

// данные Modbus
typedef union {
    tSignal         intData;
    float           floatData;
    uint16_t        ushortData[2];
    int16_t         shortData[2];
    uint8_t         byteData[4];

} tModbusData;

// структура описания запроса MODBUS
typedef struct modbusreq_t {
    tSignal ip;                             // ip адрес устройства, если ==0, то использовать протокол RTU
    uint8_t slaveAddr;                      // адрес устройства
    tFBD_MODBUS_FUNCTION funcCode;          // код функции
    uint16_t regAddr;                       // адрес регистра ModBus
    uint16_t regCount;                      // количество регистров
    tModbusData data;                       // данные, только для запросов записи
} tModbusReq;

// modbus bytes order
#define FBD_MODBUS_OPT_BO 0x04000000
// modbus words order
#define FBD_MODBUS_OPT_WO 0x08000000

// Получить статус использования Modbus заруженной схемой
// Вызывать только после выполнения "fbdSetMemory()"
// Результат выполнения:
//  FBD_MODBUS_NONE   - Modbus не используется
//  FBD_MODBUS_RTU    - Используется только Modbus RTU
//  FBD_MODBUS_TCP    - Используется только Modbus TCP
//  FBD_MODBUS_BOTH   - Используется только Modbus RTU и TCP
tFBD_MODBUS_USAGE fbdModbusUsage(void);

// Получить значения настроек Modbus RTU
// Вызывать только после выполнения "fbdSetMemory()"
// Результат выполнения:
//  false - менять настройки последовательного порта не надо (использовать текущие)
//  true  - необходимо установить значения настроек последовательного порта, настройки помещены в структуру *pnt
bool fbdModbusGetSerialSettings(tModbusRTUsettings *pnt);

// Modbus RTU

// Получить очередной запрос ModBus RTU для выполнения
// Результат выполнения:
//  false - запросов больше нет
//  true  - запрос есть, он помещен в структуру mbrequest
bool fbdGetNextModbusRTURequest(tModbusReq *mbrequest);
//
// Установка результата успешного выполнения предыдущего запроса Modbus RTU.
// Функция должна быть вызвана после успешного получения ответа на запрос Modbus RTU.
// Параметр response - данные возвращённые запросом.
void fbdSetModbusRTUResponse(tSignal response);
//
// Установить признак неуспешного результата полученного ранее запроса ModBus RTU
// Функция должна быть вызвана после неудачного выполнения запроса Modbus RTU
void fbdSetModbusRTUNoResponse(int errCode);

// Modbus TCP

// Получить очередной запрос ModBus TCP для выполнения
// Результат выполнения:
//  false - запросов больше нет
//  true  - запрос есть, он помещен в структуру mbrequest
bool fbdGetNextModbusTCPRequest(tModbusReq *mbrequest);
//
// Установка результата успешного выполнения предыдущего запроса Modbus TCP.
// Функция должна быть вызвана после успешного получения ответа на запрос Modbus TCP.
// Параметр response - данные возвращённые запросом.
void fbdSetModbusTCPResponse(tSignal response);
//
// Установить признак неуспешного результата полученного ранее запроса ModBus TCP
// Функция должна быть вызвана после неудачного выполнения запроса Modbus TCP
void fbdSetModbusTCPNoResponse(int errCode);

// -------------------------------------------------------------------------------------------------------
// Получение значений глобальных настроек схемы
// -------------------------------------------------------------------------------------------------------
// 
enum FBD_OPTIONS {
    FBD_OPT_REQ_VERSION     = 0,
    FBD_OPT_NETVAR_USE      = 1,
    FBD_OPT_NETVAR_PORT     = 2,
    FBD_OPT_NETVAR_GROUP    = 3,
    FBD_OPT_SCREEN_COUNT    = 4,
    FBD_OPT_SCHEMA_SIZE     = 5,
    FBD_OPT_HINTS_COUNT     = 6,
    FBD_OPT_MODBUSRTU_OPT   = 7
};

extern DESCR_MEM unsigned char DESCR_MEM_SUFX *fbdGlobalOptionsCount;
extern DESCR_MEM tSignal DESCR_MEM_SUFX *fbdGlobalOptions;
#define FBD_REQ_VERSION         fbdGlobalOptions[FBD_OPT_REQ_VERSION]
#define FBD_NETVAR_USE          fbdGlobalOptions[FBD_OPT_NETVAR_USE]
#define FBD_NETVAR_PORT         fbdGlobalOptions[FBD_OPT_NETVAR_PORT]
#define FBD_NETVAR_GROUP        fbdGlobalOptions[FBD_OPT_NETVAR_GROUP]
#define FBD_SCREEN_COUNT        ((*fbdGlobalOptionsCount>FBD_OPT_SCREEN_COUNT)?fbdGlobalOptions[FBD_OPT_SCREEN_COUNT]:0)
#define FBD_SCHEMA_SIZE         ((*fbdGlobalOptionsCount>FBD_OPT_SCHEMA_SIZE)?fbdGlobalOptions[FBD_OPT_SCHEMA_SIZE]:0)
#define FBD_HINTS_COUNT         ((*fbdGlobalOptionsCount>FBD_OPT_HINTS_COUNT)?fbdGlobalOptions[FBD_OPT_HINTS_COUNT]:0)
#define FBD_MODBUSRTU_OPT       ((*fbdGlobalOptionsCount>FBD_OPT_MODBUSRTU_OPT)?fbdGlobalOptions[FBD_OPT_MODBUSRTU_OPT]:0)
#define FBD_MODBUS_RETRYCOUNT   ((FBD_MODBUSRTU_OPT >> 19) & 3)
#define FBD_MODBUS_PAUSE        ((FBD_MODBUSRTU_OPT >> 21) & 1023)

// FBD_MODBUS_OPT
// |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |1 |            Pause            |  RC |SB| Par | Baud Rate |           Таймаут ответа          |
//
// 00..11: Таймаут ответа - время одидания ответа (0..4095), мс
// 12..15: Baud Rate:
//  0000 - 9600
//  0001 - 19200
//  0010 - 38400
//  0011 - 57600
//  0100 - 115200
// 16..17: Check Parity:
//  00   - None
//  01   - Odd
//  10   - Even
// 18: StopBits:
//  0    - 1
//  1    - 2
// 19..20: RC - количество повторных попыток при ошибке чтения
//  00   - 0
//  01   - 1
//  10   - 2
//  11   - 3
// 21..30: Pause - пауза между запросами (0..1023), мс
// 31:
//  0    - не менять настройки последовательного порта
//  1    - менять настройки последовательного порта

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
// возвращает указатель на текстовое описание (хинт) входа или выхода, если такого описание не найдено, то возвращает NULL
DESCR_MEM char DESCR_MEM_SUFX *fbdHMIgetIOhint(char type, char index);
//
#endif  // USE_HMI

#endif  // FBDRT_H
