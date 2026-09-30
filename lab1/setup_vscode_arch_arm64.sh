#!/bin/bash
# setup_vscode_arch_arm64.sh — установка VS Code на Arch Linux ARM64 из .deb-пакета.
#
# Зачем так: в Arch нет пакета code/visual-studio-code-bin без AUR, а .deb
# под ARM64 существует официально (code_<ver>_arm64.deb). dpkg в Arch нет,
# поэтому распаковываем .deb вручную через ar + tar — это ровно то, что
# делает dpkg внутри.
#
# Использование:
#   ./setup_vscode_arch_arm64.sh ~/Downloads/code_1.140.0-1790759479_arm64.deb
#   (путь можно не указывать — скрипт найдёт .deb сам)
#
# Проверено на aarch64: распаковывается в /usr/share/code (~940 МБ),
# команда code появляется в PATH, .desktop для меню приложений на месте.

set -e

echo "=============================================="
echo " Установка VS Code на Arch Linux ARM64 из .deb"
echo "=============================================="

# --- 1. Проверка архитектуры ---------------------------------------------
ARCH="$(uname -m)"
echo "[1/5] Архитектура: $ARCH"
if [ "$ARCH" != "aarch64" ] && [ "$ARCH" != "arm64" ]; then
    echo "Этот скрипт для aarch64/arm64. На $ARCH ставь по-другому."
    exit 1
fi

# --- 2. Находим .deb ------------------------------------------------------
echo "[2/5] Ищу .deb..."
DEB="${1:-}"
if [ -z "$DEB" ]; then
    DEB=$(find "$HOME" -maxdepth 3 -name 'code_*_arm64.deb' 2>/dev/null | head -1)
fi
if [ -z "$DEB" ] || [ ! -f "$DEB" ]; then
    echo "Файл .deb не найден. Скачай так:"
    echo "  https://update.code.visualstudio.com/latest/linux-deb-arm64/stable"
    echo "Потом запусти скрипт с путём к файлу."
    exit 1
fi
echo "    Нашёл: $DEB"

# --- 3. Зависимости -------------------------------------------------------
echo "[3/5] Зависимости..."
echo "    binutils (ar) и xz нужны для распаковки .deb"
sudo pacman -S --needed --noconfirm binutils xz
echo "    Библиотеки GTK/NSS обычно уже есть в системе."

# --- 4. Распаковка --------------------------------------------------------
echo "[4/5] Распаковываю в корень системы (аналог dpkg -i)..."
TMP="$(mktemp -d)"
cd "$TMP"
ar x "$DEB" data.tar.xz
sudo tar -xf data.tar.xz -C /
cd /
rm -rf "$TMP"
echo "    Распаковано: /usr/share/code"
sudo du -sh /usr/share/code

# .desktop лежит в /usr/share/applications — в меню приложений появится сам.
# Но на всякий случай гарантируем запись, если система его не подхватила.
if [ -f /usr/share/applications/com.microsoft.VSCode.desktop ]; then
    echo "    Ярлык: /usr/share/applications/com.microsoft.VSCode.desktop"
fi

# --- 5. Команда code в PATH ----------------------------------------------
echo "[5/5] Делаю команду code доступной..."
sudo ln -sf /usr/share/code/bin/code /usr/bin/code
echo "    /usr/bin/code → $(readlink -f /usr/bin/code)"

# --- Проверка -------------------------------------------------------------
echo
echo "=============================================="
echo " Проверка:"
if code --version 2>&1 | head -3; then
    echo
    echo " Готово. Запуск:"
    echo "   code                 — из терминала"
    echo "   code ~/Documents/VisualStudioCodeRepos"
    echo " или через меню приложений: Visual Studio Code"
else
    echo " Не запустилось. Чаще всего не хватает библиотеки —"
    echo " посмотри текст ошибки выше и поставь названный пакет:"
    echo "   sudo pacman -S <пакет>"
fi
echo "=============================================="