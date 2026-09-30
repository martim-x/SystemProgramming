# Лаба 1 — демонстрация Windows (задания 01, 02, 05, 06)

Работаем в **Visual Studio 2022**. Сначала один раз сделай подготовку — дальше
в каждом задании только свои шаги.

---

## Подготовка (один раз)

### 1. Создать библиотеку

```
File → New → Project → C++ → Dynamic-Link Library (DLL) → имя SP
```

Visual Studio сама создаст `SP/pch.h`, `SP/framework.h`, `SP/SP.cpp` и `SP.def`.

### 2. Вписать функции

```
мы в: SP → Source Files → pch.h
```

Дописать в конец:

```cpp
#include <windows.h>
```

```
мы в: SP → Source Files → SP.cpp
```

Заменить содержимое:

```cpp
#include "pch.h"

EXPORT int sum(int x, int y) { return x + y; }
EXPORT int sub(int x, int y) { return x - y; }
EXPORT int mul(int x, int y) { return x * y; }
EXPORT int div(int x, int y) { return y != 0 ? x / y : 0; }
```

```
мы в: SP → Source Files → SP.def
```

Заменить содержимое:

```
EXPORTS
    sum
    sub
    mul
    div
```

> Зачем `.def`: он же указывает компоновщику, какие функции наружу.
> Без него будет ошибка `LNK2001: unresolved external symbol sum`.

```
Build → Build Solution
```

В папке `x64/Debug` появятся `SP.dll` и `SP.lib`. Оба нужны:
`SP.dll` — сам код, `SP.lib` — имена функций для компоновщика.

**Ошибка про `pch.h`:** `SP.cpp` начинается с `#include "pch.h"`. Если VS ругается —
`Properties → C/C++ → Precompiled Headers → Precompiled Header = Use`,
`Precompiled File = pch.h`.

---

## Задание 01 — статический вызов (LoadLibrary нет, линкер сам)

```
New → Project → C++ → Console App → имя SP_01
```

**Настройка линковки** (Properties проекта SP_01):

```
Linker → Input → Additional Dependencies: SP.lib
Linker → General → Additional Library Directories: путь к папке с SP.lib
```

> `SP.lib` — файл, а не `SP.lib.lib`. Взять имя из сообщения компилятора.
> Каталог указывать **не обязательно**, если положить `SP.lib` рядом с `.cpp` —
> тогда `$(ProjectDir)` добавится сам.

```
мы в: SP_01 → Source Files → main.cpp
```

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

> `extern "C"` — иначе имя станет `?sum@@YAHHH@Z` и линкер не найдёт функцию.
> `__declspec(dllimport)` — говорит компилятору, что функция из чужой DLL.

```
Build → Build Solution
```

**Перед запуском:** скопировать `SP.dll` из `SP/x64/Debug` в `SP_01/x64/Debug`.
Без этого будет `0xc0000135: STATUS_DLL_NOT_FOUND`.

```
Start → Start Without Debugging (Ctrl+F5)
```

**Ожидаем:** `sum = 15`, `sub = 5`, `mul = 50`, `div = 2`

**Показать преподавателю разницу:**

```
мы в: окно терминала, папка x64\Debug SP_01
```

```cmd
dumpbin /dependents SP_01.exe
```

> В списке будет `SP.dll` — значит загрузчик Windows сам подгрузит её при старте.

---

## Задание 02 — динамический вызов (LoadLibrary + GetProcAddress)

```
New → Project → C++ → Console App → имя SP_02
```

**Настройку линковки НЕ меняем** — это и есть суть задания.

```
мы в: SP_02 → Source Files → main.cpp
```

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

> `BinFunc` — указатель на функцию. Присваиваем результат `GetProcAddress`.
> `FreeLibrary` в конце — обязателен, освобождает библиотеку.

```
Build → Build Solution
```

**Перед запуском:** скопировать `SP.dll` в `SP_02/x64/Debug`.

```
Ctrl+F5
```

**Ожидаем:** `sum = 15`, `sub = 5`, `mul = 50`, `div = 2`

**Показать преподавателю разницу:**

```cmd
dumpbin /dependents SP_02.exe
```

> `SP.dll` в списке **отсутствует** — в отличие от задания 01. Приложение грузит
> библиотеку само в момент работы. Это и есть главное доказательство.

---

## Задание 05 — C#, статический вызов (DllImport)

```
New → Project → C# → Console App → имя SP_05 → Framework: .NET 8.0
```

```
мы в: SP_05 → Program.cs
```

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

> `CallingConvention.Cdecl` — обя��ательно, код написан на C.
> На x64 разницы нет, на x86 без него будет `AccessViolationException`.
> `@"SP"` — чтобы не пришлось удваивать обратный слэш.

```
Build → Build Solution
```

**Перед запуском:** скопировать `SP.dll` в папку вывода `SP_05`:

```cmd
xcopy ..\SP\x64\Debug\SP.dll bin\Debug\net8.0\ 
```

> Без этого `DllNotFoundException: не удалось загрузить «SP.dll»».

```
Ctrl+F5
```

**Ожидаем:** `Сложение = 15`, `Вычитание = 5`, `Умножение = 50`, `Деление = 2`

---

## Задание 06 — C#, динамический вызов (NativeLibrary.Load)

```
New → Project → C# → Console App → имя SP_06 → Framework: .NET 8.0
```

```
мы в: SP_06 → Program.cs
```

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

> `NativeLibrary.Load` — аналог `dlopen` в C#.
> `NativeLibrary.GetExport` — аналог `dlsym`.
> `GetDelegateForFunctionPointer` превращает адрес в вызываемый делегат.
> `BinFunc` помечен `UnmanagedFunctionPointer` — чтобы .NET знал про C-соглашение.

```
Build → Build Solution
```

```cmd
xcopy ..\SP\x64\Debug\SP.dll bin\Debug\net8.0\ 
```

```
Ctrl+F5
```

**Ожидаем:** `Сложение = 15`, `Вычитание = 5`, `Умножение = 50`, `Деление = 2`

**Показать преподавателю разницу:**

> Открой `Program.cs` заданий 05 и 06 и сравни. В 05 имя `"SP"` — это **шаблон**,
> .NET сам найдёт `SP.dll` при старте. В 06 строка `"SP.dll"` — **явный вызов**
> загрузки в момент работы. Разница в одну строку, но по сути это статика против
> динамики, как в заданиях 01 и 02.

---

## Если скомпилировалось, но не запускается

| Симптом | Что делать |
|---|---|
| `0xc0000135 STATUS_DLL_NOT_FOUND` | скопировать `SP.dll` рядом с `.exe` / в папку вывода |
| `LNK2001: unresolved external symbol sum` | не прописана линковка (задание 01) или не заполнен `SP.def` |
| `LNK1112: library not found` | путь в `Additional Library Directories` указывает не туда |
| `DllNotFoundException` в C# | DLL не скопирована в `bin\Debug\net8.0\` |
| `AccessViolationException` | убрать `CallingConvention.Cdecl` нельзя, наоборот — добавить |
| ошибка про `pch.h` | `Properties → C/C++ → Precompiled Headers → Use` |

---

## Порядок выступления (Windows)

1. Показать структуру: одна библиотека `SP`, четыре приложения `01/02/05/06`.
2. Задание 01 → `Ctrl+F5` → результат.
3. `dumpbin /dependents SP_01.exe` → `SP.dll` **есть**.
4. Задание 02 → `Ctrl+F5` → результат.
5. `dumpbin /dependents SP_02.exe` → `SP.dll` **нет**. ← контраст
6. Задания 05 и 06 → сравнить `DllImport("SP")` и `NativeLibrary.Load("SP.dll")`.
