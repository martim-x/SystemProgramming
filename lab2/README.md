# Лабораторная работа 2 — File Mapping (HT API)

**Системное программирование** (преп. Смелов В.В.), ПИ+ИС-3, 4 часа.
Задание: `labs/Лабораторная_работа_02 (4 часа)/ОССП_Лабораторная_работа_10_FileMapping.pdf`
API спецификация: `labs/Лабораторная_работа_02 (4 часа)/ОССП_Лабораторная_работа_10_HT.h`

## Содержание

- [Как запустить проект](#как-запустить-проект)
- [Быстрый старт](#быстрый-старт)
- [Архитектура решения](#архитектура-решения)
- [Задание 01 — OS10_01 (встроенная реализация)](#задание-01--os10_01-встроенная-реализация)
- [Задание 02 — OS10_02 (статическая библиотека OS10_HTAPI)](#задание-02--os10_02-статическая-библиотека-os10_htapi)
- [Задание 03 — OS10_03 (два экземпляра HT)](#задание-03--os10_03-два-экземпляра-ht)
- [Задание 04 — Tests (тестовый набор)](#задание-04--tests-тестовый-набор)
- [HT API — детали реализации](#ht-api--детали-реализации)

---

## Как запустить проект

Среда: **Visual Studio 2022** с компонентом **«Разработка классических приложений на C++»**.
Платформа: **ARM64** (на машине Windows 11 ARM64).

Файл решения: `lab2/windows/lab2.sln`. Открыть двойным кликом.

**Перед первой сборкой** выбери платформу: панель сверху, где написано `Debug` → выпадающий список → **ARM64**. По умолчанию Visual Studio ставит `x64`, а проекты собраны под `ARM64`.

Собрать всё сразу: **Build → Build Solution** (`Ctrl+Shift+B`). Соберутся пять проектов:
- `OS10_HTAPI` — статическая библиотека
- `OS10_01` — приложение с встроенной реализацией HT API
- `OS10_02` — приложение, использующее `OS10_HTAPI.lib`
- `OS10_03` — приложение с двумя HT-хранилищами
- `Tests` — автотесты HT API

Порядок сборки подхватится автоматически через зависимости проекта.

### Если сборка ругается на NETSDK1004 / MSB8020

```cmd
msbuild lab2.sln -restore /t:Rebuild /p:Configuration=Release /p:Platform=ARM64
```

Запустить: правый клик по проекту → **Set as Startup Project** → `Ctrl+F5`.

### Что где лежит после сборки

| Что | Путь |
|---|---|
| `OS10_HTAPI.lib` | `windows\OS10_HTAPI\OS10_HTAPI\ARM64\Release\` |
| `OS10_01.exe` | `windows\OS10_01\OS10_01\ARM64\Release\` |
| `OS10_02.exe` | `windows\OS10_02\OS10_02\ARM64\Release\` |
| `OS10_03.exe` | `windows\OS10_03\OS10_03\ARM64\Release\` |
| `Tests.exe` | `windows\Tests\Tests\ARM64\Release\` |

---

## Быстрый старт

Задание — реализовать **HT-хранилище (Hash Table)** с использованием **File Mapping** (отображение файла в память) на Windows.

**HT API функции:**
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

**Хранение:** файл на диске + отображение в память (`CreateFileMapping` + `MapViewOfFile`). Данные организованы как хэш-таблица с **открытой адресацией** (линейное пробирование). Персистентность — через `FlushViewOfFile` + `FlushFileBuffers` (snapshot).

Карта заданий:

| № | Проект | Описание |
|---|---|---|
| 01 | OS10_01 | Реализует HT API инлайн, демонстрирует все функции |
| 02 | OS10_02 | Использует статическую библиотеку `OS10_HTAPI.lib`, повторяет OS10_01 |
| 03 | OS10_03 | Использует `OS10_HTAPI.lib`, работает с **2 HT одновременно** |
| 04 | Tests | Набор автотестов для проверки HT API |

---

## Архитектура решения

```
lab2/windows/
├── lab2.sln                    # Файл решения Visual Studio
├── OS10_HTAPI/                 # Статическая библиотека HT API
│   ├── OS10_HTAPI.vcxproj
│   ├── HT.h                    # Заголовочный файл (из задания)
│   └── HT.cpp                  # Реализация на Windows File Mapping
├── OS10_01/                    # Задание 01: встроенная реализация
│   ├── OS10_01.vcxproj
│   └── OS10_01.cpp             # Полная реализация HT API внутри
├── OS10_02/                    # Задание 02: использует OS10_HTAPI.lib
│   ├── OS10_02.vcxproj
│   └── OS10_02.cpp
├── OS10_03/                    # Задание 03: 2 экземпляра HT
│   ├── OS10_03.vcxproj
│   └── OS10_03.cpp
└── Tests/                      # Задание 04: автотесты
    ├── Tests.vcxproj
    └── Tests.cpp               # 9 тест-кейсов
```

**Принцип File Mapping в реализации:**

1. `Create` → `CreateFile` (CREATE_ALWAYS) → `CreateFileMapping` (PAGE_READWRITE) → `MapViewOfFile` (FILE_MAP_ALL_ACCESS)
2. В отображённую память записывается заголовок (Capacity, SnapInterval, MaxKeyLength, MaxPayloadLength, FileName, lastsnaptime) + массив элементов
3. Каждый элемент: `key[MaxKeyLength]` + `payload[MaxPayloadLength]` + `keylength(int)` + `payloadlength(int)`
4. Хэш-функция: djb2 (`hash = hash * 33 + byte`) по ключу, модуль Capacity
5. Коллизии — линейное пробирование: `(index + 1) % Capacity`
6. Пустой слот = `keylength == 0`
7. `Snap` → `FlushViewOfFile` + `FlushFileBuffers` + обновление `lastsnaptime`
8. `Close` → `Snap` → `UnmapViewOfFile` → `CloseHandle(FileMapping)` → `CloseHandle(File)` → `delete HTHANDLE`
9. `Open` → `CreateFile` (OPEN_EXISTING) → читать заголовок → `CreateFileMapping` → `MapViewOfFile` → восстановить `HTHANDLE`

---

## Задание 01 — OS10_01 (встроенная реализация)

**ТЗ:** Приложение `OS10_01.exe`, реализующее HT API и демонстрирующее все функции.

**Файл:** `OS10_01/OS10_01.cpp` — содержит полную реализацию namespace `HT` (те же функции, что в библиотеке, но инлайн).

**Демонстрация:**
1. Create HT (файл `test_ht1.dat`, capacity=100)
2. Insert 3 элемента
3. Get элементов
4. Update одного
5. Delete одного
6. Snap
7. Close
8. Open существующего
9. Get после повторного открытия (проверка персистентности)

**Ожидаемый вывод:**
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

---

## Задание 02 — OS10_02 (статическая библиотека OS10_HTAPI)

**ТЗ:** Статическая библиотека `OS10_HTAPI.lib` + приложение `OS10_02.exe`, функционально повторяющее OS10_01, но использующее библиотеку.

**Файлы:**
- `OS10_HTAPI/HT.h`, `HT.cpp` — реализация библиотеки
- `OS10_HTAPI/OS10_HTAPI.vcxproj` — тип `StaticLibrary`
- `OS10_02/OS10_02.cpp` — подключает `HT.h`, линкуется с `OS10_HTAPI.lib`

**Настройки линковки в OS10_02.vcxproj:**
```xml
<AdditionalIncludeDirectories>..\OS10_HTAPI;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
<AdditionalLibraryDirectories>..\OS10_HTAPI\OS10_HTAPI\$(Platform)\$(Configuration);</AdditionalLibraryDirectories>
<AdditionalDependencies>OS10_HTAPI.lib</AdditionalDependencies>
```

**Демонстрация:** идентична OS10_01, файл данных `test_ht2.dat`.

---

## Задание 03 — OS10_03 (два экземпляра HT)

**ТЗ:** Приложение `OS10_03.exe`, использующее `OS10_HTAPI.lib` и работающее с **2 HT-хранилищами одновременно**.

**Файл:** `OS10_03/OS10_03.cpp`

**Демонстрация:**
1. Create HT1 (`test_ht3a.dat`) и HT2 (`test_ht3b.dat`)
2. Insert в оба
3. Get из обоих
4. Update в HT1, Delete в HT2
5. Snap обоих
6. Close обоих
7. Open обоих → Get (проверка персистентности обоих)

**Ожидаемый вывод показывает изоляцию:** операции с HT1 не влияют на HT2 и наоборот.

---

## Задание 04 — Tests (тестовый набор)

**ТЗ:** Test Case HT API + тесты + демонстрация выполнения.

**Файл:** `Tests/Tests.cpp` — 9 тест-кейсов:

| Тест | Проверяет |
|---|---|
| `TestCreateOpen` | Create/Open, сохранение параметров, открытие несуществующего |
| `TestInsertGet` | Insert, Get, дубликат ключа, несуществующий ключ |
| `TestDelete` | Delete существующего, Get после удаления, удаление несуществующего |
| `TestUpdate` | Update существующего, Get после обновления, обновление несуществующего |
| `TestSnap` | Snap, обновление lastsnaptime, персистентность после Snap+Close+Open |
| `TestCollisionHandling` | Малая capacity (5), 5 вставок → коллизии, все читаются, вставка в полное — fail |
| `TestErrorMessages` | GetLastError после некорректных операций |
| `TestPersistenceAcrossCloseOpen` | Полный цикл Create → Insert → Snap → Close → Open → Get |
| `TestMaxSizes` | MaxKeyLength=32, MaxPayloadLength=128, превышение — fail |

**Запуск:** `Tests.exe` в консоли.

**Ожидаемый вывод:**
```
=== OS10 HT API Test Suite ===

=== Test: Create and Open ===
[PASS] Create returns non-null
[PASS] Capacity set
...
[PASS] Open non-existent returns null

=== Test: Insert and Get ===
[PASS] Insert key1
[PASS] Get key1
[PASS] Key length correct
...
[PASS] Get non-existent returns null

...

=== Summary ===
Passed: 47
Failed: 0
Total:  47

ALL TESTS PASSED!
```

---

## HT API — детали реализации

### Структуры данных

```cpp
struct HTHANDLE {
    int Capacity;               // емкость (кол-во слотов)
    int SecSnapshotInterval;    // интервал авто-snap (сек, не используется в демо)
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

В текущей реализации **нет синхронизации** (Critical Section / Mutex). Для многопоточного доступа нужно добавить `CRITICAL_SECTION` в `HTHANDLE` и `EnterCriticalSection`/`LeaveCriticalSection` в каждой функции API. Это оставлено как расширение.

### Обработка ошибок

- Все функции возвращают `BOOL` (TRUE/FALSE) или указатель (NULL = ошибка)
- Текст ошибки записывается в `HTHANDLE->LastErrorMessage`
- `GetLastError(ht)` возвращает указатель на сообщение или NULL

### Флаг `/Oi-`

Во всех `.vcxproj` добавлен:
```xml
<AdditionalOptions>/Oi- %(AdditionalOptions)</AdditionalOptions>
```
Это отключает встроенные функции компилятора (intrinsics), иначе функция с именем `div` не скомпилируется (конфликт с `div_t div(int,int)` из `<stdlib.h>`).

---

## Порядок демонстрации на защите

1. **OS10_01** — запуск, показать вывод, объяснить: «реализация внутри экзе»
2. **OS10_02** — запуск, показать вывод, указать: «использует OS10_HTAPI.lib, тот же результат»
3. **OS10_03** — запуск, показать работу с двумя HT параллельно
4. **Tests** — запуск, показать «ALL TESTS PASSED»
5. **Код** — открыть `HT.cpp`, показать:
   - `CreateFile` / `CreateFileMapping` / `MapViewOfFile`
   - структуру памяти, хэш-функцию, линейное пробирование
   - `FlushViewOfFile` / `FlushFileBuffers` в `Snap`
   - корректное закрытие дескрипторов в `Close`
6. **Вопросы** — см. ниже.

---

## Типовые вопросы преподавателя и ответы

**1. Что такое File Mapping и зачем он нужен?**
Отображение файла в виртуальную память процесса. Позволяет работать с файлом как с массивом в памяти — быстрее, чем `ReadFile`/`WriteFile`, ОС сама управляет кэшированием и подкачкой.

**2. Какие функции Windows API используются?**
`CreateFile`, `CreateFileMapping`, `MapViewOfFile`, `FlushViewOfFile`, `UnmapViewOfFile`, `CloseHandle`. Для синхронизации — `FlushFileBuffers`.

**3. Чем `CreateFileMapping` отличается от `MapViewOfFile`?**
`CreateFileMapping` создаёт объект отображения (section object) в ядре. `MapViewOfFile` отображает его (или часть) в адресное пространство процесса — возвращает указатель, по которому можно читать/писать.

**4. Как устроена хэш-таблица с открытой адресацией?**
Все элементы хранятся в одном массиве. Хэш от ключа даёт начальный индекс. Если слот занят — линейно ищем следующий свободный (`(i+1) % capacity`). Поиск идет по той же схеме до пустого слота или цикла.

**5. Как определяется, что слот пустой?**
`keylength == 0`. При удалении просто обнуляем `keylength` (lazy deletion — для открытой адресации это корректно, так как поиск идёт до пустого слота).

**6. Что делает `Snap` и зачем он нужен?**
`FlushViewOfFile` сбрасывает грязные страницы из кэша ОС в файл. `FlushFileBuffers` гарантирует запись на диск (пробивает кэш диска). Без Snap данные могут потеряться при краше.

**7. Почему `Close` вызывает `Snap`?**
Чтобы гарантировать сохранность всех изменений перед закрытием дескрипторов.

**8. Как работает `Open`?**
Открывает существующий файл, читает заголовок (первые 536 байт) чтобы узнать параметры, создаёт mapping того же размера, отображает, заполняет `HTHANDLE`.

**9. Что будет, если превысить MaxKeyLength / MaxPayloadLength?**
Функции вернут FALSE, `GetLastError` вернёт сообщение об ошибке. В памяти ничего не запишется.

**10. Почему статическая библиотека, а не DLL?**
В задании указано: «Разработайте статическую библиотеку OS10_HTAPI». Статическая линковка — код библиотеки встраивается в exe, отдельного файла библиотеки не нужно при запуске.

**11. Как проверить, что данные реально на диске?**
После `Close` открыть файл в шестнадцатеричном редакторе — виден заголовок и элементы. После `Open` + `Get` данные читаются.

**12. Что произойдёт при попытке открыть несуществующий файл?**
`CreateFile` вернёт `INVALID_HANDLE_VALUE`, `Open` вернёт NULL.

**13. Можно ли использовать один HT из нескольких процессов?**
Текущая реализация — нет (нет синхронизации). Для межпроцессного доступа нужно: именованный mutex, флаг `SEC_COMMIT` в `CreateFileMapping`, координация `Snap`/`Close`.

---

## Вывод

Лабораторная работа демонстрирует:
- Работу с **File Mapping** на Windows (`CreateFileMapping`, `MapViewOfFile`)
- Реализацию **хэш-таблицы с открытой адресацией** в отображённой памяти
- **Персистентность** через snapshot-механизм (`FlushViewOfFile` + `FlushFileBuffers`)
- Разделение на **статическую библиотеку** и прикладные приложения
- Автотестирование функционала

Все четыре задания выполняют одинаковую бизнес-логику (CRUD + Snap), отличаются только способом подключения HT API (инлайн / статическая либа / два экземпляра / тесты).