#include <stdio.h>

extern "C" int sum(int x, int y);
extern "C" int sub(int x, int y);

extern "C" int mul(int x, int y);

// "div" is an MSVC intrinsic that returns div_t. Our extern "C" int div(int,int)
// comes from SP.dll, so C4391 ("wrong return type for inlined function") is a
// false positive here. The real call goes through the SP.lib import thunk.
#pragma warning(push)
#pragma warning(disable : 4391)
extern "C" int div(int x, int y);
#pragma warning(pop)

int main(int argc, char* argv[])
{
    printf("SP_01: static link (SP.lib)\n");
    printf("sum(10, 5)  = %d\n", sum(10, 5));
    printf("sub(10, 5)  = %d\n", sub(10, 5));
    printf("mul(10, 5)  = %d\n", mul(10, 5));
    printf("div(10, 5)  = %d\n", div(10, 5));
    return 0;
}
