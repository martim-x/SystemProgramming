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