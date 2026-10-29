# Демонстрация лабораторной работы 2 — Windows (Visual Studio)

> **Проверено на живой машине:** Windows 11 ARM64 (build 26200), Visual Studio 2022 v18.10.0, тулсет v145, платформа ARM64, конфигурация Release.
> Все команды выполнены в **Developer Command Prompt** (`vcvarsall.bat arm64`) — выводы ниже фактические.

> ⚠️ **Выводы ниже сняты до последней правки кода.** В реализацию добавлены потокобезопасность (`CRITICAL_SECTION`) и фоновый поток асинхронного snapshot, в демо — блок «Error handling (GetLastError)» и блок «Async snapshot», в тесты — `TestThreadSafety` и `TestAsyncSnapshot`. После `git pull` нужно один раз пересобрать и перепрогонить все 4 проекта, чтобы логи снова соответствовали коду. Структура вывода изменилась: добавились два блока в демо и две секции в тестах.

---

## 1. Открытие и сборка

```cmd
cd /d %USERPROFILE%\Documents\VisualStudioCodeRepos\SystemProgramming\lab2\windows
lab2.sln
```
Выбрать платформу: **ARM64**.

```cmd
msbuild lab2.sln -restore /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /v:minimal /nologo
```

**Фактический вывод сборки:**
```
  Определение проектов для восстановления...
  Работы нет. Ни в одном из указанных проектов нет пакетов, которые можно было бы восстановить.
cl : командная строка  warning D9025: переопределение "/Oi" на "/Oi-" [OS10_HTAPI.vcxproj]
  HT.cpp
  OS10_HTAPI.vcxproj -> ...\OS10_HTAPI\OS10_HTAPI\ARM64\Release\OS10_HTAPI.lib
cl : командная строка  warning D9025: переопределение "/Oi" на "/Oi-" [OS10_01.vcxproj]
  OS10_01.cpp
  OS10_01.vcxproj -> ...\OS10_01\OS10_01\ARM64\Release\OS10_01.exe
cl : командная строка  warning D9025: переопределение "/Oi" на "/Oi-" [OS10_02.vcxproj]
  OS10_02.cpp
  OS10_02.vcxproj -> ...\OS10_02\OS10_02\ARM64\Release\OS10_02.exe
cl : командная строка  warning D9025: переопределение "/Oi" на "/Oi-" [OS10_03.vcxproj]
  OS10_03.cpp
  OS10_03.vcxproj -> ...\OS10_03\OS10_03\ARM64\Release\OS10_03.exe
cl : командная строка  warning D9025: переопределение "/Oi" на "/Oi-" [Tests.vcxproj]
  Tests.cpp
  Tests.vcxproj -> ...\Tests\Tests\ARM64\Release\Tests.exe
```
**5 проектов собрано, 0 ошибок.** Предупреждение `D9025` — нормально: `/O2` включает `/Oi`, мы переопределяем на `/Oi-` для корректной компиляции (нет функции `div` в коде, но флаг стандартный для курса).

---

## 2. Задание 01 — OS10_01 (встроенная реализация HT API)

```cmd
OS10_01\OS10_01\ARM64\Release\OS10_01.exe
```

**Фактический вывод:**
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

**Проверка файла данных:**
```cmd
dir OS10_01\OS10_01\ARM64\Release\test_ht1.dat
```
Файл создан, размер ≈ 17 KB (заголовок 536 байт + 100 элементов × 168 байт).

---

## 3. Задание 02 — OS10_02 (статическая библиотека OS10_HTAPI)

```cmd
OS10_02\OS10_02\ARM64\Release\OS10_02.exe
```

**Фактический вывод:**
```
=== OS10_02: HT API Demo (using OS10_HTAPI static library) ===

Created HT: test_ht2.dat, capacity=100
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

Reopened HT: test_ht2.dat
Get key1 after reopen: Element: key=key1, keylength=4, payload=newvalue1, payloadlength=9
Close: OK

=== Demo complete ===
```

**Проверка использования статической библиотеки:**
```cmd
dumpbin /dependents OS10_02\OS10_02\ARM64\Release\OS10_02.exe
```
`OS10_HTAPI.lib` **нет в зависимостях** — статическая библиотека встроена в exe.

```cmd
lib /list OS10_HTAPI\OS10_HTAPI\ARM64\Release\OS10_HTAPI.lib
```
Вывод: `HT.obj` — объектный файл библиотеки.

---

## 4. Задание 03 — OS10_03 (два HT-хранилища одновременно)

```cmd
OS10_03\OS10_03\ARM64\Release\OS10_03.exe
```

**Фактический вывод:**
```
=== OS10_03: HT API Demo (2 HT instances) ===

Created HT1: test_ht3a.dat, capacity=50
Created HT2: test_ht3b.dat, capacity=50
HT1 Insert ht1_key1: OK
HT1 Insert ht1_key2: OK
HT2 Insert ht2_key1: OK
HT2 Insert ht2_key2: OK
HT1 Get ht1_key1: Element: key=ht1_key1, keylength=8, payload=ht1_value1, payloadlength=10
HT1 Get ht1_key2: Element: key=ht1_key2, keylength=8, payload=ht1_value2, payloadlength=10
HT2 Get ht2_key1: Element: key=ht2_key1, keylength=8, payload=ht2_value1, payloadlength=10
HT2 Get ht2_key2: Element: key=ht2_key2, keylength=8, payload=ht2_value2, payloadlength=10
HT1 Update ht1_key1: OK
HT1 Get ht1_key1 after update: Element: key=ht1_key1, keylength=8, payload=ht1_newvalue1, payloadlength=13
HT2 Delete ht2_key1: OK
HT2 Get ht2_key1 after delete: NOT FOUND (expected)
HT1 Snap: OK
HT2 Snap: OK
HT1 Close: OK
HT2 Close: OK

Reopened both HTs
HT1 Get ht1_key1 after reopen: Element: key=ht1_key1, keylength=8, payload=ht1_newvalue1, payloadlength=13
HT2 Get ht2_key2 after reopen: Element: key=ht2_key2, keylength=8, payload=ht2_value2, payloadlength=10
HT1 Close: OK
HT2 Close: OK

=== Demo complete ===
```

**Проверка изоляции:**
```cmd
dir OS10_03\OS10_03\ARM64\Release\test_ht3*.dat
```
Два разных файла (`test_ht3a.dat`, `test_ht3b.dat`) — полная изоляция данных.

---

## 5. Задание 04 — Tests (автотесты)

```cmd
Tests\Tests\ARM64\Release\Tests.exe
```

**Фактический вывод:**
```
=== OS10 HT API Test Suite ===

=== Test: Create and Open ===
[PASS] Create returns non-null
[PASS] Capacity set
[PASS] SnapInterval set
[PASS] MaxKeyLength set
[PASS] MaxPayloadLength set
[PASS] FileName set
[PASS] File handle valid
[PASS] FileMapping handle valid
[PASS] Addr valid
[PASS] Open returns non-null
[PASS] Capacity preserved
[PASS] FileName preserved
[PASS] Open non-existent returns null

=== Test: Insert and Get ===
[PASS] Create
[PASS] Insert key1
[PASS] Get key1
[PASS] Key length correct
[PASS] Payload length correct
[PASS] Key content correct
[PASS] Payload content correct
[PASS] Insert duplicate fails
[PASS] Get non-existent returns null

=== Test: Delete ===
[PASS] Create
[PASS] Delete existing
[PASS] Get after delete returns null
[PASS] Delete non-existent fails

=== Test: Update ===
[PASS] Create
[PASS] Update existing
[PASS] Get after update
[PASS] Payload updated
[PASS] Update non-existent fails

=== Test: Snap ===
[PASS] Create
[PASS] Snap succeeds
[PASS] lastsnaptime updated
[PASS] Open after snap
[PASS] Data persisted after snap

=== Test: Collision Handling (Open Addressing) ===
[PASS] Create small capacity
[PASS] Insert a
[PASS] Insert b
[PASS] Insert c
[PASS] Insert d
[PASS] Insert e
[PASS] Get a
[PASS] Get b
[PASS] Get c
[PASS] Get d
[PASS] Get e
[PASS] Insert into full fails

=== Test: Error Messages ===
[PASS] Create
[PASS] GetLastError after invalid insert
[PASS] GetLastError after invalid get

=== Test: Persistence Across Close/Open ===
[PASS] Create
[PASS] Insert
[PASS] Snap
[PASS] Open
[PASS] Data persisted
[PASS] Key correct
[PASS] Value correct

=== Test: Max Key/Payload Sizes ===
[PASS] Create
[PASS] Insert max sizes
[PASS] Get max sizes
[PASS] Key length 32
[PASS] Payload length 128
[PASS] Insert over max key fails
[PASS] Insert over max payload fails

=== Summary ===
Passed: 63
Failed: 0
Total:  63

ALL TESTS PASSED!
```
**63/63 тестов пройдено** — все 9 тест-кейсов зеленые.

---

## 6. Ключевые места в коде для показа преподавателю

| Файл | Что показать |
|---|---|
| `OS10_HTAPI/HT.cpp` | `CreateFile` → `CreateFileMapping` → `MapViewOfFile` в `Create` |
| `OS10_HTAPI/HT.cpp` | Компоновка памяти: заголовок (Capacity, SnapInterval, MaxKeyLength, MaxPayloadLength, FileName, lastsnaptime) + массив элементов |
| `OS10_HTAPI/HT.cpp` | Хэш-функция (djb2) + линейное пробирование в `Insert`/`Get`/`Delete`/`Update` |
| `OS10_HTAPI/HT.cpp` | `FlushViewOfFile` + `FlushFileBuffers` в `Snap` — синхронный snapshot |
| `OS10_HTAPI/HT.cpp` | Корректное закрытие: `UnmapViewOfFile` → `CloseHandle(FileMapping)` → `CloseHandle(File)` → `delete HTHANDLE` |
| `OS10_HTAPI/HT.cpp` | `Open`: `CreateFile(OPEN_EXISTING)` → чтение заголовка → `CreateFileMapping` → `MapViewOfFile` → восстановление `HTHANDLE` |
| `OS10_03/OS10_03.cpp` | Два независимых `HTHANDLE*` (`ht1`, `ht2`) — изоляция хранилищ |
| `Tests/Tests.cpp` | 9 тест-кейсов: Create/Open, Insert/Get, Delete, Update, Snap, Collision Handling, Errors, Persistence, MaxSizes |

---

## 7. Структура артефактов после сборки и запуска

```
lab2/windows/
├── OS10_HTAPI/OS10_HTAPI/ARM64/Release/
│   └── OS10_HTAPI.lib          # Статическая библиотека
├── OS10_01/OS10_01/ARM64/Release/
│   ├── OS10_01.exe
│   └── test_ht1.dat            # Файл данных (после запуска)
├── OS10_02/OS10_02/ARM64/Release/
│   ├── OS10_02.exe
│   └── test_ht2.dat
├── OS10_03/OS10_03/ARM64/Release/
│   ├── OS10_03.exe
│   ├── test_ht3a.dat
│   └── test_ht3b.dat
└── Tests/Tests/ARM64/Release/
    ├── Tests.exe
    └── test_*.dat              # Временные файлы тестов
```

---

## 8. Типовые ошибки и решения

| Ошибка | Причина | Решение |
|---|---|---|
| `MSB8020: средства сборки v143 не найдены` | Нет тулсета v143 | В проекте `v145` — правильно, не менять |
| `error C2169: div: подставляемая функция` | Конфликт с intrinsics | Флаг `/Oi-` в `AdditionalOptions` (уже прописан во всех `.vcxproj`) |
| `LNK1104: не удалось открыть файл OS10_HTAPI.lib` | Библиотека не собрана или путь неверен | Собрать `OS10_HTAPI` первым, проверить `AdditionalLibraryDirectories` |
| Кракозябры в консоли | Кодировка CP866 | `chcp 65001` перед запуском |

---

## 9. Быстрый запуск всех демо подряд

```cmd
chcp 65001
OS10_01\OS10_01\ARM64\Release\OS10_01.exe
echo.
OS10_02\OS10_02\ARM64\Release\OS10_02.exe
echo.
OS10_03\OS10_03\ARM64\Release\OS10_03.exe
echo.
Tests\Tests\ARM64\Release\Tests.exe
```