/**
 * @file fbdrt.c
 * @author crossrw1@gmail.com
 * @brief FBD-Runtime library
 * @version 9.0
 * @date 2021-11-17
 */

#include <string.h>
#include "fbdrt.h"

#ifdef USE_HMI
// для snprintf
#include <stdio.h>
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

// -----------------------------------------------------------------------------
// FBDgetProc() и FBDsetProc() - callback, должны быть описаны в основной программе
// -----------------------------------------------------------------------------

/**
 * @brief Чтение значения входного сигнала или NVRAM.
 * Должна быть описана в основной программе.
 * 
 * @param type Tип чтения: 0 - входной сигнал, 1 - значение NVRAM
 * @param index Индекс читаемого значения
 * @return tSignal Прочитанное значение сигнала
 */
extern tSignal FBDgetProc(char type, tSignal index);

/**
 * @brief Запись значения выходного сигнала или NVRAM.
 * Должна быть описана в основной программе.
 * 
 * @param type Tип записи: 0 - выходной сигнал, 1 - значение NVRAM
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
#ifndef SPEED_OPT
DESCR_MEM char DESCR_MEM_SUFX * DESCR_MEM_SUFX fbdGetCaption(tElemIndex index);
#endif
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

void setCalcFlag(tElemIndex element);
void setRiseFlag(tElemIndex element);
void setChangeVarFlag(tElemIndex index);
void setChangeModbusFlag(tElemIndex index);
bool getAndClearChangeModbusFlag(tElemIndex index);
void setModbusNoResponse(tElemIndex index);
void setModbusResponse(tElemIndex index, tSignal response);
void fillModbusRequest(tElemIndex index, tModbusReq *mbrequest);
void setModbusFloat(tElemIndex index, float data, float mul);
unsigned long int getCoilBitsMask(unsigned count);
void swapModbusByteOrder(tModbusData *data);
void swapModbusWordOrder(tModbusData *data);

char getCalcFlag(tElemIndex element);
char getRiseFlag(tElemIndex element);

tSignal intAbs(tSignal val);

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
// текстовые описания входов и выходов, количество указано в параметре FBD_OPT_HINTS_COUNT
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

#ifdef SPEED_OPT
// только при использовании оптимизации по скорости выполнения
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
// указатели для быстрого доступа к тектовым описаниям проекта
DESCR_MEM char *fbdCaptionName;
DESCR_MEM char *fbdCaptionVersion;
DESCR_MEM char *fbdCaptionBTime;
//
#endif // SPEED_OPT

tElemIndex fbdElementsCount;
tElemIndex fbdStorageCount;
tElemIndex fbdFlagsByteCount;
tElemIndex fbdChangeVarByteCount;
tElemIndex fbdChangeModbusByteCount;

tElemIndex fbdModbusRTUCount;
tElemIndex fbdModbusTCPCount;
tElemIndex fbdModbusRTUIndex;
tElemIndex fbdModbusTCPIndex;

#ifdef USE_HMI
tElemIndex fbdWpCount;
tElemIndex fbdSpCount;
short      fbdCurrentScreen;                 // текущий экран
unsigned short fbdCurrentScreenTimer;        // таймер текущего экрана
#endif // USE_HMI
//
char fbdFirstFlag;

// массив с количествами входов для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefInputsCount[ELEM_TYPE_COUNT]       = {1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,4,3,3,5,1,1,0,2,2,2,3,2,2,2,2,2,0,1};
// массив с количествами параметров для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefParametersCount[ELEM_TYPE_COUNT]   = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,0,0,0,0,0,1,5,0,0,0,0,0,0,0,0,1,3,2};
// массив с количествами хранимых данных для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefStorageCount[ELEM_TYPE_COUNT]      = {0,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,1,2,1,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0};
//                                                                                 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3
//                                                                                 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4
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

#ifdef SPEED_OPT
// описания "быстрых" вариантов функций
// --------------------------------------------------------------------------------------------
#define FBDINPUTOFFSET(index) *(inputOffsets + (index))
#define FBDGETPARAMETER(element,index) SIGNAL_BYTE_ORDER(fbdParametersBuf[*(parameterOffsets+element)+index])
#define FBDGETSTORAGE(element,index) fbdStorageBuf[*(storageOffsets+element)+index]

#else // SPEED_OPT
// описания "медленных" вариантов функций
// --------------------------------------------------------------------------------------------
// расчет смещения на первый вход элемента
#define FBDINPUTOFFSET(index) _fbdInputOffset(index)
tOffset _fbdInputOffset(tElemIndex index)
{
    tElemIndex i = 0;
    tOffset offset = 0;
    //
    while (i < index) offset += FBDdefInputsCount[fbdDescrBuf[i++] & ELEMMASK];
    return offset;
}
// получить значение параметра элемента
#define FBDGETPARAMETER(element,index) _fbdGetParameter(element,index)
tSignal _fbdGetParameter(tElemIndex element, unsigned char index)
{
    tElemIndex elem = 0;
    tOffset offset = 0;
    //
    while (elem < element) offset += FBDdefParametersCount[fbdDescrBuf[elem++] & ELEMMASK];
    return SIGNAL_BYTE_ORDER(fbdParametersBuf[offset + index]);
}
// получить хранимое значение для элемента
#define FBDGETSTORAGE(element,index) _fbdGetStorage(element,index)
tSignal _fbdGetStorage(tElemIndex element, unsigned char index)
{
    tElemIndex elem = 0;
    tOffset offset = 0;
    //
    while (elem<element) offset += FBDdefStorageCount[fbdDescrBuf[elem++] & ELEMMASK];
    return fbdStorageBuf[offset + index];
}
#endif // SPEED_OPT

// расчет CRC32, используется при проверке корректности программы
unsigned long int fbdCRC32(DESCR_MEM unsigned char DESCR_MEM_SUFX *data, int size)
{
    unsigned long int crc = ~0;
    //
    while(size--) {
        crc ^= *data++;
        for(int i = 0; i < 8; i++) {
            unsigned long int t = ~((crc&1)-1);
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
        if(elem & 0x80) break;
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
    // проверка версии программы
    if(*fbdGlobalOptions > FBD_LIB_VERSION) return ERR_INVALID_LIB_VERSION;
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
#endif // USE_HMI
    // расчёт и проверка CRC, если указан параметр FBD_SCHEMA_SIZE
    if(FBD_SCHEMA_SIZE) {
        // если результат 0, то CRC не используется (старая версия редактора)
        if(fbdCRC32(fbdDescrBuf, FBD_SCHEMA_SIZE)) return ERR_INVALID_CHECK_SUM;
    }
    // память для флагов расчета и фронта
    fbdFlagsByteCount = (fbdElementsCount>>2) + ((fbdElementsCount&3)?1:0);
    // память для флагов изменений значения выходной переменной
    fbdChangeVarByteCount = (fbdChangeVarByteCount>>3) + ((fbdChangeVarByteCount&7)?1:0);
    // память для флагов изменений значения записи в Modbus
    fbdChangeModbusByteCount = (fbdChangeModbusByteCount>>3) + ((fbdChangeModbusByteCount&7)?1:0);
    //
#ifdef SPEED_OPT
#ifdef USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdChangeModbusByteCount + fbdElementsCount*3*sizeof(tOffset) + (fbdWpCount+fbdSpCount)*sizeof(tPointAccess);
#else  // USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdChangeModbusByteCount + fbdElementsCount*3*sizeof(tOffset);
#endif // USE_HMI
#else  // SPEED_OPT
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdChangeModbusByteCount;
#endif // SPEED_OPT
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
#ifdef SPEED_OPT
    tOffset curInputOffset = 0;
    tOffset curParameterOffset = 0;
    tOffset curStorageOffset = 0;
#ifdef USE_HMI
    tOffset curWP = 0;
    tOffset curSP = 0;
    DESCR_MEM char DESCR_MEM_SUFX *curCap;
#endif // USE_HMI
#endif // SPEED_OPT
    fbdModbusRTUIndex = MAX_INDEX;
    fbdModbusTCPIndex = MAX_INDEX;
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
            FBDsetProc(1, i, &fbdStorageBuf[i]);
        } else {
            fbdStorageBuf[i] = FBDgetProc(1, i);
        }
    }
#ifdef SPEED_OPT
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
#endif // SPEED_OPT
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
bool isScrElemVisible(tScrElemBase *elem)
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

tSignal getElementOutputValue(tElemIndex index)
{
    if(index == 0xffff) return 0;
    return fbdMemoryBuf[index];
}

void sprintf2d(char *buf, tSignal val)
{
    *(buf + 1) = val % 10 + '0';
    val /= 10;
    *(buf) = val % 10 + '0';
}

void sprintf4d(char *buf, tSignal val)
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
void drawCurrentScreen(DESCR_MEM tScreen DESCR_MEM_SUFX *screen)
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
                            FBDdrawLine(roundf(elem->x1-j*((tScrElemLine *)elem)->sine), roundf(elem->y1+j*((tScrElemLine *)elem)->cosinus), roundf(((tScrElemLine *)elem)->x2-j*((tScrElemLine *)elem)->sine), roundf(((tScrElemLine *)elem)->y2+j*((tScrElemLine *)elem)->cosinus), ((tScrElemLine *)elem)->color);
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
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_DAY));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'm':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_MONTH));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'y':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_YEAR)-2000);
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'h':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_HOUR));
                                        k += 2;
                                    }
                                    j++;
                                    break;                                
                                case 'n':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_MINUTE));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 's':
                                    if(k < (sizeof(dttext)-2)) {
                                        sprintf2d(&dttext[k], FBDgetProc(0, GP_RTC_SECOND));
                                        k += 2;
                                    }
                                    j++;
                                    break;
                                case 'Y':
                                    if(k < (sizeof(dttext)-4)) {
                                        sprintf4d(&dttext[k], FBDgetProc(0, GP_RTC_YEAR));
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
    tSignal value, param;
    tElemIndex index;
    // сброс признаков расчета и нарастающего фронта
    memset(fbdFlagsBuf, 0, fbdFlagsByteCount);
    // основной цикл расчета
    for(index=0; index < fbdElementsCount; index++) {
        unsigned char element = fbdDescrBuf[index] & ELEMMASK;
        switch(element) {
            // элементы с таймером
            case ELEM_TON:                                                  // timer TON
            case ELEM_PID:                                                  // PID
            case ELEM_SUM:                                                  // SUM
            case ELEM_TP:                                                   // timer TP
            case ELEM_GEN:                                                  // GEN
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
                FBDsetProc(0, param, &fbdMemoryBuf[index]);                 // установка значения выходного контакта
                break;
            case ELEM_OUT_VAR:                                              // выходная переменная
            case ELEM_OUT_MDBS:                                             // запись Modbus
#ifdef USE_HMI
            case ELEM_WP:                                                   // точка контроля
#endif // USE_HMI
                fbdCalcElement(index);
                break;
        }
    }
    fbdFirstFlag = 0;
}

// -------------------------------------------------------------------------------------------------------

/**
 * @brief Установить принятое по сети значение переменной
 * 
 * @param netvar Указатель на структуру описания сетевой переменной
 */
void fbdSetNetVar(tNetVar *netvar)
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
    tElemIndex index, i;
    unsigned char elem;
    // поиск следующего элемента
    index = fbdModbusRTUIndex;
    for(i=0; i < fbdElementsCount; i++) {
        // переходим к следующему элементу
        index++;
        if(index >= fbdElementsCount) index = 0;
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
                }
                // нашли подходящий элемент
                fbdModbusRTUIndex = index;
                fillModbusRequest(index, mbrequest);
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Установка результата успешного выполнения запроса Modbus RTU.
 * Должна быть вызвана после успешного выполнения запроса чтения или записи Modbus RTU.
 * @param response Данные, возвращённые запросом
 */
void fbdSetModbusRTUResponse(tSignal response)
{
    setModbusResponse(fbdModbusRTUIndex, response);
}

/**
 * @brief Установить признак неуспешного результата выполнения запроса ModBus RTU.
 * Устанавливается признак неуспешного чтения или записи Modbus для ранее полученного описания запроса.
 * Должна вызываться при любой ошибке: нет ответа, ошибка CRC, получение кода исключения и т.п.
 */
void fbdSetModbusRTUNoResponse(void)
{
    setModbusNoResponse(fbdModbusRTUIndex);
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
    // поиск следующего элемента Modbus TCP
    tElemIndex index, i;
    unsigned char elem;
    // поиск следующего элемента
    index = fbdModbusTCPIndex;
    for(i=0; i < fbdElementsCount; i++) {
        // переходим к следующему элементу
        index++;
        if(index >= fbdElementsCount) index = 0;
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
                }
                // нашли подходящий элемент
                fbdModbusTCPIndex = index;
                fillModbusRequest(index, mbrequest);
                return true;
            }
        }
    }
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
    setModbusResponse(fbdModbusTCPIndex, response);
}

/**
 * @brief Установить признак неуспешного результата выполнения запроса ModBus TCP.
 * Устанавливается признак неуспешного чтения или записи Modbus для ранее полученного описания запроса.
 */
void fbdSetModbusTCPNoResponse(void)
{
    setModbusNoResponse(fbdModbusTCPIndex);
}

#ifdef USE_HMI
#ifndef SPEED_OPT
// -------------------------------------------------------------------------------------------------------
bool fbdGetElementIndex(tSignal index, unsigned char type, tElemIndex *elemIndex)
{
    unsigned char elem;
    //
    *elemIndex = 0;
    while(1) {
        elem = fbdDescrBuf[*elemIndex];
        if(elem & 0x80) return false;
        if(elem == type) {
            if(index) index--; else break;
        }
        (*elemIndex)++;
    }
    return true;
}
#endif // SPEED_OPT

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
#ifdef SPEED_OPT
    elemIndex = (spOffsets + index)->index;
    pnt->caption = (spOffsets + index)->caption;
#else
    if(!fbdGetElementIndex(index, ELEM_SP, &elemIndex)) return false;
    pnt->caption = fbdGetCaption(elemIndex);
#endif // SPEED_OPT
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
#ifdef SPEED_OPT
    elemIndex = (spOffsets + index)->index;
#else
    if(!fbdGetElementIndex(index, ELEM_SP, &elemIndex)) return;
#endif // SPEED_OPT
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
#ifdef SPEED_OPT
    elemIndex = (wpOffsets + index)->index;
    pnt->caption = (wpOffsets + index)->caption;
#else
    if(!fbdGetElementIndex(index, ELEM_WP, &elemIndex)) return false;
    pnt->caption = fbdGetCaption(elemIndex);
#endif // SPEED_OPT
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
#ifdef SPEED_OPT
    pnt->name = fbdCaptionName?fbdCaptionName:(fbdCaptionName = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount));
    pnt->version = fbdCaptionVersion?fbdCaptionVersion:(fbdCaptionVersion = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 1));
    pnt->btime = fbdCaptionBTime?fbdCaptionBTime:(fbdCaptionBTime = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 2));
#else
    pnt->name = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount);
    pnt->version = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 1);
    pnt->btime = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 2);
#endif
}

/**
 * @brief Получение указателя на текстовое описание (хинт) входа или выхода.
 * Возвращает указатель на текстовое описание (хинт) входа или выхода, если такого описание не найдено, то возвращает NULL.
 * Значение параметра index соответствует значению параметра index функций FBDgetProc(type, index) и FBDsetProc(type, index, *value).
 * @param type Тип: 0 - входы, 1 - выходы.
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

#ifndef SPEED_OPT
// -------------------------------------------------------------------------------------------------------
// расчет указателя на текстовое описание элемента по индексу элемента
DESCR_MEM char DESCR_MEM_SUFX * fbdGetCaption(tElemIndex elemIndex)
{
    tElemIndex captionIndex, index;
    //
    index = 0;
    captionIndex = 0;
    while(index < elemIndex) {
        switch(fbdDescrBuf[index++] & ELEMMASK) {
            case ELEM_WP:
            case ELEM_SP:
                captionIndex++;
                break;
        }
    }
    return fbdGetCaptionByIndex(captionIndex);
}
#endif // SPEED_OPT
#endif // USE_HMI

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
                case ELEM_OUT_PIN:                                                      // OUTPUT PIN
                case ELEM_WP:                                                           // HMI watchpoint
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
                    s1 = FBDgetProc(0, FBDGETPARAMETER(curIndex, 0));                   // INPUT PIN
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
                    if(!FBDGETSTORAGE(curIndex, 0)) {       // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s2);     // установка таймера
                        //
                        s1 += fbdMemoryBuf[curIndex];       // сложение с предыдущим значением
                        // ограничение
                        if(s1 > 0) { if(s1 > s3) s1 = s3; } else { if(s1 < -s3) s1 = -s3; }
                    } else s1 = fbdMemoryBuf[curIndex];
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
                                    s1 = roundl(1.0*s2*(s1-s3)/s1);
#else
                                    s1 = (s2*(s1-s3))/s1;
#endif
                                    break;
                                case 2:                                 // треугольник
                                    if(s3 > (s1>>1)) {
#ifdef USE_MATH
                                        s1 = roundl(2.0*s2*(s1-s3)/s1); // нарастание
                                    } else {
                                        s1 = roundl(2.0*s2*s3/s1);      // спад
#else
                                        s1 = 2*s2*(s1-s3)/s1;           // нарастание
                                    } else {
                                        s1 = 2*s2*s3/s1;                // спад
#endif
                                    }
                                    break;
#ifdef USE_MATH
                                case 3:                                 // sin
                                    s1 = roundl(s2*sin(2.0*M_PI*(s1-s3)/s1));
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
            }
            setCalcFlag(curIndex);                                  // установка флага "вычисление выполнено"
            if(fbdDescrBuf[curIndex] & INVERTFLAG) s1 = s1?0:1;     // инверсия результата (если нужно)
            if(s1 > fbdMemoryBuf[curIndex]) setRiseFlag(curIndex);  // установка флага фронта (если он был)
            fbdMemoryBuf[curIndex] = s1;                            // сохраняем результат в буфер
        }
        // текущий элемент вычислен, пробуем достать из стека родительский элемент
        if(fbdStackPnt--) {
            curIndex = fbdStack[fbdStackPnt].index;         // восстанавливаем родительский элемент
            curInput = fbdStack[fbdStackPnt].input + 1;     // в родительском элементе сразу переходим к следующему входу
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
#ifdef SPEED_OPT
    tOffset offset = *(storageOffsets + element) + index;
#else
    tOffset offset = 0;
    tElemIndex elem = 0;
    //
    while (elem < element) offset += FBDdefStorageCount[fbdDescrBuf[elem++] & ELEMMASK];
    offset += index;
#endif // SPEED_OPT
    if(fbdStorageBuf[offset] != value) {
        fbdStorageBuf[offset] = value;
        FBDsetProc(1, offset, &fbdStorageBuf[offset]);
    }
}

/**
 * @brief Установить флаг "Элемент вычислен"
 * 
 * @param element Индекс элемента
 */
void setCalcFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<((element&3)<<1);
}

/**
 * @brief Установка флага нарастания выходного значения элемента (rising flag)
 * 
 * @param element Индекс элемента
 */
void setRiseFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<(((element&3)<<1)+1);
}

/**
 * @brief Получение значения флага "Элемент вычислен"
 * 
 * @param element Индекс элемента
 * @return char 1 - установлен, 0 - сброшен
 */
char getCalcFlag(tElemIndex element)
{
   return (fbdFlagsBuf[element>>2]&(1u<<((element&3)<<1)))?1:0;
}

/**
 * @brief Получение значения флага нарастания выходного значения элемента (rising flag)
 * 
 * @param element Индекс элемента
 * @return char 1 - установлен, 0 - сброшен
 */
char getRiseFlag(tElemIndex element)
{
    return (fbdFlagsBuf[element>>2]&(1u<<(((element&3)<<1)+1)))?1:0;
}

/**
 * @brief Установка флага изменения выходной сетевой переменной
 * 
 * @param index Индекс элемента ELEM_OUT_VAR
 */
void setChangeVarFlag(tElemIndex index)
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
void setChangeModbusFlag(tElemIndex index)
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
bool getAndClearChangeModbusFlag(tElemIndex index)
{
    tElemIndex varIndex = 0;
    // определяем номер флага
    while(index--) {
        if((fbdDescrBuf[index] & ELEMMASK) == ELEM_OUT_MDBS) varIndex++;
    }
    if(fbdChangeVarBuf[varIndex >> 3]&(1u << (varIndex & 7))) {
        // флаг установлен, сбрасываем
        fbdChangeVarBuf[varIndex>>3] &= ~(1u<<(varIndex&7));
        return true;
    } else {
        return false;
    }
}

/**
 * @brief Установка признака "нет ответа" для элемента чтения или записи Modbus
 * @param index Индекс элемента чтения или записи Modbus
 */
void setModbusNoResponse(tElemIndex index)
{
    if(index == MAX_INDEX) return;
    //
    switch (fbdDescrBuf[index] & ELEMMASK) {
        case ELEM_INP_MDBS:
            // ошибка чтения Modbus, устанавливаем значение по умолчанию
            fbdMemoryBuf[index] = FBDGETPARAMETER(index, 2);
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
void setModbusResponse(tElemIndex index, tSignal response)
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
void fillModbusRequest(tElemIndex index, tModbusReq *mbrequest)
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
void setModbusFloat(tElemIndex index, float data, float mul)
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
void swapModbusByteOrder(tModbusData *data)
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
void swapModbusWordOrder(tModbusData *data)
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
inline tSignal intAbs(tSignal val)
{
    return (val>=0)?val:-val;
}

/**
 * @brief Возвращает значение у кторого установлены count младших бит.
 * 
 * @param count Количество бит
 * @return unsigned long int Результат
 */
unsigned long int getCoilBitsMask(unsigned count)
{
    if(count < 32) {
        return (1 << count) - 1;
    } else {
        return 0xffffffff;
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
