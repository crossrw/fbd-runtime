#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include "string.h"
#include "fbdrt.h"

// -----------------------------------------------------------------------------
// Функции FBDgetProc() и FBDsetProc() должны быть реализованы в основной программе
// -----------------------------------------------------------------------------
// FBDgetProc(): чтение входного сигнала, переменной или EEPROM
// type - тип читаемого сигнала
//  0 - входной сигнал контроллера
//  1 - входная переменная контроллера
//  2 - содержимое EEPROM
// index - номер (адрес) читаемого сигнала
// результат функции - значение прочитанного сигнала
extern tSignal FBDgetProc(char type, tSignal index);

// FBDsetProc() запись выходного сигнала, переменной или EEPROM
// type - тип читаемого сигнала
//  0 - выходной сигнал контроллера
//  1 - выходная переменная контроллера
//  2 - содержимое EEPROM
// index - номер (адрес) читаемого сигнала
// value - указатель на записываемое значение
extern void FBDsetProc(char type, tSignal index, tSignal *value);
// -----------------------------------------------------------------------------

void fbdCalcElement(tElemIndex index);
tSignal fbdGetParameter(tElemIndex element);
tSignal fbdGetStorage(tElemIndex element, unsigned char index);
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value);

// элемент стека вычислений
typedef struct {
    tElemIndex index;               // индекс элемента
    unsigned char input;            // номер его входа
} tFBDStackItem;

void setCalcFlag(tElemIndex element);
void setRiseFlag(tElemIndex element);

char getCalcFlag(tElemIndex element);
char getRiseFlag(tElemIndex element);

tSignal intAbs(tSignal val);

// ----------------------------------------------------------
// массив описания схемы (может быть в ROM или RAM)
DESCR_MEM unsigned char *fbdDescrBuf;
// формат данных:
//  TypeElement1          <- типы элементов
//  TypeElement2
//  ...
//  TypeElementN
//  -1                    <- признак окончания элементов
// описания входов
DESCR_MEM tElemIndex *fbdInputsBuf;
//  InputOfElement        <- входы элементов
//  InputOfElement
//  ..
// описания параметров
DESCR_MEM tSignal *fbdParametersBuf;
//  ParameterOfElement    <- параметры элементов
//  ParameterOfElement
//  ...
// ----------------------------------------------------------
// массив промежуточных данных (только RAM)
tSignal *fbdMemoryBuf;
// формат данных:
//  OutputValue0
//  OutputValue1
//  ...
//  OutputValueN
// хранимые значения
tSignal *fbdStorageBuf;
//  Storage0
//  Storage1
//  ...
//  StorageN
// флаги расчета и фронта
char *fbdFlagsBuf;
//  Flags0
//  Flags1
//  ...
//  FlagsN

tElemIndex fbdElementsCount;
tElemIndex fbdStorageCount;
tElemIndex fbdFlagsByteCount;
//
char fbdFirstFlag;

// количество входов у элементов
ROM_CONST unsigned char FBDInputsCount[19] =     {1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,4,3};
// количество параметров у элементов
ROM_CONST unsigned char FBDParametersCount[19] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0};
// количество хранимых значений у элементов
ROM_CONST unsigned char FBDStorageCount[19]    = {0,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,0,2,1};

// --------------------------------------------------------------------------------------------

int fbdInit(DESCR_MEM unsigned char *buf)
{
    int inputs = 0;
    int parameters = 0;
    unsigned char elem;
    //
    fbdElementsCount = 0;
    fbdStorageCount = 0;
    //
    if(!buf) return -1;
    fbdDescrBuf = buf;

    while(!((elem=fbdDescrBuf[fbdElementsCount])&0x80)) {
        if(elem > 18u) return -1;
        inputs += FBDInputsCount[elem];
        parameters += FBDParametersCount[elem];
        fbdStorageCount += FBDStorageCount[elem];
        fbdElementsCount++;
    }
    // проверка правильности размеров сигнала и индекса
    if(elem != END_MARK) return -2;
    // расчет указателей
    fbdInputsBuf = (DESCR_MEM tElemIndex *)(fbdDescrBuf+fbdElementsCount+1);
    fbdParametersBuf = (DESCR_MEM tSignal *)(fbdInputsBuf+inputs);
    //
    fbdFlagsByteCount = (fbdElementsCount>>2) + ((fbdElementsCount&0x03)?1:0);
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount;
}

void fbdSetMemory(char *buf)
{
    tElemIndex i;
    fbdMemoryBuf = (tSignal *)buf;
    // расчет указателей в буфере памяти
    fbdStorageBuf = fbdMemoryBuf+fbdElementsCount;
    fbdFlagsBuf = (char *)(fbdStorageBuf+fbdStorageCount);
    // инициализация буфера памяти
    memset(fbdMemoryBuf, 0, sizeof(tSignal)*fbdElementsCount);
    for(i = 0; i < fbdStorageCount; i++) fbdStorageBuf[i] = FBDgetProc(2, i);    // восстанавливаем из EEPROM
    //
    fbdFirstFlag = 1;
}

void fbdDoStep(tSignal period)
{
    tSignal value, param;
    tElemIndex index;
    unsigned char element;
    // сброс признаков расчета и фронта
    memset(fbdFlagsBuf, 0, fbdFlagsByteCount);
    // расчет
    index = 0;
    while(1) {
        element = fbdDescrBuf[index];
        if(element > 127u) break;                                           // схема кончилась

        switch(element) {
            case 12:                                                        // элементы с таймером
            case 17:
            case 18:
                value = fbdGetStorage(index, 0);
                if(value > 0) {
                    value -= period;
                    if(value < 0) value = 0;
                    fbdSetStorage(index, 0, value);
                }
                break;
            case 0:                                                         // выходные элементы
            case 14:
                fbdCalcElement(index);
                param = fbdGetParameter(index);
                FBDsetProc(element?1:0, param, &fbdMemoryBuf[index]);       // устанавливаем значение переменной или контакта
                break;
        }
        index++;
    }
    fbdFirstFlag = 0;
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
// расчет смещения первого входа элемента
unsigned int fbdInputOffset(tElemIndex index)
{
    tElemIndex i = 0;
    unsigned int offset = 0;
    //
    while (i < index) offset += FBDInputsCount[fbdDescrBuf[i++]];
    return offset;
}
// выполнение расчета элемента
void fbdCalcElement(tElemIndex curIndex)
{
    tFBDStackItem FBDStack[FBDSTACKSIZE];       // стек расчета
    tFBDStackPnt FBDStackPnt = 0;               // указатель в стеке
    unsigned char curInput = 0;                 // текущий вход элемента
    unsigned char inputCount;                   // количество входов текущего элемента
    unsigned int baseInput;                     // смещение на первый вход текущего элемента
    tElemIndex inpIndex;
    tSignal s1,s2,s3,s4,v;                      // значения сигналов на входах
    //
    baseInput = fbdInputOffset(curIndex);       // расчет смещения на первый вход элемента
    inputCount = FBDInputsCount[fbdDescrBuf[curIndex]];
    //
    do {
        // если у текущего элемента еще есть входы
        if(curInput < inputCount) {
            // и этот вход еще не расчитан
            inpIndex = fbdInputsBuf[baseInput + curInput];
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
                baseInput = fbdInputOffset(curIndex);       // элемент сменился, расчет смещения на первый вход элемента
                inputCount = FBDInputsCount[fbdDescrBuf[curIndex]];
            }
            continue;       // следующая итерация цикла
        } else {
            // входов больше нет, а те которые есть уже рассчитаны
            // определяем значения входов (если надо)
            switch(inputCount) {
                case 4:
                    s4 = fbdMemoryBuf[fbdInputsBuf[baseInput+3]];
                case 3:
                    s3 = fbdMemoryBuf[fbdInputsBuf[baseInput+2]];
                case 2:
                    s2 = fbdMemoryBuf[fbdInputsBuf[baseInput+1]];
                case 1:
                    s1 = fbdMemoryBuf[fbdInputsBuf[baseInput]];
            }
            // вычисляем значение текущего элемента, результат в s1
            switch(fbdDescrBuf[curIndex]) {
                case 0:                                                                 // OUTPUT PIN
                case 14:                                                                // OUTPUT VAR
                    break;
                case 1:                                                                 // CONST
                    s1 = fbdGetParameter(curIndex);
                    break;
                case 2:                                                                 // NOT
                    s1 = s1?0:1;
                    break;
                case 3:                                                                 // AND
                    s1 = s1 && s2;
                    break;
                case 4:                                                                 // OR
                    s1 = s1||s2;
                    break;
                case 5:                                                                 // XOR
                    s1 = (s1?1:0)^(s2?1:0);
                    break;
                case 6:                                                                 // RSTRG
                    if(s1||s2) {
                        s1 = s1?0:1;
                        fbdSetStorage(curIndex, 0, s1);
                    } else s1 = fbdGetStorage(curIndex, 0);
                    break;
                case 7:                                                                 // DTRG
                    // смотрим установку флага фронта на входе "С"
                    if(getRiseFlag(fbdInputsBuf[baseInput+1]))
                        fbdSetStorage(curIndex, 0, s1);
                    else
                        s1 = fbdGetStorage(curIndex, 0);
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
                        if(s1>0) s1 = MAX_SIGNAL; else if(s1<0) s1 = MIN_SIGNAL; else s1 = 1;
                    } else s1 /= s2;
                    break;
                case 12:                                                                // TIMER
                    if(s1) {
                        s1 = (fbdGetStorage(curIndex, 0) == 0);
                    } else {
                        fbdSetStorage(curIndex, 0, s2);
                        s1 = 0;
                    }
                    break;
                case 13:                                                                // CMP
                    s1 = s1 > s2;
                    break;
                case 15:
                    s1 = FBDgetProc(0, fbdGetParameter(curIndex));                      // INPUT PIN
                    break;
                case 16:
                    s1 = FBDgetProc(1, fbdGetParameter(curIndex));                      // INPUT VAR
                    break;
                case 17:                                                                // PID
                    if(!fbdGetStorage(curIndex, 0)) {           // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s3);         // установка таймера
                        s2 = s1 - s2;                           // ошибка PID
                        if(!fbdFirstFlag) v = ((s1-fbdGetStorage(curIndex, 1)) << 7)/s3; else v = 0;    // скорость изменения входной величины
                        fbdSetStorage(curIndex, 1, s1);                                                 // сохранение прошлого входного значения
                        if((v < intAbs(s2))||(v > intAbs(s2*3))) {
                            s1= -(s4*((s2<<1) + v))>>7;
                        } else s1 = fbdMemoryBuf[curIndex];
                    } else s1 = fbdMemoryBuf[curIndex];
                    break;
                case 18:                                                                // SUM
                    if(!fbdGetStorage(curIndex, 0)) {       // проверка срабатывания таймера
                        fbdSetStorage(curIndex, 0, s2);     // установка таймера
                        //
                        s1 += fbdMemoryBuf[curIndex];       // сложение с предыдущим значением
                        // ограничение
                        if(s1 > 0) { if(s1 > s3) s1 = s3; } else { if(s1 < -s3) s1 = -s3; }
                    } else s1 = fbdMemoryBuf[curIndex];
                    break;
                default:
                    s1 = 0;
            }
            setCalcFlag(curIndex);                                  // элемент расчитан, ставим признак
            if(s1 > fbdMemoryBuf[curIndex]) setRiseFlag(curIndex);  // проверка на нарастающий фронт
            fbdMemoryBuf[curIndex] = s1;                            // сохраняю значение в буфере
        }
        // текущий элемент вычислен, пробуем достать из стека родительский элемент
        if(FBDStackPnt--) {
            curIndex = FBDStack[FBDStackPnt].index;         // восстанавливаем родительский элемент
            curInput = FBDStack[FBDStackPnt].input + 1;     // в родительском элементе сразу переходим к следующему входу
            baseInput = fbdInputOffset(curIndex);           // элемент сменился, расчет смещения на первый вход элемента
            inputCount = FBDInputsCount[fbdDescrBuf[curIndex]];
        } else break;                                       // если стек пуст, то расчет завершен
    } while(1);
}
// получение значения параметра элемента
tSignal fbdGetParameter(tElemIndex element)
{
    tElemIndex elem = 0;
    int offset = 0;
    //
    while (elem < element) offset += FBDParametersCount[fbdDescrBuf[elem++]];
    return fbdParametersBuf[offset];
}
// получение пямяти элемента
tSignal fbdGetStorage(tElemIndex element, unsigned char index)
{
    tElemIndex elem = 0;
    int offset = 0;
    //
    while (elem<element) offset += FBDStorageCount[fbdDescrBuf[elem++]];
    return fbdStorageBuf[offset + index];
}
// сохранение пямяти элемента
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value)
{
    tElemIndex elem = 0;
    int offset = 0;
    //
    while (elem < element) offset += FBDStorageCount[fbdDescrBuf[elem++]];
    offset += index;
    if(fbdStorageBuf[offset]!=value){
        fbdStorageBuf[offset]=value;
        FBDsetProc(2,offset,&fbdStorageBuf[offset]);    // сохраняем в EEPROM
    }
}
// установка признака расчитанного элемента
void setCalcFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |=  1u<<((element&3)<<1);
}
// установка признака нарастающего фронта на выходе элемента
void setRiseFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |=  1u<<(((element&3)<<1)+1);
}
// получение признака расчитанного элемента
char getCalcFlag(tElemIndex element)
{
   return fbdFlagsBuf[element>>2]&(1u<<((element&3)<<1))?1:0;
}
// получение признака нарастающего фронта на выходе элемента
char getRiseFlag(tElemIndex element)
{
    return fbdFlagsBuf[element>>2]&(1u<<(((element&3)<<1)+1))?1:0;
}
// абсолютное значение для типа (tSignal)
tSignal intAbs(tSignal val)
{
    return (val>=0)?val:-val;
}
