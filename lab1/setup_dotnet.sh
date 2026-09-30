#!/bin/bash
# setup_dotnet_arch_arm64.sh — установка .NET SDK на Arch Linux ARM64 (aarch64).
#
# Зачем скрипт: в репозиториях Arch Linux ARM пакета dotnet-sdk нет
# (в archlinux.org он только под x86_64). Собирать .NET из исходников НЕ нужно —
# Microsoft публикует готовые бинарники linux-arm64, ставится официальным
# установщиком dotnet-install.sh.
#
# Использование:
#   chmod +x setup_dotnet.sh && ./setup_dotnet.sh
#
# Что делает:
#   1. проверяет, что система aarch64;
#   2. ставит зависимости (gcc, curl, ca-certificates, icu);
#   3. скачивает dotnet-install.sh;
#   4. ставит SDK в ~/.dotnet (без root);
#   5. прописывает PATH в ~/.bashrc;
#   6. проверяет результат.
#
# Аргументы: ./setup_dotnet.sh [версия-канала]   (по умолчанию 8.0)

set -e

CHANNEL="${1:-8.0}"
INSTALL_DIR="$HOME/.dotnet"

echo "=============================================="
echo " Установка .NET SDK $CHANNEL на Arch Linux ARM64"
echo "=============================================="

# --- 1. Проверка архитектуры ---------------------------------------------
ARCH="$(uname -m)"
echo "[1/6] Архитектура: $ARCH"
if [ "$ARCH" != "aarch64" ] && [ "$ARCH" != "arm64" ]; then
    echo "Этот скрипт для aarch64/arm64. На $ARCH ставь .NET иначе (см. README)."
    exit 1
fi

# --- 2. Зависимости -------------------------------------------------------
echo "[2/6] Зависимости..."
if command -v pacman >/dev/null 2>&1; then
    # icu обязателен: без него dotnet падает с
    # "Couldn't find a valid ICU package installed on the system"
    sudo pacman -S --needed --noconfirm gcc curl ca-certificates icu
else
    echo "    pacman не найден (это не Arch?) — ставлю через apt для теста"
    if command -v apt-get >/dev/null 2>&1; then
        SUDO=""; [ "$(id -u)" -ne 0 ] && SUDO="sudo"
        $SUDO apt-get update -qq
        $SUDO apt-get install -y -qq curl ca-certificates libicu-dev >/dev/null
    fi
fi

# --- 3. Скачать установщик -------------------------------------------------
echo "[3/6] Скачиваю dotnet-install.sh..."
cd "$HOME"
curl -sSL https://dot.net/v1/dotnet-install.sh -o dotnet-install.sh
chmod +x dotnet-install.sh
echo "    OK"

# --- 4. Установить SDK ----------------------------------------------------
echo "[4/6] Ставлю SDK $CHANNEL в $INSTALL_DIR (это займёт минуту)..."
./dotnet-install.sh --channel "$CHANNEL" --install-dir "$INSTALL_DIR"
echo "    OK"

# --- 5. PATH в ~/.bashrc ---------------------------------------------------
echo "[5/6] Прописываю PATH в ~/.bashrc..."
export PATH="$INSTALL_DIR:$PATH"
if grep -q 'HOME/.dotnet' "$HOME/.bashrc" 2>/dev/null; then
    echo "    Уже прописано, пропускаю"
else
    printf '\n# .NET SDK\nexport DOTNET_ROOT="$HOME/.dotnet"\nexport PATH="$HOME/.dotnet:$PATH"\n' >> "$HOME/.bashrc"
    echo "    Дописал в ~/.bashrc"
fi

# --- 6. Проверка -----------------------------------------------------------
echo "[6/6] Проверяю..."
echo "    dotnet --version  → $(dotnet --version 2>&1 | head -1)"
echo "    --- рантаймы ---"
dotnet --list-runtimes 2>&1 | sed 's/^/    /'

echo
echo "=============================================="
echo " Готово. Проверь в новом терминале:"
echo "   dotnet --version"
echo "   dotnet new console -o /tmp/probe && cd /tmp/probe && dotnet run"
echo "=============================================="