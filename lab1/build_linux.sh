#!/bin/bash
# build_linux.sh — собирает и запускает Linux-часть лабы 1 (SP_03, SP_04, SP_07, SP_08).
# Работает на aarch64 (Arch Linux ARM) и x86_64.
# Требуется: g++ и .NET SDK 8.0 в PATH (см. README, раздел «Установка инструментов»).
set -e
export PATH="$HOME/.dotnet:$PATH"

LAB="$(cd "$(dirname "$0")" && pwd)"
echo "Работаем в: $LAB"

# 1. SP.so — общая библиотека
cd "$LAB/linux/SP_lib"
g++ -fPIC -shared SP.cpp -o SP.so
echo "[OK] SP.so собран, экспортирует:"
nm -D SP.so | grep -E ' T (sum|sub|mul|div)$'

# 2. SP_03 — статическая линковка
cd "$LAB/linux/SP_03"
cp ../SP_lib/SP.so .
g++ main.cpp SP.so -o SP_03
readelf -d SP_03 | grep NEEDED | grep SP.so
LD_LIBRARY_PATH=. ./SP_03

# 3. SP_04 — динамическая загрузка
cd "$LAB/linux/SP_04"
cp ../SP_lib/SP.so .
g++ main.cpp -ldl -o SP_04
./SP_04

# 4. SP_07 — C#, DllImport
cd "$LAB/linux/cs/SP_07"
dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true -v q --nologo
cp ../../SP_lib/SP.so bin/Release/net8.0/linux-arm64/publish/
cd bin/Release/net8.0/linux-arm64/publish
./SP_07

# 5. SP_08 — C#, dlopen
cd "$LAB/linux/cs/SP_08"
dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true -v q --nologo
cp ../../SP_lib/SP.so bin/Release/net8.0/linux-arm64/publish/
cd bin/Release/net8.0/linux-arm64/publish
./SP_08