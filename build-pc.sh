#!/bin/bash
# Build e execução do ROM Manager NS no PC (Linux)
# Usa meson/ninja para compilar com GLFW (janela nativa)
#
# Dependências (instalar antes):
#   Ubuntu/Debian: sudo apt install build-essential meson ninja-build pkg-config libglfw3-dev libglm-dev
#   Arch:          sudo pacman -S meson ninja glfw glm
#   Fedora:        sudo dnf install meson ninja-build glfw-devel glm-devel

set -e

BUILD_DIR="build-pc"

echo "=== ROMs Manager NS - Build PC ==="
echo ""

# Verificar dependências
for cmd in meson ninja pkg-config; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "ERRO: '$cmd' não encontrado. Instale as dependências:"
        echo "  sudo apt install build-essential meson ninja-build pkg-config libglfw3-dev libglm-dev"
        exit 1
    fi
done

# Verificar se submodule está presente
if [ ! -f "library/meson.build" ]; then
    echo "[!] Submodule borealis não encontrado. Inicializando..."
    git submodule update --init --recursive
fi

# Setup resources (symlinks)
if [ ! -L "resources/i18n" ]; then
    echo "[1/4] Configurando resources..."
    ./setup-resources.sh
else
    echo "[1/4] Resources já configurados ✓"
fi

# Configurar meson (só na primeira vez ou se não existir)
if [ ! -d "$BUILD_DIR" ]; then
    echo "[2/4] Configurando meson..."
    meson setup "$BUILD_DIR"
else
    echo "[2/4] Build dir já existe ✓"
fi

# Compilar
echo "[3/4] Compilando..."
ninja -C "$BUILD_DIR"

echo "[4/4] Build concluído!"
echo ""
echo "─────────────────────────────────────────"
echo " Executável: ./$BUILD_DIR/roms-manager-ns"
echo " Rodando..."
echo "─────────────────────────────────────────"
echo ""

# Rodar (precisa estar na raiz do projeto para encontrar resources/ e test_sd/)
./"$BUILD_DIR"/roms-manager-ns
