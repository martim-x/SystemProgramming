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