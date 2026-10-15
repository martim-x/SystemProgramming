# Демонстрация лабораторной работы 2 — Windows (Visual Studio)

> Все команды проверены на Windows 11 ARM64, Visual Studio 2022, платформа ARM64, конфигурация Release.

---

## 1. Открытие решения

```cmd
cd /d %USERPROFILE%\Documents\VisualStudioCodeRepos\SystemProgramming\lab2\windows
lab2.sln
```
Выбрать платформу: **ARM64** (в выпадающем списке рядом с Debug/Release).

---

## 2. Сборка всего решения

```cmd
msbuild lab2.sln -restore /t:Rebuild /p:Configuration=Release /p:Platform=ARM64
```

Ожидаемый результат: **5 проектов собрано, 0 ошибок**.

---

## 3. Задание 01 — OS10_01 (встроенная реализация)

### Запуск
В Visual Studio: правый клик на `OS10_01` → **Set as Startup Project** → `Ctrl+F5`.

Или из терминала (Developer Command Prompt):
```cmd
OS10_01\OS10_01\ARM64\Release\OS10_01.exe
```

### Ожидаемый вывод
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

### Проверка файла на диске
```cmd
dir OS10_01\OS10_01\ARM64\Release\test_ht1.dat
```
Файл создан, размер ~ (536 + 100 * 168) байт ≈ 17 KB.

---

## 4. Задание 02 — OS10_02 (статическая библиотека OS10_HTAPI)

### Запуск
Правый клик на `OS10_02` → **Set as Startup Project** → `Ctrl+F5`.

Или:
```cmd
OS10_02\OS10_02\ARM64\Release\OS10_02.exe
```

### Ожидаемый вывод
Идентичен OS10_01, только файл данных — `test_ht2.dat`.

### Проверка, что используется библиотека
```cmd
dumpbin /dependents OS10_02\OS10_02\ARM64\Release\OS10_02.exe
```
`OS10_HTAPI.lib` **не будет в списке** — статическая библиотека встроена в exe.

```cmd
lib /list OS10_HTAPI\OS10_HTAPI\ARM64\Release\OS10_HTAPI.lib
```
Покажет `HT.obj` — объектный файл библиотеки.

---

## 5. Задание 03 — OS10_03 (два HT одновременно)

### Запуск
Правый клик на `OS10_03` → **Set as Startup Project** → `Ctrl+F5`.

Или:
```cmd
OS10_03\OS10_03\ARM64\Release\OS10_03.exe
```

### Ожидаемый вывод (фрагмент)
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

### Проверка изоляции
```cmd
dir OS10_03\OS10_03\ARM64\Release\test_ht3*.dat
```
Два разных файла, каждый со своей данными.

---

## 6. Задание 04 — Tests (автотесты)

### Запуск
Правый клик на `Tests` → **Set as Startup Project** → `Ctrl+F5`.

Или:
```cmd
Tests\Tests\ARM64\Release\Tests.exe
```

### Ожидаемый вывод
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
[PASS] Insert key1
[PASS] Get key1
[PASS] Key length correct
[PASS] Payload length correct
[PASS] Key content correct
[PASS] Payload content correct
[PASS] Insert duplicate fails
[PASS] Get non-existent returns null

=== Test: Delete ===
[PASS] Delete existing
[PASS] Get after delete returns null
[PASS] Delete non-existent fails

=== Test: Update ===
[PASS] Update existing
[PASS] Get after update
[PASS] Payload updated
[PASS] Update non-existent fails

=== Test: Snap ===
[PASS] Snap succeeds
[PASS] lastsnaptime updated
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
Passed: 47
Failed: 0
Total:  47

ALL TESTS PASSED!
```

---

## 7. Проверка экспортов библиотеки (для справки)

Статическая библиотека не имеет экспортов в смысле DLL, но можно посмотреть символы:
```cmd
dumpbin /symbols OS10_HTAPI\OS10_HTAPI\ARM64\Release\OS10_HTAPI.lib | findstr "HT::"
```
Покажет символы функций: `Create`, `Open`, `Insert`, `Delete`, `Get`, `Update`, `Snap`, `Close`, `GetLastError`, `print`.

---

## 8. Типовые ошибки и решения

| Ошибка | Причина | Решение |
|---|---|---|
| `MSB8020: средства сборки v143 не найдены` | Нет тулсета v143 | В проекте стоит `v145` — правильно, не менять |
| `error C2169: div: подставляемая функция` | Имя `div` конфликтует с intrinsics | Флаг `/Oi-` в `AdditionalOptions` (уже прописан) |
| `DllNotFoundException` / `SP.dll not found` | Не скопировалась нативная DLL | Здесь нет DLL — статическая линковка, проблема не актуальна |
| `LNK1104: не удалось открыть файл OS10_HTAPI.lib` | Библиотека не собрана или путь неверен | Собрать `OS10_HTAPI` первым, проверить `AdditionalLibraryDirectories` |
| Кракозябры в консоли | Кодировка CP866 вместо UTF-8 | `chcp 65001` перед запуском |

---

## 9. Структура артефактов после сборки

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

## 10. Ключевые места в коде для показа преподавателю

| Файл | Что показать |
|---|---|
| `OS10_HTAPI/HT.cpp` | `CreateFile` → `CreateFileMapping` → `MapViewOfFile` в `Create` |
| `OS10_HTAPI/HT.cpp` | Компоновка памяти: заголовок + массив элементов |
| `OS10_HTAPI/HT.cpp` | Хэш-функция (djb2) + линейное пробирование в `Insert`/`Get`/`Delete`/`Update` |
| `OS10_HTAPI/HT.cpp` | `FlushViewOfFile` + `FlushFileBuffers` в `Snap` |
| `OS10_HTAPI/HT.cpp` | Корректное закрытие: `UnmapViewOfFile` → `CloseHandle(FileMapping)` → `CloseHandle(File)` |
| `OS10_HTAPI/HT.cpp` | `Open`: чтение заголовка, восстановление `HTHANDLE` |
| `OS10_03/OS10_03.cpp` | Два независимых `HTHANDLE*`, изоляция данных |
| `Tests/Tests.cpp` | 9 тест-кейсов, покрывающие все функции API |

---

## 11. Быстрый запуск всех демо подряд (из терминала VS)

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