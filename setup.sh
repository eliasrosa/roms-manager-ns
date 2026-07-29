#!/bin/bash
# Setup inicial do projeto ROM Manager NS
# Inicializa submodules e verifica dependências

set -e

echo "=== ROMs Manager NS - Setup ==="
echo ""

# Inicializar submodules
echo "[1/3] Inicializando submodules..."
git submodule update --init --recursive

# Verificar se borealis está presente
if [ -f "library/library/borealis.mk" ]; then
    echo "      ✓ Borealis encontrado"
else
    echo "      ✗ ERRO: borealis.mk não encontrado em library/library/"
    echo "        Verifique se o submodule foi clonado corretamente."
    exit 1
fi

# Verificar Docker
echo "[2/3] Verificando Docker..."
if command -v docker &> /dev/null; then
    echo "      ✓ Docker disponível ($(docker --version | cut -d' ' -f3 | tr -d ','))"
else
    echo "      ⚠ Docker não encontrado. Instale para usar ./build.sh"
    echo "        Alternativamente, instale devkitPro localmente."
fi

# Verificar devkitPro (opcional)
echo "[3/3] Verificando devkitPro..."
if [ -n "$DEVKITPRO" ] && [ -d "$DEVKITPRO" ]; then
    echo "      ✓ devkitPro encontrado em $DEVKITPRO"
else
    echo "      ⚠ devkitPro não configurado (ok se usar Docker)"
fi

echo ""
echo "=== Setup concluído! ==="
echo ""
echo "Para buildar:"
echo "  Docker:  ./build.sh"
echo "  Local:   make -j\$(nproc)"
echo ""
echo "Output: roms-manager-ns.nro → sdmc:/switch/roms-manager-ns/"
