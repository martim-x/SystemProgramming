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