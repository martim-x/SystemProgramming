#include <stdio.h>

extern "C" int sum(int x, int y);
extern "C" int sub(int x, int y);
extern "C" int mul(int x, int y);
extern "C" int div(int x, int y);

int main(int argc, char* argv[])
{
    printf("SP_03: static link (SP.so)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", div(10, 5));
    return 0;
}