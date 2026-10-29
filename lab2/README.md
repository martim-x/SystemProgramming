# Лабораторная работа 2 — File Mapping (HT API)

**Системное программирование** (преп. Смелов В.В.), ПИ+ИС-3, 4 часа.
Задание: `labs/Лабораторная_2_FileMapping/Лабораторная_2_FileMapping_HT.pdf`
API спецификация: `labs/Лабораторная_2_FileMapping/HT.h`

---

## Содержание

- [Теория и архитектура](#теория-и-архитектура)
- [Сводка терминов (File Mapping)](#сводка-терминов-file-mapping)
- [Как запустить проект](#как-запустить-проект)
- [Демонстрация (фактические выводы)](#демонстрация-фактические-выводы)
- [HT API — детали реализации](#ht-api--детали-реализации)
- [Особенности программной реализации (с кодом)](#особенности-программной-реализации-с-кодом)
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
3. Элемент: `key[MaxKeyLength]` + `payload[MaxPayloadLength]` + `keylength(int)` + `payloadlength(int)`
4. Хэш: djb2 (`hash = hash * 33 + byte`) по ключу, модуль Capacity
5. Коллизии — линейное пробирование: `(index + 1) % Capacity`
6. Пустой слот = `keylength == 0` (lazy deletion — корректно для открытой адресации)
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
│   └── OS10_01.cpp               # Полная реализация HT API внутри
├── OS10_02/                      # Задание 02: использует OS10_HTAPI.lib
│   ├── OS10_02.vcxproj
│   └── OS10_02.cpp
├── OS10_03/                      # Задание 03: 2 экземпляра HT
│   ├── OS10_03.vcxproj
│   └── OS10_03.cpp
└── Tests/                        # Задание 04: автотесты
    ├── Tests.vcxproj
    └── Tests.cpp                 # 9 тест-кейсов
```

### Карта заданий
| № | Проект | Описание |
|---|---|---|
| 01 | OS10_01 | Реализует HT API инлайн, демонстрирует все функции |
| 02 | OS10_02 | Использует статическую библиотеку `OS10_HTAPI.lib`, повторяет OS10_01 |
| 03 | OS10_03 | Использует `OS10_HTAPI.lib`, работает с **2 HT одновременно** |
| 04 | Tests | Набор автотестов (9 тест-кейсов) для проверки HT API |

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

---

## Демонстрация (фактические выводы)

**Все выводы ниже получены на живой машине Windows 11 ARM64, тулсет v145, конфигурация Release.**

> **Полные логи запуска всех 4 проектов:** см. [`windows/DEMONSTRATION-WINDOWS.md`](windows/DEMONSTRATION-WINDOWS.md)

### Краткая сводка результатов

| Проект | Статус | Ключевой результат |
|---|---|---|
| **OS10_01** (inline) | ✅ | Полный цикл Create/Insert/Get/Update/Delete/Snap/Close/Open — данные персистят |
| **OS10_02** (static lib) | ✅ | Идентичен OS10_01, использует `OS10_HTAPI.lib` |
| **OS10_03** (2 HT) | ✅ | Два независимых хранилища работают одновременно, изоляция подтверждена |
| **Tests** (9 тестов) | ✅ | **63/63 passed** — все тест-кейсы зеленые |

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
| **Нет синхронизации** | В `HTHANDLE` нет `CRITICAL_SECTION`; одновременный доступ из нескольких потоков/процессов не защищён. Требование ТЗ «потокобезопасное HT API» в этой реализации не выполнено — это выносится как известное ограничение (в ЛР11 добавляется mutex). |
| **Файл расширяется mapping'ом** | `CreateFileMapping` с явным размером увеличивает файл до этого размера; поэтому `Create` не вызывает `SetEndOfFile` вручную. |
| **`FILE_SHARE_READ \| FILE_SHARE_WRITE`** | Без флагов совместного доступа второй `CreateFile` в `Open` из другого процесса получил бы `ERROR_SHARING_VIOLATION`. |
| **Размер элемента фиксирован** | `MaxKeyLength + MaxPayloadLength + 8` — переменные по длине ключи/значения не поддерживаются, зато смещение любого элемента вычисляется арифметикой, без индексов. |
| **Заголовок 536 байт** | 4 × `int` (16) + `FileName[512]` + `time_t` (8) = 536. Это значение фигурирует и в `Create`, и в `Open`, и в тестах. |
| **`Get` возвращает копию** | `Get` делает `malloc` и копирует ключ/значение в кучу вызывающего — вызывающая сторона обязана освободить память. |
| **Флаг `/Oi-`** | Конфликт intrinsics с `div_t div(int,int)`: `cl` подставляет `div` как intrinsic, что ломает объявление из `<stdlib.h>`. |
| **Статическая, а не динамическая библиотека** | По ТЗ: `OS10_HTAPI.lib` встраивается в `.exe`, поэтому `dumpbin /dependents` не показывает её в зависимостях. |
| **Платформа ARM64** | Сборка под ARM64 (Windows 11 ARM64), тулсет v145; при `x64` проекты не соберутся без смены платформы. |

---

## Порядок защиты и типовые вопросы

### Порядок демонстрации
1. **OS10_01** — запуск, вывод, «реализация внутри экзе»
2. **OS10_02** — запуск, «использует OS10_HTAPI.lib, тот же результат»
3. **OS10_03** — запуск, работа с двумя HT параллельно
4. **Tests** — «ALL TESTS PASSED» (63/63)
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
- Автотестирование функционала (9 тест-кейсов, 63 проверки)

Все четыре задания выполняют одинаковую бизнес-логику (CRUD + Snap), отличаются только способом подключения HT API (инлайн / статическая либа / два экземпляра / тесты).