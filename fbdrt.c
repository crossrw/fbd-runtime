#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include "string.h"
#include "fbdrt.h"

// -----------------------------------------------------------------------------
// FBDgetProc() and FBDsetProc() - callback, must be present in main program
// -----------------------------------------------------------------------------
// FBDgetProc(): reading input signal, network variable or eeprom
// type - type of reading
//  0 - input signal of MCU
//  1 - input variable
//  2 - EEPROM value
// index - number (index) of reading signal
// result - signal value
extern tSignal FBDgetProc(char type, tSignal index);

// FBDsetProc() writing output signal, variable or eeprom
// type - type of writing
//  0 - output signal of MCU
//  1 - output variable
//  2 - write to EEPROM
// index - number (index) of writing signal
// value - pointer to writing value
extern void FBDsetProc(char type, tSignal index, tSignal *value);
// -----------------------------------------------------------------------------

void fbdCalcElement(tElemIndex index);
tSignal fbdGetParameter(tElemIndex element);
tSignal fbdGetStorage(tElemIndex element, unsigned char index);
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value);

// stack calculating item
typedef struct {
    tElemIndex index;               // index of element
    unsigned char input;            // input number
} tFBDStackItem;

void setCalcFlag(tElemIndex element);
void setRiseFlag(tElemIndex element);

char getCalcFlag(tElemIndex element);
char getRiseFlag(tElemIndex element);

tSignal intAbs(tSignal val);

// ----------------------------------------------------------
// scheme description array (at ROM or RAM)
DESCR_MEM unsigned char *fbdDescrBuf;
// data format:
//  TypeElement1          <- elements types
//  TypeElement2
//  ...
//  TypeElementN
//  -1                    <- end flag
// elements input descriptions
DESCR_MEM tElemIndex *fbdInputsBuf;
//  InputOfElement        <- elements inputs
//  InputOfElement
//  ..
// parameters description
DESCR_MEM tSignal *fbdParametersBuf;
//  ParameterOfElement    <- parameter of element
//  ParameterOfElement
//  ...
// ----------------------------------------------------------
// calculating data array (only RAM)
tSignal *fbdMemoryBuf;
// data format:
//  OutputValue0
//  OutputValue1
//  ...
//  OutputValueN
// saving values
tSignal *fbdStorageBuf;
//  Storage0
//  Storage1
//  ...
//  StorageN
// flags (calculated and signal rising)
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

#define ELEMMASK 0x3F
#define INVERTFLAG 0x40

#define MAXELEMTYPEVAL 21u

// inputs element count
ROM_CONST unsigned char FBDInputsCount[MAXELEMTYPEVAL+1] =     {1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,4,3,3,5,1};
// parameters element count
ROM_CONST unsigned char FBDParametersCount[MAXELEMTYPEVAL+1] = {1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0};
// saved values count
ROM_CONST unsigned char FBDStorageCount[MAXELEMTYPEVAL+1]    = {0,0,0,0,0,0,1,1,0,0,0,0,1,0,0,0,0,2,1,1,0,0};

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

    while(1) {
        elem = fbdDescrBuf[fbdElementsCount];
        if(elem & 0x80) break;
        elem &= ELEMMASK;
        if(elem > MAXELEMTYPEVAL) return -1;
        inputs += FBDInputsCount[elem];
        parameters += FBDParametersCount[elem];
        fbdStorageCount += FBDStorageCount[elem];
        fbdElementsCount++;
    }
    // check tSignal size
    if(elem != END_MARK) return -2;
    // calc pointers
    fbdInputsBuf = (DESCR_MEM tElemIndex *)(fbdDescrBuf + fbdElementsCount + 1);
    fbdParametersBuf = (DESCR_MEM tSignal *)(fbdInputsBuf + inputs);
    //
    fbdFlagsByteCount = (fbdElementsCount>>2) + ((fbdElementsCount&0x03)?1:0);
    return (fbdElementsCount + fbdStorageCount)*sizeof(tSignal) + fbdFlagsByteCount;
}

void fbdSetMemory(char *buf)
{
    tElemIndex i;
    fbdMemoryBuf = (tSignal *)buf;
    // init memory pointers
    fbdStorageBuf = fbdMemoryBuf + fbdElementsCount;
    fbdFlagsBuf = (char *)(fbdStorageBuf + fbdStorageCount);
    // init memory buf
    memset(fbdMemoryBuf, 0, sizeof(tSignal)*fbdElementsCount);
    // restore triggers values from EEPROM
    for(i = 0; i < fbdStorageCount; i++) fbdStorageBuf[i] = FBDgetProc(2, i);
    //
    fbdFirstFlag = 1;
}

void fbdDoStep(tSignal period)
{
    tSignal value, param;
    tElemIndex index;
    unsigned char element;
    // reset calculating and rising flags
    memset(fbdFlagsBuf, 0, fbdFlagsByteCount);
    // main calculating loop
    index = 0;
    while(1) {
        element = fbdDescrBuf[index];
        if(element & 0x80) break;                                           // end of schema

        switch(element & ELEMMASK) {
            case 12:                                                        // elements with timer
            case 17:
            case 18:
                value = fbdGetStorage(index, 0);
                if(value > 0) {
                    value -= period;
                    if(value < 0) value = 0;
                    fbdSetStorage(index, 0, value);
                }
                break;
            case 0:                                                         // output elements
            case 14:
                fbdCalcElement(index);
                param = fbdGetParameter(index);
                FBDsetProc(element?1:0, param, &fbdMemoryBuf[index]);       // set variable or output pin value
                break;
        }
        index++;
    }
    fbdFirstFlag = 0;
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------
// calc offset of first element input
unsigned int fbdInputOffset(tElemIndex index)
{
    tElemIndex i = 0;
    unsigned int offset = 0;
    //
    while (i < index) offset += FBDInputsCount[fbdDescrBuf[i++] & ELEMMASK];
    return offset;
}
// calculating element output value
void fbdCalcElement(tElemIndex curIndex)
{
    tFBDStackItem FBDStack[FBDSTACKSIZE];       // stack to calculate
    tFBDStackPnt FBDStackPnt = 0;               // stack pointer
    unsigned char curInput = 0;                 // current input of element
    unsigned char inputCount;                   // number of inputs of the current element
    unsigned int baseInput;                     //
    tElemIndex inpIndex;
    tSignal s1,s2,s3,s4,v;                      // inputs values
    //
    baseInput = fbdInputOffset(curIndex);       //
    inputCount = FBDInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
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
                inputCount = FBDInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
            }
            continue;       // следующая итерация цикла
        } else {
            // входов больше нет, а те которые есть уже рассчитаны
            // определяем значения входов (если надо)
            switch(inputCount) {
                case 5:
                    v = fbdMemoryBuf[fbdInputsBuf[baseInput + 4]];
                case 4:
                    s4 = fbdMemoryBuf[fbdInputsBuf[baseInput + 3]];
                case 3:
                    s3 = fbdMemoryBuf[fbdInputsBuf[baseInput + 2]];
                case 2:
                    s2 = fbdMemoryBuf[fbdInputsBuf[baseInput + 1]];
                case 1:
                    s1 = fbdMemoryBuf[fbdInputsBuf[baseInput]];
            }
            // вычисляем значение текущего элемента, результат в s1
            switch(fbdDescrBuf[curIndex] & ELEMMASK) {
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
                        if(!fbdFirstFlag) v = ((long)(s1 - fbdGetStorage(curIndex, 1)) * 128)/s3; else v = 0;    // скорость изменения входной величины
                        fbdSetStorage(curIndex, 1, s1);                                                          // сохранение прошлого входного значения
                        if((v < intAbs(s2))||(v > intAbs(s2*3))) {
                            s1= -(long)s4*(s2*2 + v) / 128;
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
                case 19:                                                                // Counter
                    if(s3) s1 = 0; else {
                        s1 = fbdGetStorage(curIndex, 0);
                        if(getRiseFlag(fbdInputsBuf[baseInput])) s1++;
                        if(getRiseFlag(fbdInputsBuf[baseInput+1])) s1--;
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
                    if(s1<0) s1 = -s1;
                    break;
                default:
                    s1 = 0;
            }
            setCalcFlag(curIndex);                                  // set calculated flag
            if(fbdDescrBuf[curIndex] & INVERTFLAG) s1 = s1?0:1;     // inverse result if need
            if(s1 > fbdMemoryBuf[curIndex]) setRiseFlag(curIndex);  // проверка на нарастающий фронт
            fbdMemoryBuf[curIndex] = s1;                            // сохраняю значение в буфере
        }
        // текущий элемент вычислен, пробуем достать из стека родительский элемент
        if(FBDStackPnt--) {
            curIndex = FBDStack[FBDStackPnt].index;         // восстанавливаем родительский элемент
            curInput = FBDStack[FBDStackPnt].input + 1;     // в родительском элементе сразу переходим к следующему входу
            baseInput = fbdInputOffset(curIndex);           // элемент сменился, расчет смещения на первый вход элемента
            inputCount = FBDInputsCount[fbdDescrBuf[curIndex] & ELEMMASK];
        } else break;                                       // если стек пуст, то расчет завершен
    } while(1);
}
// get value of element parameter
tSignal fbdGetParameter(tElemIndex element)
{
    tElemIndex elem = 0;
    unsigned int offset = 0;
    //
    while (elem < element) offset += FBDParametersCount[fbdDescrBuf[elem++] & ELEMMASK];
    return fbdParametersBuf[offset];
}
// get value of elemnt memory
tSignal fbdGetStorage(tElemIndex element, unsigned char index)
{
    tElemIndex elem = 0;
    unsigned int offset = 0;
    //
    while (elem<element) offset += FBDStorageCount[fbdDescrBuf[elem++] & ELEMMASK];
    return fbdStorageBuf[offset + index];
}
// save element memory
void fbdSetStorage(tElemIndex element, unsigned char index, tSignal value)
{
    tElemIndex elem = 0;
    unsigned int offset = 0;
    //
    while (elem < element) offset += FBDStorageCount[fbdDescrBuf[elem++] & ELEMMASK];
    offset += index;
    if(fbdStorageBuf[offset]!=value){
        fbdStorageBuf[offset]=value;
        FBDsetProc(2,offset,&fbdStorageBuf[offset]);    // save to eeprom
    }
}
// set element calculated flag
void setCalcFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |=  1u<<((element&3)<<1);
}
// set signal rising flag
void setRiseFlag(tElemIndex element)
{
    fbdFlagsBuf[element>>2] |=  1u<<(((element&3)<<1)+1);
}
// get element calculated flag
char getCalcFlag(tElemIndex element)
{
   return fbdFlagsBuf[element>>2]&(1u<<((element&3)<<1))?1:0;
}
// get signal rising flag
char getRiseFlag(tElemIndex element)
{
    return fbdFlagsBuf[element>>2]&(1u<<(((element&3)<<1)+1))?1:0;
}
// abs value for type tSignal
tSignal intAbs(tSignal val)
{
    return (val>=0)?val:-val;
}
