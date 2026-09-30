#!/bin/bash
# demo.sh — демонстрация лабораторной работы 1 преподавателю.
#
# Запуск:  cd ~/lab1 && bash demo.sh
# Или из клона: cd ~/Documents/VisualStudioCodeRepos/SystemProgramming/lab1 && bash demo.sh
#
# Скрипт показывает ВСЕ задания Linux-части (03, 04, 07, 08) и для каждого
# выводит ДОКАЗАТЕЛЬСТВО, что оно сделано правильно:
#   - задание 03: библиотека прилинкована (readelf показывает NEEDED)
#   - задание 04: библиотека НЕ прилинкована (NEEDED её не содержит)
#   - задание 03: без LD_LIBRARY_PATH падает, с ним — работает
#   - задание 07/08: оба вызывают одну и ту же нативную библиотеку
#
# Windows-часть (01, 02, 05, 06) делается в Visual Studio вручную — см. README.

export DOTNET_ROOT="$HOME/.dotnet"
export PATH="$HOME/.dotnet:$PATH"
export DOTNET_CLI_TELEMETRY_OPTOUT=1

LAB="$(cd "$(dirname "$0")" && pwd)"
cd "$LAB"

line() { printf '\n\033[1m%s\033[0m\n' "$*"; }
sub()  { printf '\033[0;36m%s\033[0m\n' "$*"; }
ok()   { printf '  \033[0;32m%s\033[0m\n' "$*"; }

echo "################################################################"
echo "#  ЛАБОРАТОРНАЯ РАБОТА 1 — ДЕМОНСТРАЦИЯ"
echo "################################################################"

# ─────────────────────────────────────────────────────────────────
line "ОКРУЖЕНИЕ"
sub "uname -m        -> $(uname -m)"
sub "g++             -> $(g++ --version | head -1)"
sub "dotnet          -> $(dotnet --version)"

# ─────────────────────────────────────────────────────────────────
line "ЗАДАНИЕ 03 — библиотека SP.so + СТАТИЧЕСКИЙ вызов"

sub "Сборка библиотеки:"
( cd linux/SP_lib && g++ -c SP.cpp -o SP.o && g++ -shared SP.o -o SP.so )
sub "  g++ -c SP.cpp -o SP.o && g++ -shared SP.o -o SP.so"

sub "Доказательство: библиотека экспортирует функции из таблицы 1:"
( cd linux/SP_lib && nm -D SP.so | grep -E ' T (sum|sub|mul|div)$' | sed 's/^/  /' )

sub "Сборка приложения:"
( cd linux/SP_03 && cp -f ../SP_lib/SP.so . && g++ main.cpp SP.so -o SP_03 )
sub "  g++ main.cpp SP.so -o SP_03"

sub "Доказательство статической линковки — SP.so в списке NEEDED:"
( cd linux/SP_03 && readelf -d SP_03 | grep NEEDED | grep SP.so | sed 's/^/  /' )
ok "библиотека известна линкеру на этапе сборки"

sub "Без LD_LIBRARY_PATH (загрузчик не находит библиотеку):"
( cd linux/SP_03 && env -u LD_LIBRARY_PATH ./SP_03 2>&1 | head -1 | sed 's/^/  /' )

sub "С LD_LIBRARY_PATH=.:"
( cd linux/SP_03 && LD_LIBRARY_PATH=. ./SP_03 | sed 's/^/  /' )
ok "задание 03 выполнено"

# ─────────────────────────────────────────────────────────────────
line "ЗАДАНИЕ 04 — ДИНАМИЧЕСКИЙ вызов через dlopen"

sub "Сборка приложения (без линковки с библиотекой!):"
( cd linux/SP_04 && cp -f ../SP_lib/SP.so . && g++ main.cpp -ldl -o SP_04 )
sub "  g++ main.cpp -ldl -o SP_04"

sub "Доказательство: SP.so в NEEDED ОТСУТСТВУЕТ — библиотека не прилинкована:"
CNT=$( cd linux/SP_04 && readelf -d SP_04 | grep -c "SP.so" )
sub "  вхождений 'SP.so' в NEEDED: $CNT"
[ "$CNT" -eq 0 ] && ok "подтверждено: на этапе сборки проект не знает о библиотеке"

sub "Запуск (LD_LIBRARY_PATH не нужен — путь задан в dlopen):"
( cd linux/SP_04 && ./SP_04 | sed 's/^/  /' )
ok "задание 04 выполнено"

# ─────────────────────────────────────────────────────────────────
line "ЗАДАНИЕ 07 — C#, СТАТИЧЕСКИЙ вызов (DllImport)"

sub "Сборка (self-contained, один файл, .NET внутри):"
( cd linux/cs/SP_07 && dotnet publish -c Release -r linux-arm64 \
    --self-contained true -p:PublishSingleFile=true -v q --nologo 2>&1 | tail -1 )
cp -f linux/SP_lib/SP.so linux/cs/SP_07/bin/Release/net8.0/linux-arm64/publish/
sub "  dotnet publish -c Release -r linux-arm64 --self-contained true -p:PublishSingleFile=true"

sub "Доказательство: библиотека лежит рядом с бинарником, вызов через DllImport:"
( cd linux/cs/SP_07/bin/Release/net8.0/linux-arm64/publish \
  && ls -lh SP_07 SP.so | awk '{print "  " $5 "\t" $9}' )

sub "Запуск:"
( cd linux/cs/SP_07/bin/Release/net8.0/linux-arm64/publish && ./SP_07 | sed 's/^/  /' )
ok "задание 07 выполнено"

# ─────────────────────────────────────────────────────────────────
line "ЗАДАНИЕ 08 — C#, ДИНАМИЧЕСКИЙ вызов (dlopen)"

sub "Сборка:"
( cd linux/cs/SP_08 && dotnet publish -c Release -r linux-arm64 \
    --self-contained true -p:PublishSingleFile=true -v q --nologo 2>&1 | tail -1 )
cp -f linux/SP_lib/SP.so linux/cs/SP_08/bin/Release/net8.0/linux-arm64/publish/

sub "Доказательство: путь ./SP.so задан ЯВНО в NativeLibrary.Load,"
sub "в отличие от DllImport(\"SP\") в задании 07."
sub "Запуск (обязательно из папки с библиотекой — путь относительный):"
( cd linux/cs/SP_08/bin/Release/net8.0/linux-arm64/publish && ./SP_08 | sed 's/^/  /' )
ok "задание 08 выполнено"

# ─────────────────────────────────────────────────────────────────
line "ВЫВОД — ВСЕ ЧЕТЫРЕ ЗАДАНИЯ"

printf '  %-8s %-22s %s\n' "задание" "вывод" "результат"
printf '  %-8s %-22s %s\n' "03" "static link (SP.so)" \
       "$( cd linux/SP_03 && LD_LIBRARY_PATH=. ./SP_03 | sed -n '2p' )"
printf '  %-8s %-22s %s\n' "04" "dynamic link (dlopen)" \
       "$( cd linux/SP_04 && ./SP_04 | sed -n '2p' )"
printf '  %-8s %-22s %s\n' "07" "DllImport из C#" \
       "$( cd linux/cs/SP_07/bin/Release/net8.0/linux-arm64/publish && ./SP_07 | sed -n '1p' )"
printf '  %-8s %-22s %s\n' "08" "dlopen из C#" \
       "$( cd linux/cs/SP_08/bin/Release/net8.0/linux-arm64/publish && ./SP_08 | sed -n '1p' )"

echo
echo "################################################################"
echo "#  Windows-часть (01, 02, 05, 06) — Visual Studio, вручную."
echo "#  Подробный порядок: README.md, раздел «Windows — задания»"
echo "################################################################"