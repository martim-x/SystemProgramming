# Демонстрация лабораторной работы 1 — Windows (Visual Studio)

> **Проверено на живой машине:** Windows 11 ARM64 (build 26200), Visual Studio 2022 v18.10.0, тулсет v145, платформа ARM64, конфигурация Release.
> Все команды ниже выполнены в **Developer Command Prompt** (`vcvarsall.bat arm64`) и дали показанный вывод.

---

## 1. Открытие решения

```cmd
cd /d %USERPROFILE%\Documents\VisualStudioCodeRepos\SystemProgramming\lab1\windows
lab1.sln
```
Выбрать платформу: **ARM64** (не x64!).

---

## 2. Сборка (clean rebuild)

```cmd
msbuild lab1.sln -restore /t:Rebuild /p:Configuration=Release /p:Platform=ARM64 /v:minimal /nologo
```

**Результат (фактический вывод):**
```
  Определение проектов для восстановления...
  Все проекты обновлены для восстановления.
  pch.cpp
  SP.cpp
  dllmain.cpp
     Создается библиотека ...\SP_lib\SP\ARM64\Release\SP.lib и объект ...\SP.exp
  SP.vcxproj -> ...\SP_lib\SP\ARM64\Release\SP.dll
  main.cpp
  SP_01.vcxproj -> ...\SP_01\SP_01\ARM64\Release\SP_01.exe
  main.cpp
  SP_02.vcxproj -> ...\SP_02\SP_02\ARM64\Release\SP_02.exe
  SP_05 -> ...\cs\SP_05\bin\Release\net10.0\SP_05.dll
  SP_06 -> ...\cs\SP_06\bin\Release\net10.0\SP_06.dll
```
**5 проектов собрано, 0 ошибок.**

---

## 3. Проверка экспорта `div` (ключевой момент защиты)

```cmd
dumpbin /exports SP_lib\SP\ARM64\Release\SP.dll
```

**Фактический вывод:**
```
Dump of file ...\SP_lib\SP\ARM64\Release\SP.dll

  Section contains the following exports for SP.dll

    ordinal hint RVA      name

          1    0 00001EAC div
          2    1 00001624 mul
          3    2 000019C0 sub
          4    3 00001534 sum
```
**Имя `div` экспортировано чисто** — без манглинга, без `.def`, без алиасов.

---

## 4. Задание 01 — статическая линковка (SP_01)

### Import table (доказательство статической линковки)
```cmd
dumpbin /dependents SP_01\SP_01\ARM64\Release\SP_01.exe
```

**Вывод:**
```
    SP.dll
    VCRUNTIME140.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    KERNEL32.dll
```
`SP.dll` **есть** в списке зависимостей — загрузчик Windows загрузит её при старте.

### Запуск
```cmd
SP_01\SP_01\ARM64\Release\SP_01.exe
```

**Фактический вывод:**
```
SP_01: static link (SP.lib)
sum(10, 5)  = 15
sub(10, 5)  = 5
mul(10, 5)  = 50
div(10, 5)  = 2
```

---

## 5. Задание 02 — динамическая линковка (SP_02)

### Import table (доказательство динамической линковки)
```cmd
dumpbin /dependents SP_02\SP_02\ARM64\Release\SP_02.exe
```

**Вывод:**
```
    VCRUNTIME140.dll
    api-ms-win-crt-stdio-l1-1-0.dll
    KERNEL32.dll
```
`SP.dll` **НЕТ** в списке — бинарник о ней ничего не знает, загружает сам через `LoadLibrary`.

### Запуск
```cmd
SP_02\SP_02\ARM64\Release\SP_02.exe
```

**Фактический вывод:**
```
SP_02: dynamic link (LoadLibrary)
sum(10, 5)  = 15
sub(10, 5)  = 5
mul(10, 5)  = 50
div(10, 5)  = 2
```

---

## 6. Задания 05 и 06 — C# P/Invoke

| Проект | Запуск | Вывод |
|---|---|---|
| `SP_05` | `cs\SP_05\bin\Release\net10.0\SP_05.exe` | `DllImport("SP")` — статический вызов |
| `SP_06` | `cs\SP_06\bin\Release\net10.0\SP_06.exe` | `NativeLibrary.Load("SP.dll")` — динамический |

Оба выдают:
```
Сложение в C++/DLL/SO: 15
Вычитание в C++/DLL/SO: 5
Умножение в C++/DLL/SO: 50
Деление в C++/DLL/SO: 2
```

---

## 7. Почему `div` работает — разбор решения (для защиты)

### Проблема
В `<stdlib.h>` (тянется через `<windows.h>`) объявлен **intrinsic**:
```c
div_t div(int numer, int denom);   // возвращает частное И остаток
```
При `/O2` MSVC включает intrinsics. Наша `int div(int,int)` — конфликт типов → `error C2169`.

### Два независимых фикса (оба обязательны)

| Фикс | Где | Что делает |
|---|---|---|
| **1. `/Oi-`** | `SP.vcxproj` → `<AdditionalOptions>/Oi- %(AdditionalOptions)</AdditionalOptions>` | Отключает intrinsics компилятора. `IntrinsicFunctions=false` **не работает** в этом MSBuild — не доходит до `cl.exe` (проверено `/v:diag`). |
| **2. Изоляция `<windows.h>`** | `pch.h` включает **только `<stdio.h>`**; `dllmain.cpp` включает `<windows.h>` с `<PrecompiledHeader>NotUsing</PrecompiledHeader>` | `SP.cpp` не видит объявление intrinsic `div` → компилятор считает нашу функцию обычной внешней. |

### Почему не «обычные» костыли

| Подход | Почему плохо |
|---|---|
| `.def` с `EXPORTS div=_div@8` | Имя в экспорте не `div`, лишний файл, легаси |
| `#pragma function(div)` | Экспорт ломается |
| `namespace` / `#define` | Не помогают — intrinsic видит имя |
| Переименование | Нарушает ТЗ |

---

## 8. Команды для быстрой проверки на защите

```cmd
REM 1. Экспорты — чистое имя div
dumpbin /exports SP_lib\SP\ARM64\Release\SP.dll

REM 2. Статическая линковка — SP.dll в зависимостях
dumpbin /dependents SP_01\SP_01\ARM64\Release\SP_01.exe

REM 3. Динамическая линковка — SP.dll НЕТ в зависимостях
dumpbin /dependents SP_02\SP_02\ARM64\Release\SP_02.exe

REM 4. Проверка флага /Oi- в командной строке компилятора
msbuild SP_lib\SP.vcxproj /v:diag 2>&1 | findstr /C:"cl.exe" | findstr /Oi-
```

---

## 9. Структура артефактов после сборки

```
lab1/windows/
├── SP_lib/SP/ARM64/Release/
│   ├── SP.dll
│   └── SP.lib
├── SP_01/SP_01/ARM64/Release/
│   ├── SP_01.exe
│   └── SP.dll (скопировалась автоматически)
├── SP_02/SP_02/ARM64/Release/
│   ├── SP_02.exe
│   └── SP.dll (скопировалась автоматически)
├── cs/SP_05/bin/Release/net10.0/
│   ├── SP_05.exe
│   └── SP.dll
└── cs/SP_06/bin/Release/net10.0/
    ├── SP_06.exe
    └── SP.dll
```

---

## 10. Типовые вопросы преподавателя — краткие ответы

| Вопрос | Ответ |
|---|---|
| Зачем `dllmain.cpp` отдельно? | Изоляция `<windows.h>` от PCH, иначе C2169 |
| Почему `/Oi-` в `AdditionalOptions`? | `IntrinsicFunctions=false` не передаётся в `cl.exe` (проверено) |
| Можно переименовать `div`? | Нет — ТЗ требует это имя |
| Почему не `.def`? | Легаси, грязный экспорт, лишний файл, не переносимо |
| Что если убрать `/O2`? | Потеря оптимизации. `/Oi-` позволяет оставить `/O2` |

---

## 11. Итог

**Два минимальных изменения решают проблему, на которой спотыкаются все:**

1. **Структурное:** `dllmain.cpp` + `NotUsing` PCH — чистая архитектура, Windows API не утекает в бизнес-код
2. **Конфигурационное:** `/Oi-` через `AdditionalOptions` — точечное отключение intrinsics

Никакого технического долга (`.def`, переименования, прагмы). Тот же `SP.cpp` собирается и на Linux (`g++ -fPIC -shared`) без изменений.