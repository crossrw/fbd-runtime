# FBD-runtime

![fbd logo](https://www.mnppsaturn.ru/fbd2/images/fbd2.png)

Библиотека предназначена для выполнения программ на языке FBD (*Язык функциональных диаграмм*) для ПЛК (*Программируемых Логических Контроллеров*).

Подробнее о языке FBD можно прочитать на сайте [Wikipedia](http://en.wikipedia.org/wiki/Function_block_diagram).

FBD - один из языков программирования описанных в международном стандарте IEC 61131-3. Данная реализация не полностью совместима с указанным стандартом, однако большинство его требований выполняется.

Редактор и симулятор выполнения программ FBD вы можете скачать по [ссылке](https://www.mnppsaturn.ru/fbd2/fbd2setup.exe).

Доступно небольшое [видео](http://youtu.be/KEGXHd6FIEI) об использовании редактора схем.

-------------------------------------------------------------------------------------

## Введение

Язык функциональных диаграмм создан для описания функционирования одного или нескольких (соединенных через информационную сеть) логических контроллеров. Язык описывает входящие в состав схемы элементы и соединяющие их цепи.

## Основные понятия

Схема на языке FBD это набор элементов и соединений (цепей) между ними.

### Элемент

Элемент схемы это минимальный функциональный блок, выполняющий определенную операцию (логическую, арифметическую, задержку и т.п.) или обеспечивающий подключение схемы к входным или выходным цепям программируемого контроллера.

* Каждый элемент должен иметь уникальное в рамках схемы идентифицирующее его имя.
* Элемент может иметь некоторое количество входов (а может и не иметь) и один или ни одного выхода. Количество входов и выходов зависит от типа элемента. На графическом изображении элементов входы всегда расположены слева, а выходы справа от элемента. Каждый вход элемента должен быть подключен к цепи, которая соединяет его с выходом другого элемента. Выход элемента может быть подключен к одному или нескольким входам других элементов (в том числе и его собственным входам). Выход элемента может быть никуда не подключен, в этом случае элемент не имеет смысла.
* Элементы, которые не имеют выхода обычно используются для обозначения аппаратных выходов программируемого контроллера, выходных сетевых переменных или точек отображения.
* Элементы, которые не имеют входа обычно используются для обозначения аппаратных входов программируемого контроллера, входных сетевых переменных или точек регулирования.
* Элемент может иметь (а может и не иметь: это зависит от типа элемента) параметры, влияющие на его функционирование. Параметры имеют фиксированное значение и задаются при разработке схемы.

### Цепь

Цепь это логическая связь между входами и выходами элементов предназначенная для передачи состояния (значения) сигнала. Каждая цепь должна быть подключена строго к одному выходу и одному или нескольким входам элементов схемы. Состояние цепи к которой не подключен ни один выход элемента является неопределенным и схема в целом, при этом, является не корректной. Цепь, которая не подключена ни к одному входу элемента является не используемой и ее состояние не вычисляется. Аналогично элементам, цепи имеют уникальные в рамках схемы имена. Цепи служат для передачи значений сигналов между элементами схемы.

### Сигнал

Сигнал представляет собой целое число со знаком. В зависимости от параметров, указанных при сборке библиотеки, сигнал может занимать 1, 2 или 4 байта. Сигналы можно использовать для логических операций. При этом значение сигнала равное 0 интерпретируется как логический "0" (False), а значение сигнала не равное 0 - как логическая "1" (True). Для входов некоторых элементов важно не само значение сигнала, а факт его нарастания (передний фронт). Такие входы на графическом изображении элемента обозначены наклонной чертой. Факт наличия переднего фронта фиксируется при любом изменении сигнала в большую сторону (Si > Si-1). Элементы, выполняющие логические функции, формируют значение сигнала "1" для логической "1" и значение сигнала "0" для логического "0".

### Пример схемы

Пример простой схемы показан на рисунке:

![fbd demo](https://www.mnppsaturn.ru/fbd2/images/fbddemo.png)

## Основные возможности библиотеки

* Нет зависимости от аппаратуры, возможность применения на любой платформе где есть компилятор языка C
* Алгоритм оптимизирован для встроенных контроллеров: PIC, AVR, ARM и т.п. Вычисление не использует рекурсию, стек данных используется очень экономно
* Поддержка архитектур Big-Endian и Litle-Endian
* Экономное использование RAM: схема из 400 элементов использует только около 1 kb ОЗУ
* Широкий диапазон поддерживаемых элементов: логические, арифметические, сравнение, таймеры, триггеры, PID. Набор элементов может быть легко расширен
* Сохранение промежуточных результатов в NVRAM (для триггеров, таймеров, точек регулирования и т.п.)
* Поддержка работы по сети (Ethernet, ModBus или подобное)
* Базовая поддержка интерфейса с оператором (HMI)

![fbd struct](https://www.mnppsaturn.ru/fbd2/images/struct.png)

## Производительность

Один цикл вычисления схемы производится в ходе выполнения функции `fbdDoStep()`. В ходе одного цикла производится расчет значений всех элементов схемы и установка значений всех сетевых переменных и выходных контактов. Время расчета зависит от множества факторов, в первую очередь от количества и типов используемых элементов в схеме. Результаты тестирования времени выполнения небольшой схемы из 10 элементов (https://www.mnppsaturn.ru/fbd2/images/generator.zip) показаны в таблице:
<table>
<tr><td><b>CPU@Freq</b></td><td><b>Компилятор</b></td><td><b>Длительность цикла</b></td><td><b>Расчет одного элемента (среднее)</b></td></tr>
<tr><td>PIC18@9.83x4MHz</td><td>xc8 v1.21</td><td>~5ms</td><td>~500&micro;s</td></tr>
<tr><td>Si8051@48MHz</td><td>Keil C51</td><td>~2.5ms</td><td>~250&micro;s</td></tr>
<tr><td>ARM920T@180MHz</td><td>gcc v4.1.2</td><td>~61&micro;s</td><td>~6.1&micro;s</td></tr>
<tr><td>i7-3770K@3.5GHz</td><td>gcc v4.4.1</td><td>~279ns</td><td>~27.9ns</td></tr>
</table>

## Ограничения

Используемый алгоритм расчета содержит определенные ограничения:

* Элемент не может иметь более одного выхода
* Поддерживается только один тип данных (`tSignal`). Это относится ко всем входам и выходам всех элементов, значениям сигналов цепей, констант. Размерность типа данных определяется при компиляции проекта. Обычно используется тип целое число со знаком.

## Элементы по функциям

### Входы/Выходы/Константы

#### Вход

Элемент имеет один выходной контакт. Входов у элемента нет. Элемент является источником значения сигнала для других подключенных к нему элементов. В зависимости от настроек, элемент может выполнять четыре разные функции:

##### Связь с входным контактом контроллера

```txt
| Pin >----
```

В этом случае у элемента имеется один параметр: номер контакта контроллера. Выходное значение сигнала элемента зависит от номера контакта и особенностей аппаратной реализации контроллера. Обычно используется для получения значений подключенных датчиков температуры, давления, состояния "сухих контактов".

##### Константа

```txt
 | Const >----
```

В этом случае у элемента имеется один параметр: значение константы. Выходное значение сигнала элемента всегда соответствует значению параметра (константе).

##### Точка регулирования

```txt
 | SetPoint >----
```

Точка регулирования необходима для реализации возможности корректировки значений настроек, используемых при вычислении схемы. В отличие от константы, значение выходного сигнала элемента может быть изменено эксплуатирующим персоналом (или наладчиком) контроллера. Изменение значения точки регулирование может быть организовано через средства человеко-машинного интерфейса (HMI) контроллера (клавиатуру, индикатор и т.п.).

##### Входная сетевая переменная

```txt
 | Var >----
```

Библиотека предусматривает возможность подключения контроллера в информационную сеть, которая объединяет несколько однотипных или разнородных контроллеров. Такая сеть может быть построена с использованием интерфейсов Ethernet, RS-485, CAN и т.п. Значение выходного сигнала соответствует значению, полученному от другого контроллера по информационной сети. У элемента есть один параметр: номер сетевой переменной.

#### Выход

Элемент имеет один входной контакт. Выходов у элемента нет. В зависимости от настроек, элемент может выполнять две разные функции:

##### Связь с выходным контактом контроллера

```txt
 ----< Pin |
```

У элемента имеется один параметр: номер выходного контакта контроллера. Интерпретация входного значения зависит от особенностей аппаратной реализации на конкретном контроллере. Обычно используется для управления дискретными и аналоговыми выходами контроллера.

##### Выходная сетевая переменная

```txt
 ----< Var |
```

У элемента имеется один параметр: номер выходной сетевой переменной. Значение поданного на вход элемента сигнала передается по информационной сети и доступно на других контроллерах через элемент "Входная сетевая переменная".

### Логические функции

#### Инверсия

```txt
    +---+
----|   O----
    +---+
```

Элемент выполняет функцию логической инверсии "NOT". Элемент имеет один вход и один выход. Таблица истинности:

<table>
 <tr><td><b>Вход</b></td><td><b>Выходной сигнал</b></td></tr>
 <tr><td>Логический "0"</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>0</td></tr>
</table>

#### Логическое "И"

```txt
    +-----+                   +-----+
----|  &  |----           ----|  &  O----
----|     |               ----|     |
    +-----+                   +-----+
```

Элемент выполняет логическую функцию "И" ("AND"). Элемент имеет два входа и один выход. Элемент имеет настройку инвертирования выходного сигнала.
Таблица истинности:

<table>
 <tr><td><b>Вход 1</b></td><td><b>Вход 2</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Логический "0"</td><td>Любое значение</td><td>0</td><td>1</td></tr>
 <tr><td>Any value</td><td>Логический "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>Логическая "1"</td><td>1</td><td>0</td></tr>
</table>

#### Логическое "ИЛИ"

```txt
    +-----+                   +-----+
----|  1  |----           ----|  1  O----
----|     |               ----|     |
    +-----+                   +-----+
```

Element has two inputs and one output. Truth table:
<table>
 <tr><td><b>Input 1</b></td><td><b>Input 2</b></td><td><b>Output</b></td><td><b>Inverse output</b></td></tr>
 <tr><td>Logical "0"</td><td>Logical "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Logical "1"</td><td>Any value</td><td>1</td><td>0</td></tr>
 <tr><td>Any value</td><td>Logical "1"</td><td>1</td><td>0</td></tr>
</table>

#### Логическое "Исключающее ИЛИ" (XOR)

```txt
    +-----+                   +-----+
----|  =1 |----           ----|  =1 O----
----|     |               ----|     |
    +-----+                   +-----+
```

XOR element has two inputs and one output. Truth table:
<table>
 <tr><td><b>Input 1</b></td><td><b>Input 2</b></td><td><b>Output</b></td><td><b>Inverse output</b></td></tr>
 <tr><td>Logical "0"</td><td>Logical "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Logical "0"</td><td>Logical "1"</td><td>1</td><td>0</td></tr>
 <tr><td>Logical "1"</td><td>Logical "0"</td><td>1</td><td>0</td></tr>
 <tr><td>Logical "1"</td><td>Logical "1"</td><td>0</td><td>1</td></tr>
</table>

### Триггеры

#### RS-триггер

```txt
    +--+----+                +--+----+
----|R |   Q|----        ----|R |  Q O----
    |  |    |                |  |    |
----|S |    |            ----|S |    |
    +--+----+                +--+----+
```

SR-latch has two inputs (S and R) and one output (Q).
While the S and R inputs are both low, feedback maintains the Q output in a constant state. If S (Set) is pulsed high while R (Reset) is held low, then the Q output is forced high, and stays high when S returns to low; similarly, if R is pulsed high while S is held low, then the Q output is forced low, and stays low when R returns to low.
Truth table:
<table>
 <tr><td><b>S</b></td><td><b>R</b></td><td><b>Qnext</b></td><td><b>Action</b></td></tr>
 <tr><td>0</td><td>0</td><td>Q</td><td>Hold state</td></tr>
 <tr><td>0</td><td>1</td><td>0</td><td>Reset</td></tr>
 <tr><td>1</td><td>0</td><td>1</td><td>Set</td></tr>
 <tr><td>1</td><td>1</td><td>?</td><td>Not allowed</td></tr>
</table>
At each change its latch value is stored in NVRAM (function is called `FBDsetProc(2, index, *value)`). At initialization scheme attempts to restore its value (the function is called `FBDgetProc(2, index)`).

#### D-триггер

```txt
    +--+----+                +--+----+
----|D |  Q |----        ----|D |  Q O----
    |  |    |                |  |    |
----/C |    |            ----/C |    |
    +--+----+                +--+----+
```

D flip-flop has two inputs (D and C) and one output (Q).
The D flip-flop captures the value of the D-input at a definite portion of the clock cycle (such as the rising edge of the clock). That captured value becomes the Q output. At other times, the output Q does not change.
Truth table:
<table>
 <tr><td><b>D</b></td><td><b>C</b></td><td><b>Qnext</b></td><td><b>Action</b></td></tr>
 <tr><td>Any value</td><td>Rising edge</td><td>D</td><td>Saving</td></tr>
 <tr><td>Any value</td><td>Non-rising</td><td>Saved D value</td><td>Hold state</td></tr>
</table>
In difference from traditional D flip-flop, input (D) and output (Q) may be any value (not only "0" and "1").
At each change its flip-flop value is stored in NVRAM (function is called `FBDsetProc(2, index, *value)`). At initialization scheme attempts to restore its value (the function is called `FBDgetProc(2, index)`).

#### Счётчик

```txt
    +--+----+
----/+ |   Q|----
----/- |    |
----|R |    |
    +--+----+
```

Element has three inputs and one output. If the input R high logic level, the output Q is reset to 0. If the input R is logic low, the output increases by 1 with a positive edge at the input "+" or decremented by 1 with a positive edge at the input "-". In the absence of a positive edge on the inputs "+" and "-" state of the output Q does not change. See table:
<table>
 <tr><td><b>+</b></td><td><b>-</b></td><td><b>R</b></td><td><b>Qnext</b></td><td><b>Action</b></td></tr>
 <tr><td>Any value</td><td>Any value</td><td>1</td><td>0</td><td>Reset</td></tr>
 <tr><td>Rising edge</td><td>Non-rising</td><td>0</td><td>Q+1</td><td>Increase</td></tr>
 <tr><td>Non-rising</td><td>Rising edge</td><td>0</td><td>Q-1</td><td>Decrease</td></tr>
 <tr><td>Non-rising</td><td>Non-rising</td><td>0</td><td>Q</td><td>Not change</td></tr>
 <tr><td>Rising edge</td><td>Rising edge</td><td>0</td><td>Q</td><td>Not change</td></tr>
</table>

### Арифметические функции

#### Сложение

```txt
    +-----+
----|  +  |----
----|     |
    +-----+
```

Element has two inputs and one output. Output value is calculated as the arithmetic sum of the values of the input signals.

#### Вычитание

```txt
    +-----+
----|  -  |----
----|     |
    +-----+
```

Element has two inputs and one output. Output value is calculated as the difference between the value of the signal at the first input signal and the value at the second input.

#### Умножение

```txt
    +-----+
----|  *  |----
----|     |
    +-----+
```

Element has two inputs and one output. The output value - the first and second multiplication signal. Overflow values are not controlled.

#### Деление

```txt
    +-----+
----|  /  |----
----|     |
    +-----+
```

Element has two inputs and one output. The output value - the value of the signal at the first input value divided by the signal on the second input. Special cases:
<table>
 <tr><td><b>Input 1</b></td><td><b>Input 2</b></td><td><b>Output</b></td></tr>
 <tr><td>0</td><td>0</td><td>1</td></tr>
 <tr><td>Any positive value</td><td>0</td><td>MAX_SIGNAL</td></tr>
 <tr><td>Any negative value</td><td>0</td><td>MIN_SIGNAL</td></tr>
</table>
I know that you can not divide by 0. :)

#### Абсолютное значение

```txt
    +---+
----|Abs|----
    +---+
```

Output element value is the absolute value of the input signal.

### Регулирование

#### PID

```txt
    +---+-----+
----|U  |    Q|----
----|Ref|     |
----|DT |     |
----|P  |     |
    +---+-----+
```

The calculation can not use the traditional PID algorithm. Instead, use the Euler method (anyway, that called him a good man, who told me about it). I heard somewhere that this algorithm is used for docking spacecraft. This algorithm has only two input parameters, quickly sets the output value and is not prone to fluctuations. Element has 4 inputs and one output. Inputs element are: U - current value controlled process, REF - reference value, DT - reaction time, P - proportionality factor. Features of the implementation can be found in the source code.

#### Интегратор

```txt
    +---+----+
----|X  |   Q|----
----|DT |    |
----|Lim|    |
    +---+----+
```

Element is used to integrate the input signal value. Can be used together with an element of PID. Element has three inputs: X - input value, DT - integrating constant, LIM - limiting the output value. Once a time DT calculated summa input signal value X with the previous value of the element. If the result is more LIM or less (-LIM), the output value will be truncated.

### Таймеры

#### Таймер TON

```txt
    +--+------+                +--+------+
----|D | TON Q|----        ----|D | TON QO----
    |  |      |                |  |      |
----|T |      |            ----|T |      |
    +--+------+                +--+------+
```

Standart on-delay timer. It can be used as a signal generator with a given period or time counter operation. At timer two inputs (D and T) and one output. If the input D is a logic 0, the output is always 0. If the input signal D logic 1 appears, then the output will be 1 in a time whose value is present at input T. Before to the expiration time T is stored at the output signal 0. See table and diagram below:
<table>
 <tr><td><b>D</b></td><td><b>Сondition</b></td><td><b>Output</b></td><td><b>Inverse output</b></td></tr>
 <tr><td>Logical "0"</td><td>Any</td><td>0</td><td>1</td></tr>
 <tr><td>Logical "1"</td><td>Time T not expired</td><td>0</td><td>1</td></tr>
 <tr><td>Logical "1"</td><td>Time T expired</td><td>1</td><td>0</td></tr>
</table>

```txt
                   On-delay (TON) timing

    +------------+       +---+   +------------+
D   |            |       |   |   |            |
  --+            +-------+   +---+            +--------------
    t0           t1      t2  t3  t4           t5
            +----+                       +----+
Q           |    |                       |    |
  ----------+    +-----------------------+    +--------------
          t0+T   t1                    t4+T   t5
_ ----------+    +-----------------------+    +--------------
Q           |    |                       |    |
            +----+                       +----+
          t0+T   t1                    t4+T   t5
```

#### Таймер TP

```txt
    +--+------+                +--+------+
----/D |  TP Q|----        ----/D |  TP QO----
    |  |      |                |  |      |
----|T |      |            ----|T |      |
    +--+------+                +--+------+
```

At timer two inputs (D and T) and one output. A rising edge starts the timer and sets the output signal 1. Output signal 1 remains until the timer expires. See diagram below:

```txt
                   Pulse (TP) timing

    +------------+       +-+ +-+     +------------+
D   |            |       | | | |     |            |
  --+            +-------+ +-+ +-----+            +-----
    t0           t1      t2  t3      t4           t5
    +-------+            +-------+   +-------+
Q   |       |            |       |   |       |
  --+       +------------+       +---+       +----------
    t0    t0+T           t2    t2+T  t4    t4+T

_ --+       +------------+       +---+       +----------
Q   |       |            |       |   |       |
    +-------+            +-------+   +-------+
    t0    t0+T           t2    t2+T  t4    t4+T
```

### Остальные элементы

#### Сравнение

```txt
    +-----+                  +-----+
----|  >  |----          ----|  >  O----
----|     |              ----|     |
    +-----+                  +-----+
```

Element compares the signal at its two inputs. Truth table:
<table>
 <tr><td><b>Сondition</b></td><td><b>Output</b></td><td><b>Inverse output</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>1</td><td>0</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>0</td><td>1</td></tr>
</table>

#### Вычисление максимума

```txt
    +-----+
----| MAX |----
----|     |
    +-----+
```

Element has two inputs and one output. Element selects the maximum value of the signal. Truth table:
<table>
 <tr><td><b>Сondition</b></td><td><b>Output</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>Input1Val</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>Input2Val</td></tr>
</table>

#### Вычисление минимума

```txt
    +-----+
----| MIN |----
----|     |
    +-----+
```

Element has two inputs and one output. Element selects the minimum value of the signal. Truth table:
<table>
 <tr><td><b>Сondition</b></td><td><b>Output</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>Input2Val</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>Input1Val</td></tr>
</table>

#### Мультиплексор

```txt
    +---+----+
----|D0 |   Q|----
----|D1 |    |
----|D2 |    |
----|D3 |    |
----|A  |    |
    +---+----+
```

Extensible multiplexer. Element has five inputs (D0-D3, A) and one output. Select one of 4 inputs depending on input A. Truth table:
<table>
 <tr><td><b>A</b></td><td><b>Output Q</b></td></tr>
 <tr><td>0</td><td>D0</td></tr>
 <tr><td>1</td><td>D1</td></tr>
 <tr><td>2</td><td>D2</td></tr>
 <tr><td>3</td><td>D3</td></tr>
</table>

### Установка библиотеки

Setting is done by editing `fbdrt.h` in the following sequence.

1. Select the data type used to store the signal.
2. Select the data type used to store the index of the item.
3. If necessary, change the definition `ROM_CONST` and `DESCR_MEM`.
4. Сhoose to use or not to use optimization execution speed (defines `SPEED_OPT`).
5. Сhoose to use or not to use HMI functions (defines `USE_HMI`).
6. Write your implementation functions `FBDgetProc()` and `FBDsetProc()`.

On the choice of the type of data signal depends with what signals can work your scheme. In addition, it affects memory usage and speed of calculation. For embedded microcontrollers that can be important. In general, the data type must describe signed integer. If you are not sure what to choose, use a 2-byte integer. In addition, you must specify the maximum and minimum value of the signal. For example, you can use the definition from limits.h. For example:

```c
// data type for FBD signal
typedef int16_t tSignal;
#define MAX_SIGNAL INT16_MAX
#define MIN_SIGNAL INT16_MIN
```

The data type of the element index affects how many elements can be in the scheme. In the general case should describe the type of unsigned integer. Type `unsigned char` will create a 255 elements, type `unsigned short` - 65535 elements. This is usually more than enough. Selected type affects the size of the memory used. Example:

```c
// data type for element index
typedef unsigned char tElemIndex;
```

Definition `ROM_CONST` and `DESCR_MEM` describe specifiers that are used to allocate memory for an array of constants and circuit description. Their values depend on the compiler and the location of arrays. For example, when using compiler xc8 (Microchip) used to place data in a FLASH must specifiers `const`:

```c
// data in ROM/FLASH
#define ROM_CONST const
// schema description
#define DESCR_MEM const
```

Inclusion of a definition `SPEED_OPT` reduces the calculation time is approximately 6 times (for medium and large schemes), the memory requirement increases by about 3 times. When you enable `SPEED_OPT`, at initialization time performed preliminary calculations pointers, that reduce runtime.

Disabling definition `USE_HMI` allow slightly reduce the size of the library code. This can be useful if your PLC is not equipped with an LCD indicator. Be careful: if you disable, setpoints can return uncertain data ! Example:

```c
// needed if you use HMI functions
#define USE_HMI
// speed optimization reduces the calculation time, but increases the size of memory (RAM) required
#define SPEED_OPT
```

Functions `FBDgetProc()` and `FBDsetProc()` provide interaction between your circuit with real hardware PLC. Function `FBDgetProc()` used for reading input signals (pin) or stored NVRAM (nonvoltage memory) values. Function `FBDsetProc()` used for writing output signals (pin) or NVRAM values. Their implementation depends on the specific task. Encouraged to adhere to the following rules:

* For discrete inputs and outputs use the values `0` and `1`.
* For analogue inputs and outputs use the values expressed in engineering units, possibly with some decimal factor. For this, in some cases, the conversion function should perform PLC raw input data to engineering units and vice versa. For example the value of temperature sensor +10.5 C must be converted to a number 105.
* Do not use a direct entry in the EEPROM because Library calls `FBDsetProc()` each time you change the value of any trigger or timer. Direct writing to EEPROM can reduce its life span. One solution is to use a delayed write or use of RAM with battery-powered.

In the absence of part of the PLC EEPROM or network access these functions can not be implemented.
Example empty (only for debug) implementation of read and write functions:

```c
tSignal FBDgetProc(char type, tSignal index)
{
    switch(type) {
    case 0:
        printf(" request InputPin(%d)\n", index);
        return 0;
    case 1:
        printf(" request NVRAM(%d)\n", index);
        return 0;
    }
}

void FBDsetProc(char type, tSignal index, tSignal *value)
{
    switch(type)
    {
    case 0:
        printf(" set OutputPin(%d) to value %d\n", index, *value);
        break;
    case 1:
        printf(" set NVRAM(%d) to value %d\n", index, *value);
        break;
    }
}
```

### Формат описания схемы

#### Внутреннее устройство

Description of the scheme is an array of data that may be created by the editor or other means. Dataset should be placed in the memory controller, such as a FLASH or RAM. A pointer to the array is passed to a function `fbdInit(DESCR_MEM unsigned char *buf)`. The array consists of three parts:

  1. List of elements (ending flag `END_MARK`)
  2. Description of connecting the input pins elements
  3. Values of elements parameters

The format of the array shown in the table below:
<table>
 <tr><td><b>Offset</b></td><td><b>Size</b></td><td><b>Value</b></td><td><b>Desciption</b></td></tr>
 <tr><td>0</td><td>1</td><td>type of element 1</td><td>description of element 1</td></tr>
 <tr><td>1</td><td>1</td><td>type of element 2</td><td>description of element 2</td></tr>
 <tr><td></td><td>1</td><td>...</td><td>...</td></tr>
 <tr><td>N-1</td><td>1</td><td>type of element N</td><td>description of element N</td></tr>
 <tr><td>N</td><td>1</td><td>END_MARK</td><td>terminator elements descriptions</td></tr>
 <tr><td>N+1</td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input 1 element 1</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input K(1) element 1</td></tr>
 <tr><td></td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input 1 element 2</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input K(2) element 2</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input 1 element N</td></tr>
 <tr><td></td><td>sizeof(tElemIndex)</td><td>element index</td><td>index of the element connected to the input K(N) element N</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter 1 of the element 1</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter 2 of the element 1</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter M(1) of the element 1</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter 1 of the element N</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter 2 of the element N</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
 <tr><td></td><td>sizeof(tSignal)</td><td>parameter value</td><td>parameter M(N) of the element N</td></tr>
 <tr><td></td><td>variable</td><td>1..255</td><td>chars of caption 1</td></tr>
 <tr><td></td><td>1</td><td>0</td><td>end of caption 1</td></tr>
 <tr><td></td><td>variable</td><td>1..255</td><td>chars of caption 2</td></tr>
 <tr><td></td><td>1</td><td>0</td><td>end of caption 2</td></tr>
 <tr><td></td><td>...</td><td>...</td><td>...</td></tr>
</table>
Where: _N_-elements count, _K(i)_-number of inputs of an element __i__, _M(i)_-number of parameters of an element __i__.

`END_MARK` is a sign of the end of the list of elements, in addition, it contains information about the size and value of the signal element index. Its value is defined in the file `fbdrt.h` as:

```c
#define END_MARK (unsigned char)((sizeof(tSignal)|(sizeof(tElemIndex)<<3))|0x80)
// bit 0-2: sizeof(tSignal)
// bit 3-4: sizeof(tElemIndex)
// bit 5:   reserved
// bit 6:   reserved
// bit 7:   1
```

Function `fbdInit()` checks the value of the `END_MARK` before starting the scheme.

Number of inputs and parameters count depend on the element type code. If the element has no inputs or parameters, the values in the array are missing. Code can be written as a bit mask:

```c
// bit 0-5: code of element type
// bit 6:   flag of inverse output
// bit 7:   0
```

Summary table of types of elements below:
<table>
 <tr><td><b>Element</b></td><td><b>Code</b></td><td><b>Code for inverted output </b></td><td><b>Inputs</b></td><td><b>Parameters</b></td></tr>
 <tr><td>Output pin</td><td>0</td><td>-</td><td>1</td><td>1</td></tr>
 <tr><td>Const</td><td>1</td><td>-</td><td>0</td><td>1</td></tr>
 <tr><td>Logical NOT</td><td>2</td><td>-</td><td>1</td><td>0</td></tr>
 <tr><td>Logical AND</td><td>3</td><td>67</td><td>2</td><td>0</td></tr>
 <tr><td>Logical OR</td><td>4</td><td>68</td><td>2</td><td>0</td></tr>
 <tr><td>Logical XOR</td><td>5</td><td>69</td><td>2</td><td>0</td></tr>
 <tr><td>SR latch</td><td>6</td><td>70</td><td>2</td><td>0</td></tr>
 <tr><td>D flip-flop</td><td>7</td><td>71</td><td>2</td><td>0</td></tr>
 <tr><td>Arithmetical ADD</td><td>8</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Arithmetical SUB</td><td>9</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Arithmetical MUL</td><td>10</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Arithmetical DIV</td><td>11</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Timer TON</td><td>12</td><td>76</td><td>2</td><td>0</td></tr>
 <tr><td>Comparator</td><td>13</td><td>77</td><td>2</td><td>0</td></tr>
 <tr><td>Output var</td><td>14</td><td>-</td><td>1</td><td>1</td></tr>
 <tr><td>Input pin</td><td>15</td><td>-</td><td>0</td><td>1</td></tr>
 <tr><td>Input var</td><td>16</td><td>-</td><td>0</td><td>1</td></tr>
 <tr><td>PID regulator</td><td>17</td><td>-</td><td>4</td><td>0</td></tr>
 <tr><td>Integrator</td><td>18</td><td>-</td><td>3</td><td>0</td></tr>
 <tr><td>Up-down counter</td><td>19</td><td>-</td><td>3</td><td>0</td></tr>
 <tr><td>Multiplexer</td><td>20</td><td>-</td><td>5</td><td>0</td></tr>
 <tr><td>Abs value</td><td>21</td><td>-</td><td>1</td><td>0</td></tr>
 <tr><td>WatchPoint</td><td>22</td><td>-</td><td>1</td><td>0</td></tr>
 <tr><td>SetPoint</td><td>23</td><td>-</td><td>1</td><td>2</td></tr>
 <tr><td>Timer TP</td><td>24</td><td>88</td><td>2</td><td>0</td></tr>
 <tr><td>Min</td><td>25</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Max</td><td>26</td><td>-</td><td>2</td><td>0</td></tr>
 <tr><td>Limit</td><td>27</td><td>-</td><td>3</td><td>0</td></tr>
 <tr><td>Equal</td><td>28</td><td>92</td><td>2</td><td>0</td></tr>
</table>

#### Example

For example, choose a small circuit consisting of a constant element (SRC1), logic inverter (NOT1) and the output terminal (OUT1), see the picture below:

![fbd example](https://www.mnppsaturn.ru/fbd2/images/demo2.png)

Description of the scheme will be as follows (for the case when the `tSignal` is defined as `short`, `tElemIndex` - `unsigned char`):

```c
{0x02, 0x01, 0x00, 0x8A, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}
```

Bytes description:
<table>
 <tr><td><b>Offset</b></td><td><b>Value</b></td><td><b>Description</b></td></tr>
 <tr><td>0</td><td>0x02</td><td>Element logical NOT</td></tr>
 <tr><td>1</td><td>0x01</td><td>Element Const</td></tr>
 <tr><td>2</td><td>0x00</td><td>Element Output pin</td></tr>
 <tr><td>3</td><td>0x8A</td><td>END_MARK</td></tr>
 <tr><td>4</td><td>0x01</td><td>Input 1 of element index 0 (NOT) connected to output element index 1 (Const)</td></tr>
 <tr><td>5</td><td>0x00</td><td>Input 1 of element index 2 (Output) connected to output element index 0 (NOT)</td></tr>
 <tr><td>6</td><td>0x0000</td><td>Parameter of element index 1 (Const). Value of constant is 0</td></tr>
 <tr><td>8</td><td>0x0000</td><td>Parameter of element index 1 (Output). Number of output pin is 0</td></tr>
</table>
Execution
=========

![fbd alg](https://www.mnppsaturn.ru/fbd2/images/alg.png)

### Initialization

Initialization is performed once at the beginning. Initialization may be performed again in the case of change circuit or a PLC reset. To initialize you must first call the function:

```c
int fbdInit(DESCR_MEM unsigned char *descr)
```

__descr__ - pointer to array of scheme description.
A negative _result_ - error:
__-1__ - invalid element code in description;
__-2__ - wrong value of END_MARK flag.
Positive _result_ - the amount of RAM needed to run the scheme.

Then, if there are no errors and enough RAM, you need to call:

```c
void fbdSetMemory(char *buf)
```

__buf__ - pointer to RAM buffer. The function does not return a result. After this call, the scheme is ready.

### Step by step execution

To calculate the circuit must periodically (eg in the main program loop) function call:

```c
void fbdDoStep(tSignal period)
```

__period__ - time elapsed from the previous call.
Each call results in the calculation of all the items and setting values of all output variables and pins.

### Timing

Some elements of the schemes use time. For each such element library organizes independent timer value is stored in RAM. Timers used to store the type `tSignal`.  Timer values changes by `period` every time the function `fbdDoStep(tSignal period)` is called. Time can be expressed in any units: sec, ms, &micro;s. Unit selection depends on the time scale in which the PLC should work. In my opinion, a decent selection of milliseconds for most cases.

## Network variables

## Human machine interface (HMI)

The library contains basic functions support HMI. This can be used if your PLC has a display (LCD). You can make character or graphical menu with which to perform the following functions:

* View values of schema points (`Watchpoints`)
* Set the values of set points (`Setpoints`)

![fbd menu](https://www.mnppsaturn.ru/fbd2/images/menu2.png)

Watchpoints is used to display the values of the signals for operator. Setpoints are used to set the reference values, used in the operation of the circuit. Each watchpoint and setpoint has the associated text caption, that can be displayed on LCD. Moreover, for setpoints are stored maximum and minimum values of the signal that it can receive.

![fbd menu2](https://https://www.mnppsaturn.ru/fbd2/images/menu3.png)

To get the value of the watchpoint, use the function:

```c
bool fbdHMIGetWP(tSignal index, tHMIdata *pnt);
// index - number of watchpoint (0..watch points count-1)
// pnt - pointer to data struct:
typedef struct {
    tSignal value;              // current point value
    tSignal lowlimit;           // low limit for value (only for setpoints)
    tSignal upperLimit;         // upper limit for value (only for setpoints)
    DESCR_MEM char *caption;    // pointer to text caption (asciiz)
} tHMIdata;
```

If the point exists, the function returns `true` and the data in the structure of `pnt`, otherwise the function returns a value `false`. Items `lowlimit` and `upperlimit` not used.
To get the value of the setpoint, use the function:

```c
bool fbdHMIGetSP(tSignal index, tHMIdata *pnt);
// index - number of setpoint (0..set points count-1)
// pnt - pointer to data structure
```

If the point exists, the function returns `true` and the data in the structure of `pnt`, otherwise the function returns a value `false`. Watchpoints and setpoints numbering starts with 0.
To set the new setpoint value, use the function:

```c
void fbdHMISetSP(tSignal index, tSignal value);
// index - number of setpoint (0..set points count-1)
// value - new value of setpoint
```

## Current status

It's works. Now the library is used in the finished projects and does not contain know the problems. Plans for fbdrt development:

* speed optimization

Plans for editor development:

* interface translation in other languages (only Russian now)
* write documentation
* ability to add markup (comments, etc.)
* add the ability to create user libraries

## Author

Alexey Lutovinin -- crossrw1@gmail.com
