# Лабораторная работа 1 — Динамические библиотеки DLL/SO

**Системное программирование** (преп. Смелов В.В.), ПИ+ИС-3, 2 часа.
Задание: `labs/СП_Лабораторная_работа_1_DLL_SO.pdf` — 8 заданий.
Лекции: `Лекция_СП_DLLXX.pdf`, `Лекция_СП_SOXX.pdf`.

## Содержание

- [Как запустить проект](#как-запустить-проект)
- [Быстрый старт](#быстрый-старт)
- [Часть I. ПРАКТИКА](#часть-i-практика)
  - [Установка инструментов (Arch Linux ARM64)](#установка-инструментов-arch-linux-arm64)
  - [Код, который нужен везде](#код-который-нужен-везде)
  - [Windows — задания 01, 02, 05, 06](#windows--задания-01-02-05-06)
  - [Linux — задания 03, 04, 07, 08](#linux--задания-03-04-07-08)
  - [Одна команда вместо всех шагов](#одна-команда-вместо-всех-шагов)
- [Часть II. ТЕОРИЯ](#часть-ii-теория)
  - [Задание в целом](#задание-в-целом)
  - [Статическая и динамическая линковка](#статическая-и-динамическая-линковка)
  - [Таблица вызовов на двух ОС](#таблица-вызовов-на-двух-ос)
  - [Почему нужен extern "C"](#почему-нужен-extern-c)
  - [Зачем .lib для DLL и почему он не нужен для .so](#зачем-lib-для-dll-и-почему-он-не-нужен-для-so)
  - [Флаги компилятора](#флаги-компилятора)
  - [Что делает DllMain](#что-делает-dllmain)
  - [Конструктор и деструктор .so](#конструктор-и-деструктор-so)
  - [Как из C вызвать C#-код и наоборот](#как-из-c-вызвать-c-код-и-наоборот)
  - [Вопросы преподавателя и ответы](#вопросы-преподавателя-и-ответы)

> **Для показа на защите**
> - Linux: `ДЕМОНСТРАЦИЯ.md` — готовые команды с пояснениями к ключам
> - Windows: `ДЕМОНСТРАЦИЯ-WINDOWS.md` — то же самое для Visual Studio
> - Разбор типовых ошибок: `ДЕМОНСТРАЦИЯ.md`, раздел «Ошибки»

---

# Как запустить проект

Обе части проверялись на живой машине: Windows 11 ARM64 и Arch Linux ARM64.
Все пути ниже — фактические, из рабочей сборки.

## Windows — через Visual Studio

Файл решения: `lab1/windows/lab1.sln`. Открыть двойным кликом.

**Перед первой сборкой** выбери платформу: панель сверху, где написано `Debug` →
выпадающий список → **ARM64**. По умолчанию Visual Studio ставит `x64`, а проекты
собраны под `ARM64`.

Собрать всё сразу: **Build → Build Solution** (`Ctrl+Shift+B`). Соберутся пять
проектов: `SP` (библиотека), `SP_01`, `SP_02`, `SP_05`, `SP_06`. Порядок сборки
подхватится сам через `ProjectReference`.

Запустить: правый клик по проекту → **Set as Startup Project** → `Ctrl+F5`.

### Что где лежит после сборки

| Что | Путь |
|---|---|
| `SP.dll` + `SP.lib` | `windows\SP_lib\SP\ARM64\Release\` |
| `SP_01.exe` + `SP.dll` | `windows\SP_01\SP_01\ARM64\Release\` |
| `SP_02.exe` + `SP.dll` | `windows\SP_02\SP_02\ARM64\Release\` |
| `SP_05.exe` + `SP.dll` | `windows\cs\SP_05\bin\Release\net10.0\win-arm64\` |
| `SP_06.exe` + `SP.dll` | `windows\cs\SP_06\bin\Release\net10.0\win-arm64\` |

`SP.dll` копируется сам при сборке — специально ничего делать не нужно.

> C#-проекты собраны под `net10.0`, потому что на машине стоит SDK `10.0.401`.
> Linux-проекты остались на `net8.0` — там SDK `8.0.425`.

### Запуск из командной строки

Открой `View → Terminal` в Visual Studio, папка `Command Prompt`:

```cmd
windows\SP_01\SP_01\ARM64\Release\SP_01.exe
windows\SP_02\SP_02\ARM64\Release\SP_02.exe
windows\cs\SP_05\bin\Release\net10.0\win-arm64\SP_05.exe
windows\cs\SP_06\bin\Release\net10.0\win-arm64\SP_06.exe
```

Если кириллица в выводе — кракозябры, переключи кодировку:

```cmd
chcp 65001
```

## Linux — через терминал

Рабочая папка: `lab1/`. В новом терминале один раз:

```bash
export DOTNET_ROOT="$HOME/.dotnet"; export PATH="$HOME/.dotnet:$PATH"
```

Полные команды — в файле `ДЕМОНСТРАЦИЯ.md`, по блоку на задание.
Коротко — всё сразу:

```bash
export DOTNET_ROOT="$HOME/.dotnet"; export PATH="$HOME/.dotnet:$PATH"
L=~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1

(cd $L/linux/SP_lib && g++ -fPIC -shared SP.cpp -o SP.so)

(cd $L/linux/SP_03 && cp ../SP_lib/SP.so . && g++ main.cpp SP.so -o SP_03 && LD_LIBRARY_PATH=. ./SP_03)

(cd $L/linux/SP_04 && cp ../SP_lib/SP.so . && g++ main.cpp -ldl -o SP_04 && ./SP_04)

(cd $L/linux/cs/SP_07 && dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true -v q --nologo && cd bin/Release/net8.0/linux-arm64/publish && cp $L/linux/SP_lib/SP.so . && ./SP_07)

(cd $L/linux/cs/SP_08 && dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true -v q --nologo && cd bin/Release/net8.0/linux-arm64/publish && cp $L/linux/SP_lib/SP.so . && ./SP_08)
```

Ждёт сборки всех четырёх Linux-заданий: `SP_03`, `SP_04`, `SP_07`, `SP_08`.
У всех результат `15`, `5`, `50`, `2`. Займёт ~2 минуты, первый `dotnet publish` дольше.

---

# Быстрый старт

Задание — собрать одну библиотеку функций и вызвать её **8 разными способами**:
из C++ и из C#, на Windows и на Linux, статически и динамически.

Функции из таблицы 1 задания:

| Функция | Результат |
|---|---|
| `int sum(int x, int y)` | `x + y` |
| `int sub(int x, int y)` | `x - y` |
| `int mul(int x, int y)` | `x * y` |
| `int div(int x, int y)` | `x / y`, при `y == 0` → `0` |

Карта заданий:

| № | ОС | Язык | Тип вызова | Результат |
|---|---|---|---|---|
| 01 | Windows | C++ | статический | `SP_01.exe` |
| 02 | Windows | C++ | динамический (`LoadLibrary`) | `SP_02.exe` |
| 03 | Linux | C++ | статический | `SP_03` |
| 04 | Linux | C++ | динамический (`dlopen`) | `SP_04` |
| 05 | Windows | C# | статический (`DllImport`) | `SP_05.exe` |
| 06 | Windows | C# | динамический (`NativeLibrary`) | `SP_06.exe` |
| 07 | Linux | C# | статический (`DllImport`) | `SP_07` |
| 08 | Linux | C# | динамический (`dlopen`) | `SP_08` |

---

# Часть I. ПРАКТИКА

## Установка инструментов (Arch Linux ARM64)

Официального пакета `dotnet-sdk` в репозиториях Arch Linux ARM нет — в
`archlinux.org` он только под `x86_64`, в `archlinuxarm` его не собирают.
**Компилировать .NET из исходников не нужно**: Microsoft публикует готовые
бинарники `linux-arm64`, ставится официальным скриптом.

Готовый скрипт в репозитории — `setup_dotnet.sh`:

```bash
chmod +x setup_dotnet.sh
./setup_dotnet.sh          # .NET 8 (по умолчанию)
./setup_dotnet.sh 10.0     # другая версия
```

Что он делает: проверяет `aarch64` → ставит `gcc curl ca-certificates icu` →
скачивает `dotnet-install.sh` → ставит SDK в `~/.dotnet` → прописывает PATH в
`~/.bashrc` → проверяет результат.

Вручную то же самое:

```bash
sudo pacman -S --needed gcc curl ca-certificates icu

cd ~
curl -sSL https://dot.net/v1/dotnet-install.sh -o dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --channel 8.0 --install-dir "$HOME/.dotnet"

echo 'export DOTNET_ROOT="$HOME/.dotnet"' >> ~/.bashrc
echo 'export PATH="$HOME/.dotnet:$PATH"' >> ~/.bashrc
```

**Пакет `icu` обязателен.** Без него SDK скачается, но первый запуск упадёт:

```
Couldn't find a valid ICU package installed on the system
```

Проверка после установки (открой новый терминал):

```bash
uname -m          # aarch64
g++ --version     # g++ (GCC) 16.1.1
dotnet --version  # 8.0.425
```

## Код, который нужен везде

Три файла библиотеки. Кладутся и в `windows/SP_lib/`, и в `linux/SP_lib/`.

> В лекции преподаватель переносит их на Linux-машину через
> `scp ".\*.cpp"` и `scp ".\*.h"` — то есть `.h` тоже едут.

**`pch.h`**

```cpp
#if !defined(PCH_H) && defined( _WIN32)
#define PCH_H

#include "framework.h"

#endif
```

**`framework.h`**

```cpp
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
```

**`SP.cpp`**

```cpp
#include "pch.h"

#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)

BOOL APIENTRY DllMain( HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH: break;
	}
	return TRUE;
}

#else
#define EXPORT extern "C"
#endif

EXPORT int sum(int x, int y) { return x + y; }
EXPORT int sub(int x, int y) { return x - y; }
EXPORT int mul(int x, int y) { return x * y; }
EXPORT int div(int x, int y) { return y != 0 ? x / y : 0; }
```

Три файла, а не один — как в лекции. `pch.h` при `_WIN32` тянет `framework.h` →
`windows.h`, а на Linux эта ветка пустая, поэтому один исходник собирается
на обеих ОС.

---

## Windows — задания 01, 02, 05, 06

Среда: Visual Studio 2022 с компонентами «Разработка классических приложений на C++»
и «Разработка приложений на .NET». Платформа одна и та же для всех проектов.

### Задание 01 — DLL + статический вызов из C++

**Требуется по ТЗ:** DLL с функциями из табл. 1; библиотека импорта; приложение
`SP_01.exe`, статически вызывающее функции; продемонстрировать.

**Шаг 1.** `New Project → C++ → Dynamic-Link Library (DLL)`, имя проекта `SP`.

**Шаг 2.** Проверь настройки PCH — `C/C++ → Precompiled Headers → Use Precompiled
Header`, **не меняй**. В шаблоне проекта DLL уже есть `pch.h` и `framework.h`,
мы просто **дописываем в них содержимое** из раздела выше. Не удаляй их: `SP.cpp`
начинается с `#include "pch.h"`.

**Шаг 3.** Замени содержимое `SP.cpp` кодом из раздела «Код, который нужен везде».

**Шаг 4.** `Build → Build Solution`. В `SP_lib\SP\ARM64\Release\` появятся **`SP.dll`** и **`SP.lib`** (п. 1 ТЗ)
и **`SP.lib`** (п. 2 ТЗ — библиотека импорта). Проверить экспорты:
`Project → Outputs → View → Module Viewer` или `dumpbin /exports SP_lib\SP\ARM64\Release\SP.dll`.

**Шаг 5.** `New Project → C++ → Console App`, имя `SP_01`. Код `main.cpp`:

```cpp
#include <stdio.h>

extern "C" int sum(int x, int y);
extern "C" int sub(int x, int y);
extern "C" int mul(int x, int y);
extern "C" int div(int x, int y);

int main(int argc, char* argv[])
{
    printf("SP_01: static link (SP.lib)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", div(10, 5));
    return 0;
}
```

**Шаг 6.** Свойства проекта `SP_01`:

```
Linker → General → Additional Library Directories:  ..\SP_lib\SP\ARM64\Release
Linker → Input → Additional Dependencies:          SP.lib
```

**Шаг 7.** `Build Solution` — `SP.dll` скопируется в `SP_01\SP_01\ARM64\Release\` автоматически.

**Шаг 8.** `Ctrl+F5`:

```
SP_01: static link (SP.lib)
sum(10, 5)  = 15
sub(10, 5)  = 5
mul(10, 5)  = 50
div(10, 5)  = 2
```

Нигде не вызывался `LoadLibrary` — загрузчик Windows сам подгрузил DLL,
потому что она прописана в таблице импортов exe.

### Задание 02 — динамический вызов из C++ через LoadLibrary

**Требуется по ТЗ:** приложение `SP_02.exe`, вызывающее функции из табл. 1
через `LoadLibrary`; продемонстрировать.

**Шаг 1.** `New Project → C++ → Console App`, имя `SP_02`. Код `main.cpp`:

```cpp
#include <stdio.h>
#include <windows.h>

typedef int (*BinFunc)(int, int);

int main(int argc, char* argv[])
{
    HMODULE hDll = LoadLibraryA("SP.dll");
    if (!hDll)
    {
        printf("LoadLibrary FAILED, error = %lu\n", GetLastError());
        return 1;
    }

    BinFunc sum  = (BinFunc)GetProcAddress(hDll, "sum");
    BinFunc sub  = (BinFunc)GetProcAddress(hDll, "sub");
    BinFunc mul  = (BinFunc)GetProcAddress(hDll, "mul");
    BinFunc divf = (BinFunc)GetProcAddress(hDll, "div");

    if (!sum || !sub || !mul || !divf)
    {
        printf("GetProcAddress FAILED\n");
        FreeLibrary(hDll);
        return 1;
    }

    printf("SP_02: dynamic link (LoadLibrary)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", divf(10, 5));

    FreeLibrary(hDll);
    return 0;
}
```

**Шаг 2.** `Build Solution` — `SP.dll` скопируется в `SP_02\SP_02\ARM64\Release\` автоматически. **Настройки линковки не нужны** —
в этом суть задания: библиотека не упоминается ни в одном свойстве проекта.

**Шаг 3.** `Ctrl+F5`:

```
SP_02: dynamic link (LoadLibrary)
sum(10, 5)  = 15
...
```

**Шаг 4.** Покажи разницу:

| | `SP_01` (статически) | `SP_02` (динамически) |
|---|---|---|
| `Additional Dependencies` | `SP.lib` | **пусто** |
| Таблица импортов exe | да, из `.lib` | нет |
| Когда грузится библиотека | загрузчиком при старте | вызовом `LoadLibrary` |
| Запустится без библиотеки | нет | да |

Проверка: `dumpbin /imports SP_01.exe` (найдёшь `SP.dll`) и
`dumpbin /imports SP_02.exe` (её не будет).

### Задание 05 — статический вызов из C#

**Требуется по ТЗ:** C#-приложение `SP_05`, статически вызывающее функции из
табл. 1 библиотеки `SP_XX.dll`; продемонстрировать.

**Шаг 1.** `New → C# → Console App`, имя `SP_05`, Framework `.NET 8.0`.
Замени `Program.cs`:

```csharp
using System;
using System.Runtime.InteropServices;

internal class Program
{
    [DllImport(@"SP", CallingConvention = CallingConvention.Cdecl)]
    public static extern int sum(int x, int y);

    [DllImport(@"SP", CallingConvention = CallingConvention.Cdecl)]
    public static extern int sub(int x, int y);

    [DllImport(@"SP", CallingConvention = CallingConvention.Cdecl)]
    public static extern int mul(int x, int y);

    [DllImport(@"SP", CallingConvention = CallingConvention.Cdecl)]
    public static extern int div(int x, int y);

    private static void Main(string[] args)
    {
        Console.WriteLine($"Сложение в C++/DLL/SO: {sum(10, 5)}");
        Console.WriteLine($"Вычитание в C++/DLL/SO: {sub(10, 5)}");
        Console.WriteLine($"Умножение в C++/DLL/SO: {mul(10, 5)}");
        Console.WriteLine($"Деление в C++/DLL/SO: {div(10, 5)}");
    }
}
```

**Шаг 2.** `Build Solution` — `SP.dll` скопируется в `cs\SP_05\bin\Release\net10.0\win-arm64\` рядом со `SP_05.exe`.

**Шаг 3.** `Ctrl+F5`:

```
Сложение в C++/DLL/SO: 15
Вычитание в C++/DLL/SO: 5
Умножение в C++/DLL/SO: 50
Деление в C++/DLL/SO: 2
```

Без скопированной DLL будет `System.DllNotFoundException`.

### Задание 06 — динамический вызов из C#

**Требуется по ТЗ:** C#-приложение `SP_06`, динамически вызывающее функции из
табл. 1 библиотеки `SP_XX.dll`; продемонстрировать.

**Шаг 1.** `New → C# → Console App`, имя `SP_06`, Framework `.NET 8.0`.
`Program.cs`:

```csharp
using System;
using System.Runtime.InteropServices;

internal class Program
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int BinFunc(int x, int y);

    private static void Main(string[] args)
    {
        IntPtr hDll = NativeLibrary.Load("SP.dll");
        if (hDll == IntPtr.Zero)
        {
            Console.WriteLine("LoadLibrary FAILED");
            return;
        }

        BinFunc sum  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "sum"));
        BinFunc sub  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "sub"));
        BinFunc mul  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "mul"));
        BinFunc divf = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "div"));

        Console.WriteLine($"Сложение в C++/DLL/SO: {sum(10, 5)}");
        Console.WriteLine($"Вычитание в C++/DLL/SO: {sub(10, 5)}");
        Console.WriteLine($"Умножение в C++/DLL/SO: {mul(10, 5)}");
        Console.WriteLine($"Деление в C++/DLL/SO: {divf(10, 5)}");

        NativeLibrary.Free(hDll);
    }
}
```

**Шаг 2.** `Build Solution` — `SP.dll` скопируется в `cs\SP_06\bin\Release\net10.0\win-arm64\` автоматически.

**Шаг 3.** `Ctrl+F5`. Вывод такой же, как у `SP_05`.

Разница: в `SP_05` адреса функций подставил компилятор через `DllImport`,
в `SP_06` мы сами в runtime спрашиваем адрес и превращаем в делегат.

---

## Linux — задания 03, 04, 07, 08

Всё в терминале, IDE не нужна.

### Задание 03 — SP.so + статический вызов

**Требуется по ТЗ:** библиотека `SP_XX.so` с функциями из табл. 1; приложение
`SP_03`, статически вызывающее функции; продемонстрировать.

**Шаг 1.** Собрать библиотеку:

```bash
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/SP_lib

g++ -c SP.cpp -o SP.o
g++ -shared SP.o -o SP.so
```

Одной командой: `g++ -fPIC -shared SP.cpp -o SP.so`.

Проверить экспорты:

```bash
nm -D SP.so | grep -E ' T (sum|sub|mul|div)$'
```

```
0000000000000604 T sum
0000000000000624 T sub
0000000000000644 T mul
0000000000000664 T div
```

**Шаг 2.** Собрать приложение:

```bash
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/SP_03
cp ../SP_lib/SP.so .
g++ main.cpp SP.so -o SP_03
```

Код `main.cpp` такой же, как у `SP_01`, но первая строка вывода:

```cpp
printf("SP_03: static link (SP.so)\n");
```

Проверить, что библиотека прилинкована:

```bash
readelf -d SP_03 | grep NEEDED
```

```
0x0000000000000001 (NEEDED)  Shared library: [SP.so]
0x0000000000000001 (NEEDED)  Shared library: [libstdc++.so.6]
0x0000000000000001 (NEEDED)  Shared library: [libm.so.6]
0x0000000000000001 (NEEDED)  Shared library: [libgcc_s.so.1]
0x0000000000000001 (NEEDED)  Shared library: [libc.so.6]
```

**Шаг 3.** Запуск:

```bash
export LD_LIBRARY_PATH=.
./SP_03
```

```
SP_03: static link (SP.so)
sum(10, 5)  = 15
sub(10, 5)  = 5
mul(10, 5)  = 50
div(10, 5)  = 2
```

Без `LD_LIBRARY_PATH`:

```
./SP_03: error while loading shared libraries: SP.so: cannot open shared object file
```

Затем `unset LD_LIBRARY_PATH`, чтобы не мешал дальше.

### Задание 04 — динамический вызов через dlopen

**Требуется по ТЗ:** приложение `SP_04`, динамически вызывающее функции из
табл. 1 через `dlopen`; продемонстрировать.

**Шаг 1.**

```bash
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/SP_lib
g++ -fPIC -shared SP.cpp -o SP.so
```

**Шаг 2.** Собрать приложение:

```bash
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/SP_04
cp ../SP_lib/SP.so .
g++ main.cpp -ldl -o SP_04
```

Код `main.cpp`:

```cpp
#include <stdio.h>
#include <dlfcn.h>

typedef int (*BinFunc)(int, int);

int main(int argc, char* argv[])
{
    void* hlib = dlopen("./SP.so", RTLD_NOW);
    if (!hlib)
    {
        printf("dlopen error: %s\n", dlerror());
        return 1;
    }

    BinFunc sum  = (BinFunc)dlsym(hlib, "sum");
    BinFunc sub  = (BinFunc)dlsym(hlib, "sub");
    BinFunc mul  = (BinFunc)dlsym(hlib, "mul");
    BinFunc divf = (BinFunc)dlsym(hlib, "div");

    if (!sum || !sub || !mul || !divf)
    {
        printf("dlsym error: %s\n", dlerror());
        dlclose(hlib);
        return 1;
    }

    printf("SP_04: dynamic link (dlopen)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", divf(10, 5));

    dlclose(hlib);
    return 0;
}
```

**Шаг 3.** Проверить, что библиотека **не** прилинкована:

```bash
readelf -d SP_04 | grep NEEDED
```

`SP.so` в выводе **отсутствует** — в отличие от `SP_03`. Это и есть разница
между заданиями 03 и 04.

**Шаг 4.** Запуск — `LD_LIBRARY_PATH` не нужен, путь указан в коде:

```bash
./SP_04
```

```
SP_04: dynamic link (dlopen)
sum(10, 5)  = 15
sub(10, 5)  = 5
mul(10, 5)  = 50
div(10, 5)  = 2
```

### Задание 07 — статический вызов из C#

**Требуется по ТЗ:** C#-приложение `SP_07`, статически вызывающее функции из
табл. 1 библиотеки `SP_XX.so`; продемонстрировать.

**Шаг 1.**

```bash
export DOTNET_ROOT="$HOME/.dotnet"
export PATH="$HOME/.dotnet:$PATH"

cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/cs/SP_07
dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true
```

**Шаг 2.** Положить библиотеку рядом с бинарником:

```bash
cp ../../SP_lib/SP.so bin/Release/net8.0/linux-arm64/publish/
```

**Шаг 3.** Запуск:

```bash
cd bin/Release/net8.0/linux-arm64/publish
./SP_07
```

```
Сложение в C++/DLL/SO: 15
Вычитание в C++/DLL/SO: 5
Умножение в C++/DLL/SO: 50
Деление в C++/DLL/SO: 2
```

`Program.cs` — тот же, что в задании 05. `LD_LIBRARY_PATH` не нужен: рантайм
.NET ищет `SP.so`/`libSP.so` в каталоге приложения сам.

### Задание 08 — динамический вызов из C# (dlopen)

**Требуется по ТЗ:** C#-приложение `SP_08`, динамически вызывающее функции из
табл. 1 библиотеки `SP_XX.so`; продемонстрировать.

**Шаг 1.**

```bash
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1/linux/cs/SP_08
dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true
```

**Шаг 2.** Запуск — **обязательно из папки** с `SP.so`, путь относительный:

```bash
cp ../../SP_lib/SP.so bin/Release/net8.0/linux-arm64/publish/
cd bin/Release/net8.0/linux-arm64/publish
./SP_08
```

```
Сложение в C++/DLL/SO: 15
Вычитание в C++/DLL/SO: 5
Умножение в C++/DLL/SO: 50
Деление в C++/DLL/SO: 2
```

`Program.cs`:

```csharp
using System;
using System.Runtime.InteropServices;

internal class Program
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int BinFunc(int x, int y);

    private static void Main(string[] args)
    {
        IntPtr hDll = NativeLibrary.Load("./SP.so");
        if (hDll == IntPtr.Zero)
        {
            Console.WriteLine("dlopen FAILED");
            return;
        }

        BinFunc sum  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "sum"));
        BinFunc sub  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "sub"));
        BinFunc mul  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "mul"));
        BinFunc divf = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(hDll, "div"));

        Console.WriteLine($"Сложение в C++/DLL/SO: {sum(10, 5)}");
        Console.WriteLine($"Вычитание в C++/DLL/SO: {sub(10, 5)}");
        Console.WriteLine($"Умножение в C++/DLL/SO: {mul(10, 5)}");
        Console.WriteLine($"Деление в C++/DLL/SO: {divf(10, 5)}");

        NativeLibrary.Free(hDll);
    }
}
```

Если запустить `./bin/.../SP_08` из папки проекта, будет
`System.DllNotFoundException` — относительный путь ищется от текущего каталога.

---

## Одна команда вместо всех шагов

Блок из файла `ДЕМОНСТРАЦИЯ.md`, раздел «03 + 04 + 07 + 08 одним блоком», собирает и запускает все четыре задания подряд:

```bash
export DOTNET_ROOT="$HOME/.dotnet"; export PATH="$HOME/.dotnet:$PATH"
cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1
```

---

# Часть II. ТЕОРИЯ

## Задание в целом

Одна библиотека с четырьмя функциями вызывается восемью способами:

- **2 языка:** C++ (нативно) и C# (через P/Invoke);
- **2 ОС:** Windows (`.dll` + `.lib`) и Linux (`.so`);
- **2 типа связывания:** неявное (на этапе компоновки) и явное (в runtime).

## Статическая и динамическая линковка

**Статическая (неявная)** — библиотека известна линкеру при сборке приложения.
Линкер прописывает в исполняемый файл метаданные, позволяющие загрузчику ОС
загрузить библиотеку вместе с программой. Приложение без библиотеки
**не запустится**.

**Динамическая (явная)** — библиотека на этапе сборки вообще не упоминается.
В runtime программа вызывает `LoadLibrary`/`dlopen`, получает адрес функции
через `GetProcAddress`/`dlsym`. Запуск программы не зависит от наличия
библиотеки.

| | Статическая | Динамическая |
|---|---|---|
| Когда решается | при сборке | при выполнении |
| Кто ищет функцию | линкер | сама программа |
| Что в бинарнике | номер импорта | ничего об этой библиотеке |
| Работает без библиотеки | нет | да |
| Можно менять библиотеку | нет, пересборка | да, подменил файл — и всё |

## Таблица вызовов на двух ОС

| Действие | Windows | Linux |
|---|---|---|
| загрузить библиотеку | `LoadLibrary("SP.dll")` | `dlopen("./SP.so", RTLD_NOW)` |
| найти функцию по имени | `GetProcAddress(h, "sum")` | `dlsym(h, "sum")` |
| получить ошибку | `GetLastError()` | `dlerror()` |
| выгрузить | `FreeLibrary(h)` | `dlclose(h)` |
| библиотека с этими функциями | `libdl` (системная) | `libdl` (флаг `-ldl`) |
| из C# | `NativeLibrary.Load` / `.GetExport` | то же самое |

В C# оба вызова одинаковые — .NET сама выбирает `LoadLibrary` или `dlopen`
в зависимости от ОС.

## Почему нужен extern "C"

Без `extern "C"` компилятор C++ добавляет к имени функции типы аргументов,
чтобы избежать перегрузок:

```
sum(int,int)  →  _Z3sumii
```

В таблице символов лежит `_Z3sumii`, а не `sum`. И `GetProcAddress(h, "sum")`,
и `dlsym(h, "sum")` ищут именно строку `"sum"` и ничего не находят.

`extern "C"` отключает искажение имён: функция получает имя `sum`.

**Отсюда правило:** `extern "C"` нужен и в библиотеке, и в приложении.
В приложении — потому что линкер тоже ищет функцию по имени.

## Зачем .lib для DLL и почему он не нужен для .so

**`.lib` (библиотека импорта)** — это не код библиотеки, а таблица «какие
функции можно вызвать». Линкер при сборке `SP_01.exe` читает `SP.lib`,
понимает, что нужна функция `sum`, и прописывает в exe номер импорта
«функция №3 из SP.dll». Без `.lib` линкер выдаст
`unresolved external symbol sum`.

У `.so` таблица экспортов **встроена в сам ELF-файл**, поэтому линкер читает
её прямо из `.so` — отдельный `.lib`-файл не нужен.

Почему `.lib` появляется сам: при включённом `__declspec(dllexport)` Visual
Studio генерирует `.lib` автоматически. Вручную его создают, когда пишут
`EXPORT` через `.def`-файл.

## Флаги компилятора

| Флаг | Зачем |
|---|---|
| `-fPIC` | позиционно-независимый код: загрузчик сам выбирает адрес в памяти, потому что одна `.so` может грузиться в несколько процессов. **Обязателен** для `.so` |
| `-shared` | собрать разделяемую библиотеку |
| `-c` | только компиляция, без линковки (получить `.o`) |
| `-ldl` | подключить `libdl` с `dlopen`/`dlsym` |
| `-L.` | искать библиотеки в текущей папке |
| `-r linux-arm64` | RID: под какую ОС/архитектуру собираем |
| `--self-contained` | вложить .NET в выходной файл |
| `-p:PublishSingleFile=true` | упаковать в один файл |

**В лекции `smwlib.so` собирается без `-fPIC`** (`g++ -shared sum.o sub.o -o smwlib.so`)
и это работает на x86_64. На ARM64 `-fPIC` нужен — компилятор без него выдаст
`relocation R_AARCH64_... can not be used when making a shared object`.

**`-fPIC` vs Windows:** у DLL свой 64-битный адресный диапазон, поэтому
проблемы позиционной независимости там нет.

## Что делает DllMain

`DllMain` — обязательная точка входа Windows DLL, вызывается ОС при загрузке
и выгрузке библиотеки (`DLL_PROCESS_ATTACH`, `DLL_PROCESS_DETACH` и др.).
Нужна, чтобы выполнить код инициализации **до** первого вызова экспортируемой
функции. На Linux аналога нет — поэтому весь блок под `#ifdef _WIN32`.

## Конструктор и деструктор .so

В Linux аналог `DllMain` — атрибуты
`__attribute__((constructor))` и `__attribute__((destructor))`
(лекция `Лекция_СП_SOXX.pdf`, слайды 15–16):

```cpp
extern "C" void __attribute__((constructor)) lib_load(void)
{
    printf("constructor\n");
}

extern "C" void __attribute__((destructor)) lib_unload(void)
{
    printf("destructor\n");
}
```

Вывод `SP_04` после пересборки:

```
constructor
SP_04: dynamic link (dlopen)
sum(10, 5)  = 15
...
destructor
```

Конструктор выполняется **в момент `dlopen`**, деструктор — **в момент `dlclose`**.
Это нагляднее любого объяснения: библиотеки на диске полмегабайта не было —
и вдруг она выполнила свой код.

## Как из C вызвать C#-код и наоборот

**Из C# в C** — атрибут `[DllImport]`:

- имя — **без расширения и без `lib`**: `DllImport("SP")`. Рантайм сам подставит
  `SP.dll` на Windows и `libSP.so`/`SP.so` на Linux;
- `extern` — обязательно: «это не тело функции, это ссылка на нативную»;
- `CallingConvention.Cdecl` — **обязателен**, наш код скомпилирован как C.
  На x64 разницы нет, на x86 будет несовпадение соглашения → `AccessViolation`;
- тело функции писать нельзя — только сигнатура.

**Динамический вариант** — вместо `extern`-объявлений получаем адрес функции
(`NativeLibrary.GetExport` — аналог `GetProcAddress`/`dlsym`) и превращаем
его в управляемый делегат через `Marshal.GetDelegateForFunctionPointer`.

**Почему в задании 08 путь `./SP.so` указан явно, а в 07 просто `SP`:** в 07
используется `DllImport("SP")` — рантайм .NET сам ищет библиотеку в каталоге
приложения. В 08 используется `NativeLibrary.Load` с **явным путём**: видно,
что происходит настоящая загрузка файла, и можно указать любой путь.

## Вопросы преподавателя и ответы

**1. Чем статическая линковка отличается от динамической?**
Статическая — библиотека известна линкеру при сборке, метаданные
прописываются в exe, загрузка до `main`. Динамическая — библиотека не
упоминается при сборке, приложение загружает её в runtime через
`LoadLibrary`/`dlopen` и ищет функции по имени через `GetProcAddress`/`dlsym`.

**2. Что делает `LoadLibrary`/`dlopen` и `GetProcAddress`/`dlsym`?**
Первое загружает библиотеку в адресное пространство процесса и возвращает
ручку. Второе ищет в ней экспортированный символ по строковому имени и
возвращает его адрес.

**3. Зачем нужен `.lib` для DLL и почему он не нужен для `.so`?**
`.lib` — таблица имён функций для линкера. У `.so` таблица экспортов встроена
в сам ELF-файл, линкер читает её прямо из `.so`.

**4. Зачем `extern "C"`?**
Без него имя функции искажается до `_Z3sumii`, и по строке `"sum"` функцию
не найти.

**5. Зачем `__declspec(dllexport)`?**
Помечает функцию как экспортируемую, линкер включает её в таблицу экспортов
DLL и генерирует `.lib`. Аналог в Linux — `-fvisibility=default` (по
умолчанию) или `__attribute__((visibility("default")))`.

**6. Что делает `DllMain` и зачем она нужна?**
Точка входа DLL на Windows, вызывается при загрузке/выгрузке. Нужна для кода
инициализации до первого вызова экспортируемой функции. Аналога на Linux нет.

**7. Что делает `LD_LIBRARY_PATH` и зачем он нужен в задании 03?**
Список каталогов для поиска библиотек. Нужен, потому что загрузчик Linux
по умолчанию не смотрит в текущем каталоге, а `SP_03` прилинкован к `SP.so`
по имени, без пути. На Windows загрузчик **автоматически** ищет DLL рядом с exe.

**8. Что такое `-fPIC`, `-shared`, `-ldl`, RID, `--self-contained`,
`PublishSingleFile`?** — см. таблицу флагов выше.

**9. Почему в лекции `sum.cpp` и `sub.cpp` без `extern "C"`, а `smwlib2.cpp`
с ним?** Это не опечатка, а смысл задания. Для неявного связывания (задание
03) `extern "C"` не нужен: обе стороны компилируются как C++, имена
искажаются одинаково и линкер их находит. Для явного связывания (задание 04)
`extern "C"` обязателен: `dlsym` ищет по строковому имени `"sum"`.

**10. Что будет, если забыть `CallingConvention.Cdecl` в C#?**
На x64 разницы нет, на x86 будет несовпадение соглашения о вызовах →
неправильная передача аргументов → `AccessViolationException` или порча памяти.

**11. Как проверить, что библиотека реально экспортирует функции?**
Windows: `dumpbin /exports SP.dll`. Linux: `nm -D SP.so | grep ' T '`.
Модуль-вьюер в Visual Studio — то же графически.

**12. Что произойдёт при вызове `div(x, 0)`?**
В коде защита `y != 0 ? x / y : 0` — вернётся `0`, а не исключение и не краш.
В ТЗ поведение при делении на ноль не задано, решение осознанное.

**13. Почему `LD_LIBRARY_PATH` нужен в задании 03, но не в 04 и 07?**
В 03 библиотека прилинкована и ищется загрузчиком по имени — нужен путь.
В 04 путь указан прямо в `dlopen("./SP.so")`. В 07 `DllImport("SP")` — рантайм
.NET сам ищет библиотеку в каталоге приложения.

**14. Почему библиотека называется `SP.so`, а не `libSP.so`?**
В Linux по соглашению разделяемые библиотеки начинаются с `lib`. Имя `SP.so`
задано преподавателем, поэтому команда линковки указывает файл напрямую
(`g++ main.cpp SP.so`), а не через `-lSP` — тот искал бы `libSP.so`.

**15. Чем `RTLD_NOW` отличается от `RTLD_LAZY`?**
`RTLD_NOW` резолвит все символы сразу — ошибки в именах функций вскрываются
на старте. `RTLD_LAZY` откладывает разрешение до первого вызова. В лекции
используется `RTLD_NOW`.

---

## Порядок демонстрации

1. **Задание 01:** вывод `SP_01.exe` + `dumpbin /dependents SP_01.exe`,
   где `SP.dll` **есть** в списке.
2. **Задание 02:** вывод `SP_02.exe` + `dumpbin /dependents SP_02.exe`,
   где `SP.dll` **нет** — контраст с заданием 01.
3. **Задания 03/04:** `g++ -shared` → `nm -D SP.so` → `readelf -d SP_03`
   (есть `SP.so`) → запуск `SP_03` → `readelf -d SP_04` (**нет** `SP.so`)
   → `./SP_04`.
4. **Задания 05–08:** выводы `SP_05`, `SP_06` на Windows и `SP_07`, `SP_08` на Linux.
5. **Вопросы** — раздел выше.

Готовые команды с ожидаемым выводом — в `ДЕМОНСТРАЦИЯ.md` (Linux) и `ДЕМОНСТРАЦИЯ-WINDOWS.md` (Windows).