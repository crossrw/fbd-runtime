#include <string.h>
#include "fbdrt.h"

// 06-01-2018
// + добавлены битовые операции
// + добавлен генератор сигналов
// + для входных сетевых переменных добавлено значение "по умолчанию"
// * изменен способ проверки версии RTL
// * добавлено определение USE_MATH, с ним для некоторых расчетов используется плавающая точка

// 12-01-2018
// * починил SPEED_OPT

// -----------------------------------------------------------------------------
// FBDgetProc() и FBDsetProc() - callback, должны быть описаны в основной программе
// -----------------------------------------------------------------------------
// FBDgetProc(): чтение значения входного сигнала или NVRAM
// type - тип чтения
//  0 - входной сигнал
//  1 - значение NVRAM
// index - индекс читаемого значения
// result - прочитанное значение сигнала
extern tSignal FBDgetProc(char type, tSignal index);

// FBDsetProc() запись значения выходного сигнала или NVRAM
// type - тип записи
//  0 - запись выходного сигнала
//  1 - запись в NVRAM
// index - индекс записываемого значения
// value - указатель на значение
extern void FBDsetProc(char type, tSignal index, tSignal *value);
// -----------------------------------------------------------------------------

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
typedef struct {
    tElemIndex index;               // индекс элемента
    unsigned char input;            // номер входа
} tFBDStackItem;

void setCalcFlag(tElemIndex element);
void setRiseFlag(tElemIndex element);
void setChangeVarFlag(tElemIndex element);

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
// флаги "значение изменилось", 1 бит для каждого элемента "Output VAR" (14)
char *fbdChangeVarBuf;
//  ChangeVar0
//  ChangeVar1
//  ...
//  ChangeVarN

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
typedef struct {
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

#ifdef USE_HMI
tElemIndex fbdWpCount;
tElemIndex fbdSpCount;
#endif // USE_HMI
//
char fbdFirstFlag;

#define ELEMMASK 0x3F
#define INVERTFLAG 0x40

#define MAXELEMTYPEVAL 32u

// массив с количествами входов для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefInputsCount[MAXELEMTYPEVAL+1] =     {1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,4,3,3,5,1,1,0,2,2,2,3,2,2,2,2,2};
// массив с количествами параметров для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefParametersCount[MAXELEMTYPEVAL+1] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,2,0,0,0,0,0,1,5,0,0,0,0,0,0,0,0,1};
// массив с количествами хранимых данных для элементов каждого типа
ROM_CONST unsigned char ROM_CONST_SUFX FBDdefStorageCount[MAXELEMTYPEVAL+1]    = {0,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,1,2,1,1,0,0,0,1,1,0,0,0,0,0,0,0,1};
//                                                                                0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3
//                                                                                0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2

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


// -------------------------------------------------------------------------------------------------------
int fbdInit(DESCR_MEM unsigned char DESCR_MEM_SUFX *buf)
{
    tOffset inputs = 0;
    tOffset parameters = 0;
    unsigned char elem;
    //
    fbdElementsCount = 0;
    fbdStorageCount = 0;
    fbdChangeVarByteCount = 0;
#ifdef USE_HMI
    fbdWpCount = 0;
    fbdSpCount = 0;
#endif // USE_HMI
    //
    if(!buf) return -1;
    fbdDescrBuf = buf;
    // цикл по всем элементам
    while(1) {
        elem = fbdDescrBuf[fbdElementsCount];
        if(elem & 0x80) break;
        elem &= ELEMMASK;
        if(elem > MAXELEMTYPEVAL) return -1;
        // подсчет всех входов
        inputs += FBDdefInputsCount[elem];
        // подсчет всех параметров
        parameters += FBDdefParametersCount[elem];
        // подсчет всех хранимых параметров
        fbdStorageCount += FBDdefStorageCount[elem];
#ifdef USE_HMI
        if(elem == 22) fbdWpCount++; else if(elem == 23) fbdSpCount++;
#endif // USE_HMI
        // подсчет выходных переменных
        if(elem == 14) fbdChangeVarByteCount++; 
        // общий подсчет элементов
        fbdElementsCount++;
    }
    // проверка правильности флага завершения
    if(elem != END_MARK) return -2;
    // расчет указателей
    fbdInputsBuf = (DESCR_MEM tElemIndex DESCR_MEM_SUFX *)(fbdDescrBuf + fbdElementsCount + 1);
    fbdParametersBuf = (DESCR_MEM tSignal DESCR_MEM_SUFX *)(fbdInputsBuf + inputs);
    fbdGlobalOptionsCount = (DESCR_MEM unsigned char DESCR_MEM_SUFX *)(fbdParametersBuf + parameters);
    fbdGlobalOptions = (DESCR_MEM tSignal DESCR_MEM_SUFX *)(fbdGlobalOptionsCount + 1);
    // проверка версии программы
    if(*fbdGlobalOptions > FBD_LIB_VERSION) return -3;

#ifdef USE_HMI
    fbdCaptionsBuf = (DESCR_MEM char DESCR_MEM_SUFX *)(fbdGlobalOptions + *fbdGlobalOptionsCount);
#endif // USE_HMI
    // память для флагов расчета и фронта
    fbdFlagsByteCount = (fbdElementsCount>>2) + ((fbdElementsCount&3)?1:0);
    // память для флагов изменений значения выходной переменной
    fbdChangeVarByteCount = (fbdChangeVarByteCount>>3) + ((fbdChangeVarByteCount&7)?1:0);
    //
#ifdef SPEED_OPT
#ifdef USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdElementsCount*3*sizeof(tOffset) + (fbdWpCount+fbdSpCount)*sizeof(tPointAccess);
#else  // USE_HMI
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount + fbdElementsCount*3*sizeof(tOffset);
#endif // USE_HMI
#else  // SPEED_OPT
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount + fbdChangeVarByteCount;
#endif // SPEED_OPT
}
// -------------------------------------------------------------------------------------------------------
void fbdSetMemory(char *buf, bool needReset)
{
    tElemIndex i;
#ifdef USE_HMI
    tHMIdata HMIdata;
#endif // USE_HMI
#ifdef SPEED_OPT
    tOffset curInputOffset = 0;
    tOffset curParameterOffset = 0;
    tOffset curStorageOffset = 0;
    unsigned char elem;
#ifdef USE_HMI
    tOffset curWP = 0;
    tOffset curSP = 0;
    DESCR_MEM char DESCR_MEM_SUFX *curCap;
#endif // USE_HMI
#endif // SPEED_OPT
    fbdMemoryBuf = (tSignal *)buf;
    // инициализация указателей
    fbdStorageBuf = fbdMemoryBuf + fbdElementsCount;
    fbdFlagsBuf = (char *)(fbdStorageBuf + fbdStorageCount);
    fbdChangeVarBuf = (char *)(fbdFlagsBuf + fbdFlagsByteCount);
    // инициализация памяти (выходы элементов)
    memset(fbdMemoryBuf, 0, sizeof(tSignal)*fbdElementsCount);
    // инициализация памяти (установка всех флагов изменения значения)
    fbdChangeAllNetVars();
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
    inputOffsets = (tOffset *)(fbdChangeVarBuf + fbdChangeVarByteCount);
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
    for(i=0;i < fbdElementsCount;i++) {
        *(inputOffsets + i) = curInputOffset;
        *(parameterOffsets + i) = curParameterOffset;
        *(storageOffsets + i) = curStorageOffset;
        //
        elem = fbdDescrBuf[i] & ELEMMASK;
        curInputOffset += FBDdefInputsCount[elem];
        curParameterOffset += FBDdefParametersCount[elem];
        curStorageOffset += FBDdefStorageCount[elem];
        //
#ifdef USE_HMI
        switch(elem) {
            case 22:
                (*(wpOffsets + curWP)).index = i;
                (*(wpOffsets + curWP)).caption = curCap;
                curWP++;
                while(*(curCap++));
                break;
            case 23:
                (*(spOffsets + curSP)).index = i;
                (*(spOffsets + curSP)).caption = curCap;
                curSP++;
                while(*(curCap++));
                break;
        }
#endif // USE_HMI
    }
#endif // SPEED_OPT
    // инициализация входных переменных
    if(needReset) {
        // при загрузке новой схемы значение входных сетевых переменных (до того, как они будут получены) равны 0, это не совсем хорошо...
        // инициализируем их значениями "по умолчанию" из программы
        for(i = 0; i < fbdElementsCount; i++) {
            if(fbdDescrBuf[i] == 16) {
                fbdSetStorage(i, 0, FBDGETPARAMETER(i, 1));
            }
        }
    }
#ifdef USE_HMI
    // инициализация значений точек регулирования (HMI setpoints)
    i = 0;
    while(fbdHMIgetSP(i, &HMIdata)) {
        // если значение точки регулирования не корректное, то устанавливаем значение по умолчанию
        if(needReset||(HMIdata.value > HMIdata.upperLimit)||(HMIdata.value < HMIdata.lowlimit)) fbdHMIsetSP(i, HMIdata.defValue);
        i++;
    }
#endif // USE_HMI
    //
    fbdFirstFlag = 1;
}
// -------------------------------------------------------------------------------------------------------
void fbdDoStep(tSignal period)
{
    tSignal value, param;
    tElemIndex index;
    unsigned char element;
    // сброс признаков расчета и нарастающего фронта
    memset(fbdFlagsBuf, 0, fbdFlagsByteCount);
    // основной цикл расчета
    for(index=0; index < fbdElementsCount; index++) {
        element = fbdDescrBuf[index] & ELEMMASK;
        switch(element) {
            // элементы с таймером
            case 12:                                                        // timer TON
            case 17:                                                        // PID
            case 18:                                                        // SUM
            case 24:                                                        // timer TP
            case 32:                                                        // GEN
                value = FBDGETSTORAGE(index, 0);
                if(value) {
                    value -= period;
                    if(value < 0) value = 0;
                    fbdSetStorage(index, 0, value);
                }
                break;
            case 0:                                                         // output PIN
                fbdCalcElement(index);
                param = FBDGETPARAMETER(index, 0);
                FBDsetProc(0, param, &fbdMemoryBuf[index]);                 // установка значения выходного контакта
                break;
            case 14:                                                        // output VAR
#ifdef USE_HMI
            case 22:                                                        // точка контроля
#endif // USE_HMI
                fbdCalcElement(index);
                break;
        }
    }
    fbdFirstFlag = 0;
}
// -------------------------------------------------------------------------------------------------------
// установить значение переменной, принятое по сети
void fbdSetNetVar(tNetVar *netvar)
{
    tElemIndex i;
    //
    for(i=0; i < fbdElementsCount; i++) {
        if(fbdDescrBuf[i] == 16) {
            if(FBDGETPARAMETER(i, 0) == (*netvar).index) {
                fbdSetStorage(i, 0, (*netvar).value);
                return;
            }
        }
    }
    return;
}
// -------------------------------------------------------------------------------------------------------
// получить значение переменной для отправки по сети
bool fbdGetNetVar(tNetVar *netvar)
{
    tElemIndex i, varindex;
    //
    varindex = 0;
    for(i=0; i < fbdElementsCount; i++) {
        if(fbdDescrBuf[i] == 14) {
            // проверяем установку флага изменений
            if(fbdChangeVarBuf[varindex>>3]&(1u<<(varindex&7))) {
                // флаг установлен, сбрасываем его
                fbdChangeVarBuf[varindex>>3] &= ~(1u<<(varindex&7));
                // номер переменной
                (*netvar).index = FBDGETPARAMETER(i, 0);
                (*netvar).value = fbdMemoryBuf[i];
                return true;
            }
            varindex++;
        }
    }
    return false;
}
// -------------------------------------------------------------------------------------------------------
// установить для всех выходных сетевых переменных признак изменения
void fbdChangeAllNetVars(void)
{
    if(fbdChangeVarByteCount > 0) memset(fbdChangeVarBuf, 255, fbdChangeVarByteCount);
}
//
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
// HMI functions
// -------------------------------------------------------------------------------------------------------
// получить значение точки регулирования
bool fbdHMIgetSP(tSignal index, tHMIdata *pnt)
{
    tElemIndex elemIndex;
    if(index >= fbdSpCount) return false;
#ifdef SPEED_OPT
    elemIndex = (*(spOffsets + index)).index;
    (*pnt).caption = (*(spOffsets + index)).caption;
#else
    if(!fbdGetElementIndex(index, 23, &elemIndex)) return false;
    (*pnt).caption = fbdGetCaption(elemIndex);
#endif // SPEED_OPT
    (*pnt).value = FBDGETSTORAGE(elemIndex, 0);
    (*pnt).lowlimit = FBDGETPARAMETER(elemIndex, 0);
    (*pnt).upperLimit = FBDGETPARAMETER(elemIndex, 1);
    (*pnt).defValue = FBDGETPARAMETER(elemIndex, 2);
    (*pnt).divider = FBDGETPARAMETER(elemIndex, 3);
    (*pnt).step = FBDGETPARAMETER(elemIndex, 4);
    return true;
}
// -------------------------------------------------------------------------------------------------------
// установить значение точки регулирования
void fbdHMIsetSP(tSignal index, tSignal value)
{
    tElemIndex elemIndex;
    if(index >= fbdSpCount) return;
#ifdef SPEED_OPT
    elemIndex = (*(spOffsets + index)).index;
#else
    if(!fbdGetElementIndex(index, 23, &elemIndex)) return;
#endif // SPEED_OPT
    fbdSetStorage(elemIndex, 0, value);
}
// -------------------------------------------------------------------------------------------------------
// получить значение точки контроля
bool fbdHMIgetWP(tSignal index, tHMIdata *pnt)
{
    tElemIndex elemIndex = 0;
    if(index >= fbdWpCount) return false;
#ifdef SPEED_OPT
    elemIndex = (*(wpOffsets + index)).index;
    (*pnt).caption = (*(wpOffsets + index)).caption;
#else
    if(!fbdGetElementIndex(index, 22, &elemIndex)) return false;
    (*pnt).caption = fbdGetCaption(elemIndex);
#endif // SPEED_OPT
    (*pnt).value = fbdMemoryBuf[elemIndex];
    (*pnt).divider = FBDGETPARAMETER(elemIndex, 0);
    return true;
}
// -------------------------------------------------------------------------------------------------------
// получить структуру с описанием проекта
void fbdHMIgetDescription(tHMIdescription *pnt)
{
#ifdef SPEED_OPT
    (*pnt).name = fbdCaptionName?fbdCaptionName:(fbdCaptionName = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount));
    (*pnt).version = fbdCaptionVersion?fbdCaptionVersion:(fbdCaptionVersion = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 1));
    (*pnt).btime = fbdCaptionBTime?fbdCaptionBTime:(fbdCaptionBTime = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 2));
#else
    (*pnt).name = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount);
    (*pnt).version = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 1);
    (*pnt).btime = fbdGetCaptionByIndex(fbdWpCount + fbdSpCount + 2);
#endif
}
// -------------------------------------------------------------------------------------------------------
// получение значений глобальных настроек схемы
tSignal fbdGetGlobalOptions(unsigned char option)
{
    return fbdGlobalOptions[option];
}
// -------------------------------------------------------------------------------------------------------
// расчет указателя на текстовое описание элемента по индексу описания
DESCR_MEM char DESCR_MEM_SUFX * fbdGetCaptionByIndex(tElemIndex captionIndex)
{
    tOffset offset = 0;
    while(captionIndex) if(!fbdCaptionsBuf[offset++]) captionIndex--;
    return &fbdCaptionsBuf[offset];
}
//
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
            case 22:
            case 23:
                captionIndex++;
                break;
        }
    }
    return fbdGetCaptionByIndex(captionIndex);
}
#endif // SPEED_OPT
#endif // USE_HMI
//
// -------------------------------------------------------------------------------------------------------
// расчет выходного значения элемента
void fbdCalcElement(tElemIndex curIndex)
{
    tFBDStackItem FBDStack[FBDSTACKSIZE];       // стек вычислений
    tFBDStackPnt FBDStackPnt;                   // указатель стека
    unsigned char curInput;                     // текущий входной контакт элемента
    unsigned char inputCount;                   // число входов текущего элемента
    tOffset baseInput;                          //
    tElemIndex inpIndex;
    tSignal s1,s2,s3,s4,v;                      // значения сигналов на входе
    //
    if(getCalcFlag(curIndex)) return;           // элемент уже рассчитан?
    //
    FBDStackPnt = 0;
    curInput = 0;
    //
    baseInput = FBDINPUTOFFSET(curIndex);       //
    inputCount = FBDdefInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
    //
    do {
        // если у текущего элемента еще есть входы
        if(curInput < inputCount) {
            // и этот вход еще не расчитан
            inpIndex = ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput + curInput]);
            // проверка: вход уже рассчитан или ко входу подключен выход этого же компонента
            if(getCalcFlag(inpIndex)||(curIndex == inpIndex)) curInput++; else {
                // ставим признак того, что элемент как-бы уже расчитан
                // это нужно в случае, если в схеме есть обратные связи
                setCalcFlag(curIndex);
                // вход еще не рассчитан, запихиваем текущий элемент и номер входа в стек
                FBDStack[FBDStackPnt].index = curIndex;
                FBDStack[FBDStackPnt++].input = curInput;
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
            // вычисляем значение текущего элемента, результат в s1
            switch(fbdDescrBuf[curIndex] & ELEMMASK) {
                case 0:                                                                 // OUTPUT PIN
                case 22:                                                                // HMI watchpoint
                    break;
                case 14:                                                                // OUTPUT VAR
                    // при изменении значения сигнала ставим флаг
                    if(fbdMemoryBuf[curIndex] != s1) {
                        setChangeVarFlag(curIndex);
                    }
                    break;
                case 1:                                                                 // CONST
                    s1 = FBDGETPARAMETER(curIndex, 0);
                    break;
                case 2:                                                                 // NOT
                    s1 = s1?0:1;
                    break;
                case 3:                                                                 // AND
                    s1 = s1 && s2;
                    break;
                case 4:                                                                 // OR
                    s1 = s1 || s2;
                    break;
                case 5:                                                                 // XOR
                    s1 = (s1?1:0)^(s2?1:0);
                    break;
                case 6:                                                                 // RSTRG
                    if(s1||s2) {
                        s1 = s1?0:1;
                        fbdSetStorage(curIndex, 0, s1);
                    } else s1 = FBDGETSTORAGE(curIndex, 0);
                    break;
                case 7:                                                                 // DTRG
                    // смотрим установку флага фронта на входе "С"
                    if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput+1])))
                        fbdSetStorage(curIndex, 0, s1);
                    else
                        s1 = FBDGETSTORAGE(curIndex, 0);
                    break;
                case 8:                                                                 // ADD
                    s1 += s2;
                    break;
                case 9:                                                                 // SUB
                    s1 -= s2;
                    break;
                case 10:                                                                // MUL
                    s1 *= s2;
                    break;
                case 11:                                                                // DIV
                    if(!s2) {
                        if(s1 > 0) s1 = MAX_SIGNAL; else if(s1 < 0) s1 = MIN_SIGNAL; else s1 = 1;
                    } else s1 /= s2;
                    break;
                case 12:                                                                // TIMER TON
                    // s1 - D
                    // s2 - T
                    if(s1) {
                        s1 = (FBDGETSTORAGE(curIndex, 0) == 0);
                    } else {
                        fbdSetStorage(curIndex, 0, s2);
                        s1 = 0;
                    }
                    break;
                case 13:                                                                // CMP
                    s1 = s1 > s2;
                    break;
                case 15:
                    s1 = FBDgetProc(0, FBDGETPARAMETER(curIndex, 0));                   // INPUT PIN
                    break;
                case 16:
                    s1 = FBDGETSTORAGE(curIndex, 0);                                    // INPUT VAR
                    break;
                case 17:                                                                // PID
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
                case 18:                                                                // SUM
                    if(!FBDGETSTORAGE(curIndex, 0)) {       // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s2);     // установка таймера
                        //
                        s1 += fbdMemoryBuf[curIndex];       // сложение с предыдущим значением
                        // ограничение
                        if(s1 > 0) { if(s1 > s3) s1 = s3; } else { if(s1 < -s3) s1 = -s3; }
                    } else s1 = fbdMemoryBuf[curIndex];
                    break;
                case 19:                                                                // Counter
                    if(s3) s1 = 0; else {
                        s1 = FBDGETSTORAGE(curIndex, 0);
                        if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput]))) s1++;
                        if(getRiseFlag(ELEMINDEX_BYTE_ORDER(fbdInputsBuf[baseInput+1]))) s1--;
                    }
                    fbdSetStorage(curIndex, 0, s1);
                    break;
                case 20:                                                                // MUX
                    v &= 3;
                    if(v==1) s1 = s2; else
                    if(v==2) s1 = s3; else
                    if(v==3) s1 = s4;
                    break;
                case 21:                                                                // ABS
                    if(s1 < 0) s1 = -s1;
                    break;
                case 23:                                                                // HMI setpoint
#ifdef USE_HMI
                    s1 = FBDGETSTORAGE(curIndex, 0);
#endif // USE_HMI
                    break;
                case 24:                                                                // TIMER TP
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
                case 25:                                                                // MIN
                    if(s2 < s1) s1 = s2;
                    break;
                case 26:                                                                // MAX
                    if(s2 > s1) s1 = s2;
                    break;
                case 27:                                                                // LIM
                    if(s1 > s2) s1 = s2; else if(s1 < s3) s1 = s3;
                    break;
                case 28:                                                                // EQ
                    s1 = s1 == s2;
                    break;
                case 29:                                                                // побитный AND
                    s1 = s1 & s2;
                    break;
                case 30:                                                                // побитный OR
                    s1 = s1 | s2;
                    break;
                case 31:                                                                // побитный XOR
                    s1 = s1 ^ s2;
                    break;
                case 32:                                                                // генератор
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
        if(FBDStackPnt--) {
            curIndex = FBDStack[FBDStackPnt].index;         // восстанавливаем родительский элемент
            curInput = FBDStack[FBDStackPnt].input + 1;     // в родительском элементе сразу переходим к следующему входу
            baseInput = FBDINPUTOFFSET(curIndex);           // элемент сменился, расчет смещения на первый вход элемента
            inputCount = FBDdefInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
        } else break;                                       // стек пуст, вычисления завершены
    } while(1);
}
// -------------------------------------------------------------------------------------------------------
// сохранить память элемента
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
    if(fbdStorageBuf[offset] != value){
        fbdStorageBuf[offset] = value;
        FBDsetProc(1, offset, &fbdStorageBuf[offset]);
    }
}
// -------------------------------------------------------------------------------------------------------
// установить флаг "Элемент вычислен"
void setCalcFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<((element&3)<<1);
}
// -------------------------------------------------------------------------------------------------------
// установка rising flag
void setRiseFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |= 1u<<(((element&3)<<1)+1);
}
// -------------------------------------------------------------------------------------------------------
// получение флага "Элемент вычислен"
char getCalcFlag(tElemIndex element)
{
   return fbdFlagsBuf[element>>2]&(1u<<((element&3)<<1))?1:0;
}
// -------------------------------------------------------------------------------------------------------
// получение флага "rising flag"
char getRiseFlag(tElemIndex element)
{
    return fbdFlagsBuf[element>>2]&(1u<<(((element&3)<<1)+1))?1:0;
}
// -------------------------------------------------------------------------------------------------------
// установка флага изменения переменной
void setChangeVarFlag(tElemIndex index)
{
    tElemIndex varIndex = 0;
    // определяем номер флага
    while(index--) {
        if((fbdDescrBuf[index] & ELEMMASK) == 14) varIndex++;
    }
    fbdChangeVarBuf[varIndex>>3] |= 1u<<(varIndex&7);
}
// -------------------------------------------------------------------------------------------------------
// расчет абсолютного значения сигнала
inline tSignal intAbs(tSignal val)
{
    return (val>=0)?val:-val;
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
