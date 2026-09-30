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