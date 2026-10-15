# Лабораторная работа 2 — File Mapping (HT API)

**Системное программирование** (преп. Смелов В.В.), ПИ+ИС-3, 4 часа.
Задание: `labs/Лабораторная_работа_02 (4 часа)/ОССП_Лабораторная_работа_10_FileMapping.pdf`
API спецификация: `labs/Лабораторная_работа_02 (4 часа)/ОССП_Лабораторная_работа_10_HT.h`

---

## Содержание

- [Теория и архитектура](#теория-и-архитектура)
- [Как запустить проект](#как-запустить-проект)
- [Демонстрация (фактические выводы)](#демонстрация-фактические-выводы)
- [HT API — детали реализации](#ht-api--детали-реализации)
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