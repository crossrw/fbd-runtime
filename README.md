# FBD-runtime

![fbd logo](https://www.mnppsaturn.ru/fbd2/images/fbd2.png)

Библиотека предназначена для выполнения программ на языке FBD (*Язык функциональных диаграмм*) для ПЛК (*Программируемых Логических Контроллеров*).

Подробнее о языке FBD можно прочитать на сайте [Wikipedia](http://en.wikipedia.org/wiki/Function_block_diagram).

FBD - один из языков программирования описанных в международном стандарте IEC 61131-3. Данная реализация не полностью совместима с указанным стандартом, однако большинство его требований реализовано.

Редактор и симулятор выполнения программ FBD вы можете скачать по [ссылке](https://www.mnppsaturn.ru/fbd2/fbd2setup.exe).

Доступно небольшое [видео](http://youtu.be/KEGXHd6FIEI) об использовании редактора схем.

## Оглавление

* [**Введение**](#a1)
* [**Возможности**](#a2)
* [**Основные понятия**](#a2)
  + [Контроллер](#a2.0)
  + [Схема](#a2.00)
  + [Элемент](#a2.1)
  + [Цепь](#a2.2)
  + [Сигнал](#a2.3)
  + [Точки регулирования и контроля](#a2.4)
  + [Сетевая переменная](#a2.6)
  + [Пример схемы](#a2.7)
* [**Ограничения**](#a3)
* [**Производительность**](#a4)
* [**Описания элементов**](#a5)
  + [Входы/Выходы/Константы/Переменные](#a5.1)
    - [Вход](#a5.1.1)
    - [Связь с входным контактом контроллера](#a5.1.2)
    - [Константа](#a5.1.3)
    - [Точка регулирования](#a5.1.4)
    - [Входная сетевая переменная](#a5.1.5)
    - [Выход](#a5.1.6)
    - [Связь с выходным контактом контроллера](#a5.1.7)
    - [Выходная сетевая переменная](#a5.1.8)
  + [Логические функции](#a5.2)
    - [Логическое "НЕ"](#a5.2.1)
    - [Логическое "И"](#a5.2.2)
    - [Логическое "ИЛИ"](#a5.2.3)
    - [Логическое "Исключающее ИЛИ"](#a5.2.4)
  + [Триггеры/счётчики](#a5.3)
    - [RS - триггер](#a5.3.1)
    - [D - триггер](#a5.3.2)
    - [Счётчик](#a5.3.3)
  + [Арифметические функции](#a5.4)
    - [Сложение](#a5.4.1)
    - [Вычитание](#a5.4.2)
    - [Умножение](#a5.4.3)
    - [Деление](#a5.4.4)
    - [Абсолютное значение](#a5.4.5)
  + [Функции регулирования](#a5.5)
    - [Регулятор](#a5.5.1)
    - [Интегрирование](#a5.5.2)
  + [Таймеры](#a5.6)
    - [Таймер TON](#a5.6.1)
    - [Таймер TP](#a5.6.1)
  + [Остальные элементы](#a5.7)
    - [Функция сравнения](#a5.7.1)
    - [Вычисление максимума](#a5.7.2)
    - [Вычисление минимума](#a5.7.3)
    - [Мультиплексор](#a5.7.4)
    - [Ограничитель](#a5.7.5)
* [**Установка библиотеки**](#a6)
* [**Формат описания схемы**](#a7)
  + [Внутреннее устройство](#a7.1)
  + [Пример](#a7.2)
* [**Выполнение**](#a8)

<a name="a1"></a>

## Введение

Язык функциональных диаграмм создан для описания функционирования одного или нескольких (соединенных через информационную сеть) логических контроллеров. Язык описывает входящие в состав схемы элементы и соединяющие их цепи.

<a name="a2"></a>

## Возможности

* Нет зависимости от аппаратуры, возможность применения на любой платформе где есть компилятор языка C
* Алгоритм оптимизирован для встроенных контроллеров: PIC, AVR, ARM и т.п. Вычисление не использует рекурсию, стек данных используется очень экономно
* Поддержка архитектур Big-Endian и Litle-Endian
* Экономное использование RAM: схема из 400 элементов использует только около 1 kb ОЗУ
* Широкий диапазон поддерживаемых элементов: логические, арифметические, сравнение, таймеры, триггеры, регулятор. Набор элементов может быть легко расширен
* Сохранение промежуточных результатов в NVRAM (для триггеров, таймеров, точек регулирования и т.п.)
* Поддержка работы по сети (Ethernet, ModBus или подобное)
* Базовая поддержка интерфейса с оператором (HMI)

<a name="a2"></a>

## Основные понятия

<a name="a2.0"></a>

### Контроллер

ПЛК – программируемый логический контроллер, представляют собой микропроцессорное устройство, предназначенное для сбора, преобразования, обработки, хранения информации и выработки команд управления, имеющий конечное количество входов и выходов, подключенных к ним датчиков, ключей, исполнительных механизмов к объекту управления, и предназначенный для работы в режимах реального времени с ограниченным вмешательством человека. Наиболее часто ПЛК используются для автоматизации технологических процессов.

Типовая структура ПЛК показана на рисунке:

![fbd struct](https://www.mnppsaturn.ru/fbd2/images/struct.png)

Обычно ПЛК предусматривает выполнение следующих функций:

* Выполнение управляющей программы реализующей алгоритм управления
* Подключение входных сигналов (input pins) внешних датчиков (температура, давление, дискретные входы, напряжение и т.п.)
* Подключение выходных сигналов (output pins) для управления внешним оборудованием (дискретные выходы, сигналы тока или напряжения)
* Подключение к информационной сети (network), объединяющей несколько ПЛК и обеспечивающей их совместное функционирование
* Содержит средства реализации интерфейса с человеком (HMI) с помощью индикатора, дисплея, клавиатуры и т.п.

Рабочий цикл ПЛК включает 4 фазы:

1. Чтения значений входных сигналов
2. Выполнение программы обработки
3. Установку значений выходных сигналов
4. Вспомогательные операции (обмен данными по информационной сети, реализация HMI и т.п.).

<a name="a2.00"></a>

### Схема

Схема на языке FBD это набор элементов, значений их параметров и соединений (цепей) между элементами.

<a name="a2.1"></a>

### Элемент

Элемент схемы это минимальный функциональный блок, выполняющий определенную операцию (логическую, арифметическую, задержку и т.п.) или обеспечивающий подключение схемы к входным или выходным цепям программируемого контроллера.

* Каждый элемент должен иметь уникальное в рамках схемы идентифицирующее его имя.
* Элемент может иметь некоторое количество входов (а может и не иметь) и один или ни одного выхода. Количество входов и выходов зависит от типа элемента. На графическом изображении элементов входы всегда расположены слева, а выходы справа от элемента. Каждый вход элемента должен быть подключен к цепи, которая соединяет его с выходом другого элемента. Выход элемента может быть подключен к одному или нескольким входам других элементов (в том числе и его собственным входам). Выход элемента может быть никуда не подключен, в этом случае элемент не имеет смысла.
* Элементы, которые не имеют выхода обычно используются для обозначения аппаратных выходов программируемого контроллера, выходных сетевых переменных или точек отображения.
* Элементы, которые не имеют входа обычно используются для обозначения аппаратных входов программируемого контроллера, входных сетевых переменных или точек регулирования.
* Элемент может иметь (а может и не иметь: это зависит от типа элемента) параметры, влияющие на его функционирование. Параметры имеют фиксированное значение и задаются при разработке схемы.

<a name="a2.2"></a>

### Цепь

Цепь это логическая связь между входами и выходами элементов предназначенная для передачи состояния (значения) сигнала. Каждая цепь должна быть подключена строго к одному выходу и одному или нескольким входам элементов схемы. Состояние цепи к которой не подключен ни один выход элемента является неопределенным. Цепь, которая не подключена ни к одному входу элемента, является не используемой и ее состояние не вычисляется. Аналогично элементам, цепи имеют уникальные в рамках схемы имена. Цепи служат для передачи значений сигналов между элементами схемы. На графическом изображении схемы цепи изображаются в виде линий, которые соединяют входы и выходы элементов.

<a name="a2.3"></a>

### Сигнал

Сигнал представляет собой целое число со знаком. В зависимости от параметров, указанных при сборке библиотеки, сигнал может занимать 1, 2 или 4 байта. Сигналы можно использовать для логических операций. При этом значение сигнала равное 0 интерпретируется как логический "0" (False), а значение сигнала не равное 0 - как логическая "1" (True). Для входов некоторых элементов важно не само значение сигнала, а факт его нарастания (передний фронт). Такие входы на графическом изображении элемента обозначены наклонной чертой. Факт наличия переднего фронта фиксируется при любом изменении сигнала в большую сторону (Si > Si-1). Элементы, выполняющие логические функции, формируют значение сигнала "1" для логической "1" и значение сигнала "0" для логического "0".

<a name="a2.4"></a>

### Точки регулирования и контроля

Точка регулирования является составной частью организации интерфейса контроллера с оператором (HMI). При помощи точек регулирования оператор может установить значения параметров, используемых при работе схемы.

Точка контроля является составной частью организации интерфейса контроллера с оператором (HMI). При помощи точек контроля оператор может просматривать значения сигналов в различных цепях схемы.

<a name="a2.6"></a>

### Сетевая переменная

Сетевые переменные служат для обмена данными между контроллерами по информационной сети. В схеме могут быть определены входные и выходные сетевые переменные. Каждая сетевая переменная идентифицируется номером. Значение сигнала поданное на выходную сетевую переменную становится доступным через имеющие тот же номер входные сетевые переменные, описанные в схемах на других контроллерах, подключенных к единой информационной сети.

<a name="a2.7"></a>

### Пример схемы

Пример простой схемы показан на рисунке:

![fbd demo](https://www.mnppsaturn.ru/fbd2/images/fbddemo.png)

<a name="a3"></a>

## Ограничения

Используемый алгоритм расчета содержит определенные ограничения:

* Элемент не может иметь более одного выхода
* Поддерживается только один тип данных (`tSignal`). Это относится ко всем входам и выходам всех элементов, значениям сигналов цепей, констант. Размерность типа данных определяется при компиляции проекта. Обычно используется тип целое число со знаком.

<a name="a4"></a>

## Производительность

Один цикл вычисления схемы производится в ходе выполнения функции `fbdDoStep()`. В ходе одного цикла производится расчет значений всех элементов схемы и установка значений всех сетевых переменных и выходных контактов. Время расчета зависит от множества факторов, в первую очередь от количества и типов используемых элементов в схеме. Результаты тестирования времени выполнения небольшой [схемы из 10 элементов](https://www.mnppsaturn.ru/fbd2/images/generator.zip) показаны в таблице:
<table>
<tr><td><b>CPU@Freq</b></td><td><b>Компилятор</b></td><td><b>Длительность цикла</b></td><td><b>Расчет одного элемента (среднее)</b></td></tr>
<tr><td>PIC18@9.83x4MHz</td><td>xc8 v1.21</td><td>~5ms</td><td>~500&micro;s</td></tr>
<tr><td>Si8051@48MHz</td><td>Keil C51</td><td>~2.5ms</td><td>~250&micro;s</td></tr>
<tr><td>ARM920T@180MHz</td><td>gcc v4.1.2</td><td>~61&micro;s</td><td>~6.1&micro;s</td></tr>
<tr><td>i7-3770K@3.5GHz</td><td>gcc v4.4.1</td><td>~279ns</td><td>~27.9ns</td></tr>
</table>

<a name="a5"></a>

## Описания элементов

<a name="a5.1"></a>

### Входы/Выходы/Константы/Переменные

<a name="a5.1.1"></a>

#### Вход

Элемент имеет один выходной контакт. Входов у элемента нет. Элемент является источником значения сигнала для других подключенных к нему элементов. В зависимости от настроек, элемент может выполнять четыре разные функции:

<a name="a5.1.2"></a>

##### Связь с входным контактом контроллера

![inpin](https://www.mnppsaturn.ru/fbd2/images/inpin.png)

В этом случае у элемента имеется один параметр: номер контакта контроллера. Выходное значение сигнала элемента зависит от номера контакта и особенностей аппаратной реализации контроллера. Обычно используется для получения значений подключенных дискретных входов, датчиков температуры, давления, АЦП и т.п. Активное состояние дискретного входа (замкнут, подано напряжение) должно соответствовать значению сигнала "1".

<a name="a5.1.3"></a>

##### Константа

![const](https://www.mnppsaturn.ru/fbd2/images/const.png)

В этом случае у элемента имеется один параметр: значение константы. Выходное значение сигнала элемента всегда соответствует значению параметра (константе).

<a name="a5.1.4"></a>

##### Точка регулирования

![sp](https://www.mnppsaturn.ru/fbd2/images/sp.png)

Точка регулирования необходима для реализации возможности корректировки значений настроек, используемых при вычислении схемы. В отличие от константы, значение выходного сигнала элемента может быть изменено эксплуатирующим персоналом (или наладчиком) контроллера. Изменение значения точки регулирование может быть организовано через средства человеко-машинного интерфейса (HMI) контроллера (клавиатуру, индикатор и т.п.). Точка регулирования имеет 4 параметра: значение по умолчанию, минимальное значение, максимальное значение, текстовая строка с описанием.

<a name="a5.1.5"></a>

##### Входная сетевая переменная

![invar](https://www.mnppsaturn.ru/fbd2/images/invar.png)

Библиотека предусматривает возможность подключения контроллера в информационную сеть, которая объединяет несколько однотипных или разнородных контроллеров. Такая сеть может быть построена с использованием интерфейсов Ethernet, RS-485, CAN и т.п. Значение выходного сигнала соответствует значению, полученному от другого контроллера по информационной сети. У элемента есть один параметр: номер сетевой переменной.

<a name="a5.1.6"></a>

#### Выход

Элемент имеет один входной контакт. Выходов у элемента нет. В зависимости от настроек, элемент может выполнять две разные функции:

<a name="a5.1.7"></a>

##### Связь с выходным контактом контроллера

![outpin](https://www.mnppsaturn.ru/fbd2/images/outpin.png)

У элемента имеется один параметр: номер выходного контакта контроллера. Интерпретация входного значения зависит от особенностей аппаратной реализации на конкретном контроллере. Обычно используется для управления дискретными и аналоговыми выходами контроллера.

<a name="a5.1.8"></a>

##### Выходная сетевая переменная

![outvar](https://www.mnppsaturn.ru/fbd2/images/outvar.png)

У элемента имеется один параметр: номер выходной сетевой переменной. Значение поданного на вход элемента сигнала передается по информационной сети и доступно на других контроллерах через элемент "Входная сетевая переменная".

<a name="a5.2"></a>

### Логические функции

<a name="a5.2.1"></a>

#### Инверсия (NOT)

![not](https://www.mnppsaturn.ru/fbd2/images/not.png)

Элемент выполняет функцию логической инверсии "NOT". Элемент имеет один вход и один выход. Таблица истинности:

<table>
 <tr><td><b>Вход</b></td><td><b>Выходной сигнал</b></td></tr>
 <tr><td>Логический "0"</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>0</td></tr>
</table>

<a name="a5.2.2"></a>

#### Логическое "И" (AND)

![and](https://www.mnppsaturn.ru/fbd2/images/and.png)

Элемент выполняет логическую функцию "И" ("AND"). Элемент имеет два входа и один выход. Элемент имеет настройку инвертирования выходного сигнала.
Таблица истинности:

<table>
 <tr><td><b>Вход 1</b></td><td><b>Вход 2</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Логический "0"</td><td>Любое значение</td><td>0</td><td>1</td></tr>
 <tr><td>Any value</td><td>Логический "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>Логическая "1"</td><td>1</td><td>0</td></tr>
</table>

<a name="a5.2.3"></a>

#### Логическое "ИЛИ" (OR)

![or](https://www.mnppsaturn.ru/fbd2/images/or.png)

Элемент выполняет логическую функцию "ИЛИ" ("OR"). Элемент имеет два входа и один выход. Элемент имеет настройку инвертирования выходного сигнала.
Таблица истинности:

<table>
 <tr><td><b>Вход 1</b></td><td><b>Вход 2</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Логический "0"</td><td>Логический "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>Любое значение</td><td>1</td><td>0</td></tr>
 <tr><td>Любое значение</td><td>Логическая "1"</td><td>1</td><td>0</td></tr>
</table>

<a name="a5.2.4"></a>

#### Логическое "Исключающее ИЛИ" (XOR)

![xor](https://www.mnppsaturn.ru/fbd2/images/xor.png)

Элемент выполняет логическую функцию "Исключающее ИЛИ" ("XOR"). Элемент имеет два входа и один выход. Элемент имеет настройку инвертирования выходного сигнала. Таблица истинности:

<table>
 <tr><td><b>Вход 1</b></td><td><b>Вход 2</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Логический "0"</td><td>Логический "0"</td><td>0</td><td>1</td></tr>
 <tr><td>Логический "0"</td><td>Логическая "1"</td><td>1</td><td>0</td></tr>
 <tr><td>Логическая "1"</td><td>Логический "0"</td><td>1</td><td>0</td></tr>
 <tr><td>Логическая "1"</td><td>Логическая "1"</td><td>0</td><td>1</td></tr>
</table>

<a name="a5.3"></a>

### Триггеры

<a name="a5.3.1"></a>

#### RS - триггер

![rs](https://www.mnppsaturn.ru/fbd2/images/rs.png)

RS - триггер имеет два входа (R-Reset и S-Set) и один выход (Q).

RS - триггер, сохраняет своё предыдущее состояние при неактивном состоянии обоих входов (сигнал логического "0") и изменяет своё состояние при подаче на один из его входов сигнала логической "1". Пока на входы R и S подан сигнал логический "0", выход Q находится в неизменном состоянии. Если на вход S (Set) подать сигнал логическая "1", то сигнал на выходе Q принимает значение "1". Если на вход R (Reset) подать сигнал логическая "1", то сигнал на выходе Q принимает значение "0". При переводе сигнала на входе S или R в состояние логического "0", сигнал на выходе Q сохраняет своё предыдущее значение. При подаче на оба входа S и R сигнала логическая "1" состояние выхода Q не определено. Элемент имеет настройку инвертирования выходного сигнала.

Таблица истинности:
<table>
 <tr><td><b>S</b></td><td><b>R</b></td><td><b>Qnext</b></td><td><b>Действие</b></td></tr>
 <tr><td>0</td><td>0</td><td>Q</td><td>Хранение</td></tr>
 <tr><td>0</td><td>1</td><td>0</td><td>Сброс</td></tr>
 <tr><td>1</td><td>0</td><td>1</td><td>Установка</td></tr>
 <tr><td>1</td><td>1</td><td>?</td><td>Не разрешено</td></tr>
</table>

Каждый раз, когда триггер изменяет своё состояние, новое значение сохраняется в энергонезависимой памяти NVRAM (путём вызова функции `FBDsetProc(1, index, *value)`). При инициализации схемы, ранее сохранённые значения триггера восстанавливаются (путём вызова функции `FBDgetProc(1, index)`).

<a name="a5.3.2"></a>

#### D - триггер

![d](https://www.mnppsaturn.ru/fbd2/images/d.png)

D - триггер имеет два входа (D-Data и C-Clock) и один выход (Q).

D - триггер при положительном перепаде (нарастании) сигнала на входе C запоминает состояние входа D и выдаёт его на выход Q. При отсутствии положительного перепада на входе C, состояние выхода Q сохраняется постоянным.

Таблица истинности:
<table>
 <tr><td><b>D</b></td><td><b>C</b></td><td><b>Qnext</b></td><td><b>Действие</b></td></tr>
 <tr><td>Любое значение</td><td>Нарастание</td><td>D</td><td>Запоминание</td></tr>
 <tr><td>Любое значение</td><td>Нет нарастания</td><td>Сохранённое значение D</td><td>Хранение</td></tr>
</table>

В отличие от традиционных D - триггеров значения сигнала на входе D и выходе Q может принимать любое значение (не только "0" и "1").

Каждый раз, когда триггер изменяет своё состояние, новое значение сохраняется в энергонезависимой памяти NVRAM (путём вызова функции `FBDsetProc(1, index, *value)`). При инициализации схемы, ранее сохранённые значения триггера восстанавливаются (путём вызова функции `FBDgetProc(1, index)`).

<a name="a5.3.3"></a>

#### Счётчик

![cnt](https://www.mnppsaturn.ru/fbd2/images/cnt.png)

Элемент имеет три входа и один выход. Если на вход R (Reset) подан сигнал логической "1", то сигнал на выходе Q принимает значение "0" (сброс счётчика). Если на вход R подан сигнал логического "0", то значение сигнала на выходе Q увеличивается на 1 при положительном перепаде на входе "+" или уменьшается на 1 при положительном перепаде на входе "-". При отсутствии положительного перепада на входах "+" и "-" значение сигнала на выходе Q не изменяется.

Таблица истинности:
<table>
 <tr><td><b>+</b></td><td><b>-</b></td><td><b>R</b></td><td><b>Qnext</b></td><td><b>Действие</b></td></tr>
 <tr><td>Любое значение</td><td>Любое значение</td><td>1</td><td>0</td><td>Сброс</td></tr>
 <tr><td>Нарастание</td><td>Нет нарастания</td><td>0</td><td>Q+1</td><td>Увеличение на 1</td></tr>
 <tr><td>Нет нарастания</td><td>Нарастание</td><td>0</td><td>Q-1</td><td>Уменьшение на 1</td></tr>
 <tr><td>Нет нарастания</td><td>Нет нарастания</td><td>0</td><td>Q</td><td>Нет изменений</td></tr>
 <tr><td>Нарастание</td><td>Нарастание</td><td>0</td><td>Q</td><td>Нет изменений</td></tr>
</table>

Каждый раз, когда счётчик изменяет своё состояние, новое значение сохраняется в энергонезависимой памяти NVRAM (путём вызова функции `FBDsetProc(1, index, *value)`). При инициализации схемы, ранее сохранённые значения счётчика восстанавливаются (путём вызова функции `FBDgetProc(1, index)`).

<a name="a5.4"></a>

### Арифметические функции

<a name="a5.4.1"></a>

#### Сложение

![add](https://www.mnppsaturn.ru/fbd2/images/add.png)

Элемент имеет два входа и один выход. Значение выходного сигнала вычисляется, как арифметическая сумма значений сигналов на входах.

<a name="a5.4.2"></a>

#### Вычитание

![sub](https://www.mnppsaturn.ru/fbd2/images/sub.png)

Элемент имеет два входа и один выход. Значение выходного сигнала вычисляется, как арифметическая разность сигнала на первом (уменьшаемое) и на втором (вычитаемое) входах.

<a name="a5.4.3"></a>

#### Умножение

![mul](https://www.mnppsaturn.ru/fbd2/images/mul.png)

Элемент имеет два входа и один выход. Значение выходного сигнала вычисляется, как арифметическое произведение сигналов на первом и втором входах.

<a name="a5.4.4"></a>

#### Деление

![div](https://www.mnppsaturn.ru/fbd2/images/div.png)

Элемент имеет два входа и один выход. Значение выходного сигнала вычисляется, как арифметическое деление значения сигнала на первом входе (делимое) на значение сигнала на втором входе (делитель).

Особые случаи:
<table>
 <tr><td><b>Вход 1</b></td><td><b>Вход 2</b></td><td><b>Выход</b></td></tr>
 <tr><td>0</td><td>0</td><td>1</td></tr>
 <tr><td>Любое положительное значение</td><td>0</td><td>MAX_SIGNAL</td></tr>
 <tr><td>Любое отрицательное значение</td><td>0</td><td>MIN_SIGNAL</td></tr>
</table>

О том, что делить на 0 нельзя я знаю. :-)

<a name="a5.4.5"></a>

#### Абсолютное значение

![abs](https://www.mnppsaturn.ru/fbd2/images/abs.png)

Элемент имеет один вход и один выход. Значение выходного сигнала есть абсолютное значение сигнала на входе.

<a name="a5.5"></a>

### Функции регулирования

<a name="a5.5.1"></a>

#### Регулятор

![pid](https://www.mnppsaturn.ru/fbd2/images/pid.png)

Элемент имеет четыре входа и один выход. Входы:

* U - текущее значение контролируемого процесса (обратная связь)
* REF - значение, которое должно быть достигнуто в процессе регулирования (уставка)
* DT - время реакции регулятора (мс)
* P - коэффициент пропорциональности

Выход Q - сигнал влияния на регулируемый процесс.

В отличие от множества других подобных реализаций, данный элемент не использует традиционный алгоритм PID регулирования. Вместо этого применен алгоритм Эйлера-Лагранжа, обеспечивающий простоту настройки, высокую скорость сходимости и отсутствие склонности к возникновению колебаний. Данный алгоритм широко используется при стыковке космических аппаратов. Подробности можно узнать из специальной литературы, на сайте проекта Wikipedia имеется [статья](https://ru.wikipedia.org/wiki/%D0%A3%D1%80%D0%B0%D0%B2%D0%BD%D0%B5%D0%BD%D0%B8%D0%B5_%D0%AD%D0%B9%D0%BB%D0%B5%D1%80%D0%B0_%E2%80%94_%D0%9B%D0%B0%D0%B3%D1%80%D0%B0%D0%BD%D0%B6%D0%B0), которая поясняет некоторые моменты алгоритма.

Подробности реализации можно увидеть в исходном коде и примерах.

<a name="a5.5.2"></a>

#### Интегратор

![sum](https://www.mnppsaturn.ru/fbd2/images/sum.png)

Элемент используется для интегрирования (суммирования) во времени значения входного сигнала. Элемент часто используется совместно с описанным выше [регулятором](#a5.5.1). Элемент имеет три входа и один выход. Входы:

* X - интегрируемый сигнал
* DT - время интегрирования (мс)
* Lim - ограничение значения выходного сигнала

Выход Q - значение величины интеграла (суммы).

Каждый интервал времени DT значение входного сигнала X складывается с текущим накопленным значением. Если новое значение больше значения Lim или меньше значения -Lim, то результат ограничивается значением Lim или -Lim. Сигнал на выходе Q принимает значение результата.

<a name="a5.6"></a>

### Таймеры

<a name="a5.6.1"></a>

#### Таймер TON

![ton](https://www.mnppsaturn.ru/fbd2/images/ton.png)

TON - это таймер с задержкой включения. Элемент может быть использован в генераторах периодических сигналов и счетчиках времени. Элемент имеет два входа и один выход:

* D - запуск таймера
* T - величина задержки (мс)
* Q - выходной логический сигнал

Если на входе D присутствует сигнал логического "0", то на выходе Q всегда сигнал "0". Если на вход D подать сигнал логической "1", то на выходе Q появится сигнал "1" через промежуток времени, соответствующий сигналу на входе T. Элемент имеет настройку, инвертирующую значение выходного сигнала.

Таблица истинности:

<table>
 <tr><td><b>D</b></td><td><b>Условие</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Логический "0"</td><td>Любое</td><td>0</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>Время T не истекло</td><td>0</td><td>1</td></tr>
 <tr><td>Логическая "1"</td><td>Время T истекло</td><td>1</td><td>0</td></tr>
</table>

Временная диаграмма таймера TON:

![dton](https://www.mnppsaturn.ru/fbd2/images/dton.png)

<a name="a5.6.2"></a>

#### Таймер TP

![tp](https://www.mnppsaturn.ru/fbd2/images/tp.png)

Элемент имеет два входа и выход:

* D - запуск таймера
* T - время задержки (мс)
* Q - выход

Положительный фронт сигнала на входе D запускает таймер и устанавливает на выходе Q значение сигнала "1". По истечении времени T значение выходного сигнала принимает значение "0". Элемент имеет настройку, инвертирующую значение выходного сигнала.

Временная диаграмма таймера TP:

![dtp](https://www.mnppsaturn.ru/fbd2/images/dtp.png)

<a name="a5.7"></a>

### Остальные элементы

<a name="a5.7.1"></a>

#### Сравнение

![cmp](https://www.mnppsaturn.ru/fbd2/images/cmp.png)

Элемент имеет два входа и выход, выполняет сравнение значений сигналов на входах 1 и 2. Элемент имеет настройку, инвертирующую значение выходного сигнала. Таблица истинности:

<table>
 <tr><td><b>Условие</b></td><td><b>Выход</b></td><td><b>Инвертированный выход</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>1</td><td>0</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>0</td><td>1</td></tr>
</table>

<a name="a5.7.2"></a>

#### Вычисление максимума

![max](https://www.mnppsaturn.ru/fbd2/images/minmax.png)

Элемент имеет два входа и выход. Выходное значение сигнала соответствует входному сигналу с максимальным значением. Таблица истинности:

<table>
 <tr><td><b>Условие</b></td><td><b>Выход</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>Input1Val</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>Input2Val</td></tr>
</table>

<a name="a5.7.3"></a>

#### Вычисление минимума

![min](https://www.mnppsaturn.ru/fbd2/images/minmax.png)

Элемент имеет два входа и выход. Выходное значение сигнала соответствует входному сигналу с минимальным значением. Таблица истинности:

<table>
 <tr><td><b>Сondition</b></td><td><b>Output</b></td></tr>
 <tr><td>Input1Val &gt; Input2Val</td><td>Input2Val</td></tr>
 <tr><td>Input1Val &lt;= Input2Val</td><td>Input1Val</td></tr>
</table>

<a name="a5.7.4"></a>

#### Мультиплексор

![ms](https://www.mnppsaturn.ru/fbd2/images/ms.png)

Расширяемый мультиплексор. Элемент имеет пять входов (D0-D3, A) и один выход. Значение выходного сигнала соответствует значению сигнала на выбранном входе D0-D3. Выбор выхода производится на основании значения сигнала на входе A. Таблица истинности:

<table>
 <tr><td><b>A</b></td><td><b>Выход Q</b></td></tr>
 <tr><td>0</td><td>D0</td></tr>
 <tr><td>1</td><td>D1</td></tr>
 <tr><td>2</td><td>D2</td></tr>
 <tr><td>3</td><td>D3</td></tr>
</table>

<a name="a5.7.5"></a>

#### Ограничитель

![lim](https://www.mnppsaturn.ru/fbd2/images/lim.png)

Ограничитель сигнала имеет 3 входа и один выход:

* D - входной сигнал
* MX - верхнее ограничение сигнала
* MN - нижнее ограничение сигнала

Выходное значение сигнала элемента соответствует входному значению ограниченному значениями сигналов на входах MX (верхнее ограничение) и MN (нижнее ограничение).

<a name="a6"></a>

### Установка библиотеки

Setting is done by editing `fbdrt.h` in the following sequence.

1. Select the data type used to store the signal.
2. Select the data type used to store the index of the item.
3. If necessary, change the definition `ROM_CONST` and `DESCR_MEM`.
4. Choose to use or not to use optimization execution speed (defines `SPEED_OPT`).
5. Choose to use or not to use HMI functions (defines `USE_HMI`).
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

<a name="a7"></a>

### Формат описания схемы

<a name="a7.1"></a>

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

<a name="a7.2"></a>

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

<a name="a8"></a>

## Выполнение

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
