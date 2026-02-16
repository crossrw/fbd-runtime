/**
 * @file fbdrt.c
 * @author crossrw1@gmail.com
 * @brief FBD-Runtime library
 * @version 11.0
 * @date 13-02-2026
 */

#include <stdlib.h>
#include <string.h>
#include "fbdrt.h"

#ifdef USE_HMI
    // для snprintf
    #include <stdio.h>
#endif

#ifdef USE_MATH
    // астротаймер
    #include "fbdsun.h"
#endif

// 06-01-2018
// + добавлены битовые операции
// + добавлен генератор сигналов
// + для входных сетевых переменных добавлено значение "по умолчанию"
// * изменен способ проверки версии RTL
// * добавлено определение USE_MATH, с ним для некоторых расчетов используется плавающая точка

// 12-01-2018
// * починил SPEED_OPT

// 14-08-2018
// + добавлены экраны интерфейса для контроллера

// 27-03-2019
// + добавлена контрольная сумма для схемы

// 29-03-2019
// + добавлены хинты для входов и выходов контроллера

// 14-11-2021
// + Modbus

// 30-09-2022
// + Задержка между запросами Modbus RTU

// 25-10-2023
// + SPEED_OPT больше не используется
// + добавлен элемент MOD
// + добавлен элемент MFUN
// + добавлен журнал событий
// + расчет количества записей в журнале
// + добавлен элемент LUT
// + добавлен элемент NLUT (Table)

// 19-06-2024
// Исправлена ошибка буфера fbdChangeVarBuf

// 13-02-2026
// Добавлен элемент SUMM (интегратор с сбросом)
// Опция сохранения значения при ошибке чтения Modbus

// -----------------------------------------------------------------------------
// FBDgetProc() и FBDsetProc() - callback, должны быть описаны в основной программе
// -----------------------------------------------------------------------------

/**
 * @brief Чтение значения входного сигнала или NVRAM.
 * Должна быть описана в основной программе.
 * 
 * @param type Tип чтения: FBD_PIN - входной сигнал, FBD_NVRAM - значение NVRAM, FBD_HRDW - значение параметра аппаратуры
 * @param index Индекс читаемого значения
 * @return tSignal Прочитанное значение сигнала
 */
extern tSignal FBDgetProc(char type, tSignal index);

/**
 * @brief Запись значения выходного сигнала или NVRAM.
 * Должна быть описана в основной программе.
 * 
 * @param type Tип записи: FBD_PIN - выходной сигнал, FBD_NVRAM - значение NVRAM
 * @param index Индекс записываемого значения
 * @param value Записываемое значение.
 */
extern void FBDsetProc(char type, tSignal index, tSignal *value);
// -----------------------------------------------------------------------------

#ifdef USE_HMI
// рисование графических примитивов
//
// рисование залитого прямоугольника
extern void FBDdrawRectangle(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color);
// рисование текста
extern void FBDdrawText(tScreenDim x1, tScreenDim y1, unsigned char font, tColor color, tColor bkcolor, bool transparent, char *text);
// рисование линии
extern void FBDdrawLine(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color);
// рисование залитого эллипса
extern void FBDdrawEllipse(tScreenDim x1, tScreenDim y1, tScreenDim x2, tScreenDim y2, tColor color);
// рисование картинки
extern void FBDdrawImage(tScreenDim x1, tScreenDim y1, tScreenDim image);
// завершение рисования экрана (копирование видеообласти)
extern void FBDdrawEnd(void);

#endif

//
void fbdCalcElement(tElemIndex index);
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value);

#ifdef USE_HMI
DESCR_MEM char DESCR_MEM_SUFX * fbdGetCaptionByIndex(tElemIndex captionIndex);
#endif

#if defined(BIG_ENDIAN) && (SIGNAL_SIZE > 1)
tSignal lotobigsign(tSignal val);
#endif // defined

#if defined(BIG_ENDIAN) && (INDEX_SIZE > 1)
tElemIndex lotobigidx(tElemIndex val);
#endif // defined

// элемент стека вычислений
typedef struct fbdstackitem_t {
    tElemIndex index;               // индекс элемента
    unsigned char input;            // номер входа
} tFBDStackItem;

static void setCalcFlag(tElemIndex element);
static void setRiseFlag(tElemIndex element);
static void setChangeVarFlag(tElemIndex index);
static void setChangeModbusFlag(tElemIndex index);
static bool getAndClearChangeModbusFlag(tElemIndex index);
static void setModbusNoResponse(tElemIndex index);
static void setModbusResponse(tElemIndex index, tSignal response);
static void fillModbusRequest(tElemIndex index, tModbusReq *mbrequest);
static void setModbusFloat(tElemIndex index, float data, float mul);
static uint32_t getCoilBitsMask(unsigned count);
static void swapModbusByteOrder(tModbusData *data);
static void swapModbusWordOrder(tModbusData *data);

static char getCalcFlag(tElemIndex element);
static char getRiseFlag(tElemIndex element);

static tSignal intAbs(tSignal val);

static tSignal interpolation(tSignal x, tSignal x1, tSignal y1, tSignal x2, tSignal y2);

#ifdef USE_EVENTS
void fbdAddLogEvent(tEventDescription eventDescription, char up);
#endif

// ----------------------------------------------------------
// массив описания схемы (расположен в ROM или RAM)
DESCR_MEM unsigned char DESCR_MEM_SUFX *fbdDescrBuf;
// формат данных:
//  TypeElement1          <- тип элемента
//  TypeElement2
//  ...
//  TypeElementN
//  -1                    <- флаг окончания описания элементов
// описания входов элемента
DESCR_MEM tElemIndex DESCR_MEM_SUFX *fbdInputsBuf;
//  InputOfElement        <- вход элемента
//  InputOfElement
//  ..
// описания параметров элементов
DESCR_MEM tSignal DESCR_MEM_SUFX *fbdParametersBuf;
//  ParameterOfElement    <- параметр элемента
//  ParameterOfElement
//  ...
// количество глобальных параметров схемы
DESCR_MEM unsigned char DESCR_MEM_SUFX *fbdGlobalOptionsCount;
// глобальные параметры схемы
DESCR_MEM tSignal DESCR_MEM_SUFX *fbdGlobalOptions;
//
#ifdef USE_HMI
// текстовые описания для HMI 
DESCR_MEM char DESCR_MEM_SUFX *fbdCaptionsBuf;
//  text, 0               <- captions
//  text, 0
//  ...
// экраны (начало выровнено по границе 4-х байт)
DESCR_MEM tScreen DESCR_MEM_SUFX *fbdScreensBuf;
// screen0
// screen1
// ...
// текстовые описания входов, выходов и событий, количество указано в параметре FBD_OPT_HINTS_COUNT
// type: 0 - вход, 1 - выход, 2 - событие
DESCR_MEM char DESCR_MEM_SUFX *fbdIOHints;
// type(char), index(char), text, 0
// type(char), index(char), text, 0
// ...
// контрольная сумма всей программы CRC32

#endif // USE_HMI

// ----------------------------------------------------------
// массив расчета схемы (расположен только в RAM)
tSignal *fbdMemoryBuf;
// формат данных:
//  OutputValue0
//  OutputValue1
//  ...
//  OutputValueN
//
// storage values
tSignal *fbdStorageBuf;
//  Storage0
//  Storage1
//  ...
//  StorageN
//
// флаги "расчет выполнен" и "нарастающий фронт", 2 бита для каждого элемента
char *fbdFlagsBuf;
//  Flags0
//  Flags1
//  ...
//  FlagsN
// флаги "значение изменилось", 1 бит для каждого элемента ELEM_OUT_VAR (14)
char *fbdChangeVarBuf;
//  ChangeVar0
//  ChangeVar1
//  ...
//  ChangeVarN
// флаги "значение изменилось", 1 бит для каждого элемента ELEM_OUT_MDBS (34)
char *fbdChangeModbusBuf;
//  ChangeModbus0
//  ChangeModbus1
//  ...
//  ChangeModbusN

tOffset *inputOffsets;
//  Смещение входа 0 элемента 0
//  Смещение входа 0 элемента 1
//  ...
//  Смещение входа 0 элемента N
tOffset *parameterOffsets;
//  Смещение параметра 0 элемента 0
//  Смещение параметра 0 элемента 1
//  ...
//  Смещение параметра 0 элемента N
tOffset *storageOffsets;
//  Смещение хранимого значения 0 элемента 0
//  Смещение хранимого значения 1
//  ...
//  Смещение хранимого значения N

#ifdef USE_HMI
// Структура для быстрого доступа к текстовым описаниям
typedef struct pointaccess_t {
    tElemIndex index;                           // point element index
    DESCR_MEM char DESCR_MEM_SUFX *caption;     // указатель на текстовую строку
} tPointAccess;
//
tPointAccess *wpOffsets;
tPointAccess *spOffsets;
#endif // USE_HMI
// указатели для быстрого доступа к текстовым описаниям проекта
DESCR_MEM char *fbdCaptionName;
DESCR_MEM char *fbdCaptionVersion;
DESCR_MEM char *fbdCaptionBTime;
//

tElemIndex fbdElementsCount;
tElemIndex fbdStorageCount;
tElemIndex fbdFlagsByteCount;
tElemIndex fbdChangeVarByteCount;
tElemIndex fbdChangeModbusByteCount;

tElemIndex fbdModbusRTUCount;
tElemIndex fbdModbusTCPCount;
tElemIndex fbdModbusRTUIndex;
tElemIndex fbdModbusTCPIndex;
short      fbdModbusRTUTimer;
short      fbdModbusRTUDelayTimer;
short      fbdModbusTCPTimer;
short      fbdLastScreenIndex;

unsigned char fbdModbusRTUErrorCounter;
unsigned char fbdModbusTCPErrorCounter;

#ifdef USE_HMI
tElemIndex fbdWpCount;
tElemIndex fbdSpCount;
short      fbdCurrentScreen;                    // текущий экран
unsigned short fbdCurrentScreenTimer;           // таймер текущего экрана

#ifdef USE_EVENTS
tElemIndex      fbdStartEventLog;               // указатель на начало журнала
unsigned char   fbdEventActiveFlags[32];        // флаги состояния событий: 256/8
tEventFlagsView fbdEventActiveTime[256];        // фиксация начала событий

bool intCurrentEventChanged;                    // признаки изменения
bool intLogEventChanged;                        // признаки изменения

#endif // USE_EVENTS
#endif // USE_HMI
//
char fbdFirstFlag;

// массив с количествами входов для элементов каждого типа
ROM_CONST uint8_t ROM_CONST_SUFX FBDdefInputsCount[ELEM_TYPE_COUNT]       = {1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,4,3,3,5,1,1,0,2,2,2,3,2,2,2,2,2,0,1,2,0,1,5,1, 5};
// массив с количествами параметров для элементов каждого типа
ROM_CONST uint8_t ROM_CONST_SUFX FBDdefParametersCount[ELEM_TYPE_COUNT]   = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,0,0,0,0,0,1,5,0,0,0,0,0,0,0,0,1,3,2,0,4,2,1,66,0};
// массив с количествами хранимых данных для элементов каждого типа
ROM_CONST uint8_t ROM_CONST_SUFX FBDdefStorageCount[ELEM_TYPE_COUNT]      = {0,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,1,2,1,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0, 1};
//                                                                           0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3 3  4
//                                                                           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9  0
// Параметры элемента ELEM_INP_MDBS (чтение Modbus)
//
// |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |  FMT/CNT  |WO|BO| FNC |   Адрес устройства    |             Адрес регистра MODBUS             |
//
// BO   - byte order: 0 - HiLo, 1 - LoHi
// WO   - word order: 0 - HiLo, 1 - LoHi
// FNC  - Modbus function: 
//  00 - read coil (0x01)
//  01 - read discrete input (0x02)
//  10 - read holding registers (0x03)
//  11 - read input registers (0x04)
// FMT/CNT - формат читаемых данных (для "read input registers" и "read holding registers") или количество регистров для чтения (для "read coil" и "read discrete input")
// FMT  - формат читаемых данных (для "read input registers" и "read holding registers"):
//  0000    - uint16 (0..65535) 1 регистр
//  0001    - int16 (0..65535)  1 регистр
//  0010    - int32             2 регистра
//  MM11    - single            2 регистра, MM - множитель 00 - 1, 01 - 10, 10 - 100, 11 - 1000
// CNT  - количество читаемых бит (для "read coil" и "read discrete input"):
//  WO = 0
//  0000    - 1
//  ...
//  1111    - 16
//  WO = 1
//  0000    - 17
//  ...
//  1111    - 32
//
// Параметры элемента ELEM_OUT_MDBS (запись Modbus)
//
// |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
// +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// |  FMT/CNT  |WO|BO| FNC |   Адрес устройства    |             Адрес регистра MODBUS             |
//
// BO   - byte order: 0 - HiLo, 1 - LoHi
// WO   - word order: 0 - HiLo, 1 - LoHi
// FNC  - Modbus function: 
//  00 - Write Single Coil (0x05)
//  01 - Write Multiple Coils (0x0F)
//  10 - Write Single Register (0x06)
//  11 - Write Multiple registers (0x10)
// FMT/CNT - формат записываемых данных (для "Write Single Register" и "Write Multiple registers") или количество регистров для записи (для "Write Multiple Coils")
// FMT  - формат данных (для "Write Single Register" и "Write Multiple registers"):
//  0000    - uint16 (0..65535) 1 регистр
//  0001    - int16 (0..65535)  1 регистр
//  0010    - int32             2 регистра
//  MM11    - single            2 регистра, MM - делитель 00 - 1, 01 - 10, 10 - 100, 11 - 1000 (только Write Multiple registers)
// CNT  - количество записываемых бит (для "Write Multiple Coils"):
//  WO = 0
//  0000    - 1
//  ...
//  1111    - 16
//  WO = 1
//  0000    - 17
//  ...
//  1111    - 32
//

// описания "быстрых" вариантов функций
// --------------------------------------------------------------------------------------------
#define FBDINPUTOFFSET(index) *(inputOffsets + (index))
#define FBDGETPARAMETER(element,index) SIGNAL_BYTE_ORDER(fbdParametersBuf[*(parameterOffsets+element)+index])
#define FBDGETSTORAGE(element,index) fbdStorageBuf[*(storageOffsets+element)+index]

/**
 * @brief Расчёт контрльной суммы CRC32
 * Используется при проверке корректности программы.
 * @param data Указатель на массив данных
 * @param size Размер массива данных
 * @return uint32_t Результат вычисления
 */
static uint32_t fbdCRC32(DESCR_MEM unsigned char DESCR_MEM_SUFX *data, int size)
{
    uint32_t crc = ~0;
    //
    while(size--) {
        crc ^= *data++;
        for(int i = 0; i < 8; i++) {
            uint32_t t = ~((crc&1)-1);
            crc = (crc>>1) ^ (0xEDB88320 & t);
        }
    }
    return crc;
}

// -------------------------------------------------------------------------------------------------------

/**
 * @brief Инициализация схемы
 * Функция должна быть вызвана один раз в самом начале работы.
 * Результат: количество ОЗУ, необходимое для выполнения схемы (значение больше 0) или (в случае ошибки) отрицательное значение:
 * ERR_INVALID_ELEMENT_TYPE - неверный код элемента в описании схемы
 * ERR_INVALID_SIZE_TYPE    - неверный размер tSignal или tElementIndex
 * ERR_INVALID_LIB_VERSION  - несовпадает версия программы и библиотеки
 * ERR_INVALID_CHECK_SUM    - неверная контрольная сумма программы

 * @param buf Указатель на массив описания схемы
 * @return Количество байт RAM, необходимое для выполнения схемы или код ошибки
 */
int fbdInit(DESCR_MEM unsigned char DESCR_MEM_SUFX *buf)
{
    tOffset inputs = 0;
    tOffset parameters = 0;
    unsigned char elem;
    //
    fbdElementsCount = 0;
    fbdStorageCount = 0;
    fbdChangeVarByteCount = 0;
    fbdChangeModbusByteCount = 0;
    //
    fbdModbusRTUIndex = MAX_INDEX;
    fbdModbusTCPIndex = MAX_INDEX;
    fbdModbusRTUErrorCounter = 0;
    fbdModbusTCPErrorCounter = 0;
    fbdModbusRTUCount = 0;
    fbdModbusTCPCount = 0;
#ifdef USE_HMI
    fbdWpCount = 0;
    fbdSpCount = 0;
    DESCR_MEM char DESCR_MEM_SUFX *curCap;
    tElemIndex i;
#endif // USE_HMI
    //
    if(!buf) return ERR_INVALID_ELEMENT_TYPE;
    fbdDescrBuf = buf;
    // цикл по всем элементам
    while(1) {
        elem = fbdDescrBuf[fbdElementsCount];
        if(elem & ELEMENDFLAG) break;
        elem &= ELEMMASK;
        if(elem >= ELEM_TYPE_COUNT) return ERR_INVALID_ELEMENT_TYPE;
        // подсчет всех входов
        inputs += FBDdefInputsCount[elem];
        // подсчет всех параметров
        parameters += FBDdefParametersCount[elem];
        // подсчет всех хранимых параметров
        fbdStorageCount += FBDdefStorageCount[elem];
        // подсчёт элементов некоторых типов
        switch (elem) {
            case ELEM_OUT_VAR:
                fbdChangeVarByteCount++;                // подсчёт выходных переменных
                break;
            case ELEM_OUT_MDBS:
                fbdChangeModbusByteCount++;             // подсчёт элементов записи в Modbus
                break;
#ifdef USE_HMI
            case ELEM_WP:
                fbdWpCount++;                           // точки контроля
                break;
            case ELEM_SP:
                fbdSpCount++;                           // точки регулирования
                break;
#endif // USE_HMI
        }
        // общий подсчёт элементов
        fbdElementsCount++;
    }
    // проверка правильности флага завершения
    if(elem != END_MARK) return ERR_INVALID_SIZE_TYPE;
    // расчет указателей
    fbdInputsBuf = (DESCR_MEM tElemIndex DESCR_MEM_SUFX *)(fbdDescrBuf + fbdElementsCount + 1);
    fbdParametersBuf = (DESCR_MEM tSignal DESCR_MEM_SUFX *)(fbdInputsBuf + inputs);
    fbdGlobalOptionsCount = (DESCR_MEM unsigned char DESCR_MEM_SUFX *)(fbdParametersBuf + parameters);
    fbdGlobalOptions = (DESCR_MEM tSignal DESCR_MEM_SUFX *)(fbdGlobalOptionsCount + 1);
    //
    // проверка версии программы
    if(*fbdGlobalOptions > FBD_LIB_VERSION) return ERR_INVALID_LIB_VERSION;
    //
    // расчёт и проверка CRC, если указан параметр FBD_SCHEMA_SIZE
    if(FBD_SCHEMA_SIZE) {
        // если результат 0, то CRC не используется (старая версия редактора)
        if(fbdCRC32(fbdDescrBuf, FBD_SCHEMA_SIZE)) return ERR_INVALID_CHECK_SUM;
    }
#ifdef USE_HMI
    // указатель на начало текстовых строк точек контроля и регулирования
    fbdCaptionsBuf = (DESCR_MEM char DESCR_MEM_SUFX *)(fbdGlobalOptions + *fbdGlobalOptionsCount);
    // после текстовых строк точек контроля и регулирования идут еще 3 строки: имя проекта, версия проекта, дата создания проекта
    // расчет указателя на первый экран
    // первый экран идет после текстовых описаний в количестве (fbdWpCount + fbdSpCount), причем его начало выровнено на 4 байта
    curCap = fbdCaptionsBuf;
    // перебираем все строки текстовых описаний
    // 3 - это имя проекта, версия проекта, дата создания проекта
    for(i=0; i < (fbdWpCount + fbdSpCount + 3); i++) {
        while(*(curCap++));
    }
    // выравнивание curCap по границе 32 бита
    while((int)curCap % 4) curCap++;
    // ставим указатель на начало экранов
    fbdScreensBuf = (DESCR_MEM tScreen DESCR_MEM_SUFX *)curCap;
    // хинты входов и выходов идут после экранов
    if(FBD_HINTS_COUNT > 0) {
        // перебираем все экраны (если экраны есть) в поисках окончания
        DESCR_MEM tScreen DESCR_MEM_SUFX *screen = fbdScreensBuf;
        i = 0;
        while (i < FBD_SCREEN_COUNT) {
            screen = (tScreen *)((char *)screen + screen->len);
            i++;
        }
        fbdIOHints = (DESCR_MEM char DESCR_MEM_SUFX *)screen;
    }
    //
#ifdef USE_EVENTS
    // указатель на начало журнала
    fbdStartEventLog = fbdStorageCount;
    // должен быть чётным
    if(fbdStartEventLog & 1) fbdStartEventLog++;
#endif // USE_EVENTS
#endif // USE_HMI
    //
    // память для флагов расчета и фронта
    fbdFlagsByteCount = (fbdElementsCount>>2) + ((fbdElementsCount&3)?1:0);
    // память для флагов изменений значения выходной переменной
    fbdChangeVarByteCount = (fbdChangeVarByteCount>>3) + ((fbdChangeVarByteCount&7)?1:0);
    // память для флагов изменений значения записи в Modbus
    fbdChangeModbusByteCount = (fbdChangeModbusByteCount>>3) + ((fbdChangeModbusByteCount&7)?1:0);
    //
#ifdef USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdChangeModbusByteCount + fbdElementsCount*3*sizeof(tOffset) + (fbdWpCount+fbdSpCount)*sizeof(tPointAccess);
#else  // USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdChangeModbusByteCount + fbdElementsCount*3*sizeof(tOffset);
#endif // USE_HMI
}

/**
 * @brief Инициализация памяти схемы
 * Функция должна быть вызвана после fbdInit().
 *
 * @param buf Указатель на буфер памяти (размер возвращается fbdInit()), используемой при расчёте схемы
 * @param needReset Признак необходимости сброса NVRAM после загрузки новой (ранее не выполнявщейся) схемы
 */
void fbdSetMemory(char *buf, bool needReset)
{
    tElemIndex i;
#ifdef USE_HMI
    tHMIdata hmiData;
#endif // USE_HMI
    tOffset curInputOffset = 0;
    tOffset curParameterOffset = 0;
    tOffset curStorageOffset = 0;
#ifdef USE_HMI
    tOffset curWP = 0;
    tOffset curSP = 0;
    DESCR_MEM char DESCR_MEM_SUFX *curCap;
#endif // USE_HMI
    fbdModbusRTUIndex = MAX_INDEX;
    fbdModbusTCPIndex = MAX_INDEX;
    //
    fbdModbusRTUErrorCounter = 0;
    fbdModbusTCPErrorCounter = 0;
    //
    fbdModbusRTUTimer = 0;
    fbdModbusTCPTimer = 0;
    fbdModbusRTUDelayTimer = 0;
    //
    fbdModbusRTUCount = 0;
    fbdModbusTCPCount = 0;
    //
    fbdMemoryBuf        = (tSignal *)buf;
    // инициализация указателей
    fbdStorageBuf       = fbdMemoryBuf + fbdElementsCount;
    fbdFlagsBuf         = (char *)(fbdStorageBuf + fbdStorageCount);
    fbdChangeVarBuf     = (char *)(fbdFlagsBuf + fbdFlagsByteCount);
    fbdChangeModbusBuf  = (char *)(fbdChangeVarBuf + fbdChangeVarByteCount);
    // инициализация памяти (выходы элементов)
    memset(fbdMemoryBuf, 0, sizeof(tSignal)*fbdElementsCount);
    // инициализация памяти (установка всех флагов изменения значения)
    fbdChangeAllNetVars();
    // установка всех флагов изменения значений записи в Modbus
    if(fbdChangeModbusByteCount > 0) memset(fbdChangeModbusBuf, 255, fbdChangeModbusByteCount);
    // восстановление значений триггеров из nvram
    for(i = 0; i < fbdStorageCount; i++) {
        if(needReset) {
            fbdStorageBuf[i] = 0;
            FBDsetProc(FBD_NVRAM, i, &fbdStorageBuf[i]);
        } else {
            fbdStorageBuf[i] = FBDgetProc(FBD_NVRAM, i);
        }
    }
    // инициализация буферов быстрого доступа
    inputOffsets = (tOffset *)(fbdChangeModbusBuf + fbdChangeModbusByteCount);
    parameterOffsets = inputOffsets + fbdElementsCount;
    storageOffsets = parameterOffsets + fbdElementsCount;
    //
    fbdCaptionName = 0;
    fbdCaptionVersion = 0;
    fbdCaptionBTime = 0;
#ifdef USE_HMI
    // инициализация буферов для быстрого доступа к watch- и set- points
    wpOffsets = (tPointAccess *)(storageOffsets + fbdElementsCount);
    spOffsets = wpOffsets + fbdWpCount;
    curCap = fbdCaptionsBuf;
#endif // USE_HMI
    for(i=0; i < fbdElementsCount; i++) {
        *(inputOffsets + i) = curInputOffset;
        *(parameterOffsets + i) = curParameterOffset;
        *(storageOffsets + i) = curStorageOffset;
        //
        unsigned char elem = fbdDescrBuf[i] & ELEMMASK;
        curInputOffset += FBDdefInputsCount[elem];
        curParameterOffset += FBDdefParametersCount[elem];
        curStorageOffset += FBDdefStorageCount[elem];
        //
#ifdef USE_HMI
        switch(elem) {
            case ELEM_WP:
                (wpOffsets + curWP)->index = i;
                (wpOffsets + curWP)->caption = curCap;
                curWP++;
                while(*(curCap++));
                break;
            case ELEM_SP:
                (spOffsets + curSP)->index = i;
                (spOffsets + curSP)->caption = curCap;
                curSP++;
                while(*(curCap++));
                break;
        }
#endif // USE_HMI
    }
    // подсчёт количества элементов Modbus и инициализация значений чтения Modbus
    // цикл по всем элементам
    for(i=0; i < fbdElementsCount; i++) {
        switch (fbdDescrBuf[i] & ELEMMASK) {
            case ELEM_INP_MDBS:
                // установка значения элемента на значение по умолчанию
                // параметр 2: значение по умолчанию
                fbdMemoryBuf[i] = FBDGETPARAMETER(i, 2);
            case ELEM_OUT_MDBS:
                // параметр 0: IP-адрес
                if(FBDGETPARAMETER(i, 0)) fbdModbusTCPCount++; else fbdModbusRTUCount++;
        }
    }
    // инициализация входных переменных
    if(needReset) {
        // при загрузке новой схемы значение входных сетевых переменных (до того, как они будут получены) равны 0, это не совсем хорошо...
        // инициализируем их значениями "по умолчанию" из программы
        for(i = 0; i < fbdElementsCount; i++) {
            if(fbdDescrBuf[i] == ELEM_INP_VAR) {
                fbdSetStorage(i, 0, FBDGETPARAMETER(i, 1));
            }
        }
    }
#ifdef USE_HMI
    // инициализация значений точек регулирования (HMI setpoints)
    i = 0;
    while(fbdHMIgetSP(i, &hmiData)) {
        // если значение точки регулирования не корректное, то устанавливаем значение по умолчанию
        if(needReset||(hmiData.value > hmiData.upperLimit)||(hmiData.value < hmiData.lowlimit)) fbdHMIsetSP(i, hmiData.defValue);
        i++;
    }
    //
    fbdCurrentScreen = -1;
    fbdCurrentScreenTimer = 0;
    //
#ifdef USE_EVENTS
    //
    // очистка флагов событий
    memset(fbdEventActiveFlags, 0, sizeof(fbdEventActiveFlags));
    //
    // если была перезапись схемы, то очищаем журнал
    if(needReset) fbdClearLogEvent();
    //
    // признаки изменения журналов
    intCurrentEventChanged = true;
    intLogEventChanged = true;
#endif // USE_EVENTS
#endif // USE_HMI
    //
    fbdFirstFlag = 1;
}

// -------------------------------------------------------------------------------------------------------
#ifdef USE_HMI

/**
 * @brief Проверка видимости экранного элемента
 * 
 * @param elem Указатель на описание экранного элемента
 * @return true Элемент видим
 * @return false Элемент не видим
 */
static bool isScrElemVisible(const tScrElemBase *elem)
{
    if(elem->visibleElem == 0xffff) return true;    // не выбран элемент видимости
    switch(elem->visibleCond) {
        case 0:
            return true;                            // условие "всегда"
        case 1:                                     // условие "равно"
            return fbdMemoryBuf[elem->visibleElem] == elem->visibleValue;
        case 2:                                     // условие "не равно"
            return fbdMemoryBuf[elem->visibleElem] != elem->visibleValue;
        case 3:                                     // условие "больше"
            return fbdMemoryBuf[elem->visibleElem] > elem->visibleValue;
        case 4:                                     // условие "меньше"
            return fbdMemoryBuf[elem->visibleElem] < elem->visibleValue;
        default:
            return true;
    }
}

static tSignal getElementOutputValue(tElemIndex index)
{
    if(index == 0xffff) return 0;
    return fbdMemoryBuf[index];
}

static void sprintf2d(char *buf, tSignal val)
{
    *(buf + 1) = val % 10 + '0';
    val /= 10;
    *(buf) = val % 10 + '0';
}

static void sprintf4d(char *buf, tSignal val)
{
    *(buf + 3) = val % 10 + '0';
    val /= 10;
    *(buf + 2) = val % 10 + '0';
    val /= 10;
    *(buf + 1) = val % 10 + '0';
    val /= 10;
    *(buf) = val % 10 + '0';
}

/**
 * @brief Отрисовка экрана
 * 
 * @param screen Указатель на описание экрана
 */
static void drawCurrentScreen(DESCR_MEM tScreen DESCR_MEM_SUFX *screen)
{
    tScrElemBase *elem;
    char dttext[32];
    char text[32];          // больше на экран не лезет
    tSignal v2;
    int i,j,k;
    char c,nc;
    float f;
    // очистка экрана
    FBDdrawRectangle(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, screen->bkcolor);
    //
    // цикл по элементам экрана
    i = 0;
    elem = (tScrElemBase *)((char *)screen + sizeof(tScreen));
    while(i++ < screen->elemCount) {
        // проверка видимости
        if(isScrElemVisible(elem)) {
            // рисование
            switch(elem->type) {
                case 0:                             // линия
                    k = ((tScrElemLine *)elem)->width;
                    if(k > 1) {
                        k--;
                        for(j=-k/2; j<=k/2 + k%2; j++) {
                            FBDdrawLine(lroundf(elem->x1-j*((tScrElemLine *)elem)->sine), lroundf(elem->y1+j*((tScrElemLine *)elem)->cosinus), lroundf(((tScrElemLine *)elem)->x2-j*((tScrElemLine *)elem)->sine), lroundf(((tScrElemLine *)elem)->y2+j*((tScrElemLine *)elem)->cosinus), ((tScrElemLine *)elem)->color);
                        }
                    } else {
                        FBDdrawLine(elem->x1, elem->y1, ((tScrElemLine *)elem)->x2, ((tScrElemLine *)elem)->y2, ((tScrElemLine *)elem)->color);
                    }
                    break;
                case 1:                             // прямоугольник
                    FBDdrawRectangle(elem->x1, elem->y1, ((tScrElemRect *)elem)->x2, ((tScrElemRect *)elem)->y2, ((tScrElemRect *)elem)->color);
                    break;
                case 2:                             // текст
                    f = getElementOutputValue(((tScrElemText *)elem)->valueElem) * (float)1.0;
                    switch (((tScrElemText *)elem)->divider) {
                        case 1:
                            f = f / (float)10.0;
                            break;
                        case 2:
                            f = f / (float)100.0;
                            break;
                        case 3:
                            f = f / (float)1000.0;
                    }
                    // форматирование даты и времени
                    j = 0;
                    k = 0;
                    do {
                        c = ((tScrElemText *)elem)->text[j++];
                        if(c == '%') {
                            nc = ((tScrElemText *)elem)->text[j];
                            switch(nc) {
                                case 'd':
                                    if(k < (sizeof(dttext)-2))  {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_DAY));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'm':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_MONTH));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'y':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_YEAR)-2000);
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'h':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_HOUR));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'n':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_MINUTE));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 's':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_SECOND));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'Y':
                                    if(k < (sizeof(dttext)-4)) {
                                        sprintf4d(&dttext[k], FBDgetProc(FBD_PIN, GP_RTC_YEAR));
                                        k += 4;
                                    }
                                    j++;
                                    break;
                                default:
                                    dttext[k++] = c;
                            }
                        } else {
                            dttext[k++] = c;
                        }
                    } while (c);
                    //
                    snprintf(text, sizeof(text), dttext, f);
                    FBDdrawText(elem->x1, elem->y1, ((tScrElemText *)elem)->font & 0x7f, ((tScrElemText *)elem)->color, ((tScrElemText *)elem)->bkcolor, (((tScrElemText *)elem)->font & 0x80) != 0, text);
                    break;
                case 3:                             // картинка
                    FBDdrawImage(elem->x1, elem->y1, ((tScrElemImage *)elem)->index);
                    break;
                case 4:                             // шкала
                    v2 = getElementOutputValue(((tScrElemGauge *)elem)->valueElem);
                    // нормирование значения
                    if(v2 < 0) v2 = 0; else
                    if(v2 > ((tScrElemGauge *)elem)->maxvalue) v2 = ((tScrElemGauge *)elem)->maxvalue;
                    //
                    if(((tScrElemGauge *)elem)->orientation == 0) {
                        // горизонтальная ориентация
                        v2 = elem->x1 + v2*(((tScrElemGauge *)elem)->x2 - elem->x1) / ((tScrElemGauge *)elem)->maxvalue;
                        // рисуем прямоугольник значения
                        FBDdrawRectangle(elem->x1, elem->y1, v2, ((tScrElemGauge *)elem)->y2, ((tScrElemGauge *)elem)->color);
                        if(v2 < ((tScrElemGauge *)elem)->x2) {
                            // рисуем прямоугольник остатка
                            FBDdrawRectangle(v2, elem->y1, ((tScrElemGauge *)elem)->x2, ((tScrElemGauge *)elem)->y2, ((tScrElemGauge *)elem)->bkcolor);
                        }
                    } else {
                        // вертикальная ориентация
                        v2 = ((tScrElemGauge *)elem)->y2 - v2*(((tScrElemGauge *)elem)->y2 - elem->y1) / ((tScrElemGauge *)elem)->maxvalue;
                        // рисуем прямоугольник значения
                        FBDdrawRectangle(elem->x1, v2, ((tScrElemGauge *)elem)->x2, ((tScrElemGauge *)elem)->y2, ((tScrElemGauge *)elem)->color);
                        if(elem->y1 < v2) {
                            // рисуем прямоугольник остатка
                            FBDdrawRectangle(elem->x1, elem->y1, ((tScrElemGauge *)elem)->x2, v2, ((tScrElemGauge *)elem)->bkcolor);
                        }
                    }
                    break;
                case 5:                             // эллипс
                    FBDdrawEllipse(elem->x1, elem->y1, ((tScrElemCircle *)elem)->x2, ((tScrElemCircle *)elem)->y2, ((tScrElemCircle *)elem)->color);
                    break;
            }
        }
        // переход к следующему элементу
        elem = (tScrElemBase *)((char *)elem + elem->len);
    }
    //
    FBDdrawEnd();
}

/**
 * @brief Выполнение одного шага вычисления схемы.
 * Функция выполняет один шаг вычисления схемы и перерисовку указанноего экрана при необходимости.
 * 
 * @param period Время с момента предыдущего вызова fbdDoStepEx(), например в мс.
 * @param screenIndex Индекс текущего отображаемого экрана.
 */
void fbdDoStepEx(tSignal period, short screenIndex)
{
    DESCR_MEM tScreen DESCR_MEM_SUFX *screen;
    int i;
    //
    fbdLastScreenIndex = screenIndex;
    // расчет схемы
    fbdDoStep(period);
    // проверка корректности номера экрана
    if(screenIndex >= FBD_SCREEN_COUNT) return;
    // не показывать экран
    if(screenIndex < 0) {
        fbdCurrentScreen = screenIndex;
        return;
    }
    // расчет указателя на экран
    i = 0;
    screen = fbdScreensBuf;
    while (i < screenIndex) {
        screen = (tScreen *)((char *)screen + screen->len);
        i++;
    }
    // проверка необходимости перерисовки
    if(screenIndex != fbdCurrentScreen) {
        // сменился экран
        fbdCurrentScreen = screenIndex;
        fbdCurrentScreenTimer = 0;
        drawCurrentScreen(screen);
    } else {
        fbdCurrentScreenTimer += period;
        if(fbdCurrentScreenTimer >= screen->period) {
            fbdCurrentScreenTimer = 0;
            drawCurrentScreen(screen);
        }
    }
}
#endif // USE_HMI

/**
 * @brief Выполнение одного шага вычисления схемы
 * Функция выполняет один шаг вычисления схемы
 * 
 * @param period время с момента предыдущего вызова fbdDoStep(), например в милисекундах
 */
void fbdDoStep(tSignal period)
{
#ifdef USE_EVENTS
    tEventDescriptionView eventFlags;
    unsigned char imessage;
    bool eventActive;
    bool eventCondMet;
#endif
    tSignal value, param;
    tElemIndex index;
    // сброс признаков расчета и нарастающего фронта
    memset(fbdFlagsBuf, 0, fbdFlagsByteCount);
    // модификация таймеров Modbus
    if(period) {
        if(period <= fbdModbusRTUDelayTimer) fbdModbusRTUDelayTimer -= period; else fbdModbusRTUDelayTimer = 0;
        if(period <= fbdModbusRTUTimer) fbdModbusRTUTimer -= period; else fbdModbusRTUTimer = 0;
        if(period <= fbdModbusTCPTimer) fbdModbusTCPTimer -= period; else fbdModbusTCPTimer = 0;
    }
    // основной цикл расчета
    for(index=0; index < fbdElementsCount; index++) {
        unsigned char element = fbdDescrBuf[index] & ELEMMASK;
        switch(element) {
            // элементы с таймером
            case ELEM_TON:                                                  // timer TON
            case ELEM_PID:                                                  // PID
            case ELEM_SUM:                                                  // SUM
            case ELEM_SUMM:                                                 // SUMM
            case ELEM_TP:                                                   // timer TP
            case ELEM_GEN:                                                  // GEN
                if(!period) break;
                value = FBDGETSTORAGE(index, 0);
                if(value) {
                    value -= period;
                    if(value < 0) value = 0;
                    fbdSetStorage(index, 0, value);
                }
                break;
            case ELEM_OUT_PIN:                                              // output PIN
                fbdCalcElement(index);
                param = FBDGETPARAMETER(index, 0);
                FBDsetProc(FBD_PIN, param, &fbdMemoryBuf[index]);           // установка значения выходного контакта
                break;
            case ELEM_OUT_VAR:                                              // выходная переменная
            case ELEM_OUT_MDBS:                                             // запись Modbus
#ifdef USE_HMI
            case ELEM_WP:                                                   // точка контроля
            case ELEM_EVENT:                                                // запись журнала
#endif // USE_HMI
                fbdCalcElement(index);
                //
#ifdef USE_EVENTS
                if(element == ELEM_EVENT) {
                    //
                    value = FBDGETPARAMETER(index, 0);
                    eventFlags.value = FBDGETPARAMETER(index, 1);
                    imessage = eventFlags.flags.imessage;
                    eventActive = (fbdEventActiveFlags[imessage>>3]&(1u<<(imessage&7))) != 0;
                    //
                    switch (eventFlags.flags.conditions) {
                        case FBD_EVENT_COND_EQU:
                            eventCondMet = (fbdMemoryBuf[index] == value);
                            break;
                        case FBD_EVENT_COND_NOTEQU:
                            eventCondMet = (fbdMemoryBuf[index] != value);
                            break;
                        case FBD_EVENT_COND_GREATER:
                            eventCondMet = (fbdMemoryBuf[index] > value);
                            break;
                        case FBD_EVENT_COND_LESS:
                            eventCondMet = (fbdMemoryBuf[index] < value);
                            break;
                        default:
                            eventCondMet = false;
                    }
                    //
                    if(eventCondMet != eventActive) {
                        // событие изменило своё состояние
                        if(eventCondMet) {
                            // событие стало активным
                            if((eventFlags.flags.logBegin)) {
                                fbdAddLogEvent(eventFlags.flags, 1);
                            }
                            // ставим флаг активности события
                            fbdEventActiveFlags[imessage>>3] |= 1u<<(imessage&7);
                        } else {
                            // событие перестало быть активным
                            if(!eventFlags.flags.cr) {
                                // если стоит флаг необходимости подтверждения, то активность не снимаем!
                                if((eventFlags.flags.logEnd)) {
                                    fbdAddLogEvent(eventFlags.flags, 0);
                                }
                                // снимаем флаг активности события
                                fbdEventActiveFlags[imessage>>3] &= ~(1u<<(imessage&7));
                            }
                        }
                        intCurrentEventChanged = true;
                    }
                }
#endif // USE_EVENT
                break;
        }
    }
    //
    fbdFirstFlag = 0;
}

// -------------------------------------------------------------------------------------------------------

/**
 * @brief Установить принятое по сети значение переменной
 * 
 * @param netvar Указатель на структуру описания сетевой переменной
 */
void fbdSetNetVar(const tNetVar *netvar)
{
    tElemIndex i;
    //
    for(i=0; i < fbdElementsCount; i++) {
        if(fbdDescrBuf[i] == ELEM_INP_VAR) {
            if(FBDGETPARAMETER(i, 0) == netvar->index) {
                fbdSetStorage(i, 0, netvar->value);
                return;
            }
        }
    }
    return;
}

/**
 * @brief Получить значение переменной для отправки по сети
 * Функцию необходимо вызывать до тех пор, пока она не вернет false.
 * 
 * @param netvar Указатель на структуру описания сетевой переменной
 * @return true Переменная для отправки есть, она помещена в структуру netvar
 * @return false Переменных для отправки больше нет
 */
bool fbdGetNetVar(tNetVar *netvar)
{
    tElemIndex i, varindex;
    //
    varindex = 0;
    for(i=0; i < fbdElementsCount; i++) {
        if(fbdDescrBuf[i] == ELEM_OUT_VAR) {
            // проверяем установку флага изменений
            if(fbdChangeVarBuf[varindex>>3]&(1u<<(varindex&7))) {
                // флаг установлен, сбрасываем его
                fbdChangeVarBuf[varindex>>3] &= ~(1u<<(varindex&7));
                // номер переменной
                netvar->index = FBDGETPARAMETER(i, 0);
                netvar->value = fbdMemoryBuf[i];
                return true;
            }
            varindex++;
        }
    }
    return false;
}

/**
 * @brief Установить для всех выходных сетевых переменных признак изменения
 * Функцию можно вызывать периодически для принудительной отправки всех переменных.
 */
void fbdChangeAllNetVars(void)
{
    if(fbdChangeVarByteCount > 0) memset(fbdChangeVarBuf, 255, fbdChangeVarByteCount);
}

/**
 * @brief Получение статуса использования Modbus схемой
 * 
 * @return tFBD_MODBUS_USAGE Статус использования Modbus
 */
tFBD_MODBUS_USAGE fbdModbusUsage(void)
{
    tFBD_MODBUS_USAGE modbusUsage = FBD_MODBUS_NONE;
    //
    if(fbdModbusRTUCount) modbusUsage |= FBD_MODBUS_RTU;
    if(fbdModbusTCPCount) modbusUsage |= FBD_MODBUS_TCP;
    //
    return modbusUsage;
}

/**
 * @brief Получение значений настроек Modbus RTU.
 * Возвращает набор настроек последовательного порта RS485 для использования обмена по протоколу Modbus RTU
 * 
 * @param pnt Указатель на структуру описания настроек
 * @return true Необходимо применить возвращённый набор настроек
 * @return false Необходимо оставить текущие настройки контроллера
 */
bool fbdModbusGetSerialSettings(tModbusRTUsettings *pnt)
{
    // проверка количества элементов работы с ModbusRTU
    if(!fbdModbusRTUCount) return false;
    //
    tSignal serialSettings = FBD_MODBUSRTU_OPT;
    // проверка бита необходимости установки настроек
    if(!(serialSettings & 0x80000000)) return false;
    // результат положительный
    if(!pnt) return true;
    // заполняем структуру
    pnt->timeout = serialSettings & 0x0fff;
    pnt->baudRate = (tFBD_BAUDRATE)((serialSettings >> 12) & 15);
    pnt->parity = (tFBD_PARITY)((serialSettings >> 16) & 3);
    pnt->stopBits = (tFBD_STOPB)((serialSettings >> 18) & 1);
    //
    return true;
}

/**
 * @brief Получение следующего запроса Modbus RTU для выполнения.
 * 
 * @param mbrequest Указатель на структуру описания запроса Modbus RTU
 * @return true Возвращён очередной запрос Modbus RTU
 * @return false Запросов Modbus RTU пока больше нет
 */
bool fbdGetNextModbusRTURequest(tModbusReq *mbrequest)
{
    if(!fbdModbusRTUCount) return false;
    // поиск следующего элемента Modbus RTU
    // проверка на паузу между любыми идущими подряд запросами (борьба с Danfoss MCX)
    if(fbdModbusRTUDelayTimer) return false;
    // проверка необходимости повторного запроса
    if(fbdModbusRTUErrorCounter) {
        // необходим повторный запрос
        fbdModbusRTUErrorCounter--;
        fillModbusRequest(fbdModbusRTUIndex, mbrequest);
        //
        return true;
    } else {
        tElemIndex index, i;
        // поиск следующего элемента
        index = fbdModbusRTUIndex;
        for(i=0; i < fbdElementsCount; i++) {
            unsigned char elem;
            // переходим к следующему элементу
            index++;
            if(index >= fbdElementsCount) {
                // проверка таймера минимального периода опроса
                if(fbdModbusRTUTimer) return false;
                fbdModbusRTUTimer = FBD_MODBUS_PAUSE;
                index = 0;
            }
            elem = fbdDescrBuf[index] & ELEMMASK;
            if((elem == ELEM_INP_MDBS)||(elem == ELEM_OUT_MDBS)) {
                // проверяем тип Modbus
                if(!FBDGETPARAMETER(index, 0)) {
                    // это Modbus RTU
                    if(elem == ELEM_OUT_MDBS) {
                        // если это элемент записи, то проверяем флаг изменений
                        // если флаг не установлен, то переходим к следующему элементу
                        // если установлен, то формируем команду записи
                        if(!getAndClearChangeModbusFlag(index)) continue;
                        // повторные запросы при записи не используем, они и так будут работать из-за флагов изменения
                        fbdModbusRTUErrorCounter = 0;
                    } else {
                        // устанавливаем количество повторных запросов
                        fbdModbusRTUErrorCounter = FBD_MODBUS_RETRYCOUNT;
                    }
                    // нашли подходящий элемент
                    fbdModbusRTUIndex = index;
                    fillModbusRequest(index, mbrequest);
                    return true;
                }
            }
        }
    }
    //
    return false;
}

/**
 * @brief Установка результата успешного выполнения запроса Modbus RTU.
 * Должна быть вызвана после успешного выполнения запроса чтения или записи Modbus RTU.
 * @param response Данные, возвращённые запросом
 */
void fbdSetModbusRTUResponse(tSignal response)
{
    fbdModbusRTUErrorCounter = 0;
    fbdModbusRTUDelayTimer = FBD_MODBUSRTU_DELAY;
    setModbusResponse(fbdModbusRTUIndex, response);
}

/**
 * @brief Установить признак не успешного результата выполнения запроса ModBus RTU.
 * Устанавливается признак не успешного чтения или записи Modbus для ранее полученного описания запроса.
 * Должна вызываться при любой ошибке: нет ответа, ошибка CRC, получение кода исключения и т.п.
 * 
 * @param errCode Код ошибки: 0 - нет ответа или ошибка CRC, !=0 - ответ с кодом исключения
 */
void fbdSetModbusRTUNoResponse(int errCode)
{
    fbdModbusRTUDelayTimer = FBD_MODBUSRTU_DELAY;
    if((errCode != 0) || (fbdModbusRTUErrorCounter == 0)) {
        fbdModbusRTUErrorCounter = 0;
        setModbusNoResponse(fbdModbusRTUIndex);
    }
}

/**
 * @brief Получение следующего запроса Modbus TCP для выполнения.
 * 
 * @param mbrequest Указатель на структуру описания запроса Modbus TCP
 * @return true Возвращён очередной запрос Modbus TCP
 * @return false Запросов Modbus TCP пока больше нет
 */
bool fbdGetNextModbusTCPRequest(tModbusReq *mbrequest)
{
    if(!fbdModbusTCPCount) return false;
    //
    // поиск следующего элемента Modbus TCP
    // проверка необходимости повторного запроса
    if(fbdModbusTCPErrorCounter) {
        // необходим повторный запрос
        fbdModbusTCPErrorCounter--;
        fillModbusRequest(fbdModbusTCPIndex, mbrequest);
        //
        return true;
    } else {
        tElemIndex index, i;
        //
        // поиск следующего элемента
        index = fbdModbusTCPIndex;
        for(i=0; i < fbdElementsCount; i++) {
            unsigned char elem;
            // переходим к следующему элементу
            index++;
            if(index >= fbdElementsCount) {
                // проверка таймера минимального периода опроса
                if(fbdModbusTCPTimer) return false;
                fbdModbusTCPTimer = FBD_MODBUS_PAUSE;
                index = 0;
            }
            elem = fbdDescrBuf[index] & ELEMMASK;
            if((elem == ELEM_INP_MDBS)||(elem == ELEM_OUT_MDBS)) {
                // проверяем тип Modbus
                if(FBDGETPARAMETER(index, 0)) {
                    // это Modbus TCP
                    if(elem == ELEM_OUT_MDBS) {
                        // если это элемент записи, то проверяем флаг изменений
                        // если флаг не установлен, то переходим к следующему элементу
                        // если установлен, то формируем команду записи
                        if(!getAndClearChangeModbusFlag(index)) continue;
                        // повторные запросы при записи не используем, они и так будут работать из-за флагов изменения
                        fbdModbusTCPErrorCounter = 0;
                    } else {
                        // устанавливаем количество повторных запросов
                        fbdModbusTCPErrorCounter = FBD_MODBUS_RETRYCOUNT;
                    }
                    // нашли подходящий элемент
                    fbdModbusTCPIndex = index;
                    fillModbusRequest(index, mbrequest);
                    return true;
                }
            }
        }
    }
    //
    return false;
}

/**
 * @brief Установка результата успешного выполнения запроса Modbus TCP.
 * Должна быть вызвана после успешного выполнения запроса чтения или записи Modbus TCP.
 * 
 * @param response Данные, возвращённые запросом
 */
void fbdSetModbusTCPResponse(tSignal response)
{
    fbdModbusTCPErrorCounter = 0;
    setModbusResponse(fbdModbusTCPIndex, response);
}

/**
 * @brief Установить признак не успешного результата выполнения запроса ModBus TCP.
 * Устанавливается признак не успешного чтения или записи Modbus для ранее полученного описания запроса.
 * 
 * @param errCode Код ошибки: 0 - нет ответа или ошибка CRC, !=0 - ответ с кодом исключения
 */
void fbdSetModbusTCPNoResponse(int errCode)
{
    if((errCode != 0) || (fbdModbusTCPErrorCounter == 0)) {
        fbdModbusTCPErrorCounter = 0;
        setModbusNoResponse(fbdModbusTCPIndex);
    }
}

#ifdef USE_EVENTS

/**
 * @brief Возвращает общее количество (активных и не активных) событий в проекте.
 * Если результат 0, то проект не использует события и журнал.
 * 
 * @return tSignal Количество событий в проекте
 */
tSignal fbdTotalEventsCount(void)
{
    return FBD_EVENTS_COUNT;
}

/**
 * @brief Получить описание активного события с указанным индексом
 * 
 * @param index Индекс события (0..fbdTotalEventsCount()-1)
 * @param event Указатель на структуру описания активного события
 * @return true Событие есть, оно помещено в структуру event
 * @return false События с таким индексом нет
 */
bool fbdGetCurrentEvent(tSignal index, tEventLogItem *event)
{
    if(index >= FBD_EVENTS_COUNT) return false;
    // проверка флага активности
    if(!(fbdEventActiveFlags[index>>3]&(1u<<(index&7)))) return false;
    //
    // возвращаем зафиксированное время
    event->flags.seconds = fbdEventActiveTime[index].flags.seconds;
    event->flags.minutes = fbdEventActiveTime[index].flags.minutes;
    event->flags.hours = fbdEventActiveTime[index].flags.hours;
    event->flags.month = fbdEventActiveTime[index].flags.month;
    event->flags.day = fbdEventActiveTime[index].flags.day;
    // важность
    event->flags.severity = fbdEventActiveTime[index].flags.severity;
    event->flags.started = 1;
    // указатель на сообщение
    event->message = fbdHMIgetIOhint(2, index);
    //
    return true;
}

/**
 * @brief Возвращает признак активности текущего события с указанным индексом 
 * 
 * @param index Индекс события (0..fbdTotalEventsCount()-1)
 * @return true Событие есть
 * @return false События нет
 */
bool fbdCurrentEventIsActive(tSignal index)
{
    if(index >= FBD_EVENTS_COUNT) return false;
    // проверка флага активности
    if(!(fbdEventActiveFlags[index>>3]&(1u<<(index&7)))) return false;
    //
    return true;
}

/**
 * @brief Подтверждение (сброс) текущего события
 * 
 * @param index Индекс события (0..fbdTotalEventsCount()-1)
 */
bool fbdConfirmCurrentEvent(tSignal index)
{
    // проверка номера
    if(index >= FBD_EVENTS_COUNT) return false;
    // проверка текущей активности
    if(!(fbdEventActiveFlags[index>>3]&(1u<<(index&7)))) return false;
    // сброс флага активности
    fbdEventActiveFlags[index>>3] &= ~(1u<<(index&7));
    //
    // необходимо добавить запись журнала
    // ищем элемент ELEM_EVENT
    tEventDescriptionView eventFlags;
    tElemIndex i, ei;
    ei = 0;
    for(i=0; i < fbdElementsCount; i++) {
        unsigned char elem = fbdDescrBuf[i] & ELEMMASK;
        if(elem == ELEM_EVENT) {
            // нашли очередное событие, смотрим на его номер
            if(index == ei) {
                // нашли то, что надо
                eventFlags.value = FBDGETPARAMETER(i, 1);
                // проверяем необходимость логирования завершения события
                if(eventFlags.flags.logEnd) fbdAddLogEvent(eventFlags.flags, 0);
                //
                return true;
            } else {
                ei++;
            }
        }
    }
    return true;
}

/**
 * @brief Проверка возможности подтверждения текущего события
 * 
 * @param index Индекс события (0..fbdTotalEventsCount()-1)
 * @return true Событие может быть подтверждено
 * @return false Событие не может быть подтверждено
 */
bool fbdCanConfirmCurrentEvent(tSignal index)
{
    // проверка номера
    if(index >= FBD_EVENTS_COUNT) return false;
    // проверка текущей активности
    if(!(fbdEventActiveFlags[index>>3]&(1u<<(index&7)))) return false;
    //
    // ищем элемент ELEM_EVENT для того, что-бы выяснить возможность его подтверждения
    tEventDescriptionView eventFlags;
    tElemIndex i, ei;
    ei = 0;
    for(i=0; i < fbdElementsCount; i++) {
        unsigned char elem = fbdDescrBuf[i] & ELEMMASK;
        if(elem == ELEM_EVENT) {
            // нашли очередное событие, смотрим на его номер
            if(index == ei) {
                // нашли то, что надо
                eventFlags.value = FBDGETPARAMETER(i, 1);
                // проверка флага подтверждения
                if(eventFlags.flags.cr) return true; else return false;
            } else {
                ei++;
            }
        }
    }
    return false;
}

/**
 * @brief Проверка корректности записи журнала
 * 
 * @param eventFlags 
 * @return true Запись корректна
 * @return false Запись не корректна
 */
static bool fbdLogEventIsValid(tEventFlagsView eventFlags)
{
    if(eventFlags.flags.sign != FBD_EVENTS_FLAG_SIGN) return false;
    return true;
}

/**
 * @brief Получить описание события из журнала
 * 
 * @param index Индекс записи в журнале событий, 0 - самое новое (последнее)
 * @param event Указатель на структуру описания события журнала
 * @return true Событие есть, оно помещено в структуру event
 * @return false События с таким индексом (и большими) в журнале нет
 */
bool fbdGetLogEvent(tSignal index, tEventLogItem *event)
{
    // проверка на границу
    if(index >= (MAXNVRAMINDEX + 1 - fbdStartEventLog) / 2) return false;
    // флаги
    tEventFlagsView eventFlags;
    eventFlags.value  = FBDgetProc(FBD_NVRAM, fbdStartEventLog + index*2);
    // проверка сигнатуры
    if(!fbdLogEventIsValid(eventFlags)) return false;
    //
    event->flags = eventFlags.flags;
    event->message = fbdHMIgetIOhint(2, FBDgetProc(FBD_NVRAM, fbdStartEventLog + index*2 + 1) & 255);
    return true;
}

/**
 * @brief Возвращает количество записей в журнале событий
 * 
 * @return int Количество записей в журнале событий
 */
int fbdGetLogEventCount(void)
{
    tEventFlagsView eventFlags;
    //
    int logEventMaxCount = (MAXNVRAMINDEX + 1 - fbdStartEventLog) / 2;
    for(int index=0; index < logEventMaxCount; index++) {
        eventFlags.value = FBDgetProc(FBD_NVRAM, fbdStartEventLog + index*2);
        if(!fbdLogEventIsValid(eventFlags)) return index;
    }
    //
    return logEventMaxCount;
}

/**
 * @brief Добавление события в журнал
 * 
 * @param eventFlags Событие с флагами
 * @param up Событие стало активным
 */
void fbdAddLogEvent(tEventDescription eventDescription, char up)
{
    // сдвиг хвоста журнала
    tSignal temp;
    int i = MAXNVRAMINDEX-1;
    while(1) {
        temp = FBDgetProc(FBD_NVRAM, i-2);
        FBDsetProc(FBD_NVRAM, i, &temp);
        temp = FBDgetProc(FBD_NVRAM, i-1);
        FBDsetProc(FBD_NVRAM, i+1, &temp);
        //
        i = i - 2;
        if(i <= fbdStartEventLog) break;
    }
    //
    // добавление нового события в начало журнала
    tEventFlagsView newItem;
    newItem.flags.seconds = FBDgetProc(FBD_PIN, GP_RTC_SECOND);
    newItem.flags.minutes = FBDgetProc(FBD_PIN, GP_RTC_MINUTE);
    newItem.flags.hours = FBDgetProc(FBD_PIN, GP_RTC_HOUR);
    newItem.flags.day = FBDgetProc(FBD_PIN, GP_RTC_DAY);
    newItem.flags.month = FBDgetProc(FBD_PIN, GP_RTC_MONTH);
    newItem.flags.severity = eventDescription.severity;
    newItem.flags.started = up;
    newItem.flags.sign = FBD_EVENTS_FLAG_SIGN;
    //
    temp = eventDescription.imessage;
    FBDsetProc(FBD_NVRAM, fbdStartEventLog, &newItem.value);
    FBDsetProc(FBD_NVRAM, fbdStartEventLog+1, &temp);
    //
    // если это начало события, то фиксируем его время
    if(up) fbdEventActiveTime[temp].value = newItem.value;
    //
    intLogEventChanged = true;
}

/**
 * @brief Очистка журнала событий
 */
void fbdClearLogEvent(void)
{
    tSignal zero = 0;
    for(int i = fbdStartEventLog; i <= MAXNVRAMINDEX; i++) {
        FBDsetProc(FBD_NVRAM, i, &zero);
    }
    intLogEventChanged = true;
}

/**
 * @brief Возвращает статус изменения журнала событий
 * 
 * @return true Изменения есть
 * @return false Изменений нет
 */
bool fbdLogEventChanged(void)
{
    bool tmp = intLogEventChanged;
    intLogEventChanged = false;
    return tmp;
}

/**
 * @brief Возвращает статус изменения текущего журнала
 * 
 * @return true Изменения есть
 * @return false Изменений нет
 */
bool fbdCurrentEventChanged(void)
{
    bool tmp = intCurrentEventChanged;
    intCurrentEventChanged = false;
    return tmp;
}

#endif  // USE_EVENTS

#ifdef USE_HMI
// -------------------------------------------------------------------------------------------------------
// HMI functions

/**
 * @brief Получить значение точки регулирования
 * 
 * @param index Индекс точки регулирования
 * @param pnt Указатель на заполяемую структуру
 * @return true Требуемая точка регулирования есть
 * @return false Требуемой точки регулирования нет
 */
bool fbdHMIgetSP(tSignal index, tHMIdata *pnt)
{
    tElemIndex elemIndex;
    if(index >= fbdSpCount) return false;
    elemIndex = (spOffsets + index)->index;
    pnt->caption = (spOffsets + index)->caption;
    pnt->value = FBDGETSTORAGE(elemIndex, 0);
    pnt->lowlimit = FBDGETPARAMETER(elemIndex, 0);
    pnt->upperLimit = FBDGETPARAMETER(elemIndex, 1);
    pnt->defValue = FBDGETPARAMETER(elemIndex, 2);
    pnt->divider = FBDGETPARAMETER(elemIndex, 3);
    pnt->step = FBDGETPARAMETER(elemIndex, 4);
    return true;
}

/**
 * @brief Установить значение точки регулирования
 * 
 * @param index Индекс точки регулирования
 * @param value Устанавливаемое значение
 */
void fbdHMIsetSP(tSignal index, tSignal value)
{
    tElemIndex elemIndex;
    if(index >= fbdSpCount) return;
    elemIndex = (spOffsets + index)->index;
    fbdSetStorage(elemIndex, 0, value);
}

/**
 * @brief Получить значение точки контроля.
 * Заполняет описание точки контроля по адресу pnt, если указанная точка имеется
 * @param index Индекс точки контроля
 * @param pnt Указатель на описание точки контроля
 * @return true Точка контроля присутствует
 * @return false Точка контроля отсутствует
 */
bool fbdHMIgetWP(tSignal index, tHMIdata *pnt)
{
    tElemIndex elemIndex = 0;
    if(index >= fbdWpCount) return false;
    elemIndex = (wpOffsets + index)->index;
    pnt->caption = (wpOffsets + index)->caption;
    pnt->value = fbdMemoryBuf[elemIndex];
    pnt->divider = FBDGETPARAMETER(elemIndex, 0);
    return true;
}
// -------------------------------------------------------------------------------------------------------

/**
 * @brief Получение структуры с описанием проекта
 * 
 * @param pnt Указатель на структуру описания проекта
 */
void fbdHMIgetDescription(tHMIdescription *pnt)
{
    pnt->name = fbdCaptionName?fbdCaptionName:(fbdCaptionName = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount));
    pnt->version = fbdCaptionVersion?fbdCaptionVersion:(fbdCaptionVersion = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 1));
    pnt->btime = fbdCaptionBTime?fbdCaptionBTime:(fbdCaptionBTime = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 2));
}

/**
 * @brief Получение указателя на текстовое описание (хинт) входа или выхода.
 * Возвращает указатель на текстовое описание (хинт) входа или выхода, если такого описание не найдено, то возвращает NULL.
 * Значение параметра index соответствует значению параметра index функций FBDgetProc(type, index) и FBDsetProc(type, index, *value).
 * @param type Тип: 0 - входы, 1 - выходы, 2 - события
 * @param index Индекс входа или выхода
 * @return DESCR_MEM* Указатель на текстовое описание входа или выхода
 */
DESCR_MEM char DESCR_MEM_SUFX *fbdHMIgetIOhint(char type, char index)
{
    tSignal hintsCount = FBD_HINTS_COUNT;
    if(!hintsCount) return NULL;                                                    // описаний нет
    DESCR_MEM char DESCR_MEM_SUFX *curHint = fbdIOHints;
    tSignal i = 0;
    // цикл перебора всех описаний
    while(1) {
        // хинты хранятся в виде последовательности описаний:
        // type(char), index(char), text, 0
        // type(char), index(char), text, 0
        // ...
        if((*curHint == type)&&(*(curHint + 1) == index)) return curHint + 2;       // описание найдено
        // переходим к следующему хинту
        i++;
        if(i >= hintsCount) return NULL;                                            // больше описаний нет
        // расчет указателя на следующий хинт
        curHint += 2;
        while(*(curHint++));
    }
}

/**
 * @brief Расчет указателя на текстовое описание элемента по индексу описания
 * 
 * @param captionIndex Индекс описания
 * @return DESCR_MEM char DESCR_MEM_SUFX Указатель на текстовое описание
 */
DESCR_MEM char DESCR_MEM_SUFX * fbdGetCaptionByIndex(tElemIndex captionIndex)
{
    tOffset offset = 0;
    while(captionIndex) if(!fbdCaptionsBuf[offset++]) captionIndex--;
    return &fbdCaptionsBuf[offset];
}

#endif // USE_HMI

// -------------------------------------------------------------------------------------------------------

/**
 * @brief Проверка нахождения текущего времени RTC в указанном диапазоне
 * 
 * @param timeOn Время включения
 * @param timeOff Время выключения
 * @return tSignal 
 */
tSignal checkTimeInRange(tSignal timeOn, tSignal timeOff)
{
    tSignal now;
    now = FBDgetProc(FBD_PIN, GP_RTC_HOUR)*3600 + FBDgetProc(FBD_PIN, GP_RTC_MINUTE)*60 + FBDgetProc(FBD_PIN, GP_RTC_SECOND);
    if((now >= timeOn) && (now <= timeOff)) return 1;
    return 0;
}

// -------------------------------------------------------------------------------------------------------

/**
 * @brief Расчёт выходного значения элемента
 * 
 * @param curIndex Индекс элемента
 */
void fbdCalcElement(tElemIndex curIndex)
{
    tFBDStackItem fbdStack[FBDSTACKSIZE];       // стек вычислений
    tFBDStackPnt fbdStackPnt;                   // указатель стека
    unsigned char curInput;                     // текущий входной контакт элемента
    unsigned char inputCount;                   // число входов текущего элемента
    tOffset baseInput;                          //
    tElemIndex inpIndex;
    tSignal s1,s2,s3,s4,v;                      // значения сигналов на входе
    struct tm local_time;
    //
    if(getCalcFlag(curIndex)) return;           // элемент уже рассчитан?
    //
    fbdStackPnt = 0;
    curInput = 0;
    //
    baseInput = FBDINPUTOFFSET(curIndex);       //
    inputCount = FBDdefInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
    //
    do {
        // если у текущего элемента ещё есть входы
        if(curInput < inputCount) {
            // и этот вход еще не расчитан
            inpIndex = ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + curInput]);
            // проверка: вход уже рассчитан или ко входу подключен выход этого же компонента
            if(getCalcFlag(inpIndex)||(curIndex == inpIndex)) curInput++; else {
                // ставим признак того, что элемент как-бы уже расчитан
                // это нужно в случае, если в схеме есть обратные связи
                setCalcFlag(curIndex);
                // вход еще не рассчитан, запихиваем текущий элемент и номер входа в стек
                fbdStack[fbdStackPnt].index = curIndex;
                fbdStack[fbdStackPnt++].input = curInput;
                // переходим к следующему дочернему элементу
                curIndex = inpIndex;
                curInput = 0;
                baseInput = FBDINPUTOFFSET(curIndex);       // элемент сменился, расчет смещения на первый вход элемента
                inputCount = FBDdefInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
            }
            continue;       // следующая итерация цикла
        } else {
            // входов больше нет, а те которые есть уже рассчитаны
            // определяем значения входов (если надо)
            switch(inputCount) {
                case 5:
                    v = fbdMemoryBuf[ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + 4])];
                case 4:
                    s4 = fbdMemoryBuf[ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + 3])];
                case 3:
                    s3 = fbdMemoryBuf[ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + 2])];
                case 2:
                    s2 = fbdMemoryBuf[ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + 1])];
                case 1:
                    s1 = fbdMemoryBuf[ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput])];
            }
            //
            // вычисляем значение текущего элемента, результат в s1
            //
            switch(fbdDescrBuf[curIndex] & ELEMMASK) {
                case ELEM_WP:                                                           // HMI watchpoint
                case ELEM_EVENT:                                                        // EVENT
                case ELEM_OUT_PIN:                                                      // OUTPUT PIN
                    break;
                case ELEM_OUT_VAR:                                                      // OUTPUT VAR
                    // при изменении значения сигнала ставим флаг
                    if(fbdMemoryBuf[curIndex] != s1) {
                        setChangeVarFlag(curIndex);
                    }
                    break;
                case ELEM_OUT_MDBS:                                                     // ELEM_OUT_MDBS
                    // при изменении значения сигнала ставим флаг
                    if(fbdMemoryBuf[curIndex] != s1) {
                        setChangeModbusFlag(curIndex);
                    }
                    break;
                case ELEM_CONST:                                                        // CONST
                    s1 = FBDGETPARAMETER(curIndex, 0);
                    break;
                case ELEM_NOT:                                                          // NOT
                    s1 = s1?0:1;
                    break;
                case ELEM_AND:                                                          // AND
                    s1 = s1 && s2;
                    break;
                case ELEM_OR:                                                           // OR
                    s1 = s1 || s2;
                    break;
                case ELEM_XOR:                                                          // XOR
                    s1 = (s1?1:0)^(s2?1:0);
                    break;
                case ELEM_RSTRG:                                                        // RSTRG
                    if(s1||s2) {
                        s1 = s1?0:1;
                        fbdSetStorage(curIndex, 0, s1);
                    } else s1 = FBDGETSTORAGE(curIndex, 0);
                    break;
                case ELEM_DTRG:                                                         // DTRG
                    // s1 - D
                    // s2 - C
                    // смотрим установку флага фронта на входе "С"
                    if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput+1])))
                        fbdSetStorage(curIndex, 0, s1);
                    else
                        s1 = FBDGETSTORAGE(curIndex, 0);
                    break;
                case ELEM_ADD:                                                          // ADD
                    s1 += s2;
                    break;
                case ELEM_SUB:                                                          // SUB
                    s1 -= s2;
                    break;
                case ELEM_MUL:                                                          // MUL
                    s1 *= s2;
                    break;
                case ELEM_DIV:                                                          // DIV
                    if(!s2) {
                        if(s1 > 0) s1 = MAX_SIGNAL; else if(s1 < 0) s1 = MIN_SIGNAL; else s1 = 1;
                    } else s1 /= s2;
                    break;
                case ELEM_MOD:                                                          // MOD
                    if(s2) s1 %= s2;
                    break;
                case ELEM_MFUN:                                                         // MFUN
                    // тип многофункционального элемента
                    switch (FBDGETPARAMETER(curIndex, 0) & 0x000000ff) {
                        case 0:
                            // сигнал начального сброса после запуска схемы
                            s1 = fbdFirstFlag;
                            break;
                        case 1:
                            // случайное число: 0..7FFFFFFF
                            s1 = rand();
                            break;
                        case 2:
                            // статус Ethernet
                            s1 = FBDgetProc(FBD_HRDW, FBD_HRDW_ETH);
                            break;
                        case 3:
                            // статус NTP
                            s1 = FBDgetProc(FBD_HRDW, FBD_HRDW_NTP);
                            break;
                        case 4:
                            // статус батареи
                            s1 = FBDgetProc(FBD_HRDW, FBD_HRDW_BAT);
                            break;
                        case 5:
                            // индекс текущего экрана
                            s1 = fbdLastScreenIndex;
                            break;
                        case 6:
                            {
                            // астро-таймер
                            // FBDGETPARAMETER(curIndex, 1)  - широта
                            // FBDGETPARAMETER(curIndex, 2)  - долгота
#ifdef USE_MATH
                            //
                            local_time.tm_year = FBDgetProc(FBD_PIN, GP_RTC_YEAR) - 1900;
                            local_time.tm_mon  = FBDgetProc(FBD_PIN, GP_RTC_MONTH) - 1;
                            local_time.tm_mday = FBDgetProc(FBD_PIN, GP_RTC_DAY);
                            //
                            local_time.tm_hour = FBDgetProc(FBD_PIN, GP_RTC_HOUR);
                            local_time.tm_min  = FBDgetProc(FBD_PIN, GP_RTC_MINUTE);
                            local_time.tm_sec  = FBDgetProc(FBD_PIN, GP_RTC_SECOND);
                            local_time.tm_isdst = 0;
                            time_t utcTime = mktime(&local_time);
                            //
                            utcTime -= FBDgetProc(FBD_HRDW, FBD_HRDW_TZO)*60;
                            //
                            s1 = sunPosition(FBDGETPARAMETER(curIndex, 1) / 10000.0, FBDGETPARAMETER(curIndex, 2) / 10000.0, utcTime);
#else
                            s1 = 0;
#endif
                            }
                            break;
                        case 7:
                            // элемент недельного календаря
                            // FBDGETPARAMETER(curIndex, 1)  - время включения
                            // FBDGETPARAMETER(curIndex, 2)  - время выключения
                            // FBDGETPARAMETER(curIndex, 3)  - дни недели
                            //
                            // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
                            // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                            // |                                   Резерв                                 |Вс Сб Пт Чт Ср Вт Пн|
                            //
                            if(FBDGETPARAMETER(curIndex, 3) & (1u<<(FBDgetProc(FBD_PIN, GP_RTC_WDAY)-1))){
                                // день недели совпадает
                                // проверка времени
                                s1 = checkTimeInRange(FBDGETPARAMETER(curIndex, 1), FBDGETPARAMETER(curIndex, 2));
                            } else s1 = 0;
                            break;
                        case 8:
                            // элемент месячного календаря
                            // FBDGETPARAMETER(curIndex, 1)  - время включения
                            // FBDGETPARAMETER(curIndex, 2)  - время выключения
                            // FBDGETPARAMETER(curIndex, 3)  - дни месяца
                            //
                            // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
                            // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                            // |  |31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1|
                            //
                            if(FBDGETPARAMETER(curIndex, 3) & (1u<<(FBDgetProc(FBD_PIN, GP_RTC_DAY)-1))){
                                // день месяца совпадает
                                // проверка времени
                                s1 = checkTimeInRange(FBDGETPARAMETER(curIndex, 1), FBDGETPARAMETER(curIndex, 2));
                            } else s1 = 0;
                            break;
                        case 9:
                            // элемент годового календаря
                            // FBDGETPARAMETER(curIndex, 1)  - время включения
                            // FBDGETPARAMETER(curIndex, 2)  - время выключения
                            // FBDGETPARAMETER(curIndex, 3)  - день/месяц
                            //
                            // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
                            // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                            // |                                  Резерв                            |   Месяц   |     День     |
                            //
                            if(((FBDGETPARAMETER(curIndex, 3) & 0x1f) + 1) == FBDgetProc(FBD_PIN, GP_RTC_DAY) && (((FBDGETPARAMETER(curIndex, 3) >> 5) & 0x0f) + 1) == FBDgetProc(FBD_PIN, GP_RTC_MONTH)){
                                // день и месяц совпадают
                                // проверка времени
                                s1 = checkTimeInRange(FBDGETPARAMETER(curIndex, 1), FBDGETPARAMETER(curIndex, 2));
                            } else s1 = 0;
                            break;
                        case 10:
                            // состояние события
                            // FBDGETPARAMETER(curIndex, 1)  - индекс события
                            v = FBDGETPARAMETER(curIndex, 1);
                            if(v < FBD_EVENTS_COUNT) {
                                s1=(fbdEventActiveFlags[v>>3]&(1u<<(v&7)))?1:0;
                            } else s1 = 0;
                            break;
                        // case 11: результат чтения Modbus
                        // case 12: результат записи Modbus


// 00           Успех
// 01  (0x01)   Illegal Function                        The function code is invalid or not allowed for the slave device.
// 02  (0x02)   Illegal Data Address                    The requested data address range is not valid for the slave device.
// 03  (0x03)   Illegal Data Value                      A value in the query's data field is unacceptable for the slave.
// 04  (0x04)   Slave Device Failure                    An unrecoverable error, such as a hardware failure, occurred in the slave.
// 05  (0x05)   Acknowledge                             The slave accepted the request but needs extended processing time. The master should poll later.
// 06  (0x06)   Slave Device Busy                       The slave is processing a long command; the master should retry later.
// 07  (0x07)   Negative Acknowledge                    The slave cannot perform the requested program function (used with program commands 13 or 14).
// 08  (0x08)   Memory Parity Error                     The slave detected a parity error in extended memory while reading a record file (used with function codes 20 and 21).
// 10  (0x0A)   Gateway Path Unavailable                The gateway could not allocate a communication path.
// 11  (0x0B)   Gateway Target Device Failed to Respond No response was received from the target device through the gateway.
// 255 (0xFF)   Другая ошибка (нет ответа, ошибка CRC, некорректный формат пакета и тп)

                        default:
                            s1 = 0;
                            break;
                    }
                    break;
                case ELEM_TON:                                                          // TIMER TON
                    // s1 - D
                    // s2 - T
                    if(s1) {
                        s1 = (FBDGETSTORAGE(curIndex, 0) == 0);
                    } else {
                        fbdSetStorage(curIndex, 0, s2);
                        s1 = 0;
                    }
                    break;
                case ELEM_CMP:                                                          // CMP
                    s1 = s1 > s2;
                    break;
                case ELEM_INP_PIN:
                    s1 = FBDgetProc(FBD_PIN, FBDGETPARAMETER(curIndex, 0));             // INPUT PIN
                    break;
                case ELEM_INP_VAR:
                    s1 = FBDGETSTORAGE(curIndex, 0);                                    // INPUT VAR
                    break;
                case ELEM_INP_MDBS:
                    // просто последнее значение
                    s1 = fbdMemoryBuf[curIndex];                                        // ELEM_INP_MDBS
                    break;
                case ELEM_PID:                                                          // PID
                    if(!FBDGETSTORAGE(curIndex, 0)) {           // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s3);         // установка таймера
                        s2 = s1 - s2;                           // ошибка PID
                        // error limit
                        //v = MAX_SIGNAL/2/s4;
                        //if(intAbs(s2) > v) s2 = (s2>0)?v:-v;
                        //
                        if(!fbdFirstFlag) v = ((tLongSignal)(s1 - FBDGETSTORAGE(curIndex, 1)) * 128)/s3; else v = 0;    // скорость изменения входной величины
                        fbdSetStorage(curIndex, 1, s1);                                                                 // сохранение прошлого входного значения
                        if((v < intAbs(s2))||(v > intAbs(s2*3))) {
                            s1= -(tLongSignal)s4*(s2*2 + v) / 128;
                        } else s1 = fbdMemoryBuf[curIndex];
                    } else s1 = fbdMemoryBuf[curIndex];
                    break;
                case ELEM_SUM:                                                          // SUM
                    // s1 - X   (входное значение)
                    // s2 - dT  (таймер)
                    // s3 - Lim (ограничение)
                    //
                    if(!FBDGETSTORAGE(curIndex, 0)) {       // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s2);     // установка таймера
                        //
                        s1 += fbdMemoryBuf[curIndex];       // сложение с предыдущим значением
                        // ограничение
                        if(s1 > 0) { if(s1 > s3) s1 = s3; } else { if(s1 < -s3) s1 = -s3; }
                    } else s1 = fbdMemoryBuf[curIndex];
                    break;
                case ELEM_SUMM:                                                         // SUMM (с установкой значения)
                    // s1 - X   (входное значение)
                    // s2 - dT  (таймер)
                    // s3 - Lim (ограничение)
                    // s4 - D   (записываемое значение)
                    // s5 - C   (фронт записи)
                    //
                    if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput+4]))) {
                        // обнаружен фронт записи значения
                        s1 = s4;
                        if(s1 > 0) {
                            if(s1 > s3) s1 = s3;
                        } else { 
                            if(s1 < -s3) s1 = -s3;
                        }
                    } else {
                        // флага записи не было
                        if(!FBDGETSTORAGE(curIndex, 0)) {       // проверка срабатывания таймера
                            fbdSetStorage(curIndex, 0, s2);     // установка таймера
                            //
                            s1 += fbdMemoryBuf[curIndex];       // сложение с предыдущим значением
                            // ограничение
                            if(s1 > 0) {
                                if(s1 > s3) s1 = s3;
                            } else {
                                if(s1 < -s3) s1 = -s3;
                            }
                        } else s1 = fbdMemoryBuf[curIndex];
                    }
                    break;
                case ELEM_COUNTER:                                                      // Counter
                    if(s3) s1 = 0; else {
                        s1 = FBDGETSTORAGE(curIndex, 0);
                        if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput]))) s1++;
                        if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput+1]))) s1--;
                    }
                    fbdSetStorage(curIndex, 0, s1);
                    break;
                case ELEM_MUX:                                                          // MUX
                    v &= 3;
                    if(v==1) s1 = s2; else
                    if(v==2) s1 = s3; else
                    if(v==3) s1 = s4;
                    break;
                case ELEM_ABS:                                                          // ABS
                    if(s1 < 0) s1 = -s1;
                    break;
                case ELEM_SP:                                                           // HMI setpoint
#ifdef USE_HMI
                    s1 = FBDGETSTORAGE(curIndex, 0);
#endif // USE_HMI
                    break;
                case ELEM_TP:                                                           // TIMER TP
                    // s1 - D
                    // s2 - T
                    if(FBDGETSTORAGE(curIndex, 0)) {
                        s1 = 1;
                    } else {
                        if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput])) && s2) {
                            fbdSetStorage(curIndex, 0, s2);
                            s1 = 1;
                        } else s1 = 0;
                    }
                    break;
                case ELEM_MIN:                                                          // MIN
                    if(s2 < s1) s1 = s2;
                    break;
                case ELEM_MAX:                                                          // MAX
                    if(s2 > s1) s1 = s2;
                    break;
                case ELEM_LIM:                                                          // LIM
                    if(s1 > s2) s1 = s2; else if(s1 < s3) s1 = s3;
                    break;
                case ELEM_EQ:                                                           // EQ
                    s1 = s1 == s2;
                    break;
                case ELEM_BAND:                                                         // побитный AND
                    s1 = s1 & s2;
                    break;
                case ELEM_BOR:                                                          // побитный OR
                    s1 = s1 | s2;
                    break;
                case ELEM_BXOR:                                                         // побитный XOR
                    s1 = s1 ^ s2;
                    break;
                case ELEM_GEN:                                                          // генератор
                    if(s1 > 0) {                                        // s1 - enable, период; s2 - амплитуда
                        s3 = FBDGETSTORAGE(curIndex, 0);                // s3 - остаток таймера периода
                        if(s3) {
                            switch (FBDGETPARAMETER(curIndex, 0)) {     // тип генератора
                                case 0:                                 // меандр
                                    s1 = (s3 > (s1>>1))?0:s2;
                                    break;
                                case 1:                                 // пила
#ifdef USE_MATH
                                    s1 = lroundf(1.0*s2*(s1-s3)/s1);
#else
                                    s1 = (s2*(s1-s3))/s1;
#endif
                                    break;
                                case 2:                                 // треугольник
                                    if(s3 > (s1>>1)) {
#ifdef USE_MATH
                                        s1 = lroundf(2.0*s2*(s1-s3)/s1); // нарастание
                                    } else {
                                        s1 = lroundf(2.0*s2*s3/s1);      // спад
#else
                                        s1 = 2*s2*(s1-s3)/s1;           // нарастание
                                    } else {
                                        s1 = 2*s2*s3/s1;                // спад
#endif
                                    }
                                    break;
#ifdef USE_MATH
                                case 3:                                 // sin
                                    s1 = lroundf(s2*sinf(2.0*M_PI*(s1-s3)/s1));
                                    break;
#endif
                            }
                        } else {
                            // запуск таймера
                            fbdSetStorage(curIndex, 0, s1);
                            // старт генератора с 0
                            s1 = 0;
                        }
                    } else {
                        // генератор остановлен
                        s1 = 0;
                    }
                    break;
                case ELEM_LUT:                                                          // Logical LUT
                    if(s1) s1=1; else s1=0;
                    if(s2) s1+=2;
                    if(s3) s1+=4;
                    if(s4) s1+=8;
                    if(v)  s1+=16;
                    //
                    s2 = FBDGETPARAMETER(curIndex, 0);
                    s1 = (s2 >> s1) & 1;
                    break;
                case ELEM_NLUT:                                                         // Numeric LUT
                    //
                    // FBDGETPARAMETER(curIndex, 0)  - значение по умолчанию
                    // FBDGETPARAMETER(curIndex, 1):
                    // |31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
                    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
                    // |                                Резерв                              |IP| Резерв | Кол-во точек |
                    // биты 0..4: индекс максимальной точки - 1
                    // бит  8:    использовать интерполяцию
                    //
                    // FBDGETPARAMETER(curIndex, 2)  - x0
                    // FBDGETPARAMETER(curIndex, 3)  - y0
                    // ...
                    // FBDGETPARAMETER(curIndex, 62) - x30
                    // FBDGETPARAMETER(curIndex, 63) - y30
                    // FBDGETPARAMETER(curIndex, 64) - x31
                    // FBDGETPARAMETER(curIndex, 65) - y31
                    //
                    {
                        tSignal x2;
                        int maxPntIndex = (FBDGETPARAMETER(curIndex, 1) & 0x1F) + 1;
                        if(maxPntIndex > 31) maxPntIndex = 31;
                        //
                        if((FBDGETPARAMETER(curIndex, 1) & 0x100)) {
                            // использовать интерполяцию
                            // цикл, начиная со второй точки, минимальное количество точек: 2
                            for(v=1; v <= maxPntIndex; v++) {
                                x2 = FBDGETPARAMETER(curIndex, v*2+2);
                                if((s1 <= x2) || (v == maxPntIndex)) {
                                    s1 = interpolation(s1, FBDGETPARAMETER(curIndex, (v-1)*2+2), FBDGETPARAMETER(curIndex, (v-1)*2+3), x2, FBDGETPARAMETER(curIndex, v*2+3));
                                    break;
                                }
                            }
                        } else {
                            // не использовать интерполяцию
                            for(v=0; v <= maxPntIndex; v++) {
                                x2 = FBDGETPARAMETER(curIndex, v*2+2);
                                if(s1 == x2) {
                                    s1 = FBDGETPARAMETER(curIndex, v*2+3);
                                    break;
                                } else {
                                    // если значение меньше, то дальше нет смысла перебирать таблицу
                                    if(s1 < x2) {
                                        s1 = FBDGETPARAMETER(curIndex, 0);
                                        break;
                                    }
                                }
                            }
                            // если не нашли, то значение по умолчанию
                            if(v > maxPntIndex) s1 = FBDGETPARAMETER(curIndex, 0);
                        }
                    }
                    break;
            } // switch
            //
            setCalcFlag(curIndex);                                  // установка флага "вычисление выполнено"
            if(fbdDescrBuf[curIndex] & INVERTFLAG) s1 = s1?0:1;     // инверсия результата (если нужно)
            if(s1 > fbdMemoryBuf[curIndex]) setRiseFlag(curIndex);  // установка флага фронта (если он был)
            fbdMemoryBuf[curIndex] = s1;                            // сохраняем результат в буфер
        }
        // текущий элемент вычислен, пробуем достать из стека родительский элемент
        if(fbdStackPnt--) {
            curIndex = fbdStack[fbdStackPnt & FBDSTACKSIZEMASK].index;         // восстанавливаем родительский элемент
            curInput = fbdStack[fbdStackPnt & FBDSTACKSIZEMASK].input + 1;     // в родительском элементе сразу переходим к следующему входу
            baseInput = FBDINPUTOFFSET(curIndex);           // элемент сменился, расчет смещения на первый вход элемента
            inputCount = FBDdefInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
        } else break;                                       // стек пуст, вычисления завершены
    } while(1);
}

/**
 * @brief Сохранить значение в EEPROM памяти элемента
 * 
 * @param element Индекс элемента
 * @param index Индекс ячейки памяти
 * @param value Значение
 */
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value)
{
    tOffset offset = *(storageOffsets + element) + index;
    if(fbdStorageBuf[offset] != value) {
        fbdStorageBuf[offset] = value;
        FBDsetProc(FBD_NVRAM, offset, &fbdStorageBuf[offset]);
    }
}

/**
 * @brief Установить флаг "Элемент вычислен"
 * 
 * @param element Индекс элемента
 */
static void setCalcFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<((element&3)<<1);
}

/**
 * @brief Установка флага нарастания выходного значения элемента (rising flag)
 * 
 * @param element Индекс элемента
 */
static void setRiseFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<(((element&3)<<1)+1);
}

/**
 * @brief Получение значения флага "Элемент вычислен"
 * 
 * @param element Индекс элемента
 * @return char 1 - установлен, 0 - сброшен
 */
static char getCalcFlag(tElemIndex element)
{
   return (fbdFlagsBuf[element>>2]&(1u<<((element&3)<<1)))?1:0;
}

/**
 * @brief Получение значения флага нарастания выходного значения элемента (rising flag)
 * 
 * @param element Индекс элемента
 * @return char 1 - установлен, 0 - сброшен
 */
static char getRiseFlag(tElemIndex element)
{
    return (fbdFlagsBuf[element>>2]&(1u<<(((element&3)<<1)+1)))?1:0;
}

/**
 * @brief Установка флага изменения выходной сетевой переменной
 * 
 * @param index Индекс элемента ELEM_OUT_VAR
 */
static void setChangeVarFlag(tElemIndex index)
{
    tElemIndex varIndex = 0;
    // определяем номер флага
    while(index--) {
        if((fbdDescrBuf[index] & ELEMMASK) == ELEM_OUT_VAR) varIndex++;
    }
    fbdChangeVarBuf[varIndex>>3] |= 1u<<(varIndex&7);
}

/**
 * @brief Установка флага изменения значение Modbus
 * 
 * @param index Индекс элемента записи Modbus
 */
static void setChangeModbusFlag(tElemIndex index)
{
    tElemIndex varIndex = 0;
    // определяем номер флага
    while(index--) {
        if((fbdDescrBuf[index] & ELEMMASK) == ELEM_OUT_MDBS) varIndex++;
    }
    fbdChangeModbusBuf[varIndex>>3] |= 1u<<(varIndex&7);
}

/**
 * @brief Возвращает и сбрасыввет, если он установлен, флаг изменения значения записи в Modbus
 * 
 * @param index Индекс элемента записи Modbus
 * @return true Флаг установлен и был сброшен
 * @return false Флаг сброшен
 */
static bool getAndClearChangeModbusFlag(tElemIndex index)
{
    tElemIndex varIndex = 0;
    // определяем номер флага
    while(index--) {
        if((fbdDescrBuf[index] & ELEMMASK) == ELEM_OUT_MDBS) varIndex++;
    }
    //
    if(fbdChangeModbusBuf[varIndex >> 3]&(1u << (varIndex & 7))) {
        // флаг установлен, сбрасываем
        fbdChangeModbusBuf[varIndex>>3] &= ~(1u<<(varIndex&7));
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Установка признака "нет ответа" для элемента чтения или записи Modbus
 * @param index Индекс элемента чтения или записи Modbus
 */
static void setModbusNoResponse(tElemIndex index)
{
    if(index == MAX_INDEX) return;
    //
    switch (fbdDescrBuf[index] & ELEMMASK) {
        case ELEM_INP_MDBS:
            // ошибка чтения Modbus
            // если не установлен флаг FBD_MODBUSSAVEOLDVALUE, то устанавливаем значение по умолчанию
            if(!FBD_MODBUSSAVEOLDVALUE) fbdMemoryBuf[index] = FBDGETPARAMETER(index, 2);
            break;
        case ELEM_OUT_MDBS:
            // ошибка записи Modbus, устанавливаем флаг необходимости повторной записи
            setChangeModbusFlag(index);
            break;
    }
}

/**
 * @brief Разбор и установка принятых по Modbus данных
 * 
 * @param index Индекс элемента чтения или записи Modbus
 * @param response Принятые данные
 */
static void setModbusResponse(tElemIndex index, tSignal response)
{
    tSignal options;
    tModbusData data;
    unsigned char fmtcnt;
    //
    if(index == MAX_INDEX) return;
    // если это не элемент чтения Modbus, то ничего не далаем
    // для элементов записи Modbus флаг изменений сбрасывается при формировании запроса
    if((fbdDescrBuf[index] & ELEMMASK) != ELEM_INP_MDBS) return;
    // чтение выполнено успешно
    options = FBDGETPARAMETER(index, 1);
    fmtcnt = (options >> 28) & 15;
    //
    switch ((options >> 24) & 3) {
        case 0:
        case 1:
            // FBD_MODBUS_READ_COILS
            // FBD_MODBUS_READ_DISCRETE_INPUTS
            fmtcnt++;
            if(options & FBD_MODBUS_OPT_WO) fmtcnt += 16;
            // обнуляем ненужные биты
            fbdMemoryBuf[index] = response & getCoilBitsMask(fmtcnt);
            //
            break;
        case 2:
        case 3:
            // FBD_MODBUS_READ_HOLDING_REGISTERS
            // FBD_MODBUS_READ_INPUT_REGISTERS
            data.intData = response;
            // изменение порядка байт
            if(!(options & FBD_MODBUS_OPT_BO)) swapModbusByteOrder(&data);
            //
            switch (fmtcnt & 3) {
                case 0:
                    // uint16 (0..65535) 1 регистр
                    fbdMemoryBuf[index] = data.ushortData[0];
                    //
                    break;
                case 1:
                    // int16 (0..65535)  1 регистр
                    fbdMemoryBuf[index] = data.shortData[0];
                    //
                    break;
                case 2:
                    // int32             2 регистра
                    // изменение порядка слов
                    if(!(options & FBD_MODBUS_OPT_WO)) swapModbusWordOrder(&data);
                    fbdMemoryBuf[index] = data.intData;
                    //
                    break;
                case 3:
                    // single            2 регистра, MM - множитель 00 - 1, 01 - 10, 10 - 100, 11 - 1000
                    // изменение порядка слов
                    if(!(options & FBD_MODBUS_OPT_WO)) swapModbusWordOrder(&data);
                    // множитель
                    switch ((fmtcnt >> 2) & 3) {
                        case 0:
                            setModbusFloat(index, data.floatData, (float)1.0);
                            break;
                        case 1:
                            setModbusFloat(index, data.floatData, (float)10.0);
                            break;
                        case 2:
                            setModbusFloat(index, data.floatData, (float)100.0);
                            break;
                        case 3:
                            setModbusFloat(index, data.floatData, (float)1000.0);
                            break;
                    }
                    break;
            }
    }
}

/**
 * @brief Заполнение структуры запроса Modbus
 *
 * @param index Индекс элемента чтения или записи Modbus
 * @param mbrequest Указатель на структуру запроса Modbus
 */
static void fillModbusRequest(tElemIndex index, tModbusReq *mbrequest)
{
    unsigned char fmtcnt;
    tSignal options = FBDGETPARAMETER(index, 1);
    //
    mbrequest->ip = FBDGETPARAMETER(index, 0);
    mbrequest->regAddr = options & 0x0000ffff;
    mbrequest->slaveAddr = (options >> 16) & 0xff;
    fmtcnt = (options >> 28) & 15;
    //
    if((fbdDescrBuf[index] & ELEMMASK) == ELEM_INP_MDBS) {
        // чтение Modbus
        switch ((options >> 24) & 3) {
            case 0:
                mbrequest->funcCode = FBD_MODBUS_READ_COILS;
                mbrequest->regCount = fmtcnt + 1;
                if(options & FBD_MODBUS_OPT_WO) mbrequest->regCount += 16;
                break;
            case 1:
                mbrequest->funcCode = FBD_MODBUS_READ_DISCRETE_INPUTS;
                mbrequest->regCount = fmtcnt + 1;
                if(options & FBD_MODBUS_OPT_WO) mbrequest->regCount += 16;
                break;
            case 2:
                mbrequest->funcCode = FBD_MODBUS_READ_HOLDING_REGISTERS;
                // FMT  - формат читаемых данных (для "read input registers" и "read holding registers"):
                //  0000    - uint16 (0..65535) 1 регистр
                //  0001    - int16 (0..65535)  1 регистр
                //  0010    - int32             2 регистра
                //  MM11    - single            2 регистра, MM - множитель 00 - 1, 01 - 10, 10 - 100, 11 - 1000
                mbrequest->regCount = (fmtcnt & 2)?2:1;
                break;
            case 3:
                mbrequest->funcCode = FBD_MODBUS_READ_INPUT_REGISTERS;
                mbrequest->regCount = (fmtcnt & 2)?2:1;
                break;
        }
    } else {
        // запись Modbus
        switch ((options >> 24) & 3) {
            case 0:
                // FBD_MODBUS_WRITE_SINGLE_COIL
                mbrequest->funcCode = FBD_MODBUS_WRITE_SINGLE_COIL;
                mbrequest->regCount = 1;
                mbrequest->data.intData = fbdMemoryBuf[index]?0x00ff:0;
                break;
            case 1:
                // FBD_MODBUS_WRITE_MULTIPLE_COILS
                mbrequest->funcCode = FBD_MODBUS_WRITE_MULTIPLE_COILS;
                mbrequest->regCount = fmtcnt + 1;
                if(options & FBD_MODBUS_OPT_WO) mbrequest->regCount += 16;
                mbrequest->data.intData = fbdMemoryBuf[index];
                break;
            case 2:
                // FBD_MODBUS_WRITE_SINGLE_REGISTER
                mbrequest->funcCode = FBD_MODBUS_WRITE_SINGLE_REGISTER;
                mbrequest->regCount = 1;
                mbrequest->data.intData = fbdMemoryBuf[index];
                if(!(options & FBD_MODBUS_OPT_BO)) swapModbusByteOrder(&mbrequest->data);
                break;
            case 3:
                mbrequest->funcCode = FBD_MODBUS_WRITE_MULTIPLE_REGISTERS;
                // FBD_MODBUS_WRITE_MULTIPLE_REGISTERS
                // формат данных
                switch (fmtcnt & 3) {
                    case 0:
                    case 1:
                        // UINT16, INT16
                        mbrequest->regCount = 1;
                        mbrequest->data.intData = fbdMemoryBuf[index];
                        // порядок байт
                        if(!(options & FBD_MODBUS_OPT_BO)) swapModbusByteOrder(&mbrequest->data);
                        //
                        break;
                    case 2:
                        // INT32
                        mbrequest->regCount = 2;
                        mbrequest->data.intData = fbdMemoryBuf[index];
                        // порядок байт и слов
                        if(!(options & FBD_MODBUS_OPT_BO)) swapModbusByteOrder(&mbrequest->data);
                        if(!(options & FBD_MODBUS_OPT_WO)) swapModbusWordOrder(&mbrequest->data);
                        //
                        break;
                    case 3:
                        // float
                        mbrequest->regCount = 2;
                        // приведение к типу single precission float
                        mbrequest->data.floatData = fbdMemoryBuf[index];
                        // делитель
                        switch ((fmtcnt >> 2) & 3) {
                            case 0:
                                // mbrequest->data.floatData /= 1;
                                break;
                            case 1:
                                mbrequest->data.floatData /= (float)10.0;
                                break;
                            case 2:
                                mbrequest->data.floatData /= (float)100.0;
                                break;
                            case 3:
                                mbrequest->data.floatData /= (float)1000.0;
                                break;
                        }
                        // порядок байт и слов
                        if(!(options & FBD_MODBUS_OPT_BO)) swapModbusByteOrder(&mbrequest->data);
                        if(!(options & FBD_MODBUS_OPT_WO)) swapModbusWordOrder(&mbrequest->data);
                        //
                        break;
                }
                //
                break;
        }
    }
}

/**
 * @brief Проверка и установка значения чтения Modbus с типом float
 * 
 * @param index Индекс элемента чтения Modbus
 * @param data Новые данные
 * @param mul Множитель
 */
static void setModbusFloat(tElemIndex index, float data, float mul)
{
    int fpc = fpclassify(data);
    if((fpc == FP_NORMAL) || (fpc == FP_ZERO)) {
        data = roundf(data * mul);
        if((data >= MIN_SIGNAL)&&(data <= MAX_SIGNAL)) {
            // корректное значение, устанавливаем его
            fbdMemoryBuf[index] = data;
            return;
        }
    }
    // значение float некорректно, ставим значение по умолчанию
    fbdMemoryBuf[index] = FBDGETPARAMETER(index, 2);
}

/**
 * @brief Меняет порядок байтов в данных Modbus
 * @param data Указатель на данные
 */
static void swapModbusByteOrder(tModbusData *data)
{
    unsigned char t;
    //
    t = (*data).byteData[0];
    (*data).byteData[0] = (*data).byteData[1];
    (*data).byteData[1] = t;
    //
    t = (*data).byteData[2];
    (*data).byteData[2] = (*data).byteData[3];
    (*data).byteData[3] = t;
}

/**
 * @brief Меняет порядок слов в данных Modbus
 * @param data Указатель на данные
 */
static void swapModbusWordOrder(tModbusData *data)
{
    unsigned short t;
    //
    t = (*data).ushortData[0];
    (*data).ushortData[0] = (*data).ushortData[1];
    (*data).ushortData[1] = t;
}

/**
 * @brief Вычисление абсолютного значения сигнала
 * 
 * @param val Значение сигнала
 * @return tSignal Абсолютное значение сигнала
 */
static inline tSignal intAbs(tSignal val)
{
    return (val>=0)?val:-val;
}

/**
 * @brief Возвращает значение у которого установлены count младших бит.
 * 
 * @param count Количество бит
 * @return uint32_t Результат
 */
static uint32_t getCoilBitsMask(unsigned count)
{
    if(count < 32) {
        return (1u << count) - 1;
    } else {
        return 0xffffffff;
    }
}

/**
 * @brief Интерполяция по двум точкам
 * 
 * @param x Входное значение
 * @param x1 Точка 1, X
 * @param y1 Точка 1, Y
 * @param x2 Точка 2, X
 * @param y2 Точка 2, Y
 * @return tSignal Вычисленное значение Y
 */
static tSignal interpolation(tSignal x, tSignal x1, tSignal y1, tSignal x2, tSignal y2)
{
    if(x == x1) return y1;
    if(x == x2) return y2;
    //
    if(x1 == x2) {
        // особые случаи
        if(y1 == y2) {
            // если точки слились
            return y1;
        } else {
            // бесконечность
            if(x > x1) return MAX_SIGNAL; else return MIN_SIGNAL;
        }
    } else {
        // простая интерполяция
#ifdef USE_MATH
        return lroundf(((float)x-x1)*(float)(y2-y1)/(float)(x2-x1) + y1);
#else
        return ((x-x1)*(y2-y1)/(x2-x1) + y1);
#endif
    }
}

#if defined(BIG_ENDIAN) && (SIGNAL_SIZE > 1)
typedef union {
    tSignal value;
#if SIGNAL_SIZE == 2
    char B[2];
#elif SIGNAL_SIZE == 4
    char B[4];
#endif
} teus;
// -------------------------------------------------------------------------------------------------------
tSignal lotobigsign(tSignal val)
{
    teus uval;
    char t;
    //
    uval.value = val;
    t = uval.B[0];
#if (SIGNAL_SIZE == 2)
    uval.B[0] = uval.B[1];
    uval.B[1] = t;
#else
    uval.B[0] = uval.B[3];
    uval.B[3] = t;
    t = uval.B[1];
    uval.B[1] = uval.B[2];
    uval.B[2] = t;
#endif // SIGNAL_SIZE
    return uval.value;
}
#endif // defined
//
#if defined(BIG_ENDIAN) && (INDEX_SIZE > 1)
typedef union {
    tSignal value;
    char B[2];
} teui;
// -------------------------------------------------------------------------------------------------------
tElemIndex lotobigidx(tElemIndex val)
{
    teui uval;
    char t;
    //
    uval.value = val;
    t = uval.B[0];
    uval.B[0] = uval.B[1];
    uval.B[1] = t;
    return uval.value;
}
#endif // defined
