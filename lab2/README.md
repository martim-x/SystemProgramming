# Лабораторная работа 2 — File Mapping (HT API)

**Системное программирование** (преп. Смелов В.В.), ПИ+ИС-3, 4 часа.
Задание: `labs/Лабораторная_2_FileMapping/Лабораторная_2_FileMapping_HT.pdf`
API спецификация: `labs/Лабораторная_2_FileMapping/HT.h`

---

## Содержание

- [Теория и архитектура](#теория-и-архитектура)
- [Сводка терминов (File Mapping)](#сводка-терминов-file-mapping)
- [Сущности и их жизненный цикл](#сущности-и-их-жизненный-цикл)
- [Как запустить проект](#как-запустить-проект)
- [Демонстрация (фактические выводы)](#демонстрация-фактические-выводы)
- [HT API — детали реализации](#ht-api--детали-реализации)
- [Особенности программной реализации (с кодом)](#особенности-программной-реализации-с-кодом)
- [Разбор реализации: Insert, Update, Delete](#разбор-реализации-insert-update-delete)
- [Test Case HT API](#test-case-ht-api)
- [Порядок защиты и типовые вопросы](#порядок-защиты-и-типовые-вопросы)

---

## Теория и архитектура

### Назначение
Реализовать **HT-хранилище (Hash Table)** с использованием **File Mapping** (отображение файла в память) на Windows API.

### HT API функции
- `Create` — создать и открыть HT-хранилище
- `Open` — открыть существующее HT-хранилище
- `Insert` — добавить элемент (ключ/значение)
- `Delete` — удалить элемент по ключу
- `Get` — прочитать элемент по ключу
- `Update` — изменить значение элемента
- `Snap` — синхронное сохранение (flush) образа в файл
- `Close` — Snap + закрыть дескрипторы + освободить память
- `GetLastError` — получить сообщение последней ошибки
- `print` — вывод элемента в консоль

### Принцип File Mapping
1. `Create` → `CreateFile(CREATE_ALWAYS)` → `CreateFileMapping(PAGE_READWRITE)` → `MapViewOfFile(FILE_MAP_ALL_ACCESS)`
2. В память записывается **заголовок** (Capacity, SnapInterval, MaxKeyLength, MaxPayloadLength, FileName, lastsnaptime) + **массив элементов**
3. Элемент (**два разных смысла** — не путать):
   - `struct Element` в API — дескриптор операции: **указатели** `key`, `payload` + длины `keylength`, `payloadlength`; живёт на стеке вызывающего кода;
   - слот в образе файла — фиксированная запись: `key[MaxKeyLength]` + `payload[MaxPayloadLength]` + `keylength(int)` + `payloadlength(int)`; `keylength == 0` — слот пуст.
   Подробно: [Формат элемента в памяти](#3-формат-элемента-в-памяти) и [Сущности и их жизненный цикл](#сущности-и-их-жизненный-цикл).
4. Хэш: djb2 (`hash = hash * 33 + byte`) по ключу, модуль Capacity. Это **накопитель**: каждый байт преобразует предыдущее значение `hash`, а не «хэш от уже готового хэша».
   Подробно с трассировкой: [Почему «хэш берётся от хэша»](#почему-хэш-берётся-от-хэша).
5. Коллизии — линейное пробирование: `(index + 1) % Capacity`
6. Пустой слот = `keylength == 0` (lazy deletion — но цепочку пробирования она разрывает, см. оговорку в разделе «Сущности»)
7. `Snap` → `FlushViewOfFile` + `FlushFileBuffers` + обновление `lastsnaptime`
8. `Close` → `Snap` → `UnmapViewOfFile` → `CloseHandle(FileMapping)` → `CloseHandle(File)` → `delete HTHANDLE`
9. `Open` → `CreateFile(OPEN_EXISTING)` → чтение заголовка → `CreateFileMapping` → `MapViewOfFile` → восстановление `HTHANDLE`

### Архитектура решения
```
lab2/windows/
├── lab2.sln                      # Visual Studio Solution
├── OS10_HTAPI/                   # Статическая библиотека HT API
│   ├── OS10_HTAPI.vcxproj
│   ├── HT.h                      # Заголовочный (из задания)
│   └── HT.cpp                    # Реализация на Windows File Mapping
├── OS10_01/                      # Задание 01: встроенная реализация
│   ├── OS10_01.vcxproj
│   ├── HT.h                      # Копия интерфейса — файл проекта приложения
│   ├── HT.cpp                    # Копия реализации — компилируется в составе OS10_01.exe
│   └── OS10_01.cpp               # Демонстрация всех функций HT API
├── OS10_02/                      # Задание 02: использует OS10_HTAPI.lib
│   ├── OS10_02.vcxproj
│   └── OS10_02.cpp
├── OS10_03/                      # Задание 03: 2 экземпляра HT
│   ├── OS10_03.vcxproj
│   └── OS10_03.cpp
└── Tests/                        # Задание 04: автотесты
    ├── Tests.vcxproj
    └── Tests.cpp                 # 11 тест-кейсов
```

### Карта заданий
| № | Проект | Описание |
|---|---|---|
| 01 | OS10_01 | Реализует HT API инлайн, демонстрирует все функции |
| 02 | OS10_02 | Использует статическую библиотеку `OS10_HTAPI.lib`, повторяет OS10_01 |
| 03 | OS10_03 | Использует `OS10_HTAPI.lib`, работает с **2 HT одновременно** |
| 04 | Tests | Набор автотестов (11 тест-кейсов) для проверки HT API |

---

## Сводка терминов (File Mapping)

### File Mapping — отображение файла в память

**File Mapping** — механизм ОС, при котором содержимое файла на диске становится доступным программе как обычная область виртуальной памяти. Программа не вызывает `ReadFile`/`WriteFile`, а читает и пишет по указателю, как в массив. ОС сама подкачивает нужные страницы с диска и сбрасывает изменённые обратно.

### File View — представление файла

**File View** (дословно «представление файла») — это та часть адресного пространства процесса, куда отображён файл. Именно `File View` возвращается указателем из `MapViewOfFile`, и именно через него программа читает и пишет данные. Ключевая мысль: файл на диске и его представление в памяти — это **одни и те же байты**, а не копия; изменения в памяти становятся изменениями файла после сброса (flush).

### Проецирование (projection / mapping)

**Проецирование** — операция установления соответствия между диапазоном файла на диске и диапазоном виртуальных адресов процесса. После проецирования доступ к памяти по этому диапазону адресов фактически является доступом к файлу. Отсюда же термин «образ файла в памяти».

Проецирование выполняется в два шага, и это два разных объекта:

1. **Объект section (section object)** — создаётся в ядре ОС и описывает, какой файл и с какими правами будет отображён. Это ещё не память процесса.
2. **Представление (view)** — участок адресного пространства конкретного процесса, связанный с этим section object.

### CreateFileMapping

```cpp
HANDLE CreateFileMappingA(
    HANDLE hFile,                  // дескриптор файла (или INVALID_HANDLE_VALUE для pagefile)
    LPSECURITY_ATTRIBUTES lpAttr,  // атрибуты защиты, обычно NULL
    DWORD flProtect,               // права: PAGE_READWRITE / PAGE_READONLY / PAGE_EXECUTE_READ
    DWORD dwMaximumSizeHigh,       // старшие 32 бита размера
    DWORD dwMaximumSizeLow,        // младшие 32 бита размера
    LPCSTR lpName                  // имя объекта (для разделения между процессами), иначе NULL
);
```

**Что делает:** создаёт в ядре **объект section** (проекцию файла), но **не отображает** его в адресное пространство процесса. Возвращает `HANDLE` — дескриптор объекта section, а не указатель на память.

**Важно:** сам по себе `CreateFileMapping` не даёт доступа к данным. Пока не вызван `MapViewOfFile`, у процесса нет ни одного адреса, по которому видны байты файла.

Если `dwMaximumSize` = 0, размер берётся из текущего размера файла — поэтому перед отображением файл нужно **расширить** до нужного размера (`SetFilePointer` + `SetEndOfFile`), иначе отображение будет короче ожидаемого.

### MapViewOfFile

```cpp
LPVOID MapViewOfFile(
    HANDLE hFileMappingObject,  // дескриптор объекта section из CreateFileMapping
    DWORD  dwDesiredAccess,     // FILE_MAP_READ / FILE_MAP_WRITE / FILE_MAP_ALL_ACCESS
    DWORD  dwFileOffsetHigh,    // смещение в файле, старшие 32 бита
    DWORD  dwFileOffsetLow,     // смещение в файле, младшие 32 бита
    SIZE_T dwNumberOfBytesToMap // сколько байт отобразить (0 = весь section)
);
```

**Что делает:** отображает объект section в адресное пространство **вызывающего процесса** и возвращает указатель `LPVOID` на начало **File View**. После этого работа с файлом идёт как с массивом:

```cpp
char* data = (char*)MapViewOfFile(hMapping, FILE_MAP_ALL_ACCESS, 0, 0, size);
data[0] = 'A';          // это запись в файл (после flush)
int value = *(int*)(data + 16);  // это чтение из файла
```

**Важно:** один и тот же объект section можно отобразить несколько раз и в разные процессы — у каждого процесса будет свой адрес, но данные общие. Именно на этом построен межпроцессный обмен в лабораторной 11.

### Разница CreateFileMapping / MapViewOfFile

| | `CreateFileMapping` | `MapViewOfFile` |
|---|---|---|
| Что создаёт | объект section в ядре | представление (view) в адресном пространстве процесса |
| Что возвращает | `HANDLE` (дескриптор) | `LPVOID` (указатель на память) |
| Есть ли доступ к данным | нет | да, чтение/запись по указателю |
| Область действия | системная (объект ядра) | конкретный процесс |
| Аналог в Linux | — (аналог `mmap` делает сразу оба шага) | `mmap` |

### Связанные термины

| Термин | Значение |
|---|---|
| **Section object** | объект ядра, описывающий отображаемый файл и права доступа |
| **View / File View** | участок адресного пространства процесса, связанный с section object |
| **Проецирование** | установление соответствия «файл ↔ виртуальные адреса» |
| **Page** | страница памяти (обычно 4 КБ) — единица подкачки при отображении |
| **Flush / Snap** | сброс изменённых («грязных») страниц из памяти в файл |
| **PAGE_READWRITE** | права объекта section: чтение и запись |
| **FILE_MAP_ALL_ACCESS** | права представления: полный доступ |
| **Shared memory** | разделяемая память — одно представление, видимое нескольким процессам |
| **`FlushViewOfFile`** | просит ОС записать изменённые страницы представления в файл |
| **`FlushFileBuffers`** | гарантирует, что данные файла реально ушли на диск (а не только в кэш) |
| **`UnmapViewOfFile`** | снимает представление (освобождает диапазон адресов процесса) |
| **`CloseHandle`** | закрывает дескриптор файла или объекта section |

### Аналоги в Linux

| Windows | Linux |
|---|---|
| `CreateFileMapping` | — (нет отдельного шага) |
| `CreateFileMapping` + `MapViewOfFile` | `mmap` |
| `UnmapViewOfFile` | `munmap` |
| `FlushViewOfFile` + `FlushFileBuffers` | `msync` |
| изменение прав представления | `mprotect` |
| изменение размера представления | `mremap` |

---

## Сущности и их жизненный цикл

В лабе четыре уровня сущностей. Их важно не смешивать: одна и та же «вещь» на разных уровнях выглядит по-разному.

```
Уровень 0 — ОС:        файл на диске → HANDLE файла → объект section → File View
Уровень 1 — API:       HTHANDLE  +  Element (дескрипторы в памяти программы)
Уровень 2 — образ:     заголовок (536 Б) + массив слотов
Уровень 3 — вычисл.:   hash, index, offset (временные переменные)
```

### Уровень 0 — ресурсы ОС

| Сущность | Что это | Кто создаёт | Кто уничтожает |
|---|---|---|---|
| **Файл хранилища** (`test_ht1.dat`) | Байты на диске: заголовок + слоты. Живёт после завершения программы | `CreateFileA(CREATE_ALWAYS)` в `Create` | вручную (удаление файла) |
| **`HANDLE File`** | Дескриптор открытого файла, поле `HTHANDLE::File` | `CreateFileA` | `CloseHandle` в `Close` |
| **Объект section** (`HANDLE FileMapping`) | Объект ядра: «какой файл и с какими правами проецировать». Памяти процесса ещё не занимает | `CreateFileMappingA(PAGE_READWRITE)` | `CloseHandle` в `Close` |
| **File View** (`LPVOID Addr`) | Участок адресного пространства процесса, куда отображён файл. Через него идут все чтения/записи | `MapViewOfFile(FILE_MAP_ALL_ACCESS)` | `UnmapViewOfFile` в `Close` |
| **Файловый кэш ОС** | Неявный ресурс: страницы, изменённые в памяти, но ещё не на диске | ОС | `FlushViewOfFile` + `FlushFileBuffers` в `Snap` |

Порядок создания и разрушения строго симметричен:

```
Create:  файл → section → view
Close:   view → section → файл     (обратный порядок, иначе утечка/ошибка)
```

### Уровень 1 — дескрипторы API (живут в памяти программы)

#### `HTHANDLE` — дескриптор хранилища

| Поле | Тип | Назначение | Кто меняет |
|---|---|---|---|
| `Capacity` | `int` | ёмкость: сколько слотов в массиве | `Create` |
| `SecSnapshotInterval` | `int` | период авто-сохранения, сек | `Create` (см. оговорку ниже) |
| `MaxKeyLength` | `int` | максимальная длина ключа | `Create` |
| `MaxPayloadLength` | `int` | максимальная длина значения | `Create` |
| `FileName[512]` | `char` | имя файла хранилища | `Create` |
| `File` | `HANDLE` | дескриптор файла | `Create`/`Open`, обнуляется в `Close` |
| `FileMapping` | `HANDLE` | дескриптор объекта section | `Create`/`Open`, обнуляется в `Close` |
| `Addr` | `LPVOID` | указатель на File View | `Create`/`Open`, обнуляется в `Close` |
| `LastErrorMessage[512]` | `char` | текст последней ошибки | любая функция через `SetLastError` |
| `lastsnaptime` | `time_t` | время последнего `Snap` | `Snap`, читается в `Open` |
| `cs` | `CRITICAL_SECTION` | защита API (потокобезопасность) | инициализируется в конструкторе, используется обёртками |
| `SnapThread` | `HANDLE` | поток фонового snapshot | `Create`/`Open`, закрывается в `Close` |
| `StopEvent` | `HANDLE` | событие остановки фонового потока | `Create`/`Open`, сигнал в `Close` |
| `snapshotsmade` | `int` | число выполненных snapshot | `SnapInternal` (инкремент) |

Создаётся: `Create` (новый) или `Open` (из файла). Уничтожается: `Close` (`delete h`).

#### `Element` — дескриптор операции

Это **не** запись в файле, а «заявка»: адреса ключа и значения в памяти вызывающего кода.

| Поле | Тип | Что содержит |
|---|---|---|
| `key` | `const void*` | **указатель** на байты ключа |
| `keylength` | `int` | сколько байт ключа значимо |
| `payload` | `const void*` | **указатель** на байты значения |
| `payloadlength` | `int` | сколько байт значения значимо |

Живёт на стеке вызывающего кода, в хранилище не копируется. Внутри функций из него **копируются байты** по указателям. Четыре конструктора:

```cpp
Element();                                                          // пустой
Element(const void* key, int keylength);                            // для Get/Delete
Element(const void* key, int kl, const void* payload, int pl);      // для Insert
Element(Element* old, const void* newPayload, int newPl);           // для Update
```

### Уровень 2 — образ файла (то, что лежит внутри File View)

#### Заголовок — 536 байт в начале отображения

| Смещение | Размер | Поле |
|---|---|---|
| 0 | 4 | `Capacity` |
| 4 | 4 | `SecSnapshotInterval` |
| 8 | 4 | `MaxKeyLength` |
| 12 | 4 | `MaxPayloadLength` |
| 16 | 512 | `FileName[512]` |
| 528 | 8 | `lastsnaptime` (`time_t`) |
| **536** | | **начало массива слотов** |

Заголовок пишется один раз в `Create`, читается в `Open` — по нему `Open` узнаёт размеры, чтобы создать section правильного размера.

#### Слот (элемент образа)

Фиксированные `MaxKeyLength + MaxPayloadLength + 8` байт:

| Смещение внутри слота | Размер | Поле |
|---|---|---|
| 0 | `MaxKeyLength` | `key[]` — ключ, добитый нулями |
| `MaxKeyLength` | `MaxPayloadLength` | `payload[]` — значение, добитое нулями |
| `MaxKeyLength + MaxPayloadLength` | 4 | `keylength` — реальная длина ключа; **0 = слот пуст** |
| `+4` | 4 | `payloadlength` — реальная длина значения |

Адрес слота — чистая арифметика, без индексов и обхода:

```cpp
offset = 536 + (MaxKeyLength + MaxPayloadLength + 8) * index
```

Пример для демо (`MaxKeyLength=32`, `MaxPayloadLength=128`, `Capacity=100`):

| Что | Расчёт | Значение |
|---|---|---|
| Заголовок | 4×`int` + 512 + `time_t` | 536 байт |
| Размер слота | 32 + 128 + 4 + 4 | 168 байт |
| Образ файла | 536 + 168 × 100 | 17 336 байт ≈ 17 КБ |

### Уровень 3 — временные величины

| Сущность | Тип | Где появляется | Что означает |
|---|---|---|---|
| `hash` | `unsigned int` | `HashFunction` | накопитель djb2, 32-бит с заворачиванием |
| `index` | `int` | `Insert`/`Get`/`Delete`/`Update` | номер слота `0…Capacity-1`, двигается `(index+1) % Capacity` |
| `offset` | `size_t` | те же функции | байтовое смещение от начала File View |
| `storageSize` | `size_t` | `Create`/`Open` | полный размер образа = 536 + слот × Capacity |

### Что делаем с каждой сущностью

#### Жизненный цикл

```
Create(cap, interval, maxKey, maxPayload, name)
   ├─ создаёт файл, section, view
   ├─ пишет заголовок (536 Б)
   └─ обнуляет массив слотов
        ↓
Insert / Get / Update / Delete   ← работа только с File View, файл на диске не трогается
        ↓
Snap (или Close, который сам вызывает Snap)
   FlushViewOfFile → FlushFileBuffers → lastsnaptime = time(NULL)
        ↓
Close
   view → section → файл → delete HTHANDLE
```

Второй вход:

```
Open(name)
   ├─ CreateFile(OPEN_EXISTING)
   ├─ ReadFile → разбор заголовка (536 Б)
   ├─ CreateFileMapping + MapViewOfFile
   └─ восстановление HTHANDLE из заголовка
        ↓
   те же операции, затем Close
```

#### Операции API: вход → что меняется → выход

| Функция | Что на входе | Что делает с образом | Возврат |
|---|---|---|---|
| `Create` | параметры + имя файла | пишет заголовок, обнуляет слоты | `HTHANDLE*` / `NULL` |
| `Open` | имя файла | ничего не пишет, читает заголовок | `HTHANDLE*` / `NULL` |
| `Insert` | `Element` (ключ+значение) | ищет пустой слот, копирует байты, ставит длины | `TRUE/FALSE` |
| `Get` | `Element` (только ключ) | ничего не меняет, **копирует** найденное в `malloc` | `Element*` / `NULL` |
| `Update` | ключ + новые байты значения | перезаписывает `payload[]`, меняет `payloadlength` | `TRUE/FALSE` |
| `Delete` | `Element` (только ключ) | ставит `keylength = 0` (lazy deletion) | `TRUE/FALSE` |
| `Snap` | `HTHANDLE*` | flush view → flush file buffers, пишет `lastsnaptime` | `TRUE/FALSE` |
| `Close` | `HTHANDLE*` | `Snap` + освобождение всех трёх дескрипторов + `delete` | `TRUE/FALSE` |
| `GetLastError` | `HTHANDLE*` | читает `LastErrorMessage` | `char*` / `NULL` |
| `print` | `Element*` | печатает в консоль, ничего не меняет | `void` |

#### Внутренние (не экспортируются)

| Функция | Роль |
|---|---|
| `SetLastError` | записывает текст ошибки в `HTHANDLE::LastErrorMessage` |
| `CalculateStorageSize` | 536 + слот × Capacity |
| `GetElementOffset` | адрес слота по индексу |
| `IsSlotEmpty` | `keylength == 0` |
| `HashFunction` | djb2 → `% Capacity` |

#### Кто владеет памятью

| Что | Владелец | Кто освобождает |
|---|---|---|
| Байты ключа/значения, переданные в `Insert` | вызывающий код | вызывающий код (копия уже в файле) |
| `Element*`, который вернул `Get` | **вызывающий код** | он же: `free(key)`, `free(payload)`, `delete element` |
| `HTHANDLE` | API | `Close` |
| File View | процесс | `UnmapViewOfFile` в `Close` |
| Объект section | процесс | `CloseHandle` в `Close` |

В демо `OS10_01.cpp` это видно буквально:

```cpp
HT::Element* r1 = HT::Get(ht, &g1);
if (r1) { print(r1); free((void*)r1->key); free((void*)r1->payload); delete r1; }
```

### Потокобезопасность и асинхронный snapshot

Оба требования вводной части ТЗ реализованы.

**Потокобезопасность.** В `HTHANDLE` добавлено поле `CRITICAL_SECTION cs`. Каждая публичная операция — тонкая обёртка: берёт блокировку, вызывает внутреннюю `*Internal`-функцию, отпускает блокировку.

```cpp
BOOL Insert(const HTHANDLE* h, const Element* element)
{
    if (!h) return FALSE;
    HTHANDLE* mh = (HTHANDLE*)h;
    EnterCriticalSection(&mh->cs);
    BOOL result = InsertInternal(h, element);
    LeaveCriticalSection(&mh->cs);
    return result;
}
```

Почему обёртки, а не блокировка внутри самих функций: у `InsertInternal` шесть точек выхода, у `GetInternal` — три. Забытый `LeaveCriticalSection` на любой из них даёт вечный дедлок, который не проявляется в однопоточных тестах. Обёртка гарантирует ровно один `Enter`/`Leave` на вызов.

`CRITICAL_SECTION` рекурсивна для одного потока, но вложенных захватов в коде нет: `Close` вызывает `SnapInternal` (не `Snap`) уже под блокировкой.

**Асинхронный snapshot.** В `Create`/`Open` при `SecSnapshotInterval > 0` создаётся событие `StopEvent` и поток `SnapThread`:

```cpp
static DWORD WINAPI SnapshotThreadProc(LPVOID parameter)
{
    HTHANDLE* h = (HTHANDLE*)parameter;
    DWORD intervalMs = (DWORD)h->SecSnapshotInterval * 1000;

    for (;;)
    {
        DWORD wait = WaitForSingleObject(h->StopEvent, intervalMs);
        if (wait != WAIT_TIMEOUT) break;      // подан сигнал остановки

        EnterCriticalSection(&h->cs);
        SnapInternal(h);
        LeaveCriticalSection(&h->cs);
    }
    return 0;
}
```

- `WaitForSingleObject` с таймаутом заменяет `Sleep` в цикле: поток просыпается либо по периоду, либо мгновенно по `SetEvent`, без задержки на весь интервал.
- Поток берёт тот же `cs`, поэтому фоновый flush не пересекается с `Insert`/`Update`.
- `snapshotsmade` — счётчик выполненных snapshot, по нему демо и тест проверяют, что фоновый поток реально работал.

**Порядок в `Close` — критичен.** Сначала останавливается поток, только потом захватывается `cs`:

```cpp
StopSnapshotThread(mh);          // SetEvent → WaitForSingleObject(hThread) → CloseHandle

EnterCriticalSection(&mh->cs);   // здесь поток уже мёртв и не ждёт cs
SnapInternal(mh);
UnmapViewOfFile(...); CloseHandle(...);
LeaveCriticalSection(&mh->cs);
DeleteCriticalSection(&mh->cs);
```

Если сделать наоборот — сначала `EnterCriticalSection`, а внутри ждать поток, — поток в это время стоит на `EnterCriticalSection` и ждёт освобождения `cs`. Классический deadlock: каждый ждёт другого.

### Оговорка, которая осталась

**Lazy deletion ломает цепочку пробирования.** `Delete` ставит `keylength = 0`, а поиск останавливается на первом пустом слоте. Если удалить элемент из середины цепочки, элементы, вставленные после него, станут недостижимы. В классической открытой адресации для этого нужны «надгробия» (tombstones). В тестах этот случай не проверяется — коллизионный тест вставляет 5 ключей в ёмкость 7 без удалений.

---

## Как запустить проект

**Среда:** Visual Studio 2022 с компонентом «Разработка классических приложений на C++».  
**Платформа:** **ARM64** (на машине Windows 11 ARM64).

1. Открыть `lab2/windows/lab2.sln` двойным кликом.
2. Выбрать платформу: панель сверху (`Debug`/`Release`) → **ARM64** (не x64!).
3. **Build → Build Solution** (`Ctrl+Shift+B`). Соберутся 5 проектов.
4. Запуск: правый клик по проекту → **Set as Startup Project** → `Ctrl+F5`.

### Командная строка (Developer Command Prompt)
```cmd
cd /d %USERPROFILE%\Documents\VisualStudioCodeRepos\SystemProgramming\lab2\windows
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64
msbuild lab2.sln -restore /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /v:minimal /nologo
```

### Артефакты после сборки
| Что | Путь |
|---|---|
| `OS10_HTAPI.lib` | `OS10_HTAPI\OS10_HTAPI\ARM64\Release\` |
| `OS10_01.exe` | `OS10_01\OS10_01\ARM64\Release\` |
| `OS10_02.exe` | `OS10_02\OS10_02\ARM64\Release\` |
| `OS10_03.exe` | `OS10_03\OS10_03\ARM64\Release\` |
| `Tests.exe` | `Tests\Tests\ARM64\Release\` |

### Как проекты видят библиотеку

`OS10_02`, `OS10_03` и `Tests` ссылаются на `OS10_HTAPI` **как на проект**, а не через настройки компоновщика:

```xml
<ItemGroup>
  <ProjectReference Include="..\OS10_HTAPI\OS10_HTAPI.vcxproj">
    <Project>{1A7C4E20-0B31-4C7D-9E11-000000000010}</Project>
  </ProjectReference>
</ItemGroup>
```

Это даёт три вещи, которых не было при ручной линковке через `AdditionalDependencies`:

1. **Порядок сборки** — MSBuild сам соберёт `OS10_HTAPI` раньше зависимых проектов. Не нужно предварительно собирать библиотеку вручную.
2. **Путь к `.lib`** — берётся из `OutDir` проекта-цели, `AdditionalLibraryDirectories` не нужен.
3. **Обновление** — изменение в библиотеке автоматически вызывает перелинковку зависимых приложений.

Директории заголовков (`AdditionalIncludeDirectories = ..\OS10_HTAPI`) остаются вручную: `ProjectReference` в C++ не добавляет include-пути — только линковку и порядок сборки.

---

## Демонстрация (фактические выводы)

**Все выводы ниже получены на живой машине Windows 11 ARM64, тулсет v145, конфигурация Release.**

> ⚠️ Логи сняты до добавления потокобезопасности и async snapshot: в них нет блоков «Error handling» и «Async snapshot», а тестов 9 вместо 11. После `git pull` нужен один перепрогон — структура вывода описана в исходниках.

> **Полные логи запуска всех 4 проектов:** см. [`windows/DEMONSTRATION-WINDOWS.md`](windows/DEMONSTRATION-WINDOWS.md)

### Краткая сводка результатов

| Проект | Статус | Ключевой результат |
|---|---|---|
| **OS10_01** (inline) | ✅ | Полный цикл Create/Insert/Get/Update/Delete/Snap/Close/Open — данные персистят |
| **OS10_02** (static lib) | ✅ | Идентичен OS10_01, использует `OS10_HTAPI.lib` |
| **OS10_03** (2 HT) | ✅ | Два независимых хранилища работают одновременно, изоляция подтверждена |
| **Tests** (11 тестов) | ⚠️ | 63/63 passed на прогоне из 9 тестов (до добавления потокобезопасности и async snapshot). Прогон 11 тестов — после `git pull` на Windows |

### Пример вывода OS10_01
```
=== OS10_01: HT API Demo (inline implementation) ===
Created HT: test_ht1.dat, capacity=100
Insert key1: OK
Insert key2: OK
Insert key3: OK
Get key1: Element: key=key1, keylength=4, payload=value1, payloadlength=6
Get key2: Element: key=key2, keylength=4, payload=value2, payloadlength=6
Update key1: OK
Get key1 after update: Element: key=key1, keylength=4, payload=newvalue1, payloadlength=9
Delete key2: OK
Get key2 after delete: NOT FOUND (expected)
Snap: OK
Close: OK
Reopened HT: test_ht1.dat
Get key1 after reopen: Element: key=key1, keylength=4, payload=newvalue1, payloadlength=9
Close: OK
=== Demo complete ===
```

### Пример вывода Tests
```
=== OS10 HT API Test Suite ===
=== Test: Create and Open === [13 PASS]
=== Test: Insert and Get === [9 PASS]
=== Test: Delete === [4 PASS]
=== Test: Update === [4 PASS]
=== Test: Snap === [4 PASS]
=== Test: Collision Handling === [11 PASS]
=== Test: Error Messages === [3 PASS]
=== Test: Persistence Across Close/Open === [7 PASS]
=== Test: Max Key/Payload Sizes === [8 PASS]
=== Summary ===
Passed: 63
Failed: 0
Total:  63
ALL TESTS PASSED!
```

---

## HT API — детали реализации

### Структуры данных
```cpp
struct HTHANDLE {
    int Capacity;               // емкость (кол-во слотов)
    int SecSnapshotInterval;    // интервал авто-snap (сек)
    int MaxKeyLength;           // макс. длина ключа
    int MaxPayloadLength;       // макс. длина значения
    char FileName[512];         // имя файла
    HANDLE File;                // дескриптор файла
    HANDLE FileMapping;         // дескриптор mapping
    LPVOID Addr;                // адрес отображения (MapViewOfFile)
    char LastErrorMessage[512]; // последняя ошибка
    time_t lastsnaptime;        // время последнего Snap
};

struct Element {
    const void* key;
    int keylength;
    const void* payload;
    int payloadlength;
};
```

### Компоновка памяти (MapView)
```
+---------------------------+
| Capacity (int)            |
| SecSnapshotInterval (int) |
| MaxKeyLength (int)        |
| MaxPayloadLength (int)    |
| FileName[512]             |
| lastsnaptime (time_t)     |  ← заголовок: 24 + 512 = 536 байт
+---------------------------+
| Element[0]                |
|   key[MaxKeyLength]       |
|   payload[MaxPayloadLength]|
|   keylength (int)         |
|   payloadlength (int)     |
+---------------------------+
| Element[1]                |
|   ...                     |
+---------------------------+
| ... Element[Capacity-1]   |
+---------------------------+
```
Размер элемента = `MaxKeyLength + MaxPayloadLength + 8` байт.

### Потокобезопасность
В текущей реализации **нет синхронизации**. Для многопоточного доступа нужно добавить `CRITICAL_SECTION` в `HTHANDLE` и `EnterCriticalSection`/`LeaveCriticalSection` в каждой функции API.

### Обработка ошибок
- Функции возвращают `BOOL` (TRUE/FALSE) или указатель (NULL = ошибка)
- Текст ошибки → `HTHANDLE->LastErrorMessage`
- `GetLastError(ht)` возвращает указатель на сообщение или NULL

### Флаг `/Oi-`
Во всех `.vcxproj`:
```xml
<AdditionalOptions>/Oi- %(AdditionalOptions)</AdditionalOptions>
```
Отключает intrinsics компилятора (конфликт с `div_t div(int,int)` из `<stdlib.h>`).

---

## Особенности программной реализации (с кодом)

Ниже — фактический код из `OS10_HTAPI/HT.cpp`, по шагам: как файл проецируется, как устроен образ и как выполняются операции.

### 1. Создание хранилища: CreateFile → CreateFileMapping → MapViewOfFile

Три вызова подряд — три разных объекта: файл, объект section, представление.

```cpp
HTHANDLE* Create(int Capacity, int SecSnapshotInterval, int MaxKeyLength,
                 int MaxPayloadLength, const char FileName[512])
{
    HTHANDLE* h = new HTHANDLE(Capacity, SecSnapshotInterval,
                               MaxKeyLength, MaxPayloadLength, FileName);

    // 1. Открываем/создаём файл на диске
    h->File = CreateFileA(FileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,   // чтобы Open() из другого процесса не падал
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h->File == INVALID_HANDLE_VALUE) { /* ошибка */ }

    size_t storageSize = CalculateStorageSize(h);

    // 2. Создаём объект section в ядре (память процесса ещё не занята)
    h->FileMapping = CreateFileMappingA(h->File, NULL, PAGE_READWRITE,
        (DWORD)(storageSize >> 32),           // старшие 32 бита размера
        (DWORD)(storageSize & 0xFFFFFFFF),    // младшие 32 бита
        NULL);                                // без имени — не разделяемый
    if (!h->FileMapping) { /* ошибка */ }

    // 3. Проецируем section в адресное пространство — получаем File View
    h->Addr = MapViewOfFile(h->FileMapping, FILE_MAP_ALL_ACCESS, 0, 0, storageSize);
    if (!h->Addr) { /* ошибка */ }

    // 4. Размечаем заголовок прямо в отображённой памяти
    char* ptr = (char*)h->Addr;
    *(int*)ptr = h->Capacity;             ptr += sizeof(int);
    *(int*)ptr = h->SecSnapshotInterval;  ptr += sizeof(int);
    *(int*)ptr = h->MaxKeyLength;         ptr += sizeof(int);
    *(int*)ptr = h->MaxPayloadLength;     ptr += sizeof(int);
    strcpy_s((char*)ptr, 512, h->FileName); ptr += 512;
    *(time_t*)ptr = 0;                    // lastsnaptime

    // 5. Обнуляем область элементов
    memset(ptr + sizeof(time_t), 0,
           storageSize - (ptr - (char*)h->Addr) - sizeof(time_t));

    return h;
}
```

**Ключевая деталь:** размер вычисляется до создания mapping, потому что `CreateFileMapping` с `dwMaximumSize != 0` задаёт размер section, а `MapViewOfFile` с `dwNumberOfBytesToMap = storageSize` — размер представления. Если передать 0, представление ограничится текущим размером файла (а он у `CREATE_ALWAYS` нулевой).

```cpp
size_t CalculateStorageSize(const HTHANDLE* h)
{
    size_t headerSize  = sizeof(int) * 4 + 512 + sizeof(time_t);            // 536 байт
    size_t elementSize = h->MaxKeyLength + h->MaxPayloadLength + sizeof(int) * 2;
    return headerSize + elementSize * (size_t)h->Capacity;
}
```

### 2. Открытие существующего хранилища: Open

`Open` не создаёт файл, а читает заголовок обычным `ReadFile`, чтобы узнать параметры, и только потом проецирует.

```cpp
HTHANDLE* Open(const char FileName[512])
{
    HANDLE file = CreateFileA(FileName, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);   // OPEN_EXISTING, не CREATE
    if (file == INVALID_HANDLE_VALUE) return nullptr;

    // Читаем заголовок обычным ReadFile — параметры хранилища ещё неизвестны
    char header[512 + sizeof(int) * 4 + sizeof(time_t)];
    DWORD bytesRead;
    ReadFile(file, header, sizeof(header), &bytesRead, NULL);

    char* ptr = header;
    int Capacity          = *(int*)ptr; ptr += sizeof(int);
    int SecSnapshotInterval = *(int*)ptr; ptr += sizeof(int);
    int MaxKeyLength      = *(int*)ptr; ptr += sizeof(int);
    int MaxPayloadLength  = *(int*)ptr; ptr += sizeof(int);
    char name[512]; strcpy_s(name, (char*)ptr); ptr += 512;
    time_t lastsnaptime   = *(time_t*)ptr;

    size_t storageSize = sizeof(int) * 4 + 512 + sizeof(time_t) +
        ((size_t)MaxKeyLength + (size_t)MaxPayloadLength + sizeof(int) * 2) * Capacity;

    // Теперь можно создавать mapping: размер section = 0 → берётся размер файла
    HANDLE mapping = CreateFileMappingA(file, NULL, PAGE_READWRITE, 0, 0, NULL);
    LPVOID addr = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, storageSize);

    HTHANDLE* h = new HTHANDLE(Capacity, SecSnapshotInterval,
                               MaxKeyLength, MaxPayloadLength, name);
    h->File = file; h->FileMapping = mapping; h->Addr = addr;
    h->lastsnaptime = lastsnaptime;
    return h;
}
```

**Особенность:** здесь `CreateFileMappingA(..., 0, 0, NULL)` — размер section берётся из размера файла на диске. Это корректно, потому что файл уже размечен предыдущим `Create` и имеет точный размер.

### 3. Формат элемента в памяти

Слово «элемент» здесь означает **два разных объекта** — их важно не путать.

**`struct Element`** (из `HT.h`) — дескриптор операции, живёт в памяти программы:

```cpp
struct Element {
    const void* key;      // УКАЗАТЕЛЬ на ключ (не сами байты)
    int         keylength;
    const void* payload;  // УКАЗАТЕЛЬ на значение
    int         payloadlength;
};
```

Это «заявка»: программа говорит `Insert(ht, &element)`, где `element` содержит адреса ключа и значения в **своей** памяти. Внутри `HT.cpp` эти байты копируются в отображённый файл — сам `Element` в хранилище не попадает.

**Элемент (слот) образа файла** — фиксированная запись внутри File View:

```
offset = 536 + (MaxKeyLength + MaxPayloadLength + 8) * index

+------------------+------------------+-----------+---------------+
| key[MaxKeyLength]| payload[MaxPl..] | keylength | payloadlength |
+------------------+------------------+-----------+---------------+
                                     ^ keylength == 0  → слот пуст
```

```cpp
size_t GetElementOffset(const HTHANDLE* h, int index)
{
    size_t headerSize  = sizeof(int) * 4 + 512 + sizeof(time_t);
    size_t elementSize = h->MaxKeyLength + h->MaxPayloadLength + sizeof(int) * 2;
    return headerSize + elementSize * (size_t)index;
}

BOOL IsSlotEmpty(const HTHANDLE* h, int index)
{
    char* elementPtr = (char*)h->Addr + GetElementOffset(h, index);
    int keylength = *(int*)(elementPtr + h->MaxKeyLength + h->MaxPayloadLength);
    return keylength == 0;          // пустой слот определяется по keylength
}
```

### 4. Хэш-функция djb2 и линейное пробирование

```cpp
int HashFunction(const void* key, int keylength, int capacity)
{
    const unsigned char* bytes = (const unsigned char*)key;
    unsigned int hash = 5381;
    for (int i = 0; i < keylength; i++)
        hash = ((hash << 5) + hash) + bytes[i];   // hash * 33 + byte
    return (int)(hash % (unsigned int)capacity);
}
```

#### Почему «хэш берётся от хэша»

Это не повторный вызов хэш-функции, а **накопитель** (running hash): переменная `hash` хранит промежуточный результат, и каждый следующий байт **преобразует** его. Если развернуть цикл, получается полином по основанию 33:

```
h = b₀·33ⁿ⁻¹ + b₁·33ⁿ⁻² + … + bₙ₋₁
```

То есть каждый байт входит со своим весом: первый умножается 33 много раз, последний — один. Поэтому `"abc"` и `"cba"` дают разные хэши — в отличие от простой суммы байтов, где анаграммы совпали бы.

`hash*33` записано как `(hash << 5) + hash`: 33 = 32 + 1, поэтому умножение заменено сдвигом и сложением — две дешёвые операции вместо одной дорогой.

Трассировка ключа `key1` (проверено компиляцией самой функции):

| Шаг | Байт | `hash = hash*33 + byte` |
|---|---|---|
| старт | — | 5 381 |
| 1 | `k` = 0x6B | 177 680 |
| 2 | `e` = 0x65 | 5 863 541 |
| 3 | `y` = 0x79 | 193 496 974 |
| 4 | `1` = 0x31 | 2 090 432 895 (0x7C99797F) |

`2 090 432 895 % 100 = 95` → слот №95. Соседние ключи: `key2` → 96, `key3` → 97.

Два момента, о которых спрашивают:

- **Переполнение — это нормально.** `unsigned int` 32-битный, значение заворачивается по модулю 2³²; для unsigned в C++ это определено стандартом, не UB. Именно поэтому «сырой» хэш `key1` — 2 090 432 895, а не 6 385 400 191 (результат без заворачивания).
- **`% capacity` в конце** превращает большое число в индекс массива `0…Capacity-1`. При `Capacity = 100` индексы 95/96/97 соседние, коллизии нет. При малой ёмкости два разных ключа дают один индекс — тогда включается линейное пробирование.

Почему основание 33: нечётное (хорошо «размазывает» биты), раскладывается в сдвиг+сложение (быстро), классическое значение из djb2 Дэна Бернстайна.

#### Линейное пробирование

Поиск слота — линейное пробирование по кругу, с защитой от бесконечного цикла:

```cpp
int startIndex = HashFunction(element->key, element->keylength, h->Capacity);
int index = startIndex;

for (int i = 0; i < h->Capacity; i++)
{
    if (IsSlotEmpty(h, index)) { /* вставляем сюда */ }

    // ключ уже есть → вставка запрещена
    size_t offset = GetElementOffset(h, index);
    char* existing = (char*)h->Addr + offset;
    int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);
    if (existingKeyLength == element->keylength &&
        memcmp(existing, element->key, element->keylength) == 0)
    { SetLastError((HTHANDLE*)h, "Key already exists"); return FALSE; }

    index = (index + 1) % h->Capacity;
    if (index == startIndex) break;    // обошли весь массив
}
```

### 5. Вставка: ключ и значение копируются по фиксированным смещениям

```cpp
size_t offset = GetElementOffset(h, index);
char* dest = (char*)h->Addr + offset;

memcpy(dest, element->key, element->keylength);
memset(dest + element->keylength, 0, h->MaxKeyLength - element->keylength);   // добивка нулями

memcpy(dest + h->MaxKeyLength, element->payload, element->payloadlength);
memset(dest + h->MaxKeyLength + element->payloadlength, 0,
       h->MaxPayloadLength - element->payloadlength);

*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength) = element->keylength;
*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = element->payloadlength;
```

Записи идут **сразу в отображённую память**, без промежуточного буфера и без `WriteFile`. Именно поэтому операции с HT — это обычные обращения к памяти.

### 6. Удаление: lazy deletion

```cpp
if (existingKeyLength == element->keylength &&
    memcmp(existing, element->key, element->keylength) == 0)
{
    *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength) = 0;  // просто keylength = 0
    return TRUE;
}
```

**Почему это корректно для открытой адресации в данной реализации:** поиск (`Get`, `Delete`, `Update`) останавливается на первом слоте с `keylength == 0`. Если бы удаление обнуляло слот в середине цепочки пробирования, поиск элементов, вставленных после него, сломался бы. В классической открытой адресации для этого используют «надгробия» (tombstones). Здесь же цепочка не разрывается только потому, что после удаления элемент считается отсутствующим, а вставка может занять этот слот — при этом элемент, который был дальше по цепочке, станет недостижим. Это известное ограничение реализации, о котором стоит сказать на защите.

### 7. Snap: сброс представления и файловых буферов

```cpp
BOOL Snap(const HTHANDLE* h)
{
    if (!FlushViewOfFile(h->Addr, 0)) {          // 0 = сбросить всё представление
        SetLastError((HTHANDLE*)h, "FlushViewOfFile failed");
        return FALSE;
    }
    if (!FlushFileBuffers(h->File)) {            // гарантия записи на диск
        SetLastError((HTHANDLE*)h, "FlushFileBuffers failed");
        return FALSE;
    }
    ((HTHANDLE*)h)->lastsnaptime = time(NULL);
    return TRUE;
}
```

Два вызова нужны потому, что это **два разных уровня кэширования**: `FlushViewOfFile` переносит изменённые страницы представления в файловый кэш ОС, `FlushFileBuffers` — из файлового кэша на устройство.

### 8. Close: Snap и корректный порядок освобождения

```cpp
BOOL Close(const HTHANDLE* h)
{
    BOOL result = Snap(h);              // 1. сохранить изменения

    if (h->Addr)        UnmapViewOfFile(h->Addr);        // 2. снять представление
    if (h->FileMapping) CloseHandle(h->FileMapping);     // 3. закрыть section
    if (h->File)        CloseHandle(h->File);            // 4. закрыть файл

    delete h;                           // 5. освободить структуру
    return result;
}
```

**Порядок принципиален:** нельзя закрыть `FileMapping` раньше, чем снято представление, и нельзя закрыть `File` раньше, чем закрыт section.

### 9. Особенности, о которых спрашивают

| Особенность | Суть |
|---|---|
| **Потокобезопасность (реализована)** | В `HTHANDLE` есть `CRITICAL_SECTION cs`; все публичные операции — обёртки, берущие блокировку. Проверяется тестом `TestThreadSafety`: 2 потока по 100 ключей в одно хранилище. |
| **Файл расширяется mapping'ом** | `CreateFileMapping` с явным размером увеличивает файл до этого размера; поэтому `Create` не вызывает `SetEndOfFile` вручную. |
| **`FILE_SHARE_READ \| FILE_SHARE_WRITE`** | Без флагов совместного доступа второй `CreateFile` в `Open` из другого процесса получил бы `ERROR_SHARING_VIOLATION`. |
| **Размер элемента фиксирован** | `MaxKeyLength + MaxPayloadLength + 8` — переменные по длине ключи/значения не поддерживаются, зато смещение любого элемента вычисляется арифметикой, без индексов. |
| **Заголовок 536 байт** | 4 × `int` (16) + `FileName[512]` + `time_t` (8) = 536. Это значение фигурирует и в `Create`, и в `Open`, и в тестах. |
| **`Get` возвращает копию** | `Get` делает `malloc` и копирует ключ/значение в кучу вызывающего — вызывающая сторона обязана освободить память. |
| **Флаг `/Oi-`** | Конфликт intrinsics с `div_t div(int,int)`: `cl` подставляет `div` как intrinsic, что ломает объявление из `<stdlib.h>`. |
| **Статическая, а не динамическая библиотека** | По ТЗ: `OS10_HTAPI.lib` встраивается в `.exe`, поэтому `dumpbin /dependents` не показывает её в зависимостях. |
| **Платформа ARM64** | Сборка под ARM64 (Windows 11 ARM64), тулсет v145; при `x64` проекты не соберутся без смены платформы. |

---

## Разбор реализации: Insert, Update, Delete

Три функции-мутатора. У них общий каркас (валидация → хэш → линейное пробирование → работа со слотом), но разная семантика записи.

### Общий каркас (все три функции)

```
1. Проверка параметров          → при ошибке SetLastError + FALSE/nullptr
2. startIndex = djb2(key) % Capacity
3. Линейное пробирование        → до Capacity шагов, с заворотом на 0
4. Работа со слотом по offset   → key[], payload[], keylength, payloadlength
```

Адрес слота считается арифметикой, без индексов и поиска:

```cpp
offset = 536 + (MaxKeyLength + MaxPayloadLength + 8) * index
```

---

### Insert — вставка нового элемента

```cpp
BOOL Insert(const HTHANDLE* h, const Element* element)
```

#### Шаг 1. Валидация — 7 условий в одном `if`

```cpp
if (!h || !element || !element->key || element->keylength <= 0 ||
    element->keylength > h->MaxKeyLength || !element->payload ||
    element->payloadlength <= 0 || element->payloadlength > h->MaxPayloadLength)
{
    SetLastError((HTHANDLE*)h, "Invalid parameters for Insert");
    return FALSE;
}
```

| Условие | Что ловит |
|---|---|
| `!h` | невалидный дескриптор хранилища |
| `!element` | передан `nullptr` вместо элемента |
| `!element->key` | ключ не задан |
| `keylength <= 0` | пустой ключ |
| `keylength > MaxKeyLength` | ключ не влезает в слот |
| `!element->payload` | значение не задано |
| `payloadlength <= 0` | пустое значение (API требует непустое) |
| `payloadlength > MaxPayloadLength` | значение не влезает в слот |

`!h` стоит первым — иначе `SetLastError((HTHANDLE*)h, ...)` разыменовал бы `nullptr` (внутри он проверяет `h && msg`, но порядок вычисления в C++ не гарантирует защиту при прямом обращении к полям).

#### Шаг 2. Хэш и стартовый слот

```cpp
int startIndex = HashFunction(element->key, element->keylength, h->Capacity);
int index = startIndex;
```

#### Шаг 3. Пробирование: два выхода и две ветки

```cpp
for (int i = 0; i < h->Capacity; i++)
{
    if (IsSlotEmpty(h, index))
    {
        // ВЕТКА А: нашли свободный слот → пишем сюда
    }

    // ВЕТКА Б: слот занят — проверяем, не тот же это ключ
    ...
    index = (index + 1) % h->Capacity;
    if (index == startIndex) break;
}
SetLastError((HTHANDLE*)h, "Storage full");
return FALSE;
```

Двойная защита от зацикливания:

- `i < h->Capacity` — не больше одного полного круга;
- `if (index == startIndex) break` — выходим, как только вернулись в начало.

Одного условия мало: первое ограничивает итерации, второе корректно обрабатывает заворот `(index + 1) % Capacity`.

#### Ветка А — запись в свободный слот

```cpp
size_t offset = GetElementOffset(h, index);
char* dest = (char*)h->Addr + offset;

memcpy(dest, element->key, element->keylength);
memset(dest + element->keylength, 0, h->MaxKeyLength - element->keylength);

memcpy(dest + h->MaxKeyLength, element->payload, element->payloadlength);
memset(dest + h->MaxKeyLength + element->payloadlength, 0, h->MaxPayloadLength - element->payloadlength);

*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength) = element->keylength;
*(int*)(dest + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = element->payloadlength;

return TRUE;
```

Что здесь важно:

- Копируются ровно `keylength` байт, **хвост слота забивается нулями** (`memset`). Это нужно, чтобы старые данные от ранее удалённого элемента не «просвечивали» и чтобы сравнение ключей было предсказуемым.
- `payload` пишется не в начало слота, а со смещением `+ MaxKeyLength` — слот всегда фиксированной ширины.
- Длины пишутся **после** данных, в конец слота: `keylength`, затем `payloadlength`.
- `payload` может быть бинарным (`void*`), поэтому используется `memcpy`, а не `strcpy`.

#### Ветка Б — проверка дубликата

```cpp
size_t offset = GetElementOffset(h, index);
char* existing = (char*)h->Addr + offset;
int existingKeyLength = *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength);

if (existingKeyLength == element->keylength && memcmp(existing, element->key, element->keylength) == 0)
{
    SetLastError((HTHANDLE*)h, "Key already exists");
    return FALSE;
}
```

Два условия, оба обязательны:

- `existingKeyLength == element->keylength` — сравнение длин отсекает почти все ложные совпадения (`"key"` и `"key1"` не равны, хотя первые 3 байта совпадают);
- `memcmp(...) == 0` — побайтовое сравнение ключа.

**Почему дубликат ищется именно во время пробирования, а не отдельным проходом:** ключ может лежать не в своём «хэш-слоте», а дальше по цепочке — туда его положил другой Insert при коллизии. Полный проход по цепочке и есть поиск.

**Почему пустой слот означает «ключа точно нет»:** в открытой адресации вставка всегда идёт в первый свободный слот от `startIndex`. Если на пути встретился пустой слот — значит, этот ключ никогда не вставлялся, иначе он занял бы именно его. Отсюда: при `IsSlotEmpty` поиск можно прекращать.

#### Шаг 4 — «Storage full»

Цикл дошёл до конца и не нашёл ни свободного слота, ни совпадения → таблица заполнена, вставка невозможна.

---

### Update — изменение значения по ключу

```cpp
BOOL Update(const HTHANDLE* h, const Element* oldElement, const void* newPayload, int newPayloadLength)
```

#### Валидация

```cpp
if (!h || !oldElement || !oldElement->key || oldElement->keylength <= 0 ||
    !newPayload || newPayloadLength <= 0 || newPayloadLength > h->MaxPayloadLength)
{
    SetLastError((HTHANDLE*)h, "Invalid parameters for Update");
    return FALSE;
}
```

Проверяется только **новая** длина значения. Длина ключа не ограничивается сверху — если она больше `MaxKeyLength`, ключ просто не будет найден (сравнение с `existingKeyLength` не совпадёт), и вернётся «Key not found».

#### Поиск элемента

Тот же цикл пробирования: `HashFunction` → пошаговый `(index + 1) % Capacity` → при пустом слоте выход с «Key not found».

#### Запись нового значения

```cpp
if (existingKeyLength == oldElement->keylength &&
    memcmp(existing, oldElement->key, oldElement->keylength) == 0)
{
    memcpy(existing + h->MaxKeyLength, newPayload, newPayloadLength);
    memset(existing + h->MaxKeyLength + newPayloadLength, 0,
           h->MaxPayloadLength - newPayloadLength);
    *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength + sizeof(int)) = newPayloadLength;
    return TRUE;
}
```

Четыре отличия от Insert:

| | Insert | Update |
|---|---|---|
| Ключ | копируется в слот | **не трогается** |
| `keylength` | записывается | **не меняется** |
| `payload` | копируется | перезаписывается |
| `payloadlength` | записывается | обновляется на новую длину |

**Почему элемент не переносится в другой слот.** Слот определяется хэшем **ключа**, а ключ не изменился — значит, `index` остаётся тем же. Никакого перепробирования и перемещения не нужно: это главное удобство обновления по ключу.

Смена длины значения безопасна, потому что `payload[]` — фиксированный массив `MaxPayloadLength`: новое значение короче — хвост забивается нулями, новое длиннее разрешённого — отсекается валидацией.

---

### Delete — удаление элемента

```cpp
BOOL Delete(const HTHANDLE* h, const Element* element)
```

#### Валидация — минимальная

```cpp
if (!h || !element || !element->key || element->keylength <= 0)
{
    SetLastError((HTHANDLE*)h, "Invalid parameters for Delete");
    return FALSE;
}
```

Значение (`payload`) не проверяется и не читается: удаление адресуется только ключом. Поэтому в вызывающем коде используется конструктор `Element(key, keylength)` без payload.

#### Поиск и «удаление»

```cpp
if (existingKeyLength == element->keylength && memcmp(existing, element->key, element->keylength) == 0)
{
    // Ленивое удаление: помечаем слот свободным, не перемещая данные
    *(int*)(existing + h->MaxKeyLength + h->MaxPayloadLength) = 0;
    return TRUE;
}
```

Удаление **ленивое (lazy)**: обнуляется только `keylength`, а байты ключа и значения остаются в файле. Ничего не сдвигается, свободные слоты не «схлопываются» — иначе пришлось бы перестраивать всю цепочку пробирования.

Почему `keylength = 0` — достаточный признак:

- `IsSlotEmpty` смотрит именно на это поле;
- нулевая длина не может встретиться у живого элемента (валидация требует `keylength > 0`);
- `memcmp` при сравнении с живым ключом даст ложное совпадение только если длины равны, а `0 != keylength > 0`.

#### Два следствия, о которых надо знать

**1. Данные остаются в файле.** После Delete значение физически лежит в образе и в файле — читается любым hex-редактором. Если хранилище использовалось для чувствительных данных, нужна затирка (`memset` области payload перед обнулением `keylength`). В текущей реализации её нет.

**2. Цепочка пробирования разрывается.** Пример: ёмкость 7, ключи `A`, `B` дают одинаковый `startIndex = 3`.

```
было:   [.., A(3), B(4), ..]        B найден: 3 → 4
Delete(A): [.., --(3), B(4), ..]    B НЕ найден: слот 3 пуст → выход
```

Поиск останавливается на первом пустом слоте, поэтому `B` становится недостижим, хотя физически лежит в слоте 4. Классическое решение — «надгробия» (tombstones): отдельное значение, помечающее «здесь был элемент, цепочку не рви». В этой реализации tombstones не применяются, и это задокументировано как известное ограничение.

---

### Сводка по трём функциям

| | Insert | Update | Delete |
|---|---|---|---|
| Что принимает | ключ + значение | ключ + новое значение | только ключ |
| Что пишет в слот | всё: ключ, значение, обе длины | только `payload` и `payloadlength` | только `keylength = 0` |
| Меняет позицию элемента | да (новый слот) | нет (тот же слот) | нет |
| Основная ошибка | «Key already exists» / «Storage full» | «Key not found» | «Key not found» |
| Сложность (средняя) | O(1) | O(1) | O(1) |
| Сложность (худшая) | O(Capacity) | O(Capacity) | O(Capacity) |

Средняя O(1) — при коэффициенте заполнения α = count / Capacity заметно меньше 1. Худшая O(Capacity) — когда все ключи коллизируют и таблица почти заполнена.

---

## Test Case HT API

Задание 04 требует «Разработайте Test Case HT API» и тесты, соответствующие ему. Ниже — таблица тест-кейсов, реализованных в `windows/Tests/Tests.cpp`.

| ID | Цель | Шаги | Ожидаемый результат |
|---|---|---|---|
| TC-01 | Создание и открытие хранилища | `Create` → проверить поля → `Close` → `Open` | Ненулевой `HTHANDLE`; `Capacity`, `SecSnapshotInterval`, `MaxKeyLength`, `MaxPayloadLength`, `FileName` совпадают с заданными; `File`, `FileMapping`, `Addr` валидны. `Open` возвращает хранилище с теми же параметрами; `Open` несуществующего файла → `NULL` |
| TC-02 | Вставка и чтение | `Insert("key1")` → `Get("key1")` | `Insert` = TRUE; `Get` возвращает элемент с длиной ключа 4, длиной значения 6 и совпадающим содержимым. Повторный `Insert` того же ключа → FALSE; `Get` отсутствующего ключа → `NULL` |
| TC-03 | Удаление | `Insert` → `Delete` → `Get` | `Delete` = TRUE; `Get` после удаления → `NULL`; `Delete` отсутствующего ключа → FALSE |
| TC-04 | Изменение значения | `Insert` → `Update` → `Get` | `Update` = TRUE; `Get` возвращает новое значение нужной длины; `Update` отсутствующего ключа → FALSE |
| TC-05 | Snapshot | `Insert` → `Snap` → проверить `lastsnaptime` → `Close` → `Open` | `Snap` = TRUE; `lastsnaptime` обновился (не 0); после `Open` данные на месте |
| TC-06 | Коллизии | Ёмкость 7, вставить 5 ключей | Все 5 ключей вставлены и читаются; вставка в заполненную таблицу → FALSE |
| TC-07 | Тексты ошибок | Некорректные `Insert` и `Get` | `GetLastError` возвращает непустую строку после каждой ошибки |
| TC-08 | Персистентность | `Create` → `Insert` → `Close` → `Open` → `Get` | После переоткрытия ключ и значение совпадают |
| TC-09 | Граничные размеры | Вставка ключа 32 байта и значения 128 байт; затем 33 и 129 | Максимальные размеры принимаются; превышение → FALSE |
| TC-10 | Потокобезопасность | 2 потока × 100 `Insert` в одно хранилище → 100 `Get` | Все ключи читаются, потерь нет; счётчик вставок соответствует ожидаемому |
| TC-11 | Асинхронный snapshot | `Create` с `SecSnapshotInterval = 1` → `Sleep(2500)` без вызова `Snap` | `snapshotsmade` вырос — фоновый поток сработал сам |

Запуск: `Tests.exe` → в конце `ALL TESTS PASSED!` и сводка `Passed / Failed / Total`.

Комментариев в исходниках нет намеренно: все пояснения к реализации собраны в этом README, код остаётся чистым.

---

## Порядок защиты и типовые вопросы

### Порядок демонстрации
1. **OS10_01** — запуск, вывод, «реализация внутри экзе»
2. **OS10_02** — запуск, «использует OS10_HTAPI.lib, тот же результат»
3. **OS10_03** — запуск, работа с двумя HT параллельно
4. **Tests** — «ALL TESTS PASSED» (63/63 на прогоне из 9 тестов; после добавления 2 тестов ожидается 70/70)
5. **Код** — `HT.cpp`: `CreateFile`/`CreateFileMapping`/`MapViewOfFile`, структура памяти, хэш + линейное пробирование, `FlushViewOfFile`/`FlushFileBuffers` в `Snap`, корректное закрытие в `Close`, `Open` с восстановлением заголовка

### Типовые вопросы преподавателя

| Вопрос | Ответ |
|---|---|
| Что такое File Mapping? | Отображение файла в виртуальную память. Работа как с массивом — быстрее `ReadFile`/`WriteFile`, ОС управляет кэшем. |
| Какие Windows API используются? | `CreateFile`, `CreateFileMapping`, `MapViewOfFile`, `FlushViewOfFile`, `UnmapViewOfFile`, `CloseHandle`, `FlushFileBuffers`. |
| Чем `CreateFileMapping` ≠ `MapViewOfFile`? | Первое создаёт section object в ядре. Второе отображает его в адресное пространство — возвращает указатель для чтения/записи. |
| Как устроена хэш-таблица с открытой адресацией? | Массив элементов. Хэш → начальный индекс. Занят → линейно `(i+1)%capacity`. Поиск до пустого слота. |
| Как определяется пустой слот? | `keylength == 0`. Удаление = обнуление `keylength` (lazy deletion — корректно для открытой адресации). |
| Что делает `Snap`? | `FlushViewOfFile` сбрасывает грязные страницы в файл. `FlushFileBuffers` гарантирует запись на диск. Без Snap — потеря данных при краше. |
| Почему `Close` вызывает `Snap`? | Гарантия сохранности изменений перед закрытием дескрипторов. |
| Как работает `Open`? | `CreateFile(OPEN_EXISTING)` → читает заголовок (536 байт) → создаёт mapping того же размера → отображает → заполняет `HTHANDLE`. |
| Превышение MaxKeyLength/MaxPayloadLength? | FALSE, `GetLastError` — сообщение. В память ничего не пишется. |
| Почему статическая библиотека, а не DLL? | ТЗ: «Разработайте статическую библиотеку OS10_HTAPI». Код встраивается в exe, отдельного файла не нужно. |
| Как проверить данные на диске? | Hex-редактор после `Close` — виден заголовок и элементы. После `Open`+`Get` — читаются. |
| Несуществующий файл? | `CreateFile` → `INVALID_HANDLE_VALUE`, `Open` → NULL. |
| Можно ли использовать HT из нескольких процессов? | Текущая реализация — нет (нет синхронизации). Нужен named mutex, `SEC_COMMIT`, координация Snap/Close. |

---

## Вывод

Лабораторная работа демонстрирует:
- Работу с **File Mapping** на Windows (`CreateFileMapping`, `MapViewOfFile`)
- Реализацию **хэш-таблицы с открытой адресацией** в отображённой памяти
- **Персистентность** через snapshot-механизм (`FlushViewOfFile` + `FlushFileBuffers`)
- Разделение на **статическую библиотеку** и прикладные приложения
- Автотестирование функционала (11 тест-кейсов)

Все четыре задания выполняют одинаковую бизнес-логику (CRUD + Snap), отличаются только способом подключения HT API (инлайн / статическая либа / два экземпляра / тесты).