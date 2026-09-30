using System;
using System.Runtime.InteropServices;

internal class Program
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate int BinFunc(int x, int y);

    private static void Main(string[] args)
    {
        IntPtr handle = NativeLibrary.Load("./SP.so");
        if (handle == IntPtr.Zero)
        {
            Console.WriteLine("dlopen FAILED");
            return;
        }

        BinFunc sum  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(handle, "sum"));
        BinFunc sub  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(handle, "sub"));
        BinFunc mul  = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(handle, "mul"));
        BinFunc divf = Marshal.GetDelegateForFunctionPointer<BinFunc>(NativeLibrary.GetExport(handle, "div"));

        Console.WriteLine($"Сложение в C++/DLL/SO: {sum(10, 5)}");
        Console.WriteLine($"Вычитание в C++/DLL/SO: {sub(10, 5)}");
        Console.WriteLine($"Умножение в C++/DLL/SO: {mul(10, 5)}");
        Console.WriteLine($"Деление в C++/DLL/SO: {divf(10, 5)}");

        NativeLibrary.Free(handle);
    }
}