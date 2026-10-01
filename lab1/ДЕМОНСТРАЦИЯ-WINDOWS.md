# Лаба 1 — Windows (задания 01, 02, 05, 06)

В Visual Studio часть работы — мышью, но весь код и все команды — чистыми блоками, копируются целиком.

---

# Как запустить — пошагово

Этот раздел нужен, чтобы запустить **все четыре задания**. Дальше по тексту — подробности и теория.

## Шаг 1. Открыть решение

Найди в проводнике файл:

```
Documents\VisualStudioCodeRepos\SystemProgramming\lab1\windows\lab1.sln
```

**Двойной клик по `lab1.sln`.** Жди пока Visual Studio загрузится.

Слева появится панель **Solution Explorer** («Обозреватель решений») с пятью проектами:

```
SP        библиотека
SP_01     задание 01
SP_02     задание 02
SP_05     задание 05
SP_06     задание 06
```

## Шаг 2. Выбрать платформу ARM64

Сверху, где написано `Debug`, есть выпадающий список. Нажми его → выбери **ARM64**.

**Это обязательный шаг.** По умолчанию там стоит `x64`, а проекты собраны под
`ARM64`. При `x64` сборка упадёт с ошибкой.

Слева от него убедись, что стоит `Release`.

## Шаг 3. Собрать

Меню **Build → Build Solution** или `Ctrl+Shift+B`.

Жди 30–60 секунд. Внизу должно появиться:

```
Сборка успешно завершена. Ошибок: 0
```

Если видишь `NETSDK1004` — переходи к разделу [«Если сборка ругается на NETSDK1004»](#если-сборка-ругается-на-netsdk1004).

## Шаг 4. Запустить задание

Повтори эти 4 действия для каждого из четырёх заданий:

1. В **Solution Explorer** найди нужный проект — `SP_01`.
2. **Правый клик** по нему → **Set as Startup Project**.
3. Теперь `Ctrl+F5`.

Повтори для `SP_02`, потом `SP_05`, потом `SP_06`.

## Шаг 5. Главный подвох — окно закрывается

В коде **нет паузы**. При `Ctrl+F5` окно консоли открывается, печатает 4 строки и
**сразу закрывается**. На защите ты ничего не увидишь — только мелькнёт окно.

### Что делать вместо этого

Открой встроенный терминал: **View → Terminal** или `Ctrl+` (обратная кавычка).
Выбери **Command Prompt**. Копируй туда:

```cmd
windows\SP_01\SP_01\ARM64\Release\SP_01.exe
```

Окно останется на экране, вывод можно рассмотреть и заскриншотить.

### Если хочешь паузу прямо в коде

В самый конец `main.cpp`, перед `return 0;` добавь одну строку:

```cpp
system("pause");
```

Тогда при `Ctrl+F5` окно будет ждать нажатия любой клавиши.

---

Сначала подготовка (один раз), потом каждое задание.

---

## Подготовка — создать библиотеку

### Шаг 1. Создать проект

```
File → New → Project → C++ → Dynamic-Link Library (DLL) → имя SP → Create
```

Visual Studio создаст четыре файла: `pch.h`, `framework.h`, `SP.cpp`, `SP.def`.

### Шаг 2. Открыть `SP → Source Files → pch.h`, дописать в конец

```cpp
#include <windows.h>
```

### Шаг 3. Открыть `SP → Source Files → SP.cpp`, заменить содержимое целиком

```cpp
#include "pch.h"

EXPORT int sum(int x, int y) { return x + y; }
EXPORT int sub(int x, int y) { return x - y; }
EXPORT int mul(int x, int y) { return x * y; }
EXPORT int div(int x, int y) { return y != 0 ? x / y : 0; }
```

### Шаг 4. Открыть `SP → Source Files → SP.def`, заменить содержимое целиком

```
EXPORTS
    sum
    sub
    mul
    div
```

### Шаг 5. Собрать

```
Build → Build Solution
```

### Пояснения к шагам

| Что | Расшифровка |
|---|---|
| `#include <windows.h>` в `pch.h` | подключаем Windows API. Он объявляет макрос `EXPORT` (`__declspec(dllexport)`), а также типы `BOOL`, `HMODULE`, `LPVOID`, `DWORD`, которые нужны для точки входа `DllMain` |
| `#include "pch.h"` в `SP.cpp` | **обязательная первая строка.** Visual Studio включает режим Precompiled Headers: `SP.cpp` компилируется отдельно, и если он не подключит `pch.h` — ошибка `fatal error C1010: precompiled header file not found` |
| `EXPORT` перед каждой функцией | макрос из `windows.h`, раскрывается в `__declspec(dllexport)` — «эту функцию видно снаружи». Без неё функция есть в DLL, но вызвать её из другого проекта нельзя |
| `int div(int x, int y)` | функция называется `div`, а делить на ноль нельзя, поэтому `y != 0 ? x / y : 0` — защита, чтобы программа не упала на демонстрации |
| `SP.def` | файл описания экспортов. Он говорит компоновщику: вот эти имена функций нужно отдать наружу. **Без него** при линковке будет `LNK2001: unresolved external symbol sum` |
| `SP.dll` | получившийся файл библиотеки — сам машинный код |
| `SP.lib` | таблица имён экспортов. Это не код, а «оглавление» для компоновщика. Прилинковывается к приложению, но в exe не попадает |

### Если Visual Studio ругается на `pch.h`

```
Properties → C/C++ → Precompiled Headers → Precompiled Header: Use
Properties → C/C++ → Precompiled Headers → Precompiled File: pch.h
```

---

## Задание 01 — статический вызов

### Шаг 1. Создать приложение

```
File → New → Project → C++ → Console App → имя SP_01 → Create
```

### Шаг 2. Настроить линковку

Правый клик по проекту `SP_01` в Solution Explorer → `Properties`. Открыть `Linker → Input` и `Linker → General`.

| Вкладка | Поле | Что вписать |
|---|---|---|
| `Linker → Input` | `Additional Dependencies` | `SP.lib` |
| `Linker → General` | `Additional Library Directories` | полный путь к папке с `SP.lib` |

> Каталог можно не указывать, если положить `SP.lib` рядом с `.cpp` — тогда Visual Studio подставит `$(ProjectDir)` сама.

### Шаг 3. Открыть `SP_01 → Source Files → main.cpp`, заменить содержимое

```cpp
#include <stdio.h>

extern "C" __declspec(dllimport) int sum(int x, int y);
extern "C" __declspec(dllimport) int sub(int x, int y);
extern "C" __declspec(dllimport) int mul(int x, int y);
extern "C" __declspec(dllimport) int div(int x, int y);

int main()
{
    printf("SP_01: static link (SP.dll)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", div(10, 5));
    return 0;
}
```

### Пояснения

| Что | Расшифровка |
|---|---|
| `extern "C"` | объявление функции как C-совместимой. Без него компилятор C++ исказит имя в `?sum@@YAHHH@Z`, и линкер, ищущий `sum`, её не найдёт |
| `__declspec(dllimport)` | «эта функция приходит извне, из другой DLL». Компилятор не будет искать её у себя, а линковщик возьмёт адрес из `SP.lib`. **Не путать** с `dllexport` — тем, что было в самой библиотеке |
| `#include <stdio.h>` | нужен ради `printf`. В Windows это не `iostream` |
| `%d` | формат для целого числа. `sum` возвращает `int` |

### Шаг 4. Собрать, скопировать DLL, запустить

```
Build → Build Solution
```

Открыть `View → Terminal`, выбрать `Command Prompt`. Из корня решения `SP_01`:

```cmd
SP_01\SP_01\ARM64\Release\SP_01.exe
```

| Ключ | Значение |
|---|---|
| `SP_01\SP_01\` | вложенная папка по имени проекта — MSBuild добавляет её сам в `OutDir` |
| `ARM64\Release\` | платформа и конфигурация сборки |
| `dumpbin /dependents` | утилита Visual Studio, показывает список DLL от которых зависит exe |

`SP.dll` копируется в эту папку **автоматически** при сборке — отдельной
командой копировать не нужно.

**Ожидаем:** `sum = 15`, `sub = 5`, `mul = 50`, `div = 2`

### Шаг 5. Доказательство для преподавателя

```cmd
dumpbin /dependents SP_01\SP_01\ARM64\Release\SP_01.exe
```

| Ключ | Значение |
|---|---|
| `dumpbin` | утилита Visual Studio для разбора PE-файлов |
| `/dependents` | показать список DLL, от которых зависит exe |

В списке будет строка `SP.dll`. Это значит: библиотека прописана в бинарнике, и загрузчик Windows подгрузит её **сам** при старте приложения. Статическая линковка.

---

## Задание 02 — динамический вызов (LoadLibrary)

### Шаг 1. Создать приложение

```
File → New → Project → C++ → Console App → имя SP_02 → Create
```

### Шаг 2. Линковку НЕ настраиваем

Это и есть суть задания — приложение не должно знать о библиотеке на этапе сборки.

### Шаг 3. Открыть `SP_02 → Source Files → main.cpp`, заменить содержимое

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

### Пояснения

| Что | Расшифровка |
|---|---|
| `LoadLibraryA("SP.dll")` | **аналог `dlopen` в Linux.** Загружает библиотеку в память прямо сейчас, во время работы программы. Возвращает `HMODULE` — дескриптор загруженной библиотеки |
| буква `A` в `LoadLibraryA` | «ANSI» — строка в однобайтовой кодировке. Есть ещё `W`-версия для Unicode |
| `GetProcAddress(hDll, "sum")` | **аналог `dlsym`.** Ищет функцию по имени в уже загруженной библиотеке и возвращает её адрес |
| `typedef int (*BinFunc)(int, int);` | объявляем тип «указатель на функцию, которая берёт два `int` и возвращает `int`». Без такого типа нельзя корректно хранить результат `GetProcAddress` |
| `(BinFunc)` | приведение типа: адрес функции → указатель на функцию. Явно, потому что `GetProcAddress` возвращает универсальный `FARPROC` |
| `if (!hDll)` | проверка на NULL — библиотека не нашлась. `!hDll` истинно, когда указатель равен нулю |
| `GetLastError()` | код последней ошибки Windows. Число `126` = файл не найден, `126`/`2` = DLL не найдена |
| `FreeLibrary(hDll)` | **обязателен в конце.** Выгружает библиотеку и уменьшает счётчик. Забыть — утечка памяти, при повторных вызовах DLL не выгрузится никогда |
| `%lu` | формат для `unsigned long`, каким возвращается `GetLastError` |

### Шаг 4. Собрать, скопировать, запустить

```
Build → Build Solution
```

```cmd
SP_02\SP_02\ARM64\Release\SP_02.exe
```

`SP.dll` копируется в папку вывода автоматически при сборке.

**Ожидаем:** `sum = 15`, `sub = 5`, `mul = 50`, `div = 2`

### Шаг 5. Доказательство для преподавателя

```cmd
dumpbin /dependents SP_02\SP_02\ARM64\Release\SP_02.exe
```

`SP.dll` в списке **отсутствует**. В отличие от задания 01 — библиотеки в бинарнике нет, программа грузит её сама в момент работы. Это главное доказательство динамической линковки.

---

## Задание 05 — C#, статический вызов (DllImport)

### Шаг 1. Создать приложение

```
File → New → Project → C# → Console App → имя SP_05 → Framework: .NET 8.0 → Create
```

### Шаг 2. Открыть `SP_05 → Program.cs`, заменить содержимое

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

### Пояснения

| Что | Расшифровка |
|---|---|
| `using System.Runtime.InteropServices;` | подключает пространство имён, где живут `DllImport` и `CallingConvention` |
| `[DllImport(...)]` | атрибут «объявлено в DLL». Читается при вызове — .NET загружает библиотеку и подставляет реальный адрес функции. **Работает только со статически известным именем** |
| `extern` | функция объявлена, но тела нет — она не в нашей программе, а в DLL |
| `public static` | видно всем, можно звать без создания объекта |
| `@"SP"` | префикс `@` перед строкой отключает обработку escape-последовательностей. Без него пришлось бы писать `@"SP"` вместо обычного `"SP"` с удвоенным слэшем |
| имя `"SP"` **без `.dll`** | .NET сам дописывает расширение и ищет `SP.dll`. Это не ошибка, так и задумано для переносимости |
| `CallingConvention.Cdecl` | соглашение о вызове — как передавать аргументы и кто чистит стек. **Обязательно**, потому что наша DLL собрана как C-код. На x64 разницы нет, на x86 без этого будет `AccessViolationException` при первом же вызове |
| `$"..."` | интерполированная строка — вместо конкатенации. Проще читается |

### Шаг 3. Собрать, скопировать DLL, запустить

```
Build → Build Solution
```

```cmd
cs\SP_05\bin\Release\net10.0\SP_05.exe
```

| Ключ | Значение |
|---|---|
| `bin\Release\` | папка вывода, конфигурация `Release` |
| `net10.0` | целевой фреймворк — берётся из `<TargetFramework>` в `.csproj` |
| без `win-arm64` | при сборке через решение Visual Studio папка называется просто `net10.0`; `win-arm64` появляется только при `dotnet publish -r win-arm64` из командной строки |
| `chcp 65001` | если кириллица в выводе — кракозябры, эта команда переключает кодировку терминала на UTF-8 |

**Ожидаем:** `Сложение = 15`, `Вычитание = 5`, `Умножение = 50`, `Деление = 2`

---

## Задание 06 — C#, динамический вызов (NativeLibrary.Load)

### Шаг 1. Создать приложение

```
File → New → Project → C# → Console App → имя SP_06 → Framework: .NET 8.0 → Create
```

### Шаг 2. Открыть `SP_06 → Program.cs`, заменить содержимое

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

### Пояснения

| Что | Расшифровка |
|---|---|
| `NativeLibrary.Load("SP.dll")` | **аналог `dlopen` в C#.** Загружает библиотеку в момент работы. Возвращает `IntPtr` — необрабатываемый указатель на модуль |
| `hDll == IntPtr.Zero` | проверка «не загрузилась ли библиотека». `IntPtr.Zero` — это ноль |
| `NativeLibrary.GetExport(hDll, "sum")` | **аналог `dlsym`.** Возвращает `IntPtr` — адрес функции |
| `Marshal.GetDelegateForFunctionPointer` | превращает голый адрес в вызываемый делегат. Без этого шага `IntPtr` нельзя вызвать как функцию |
| `[UnmanagedFunctionPointer]` | помечает делегат как «указатель на нативную функцию». Атрибут на самом деле не нужен, но без него код читается как «указатель на C#-метод», а это разные вещи |
| `delegate int BinFunc(int x, int y);` | объявление типа делегата: сигнатура функции `int(int, int)`. Должна **точно** совпадать с реальной сигнатурой из DLL, иначе упадёт |
| `NativeLibrary.Free(hDll)` | выгружает библиотеку. Обязателен, как и `FreeLibrary` в задании 02 |

### Шаг 3. Собрать, скопировать, запустить

```
Build → Build Solution
```

```cmd
cs\SP_06\bin\Release\net10.0\SP_06.exe
```

**Ожидаем:** `Сложение = 15`, `Вычитание = 5`, `Умножение = 50`, `Деление = 2`

### Шаг 4. Доказательство для преподавателя

Открыть `Program.cs` заданий 05 и 06 и сравнить одну строку:

| Задание 05 | Задание 06 |
|---|---|
| `[DllImport(@"SP", CallingConvention = ...)]` | `NativeLibrary.Load("SP.dll")` |
| имя-шаблон, .NET ищет библиотеку **сам при старте** | **явная** загрузка в момент работы |
| статическая линковка | динамическая линковка |

Разница в одной строке, по сути это тот же контраст, что в заданиях 01 и 02.

---

## Если скомпилировалось, но не запускается

| Симптом | Причина | Что делать |
|---|---|---|
| `0xc0000135 STATUS_DLL_NOT_FOUND` | DLL не рядом с `.exe` | пересобери — `SP.dll` копируется автоматически |
| `LNK2001: unresolved external symbol sum` | нет линковки или пуст `SP.def` | задание 01: проверь `ProjectReference` на `SP.vcxproj`; библиотека: заполни `SP.def` |
| `LNK1112: library not found` | неверный путь к `.lib` | проверь `Additional Library Directories` |
| `LoadLibrary FAILED, error = 126` | DLL нет в папке программы | та же сборка, что и выше |
| `GetProcAddress FAILED` | функция не экспортирована | проверь `EXPORT` в `SP.cpp` и `SP.def` |
| `DllNotFoundException` в C# | DLL не скопирована в папку вывода | пересобери проект — таргет `CopySPLib` копирует DLL сам |
| `AccessViolationException` | не указано соглашение о вызовах | добавить `CallingConvention.Cdecl` |
| `error NETSDK1004: project.assets.json не найден` | нет папки `obj\` после `git clean` или свежего клона | собери через Visual Studio, либо добавь ключ `-restore` к `msbuild` |
| `error MSB8020: средств сборки не найдено` | в проекте указан чужой тулсет | в `.vcxproj` должно быть `<PlatformToolset>v145</PlatformToolset>` |
| `error C2169: div — подставляемая функция` | имя `div` занято в CRT | функция в `SP.cpp` называется `div_int`, экспорт `div` идёт через алиас в `SP.def` |
| `fatal error C1010: precompiled header not found` | в `SP.cpp` нет `#include "pch.h"` | вернуть первую строку или отключить PCH в свойствах |

---

## Порядок выступления

1. Показать структуру: одна библиотека `SP`, четыре приложения `01/02/05/06`.
2. Задание 01 → запуск → результат.
3. `dumpbin /dependents SP_01.exe` → `SP.dll` **есть**.
4. Задание 02 → запуск → результат.
5. `dumpbin /dependents SP_02.exe` → `SP.dll` **нет**. ← главный контраст
6. Задания 05 и 06 → сравнить `DllImport("SP")` и `NativeLibrary.Load("SP.dll")`.
