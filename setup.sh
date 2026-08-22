#!/bin/bash
# Setup inicial do projeto ROMs Manager NS
# Inicializa o submodule, copia os resources e verifica as dependências

set -e

echo "=== ROMs Manager NS - Setup ==="
echo ""

# Submodule do Borealis
echo "[1/4] Inicializando submodule..."
git submodule update --init --recursive

# Sanidade: conferir o CMakeLists do Borealis, que é o build real.
# (Antes esta verificação procurava borealis.mk, que não existe na branch
#  wiliwili do fork — o script falhava sempre neste ponto.)
if [ -f "library/library/CMakeLists.txt" ]; then
    echo "      ✓ Borealis encontrado"
else
    echo "      ✗ ERRO: library/library/CMakeLists.txt não encontrado"
    echo "        O submodule não foi clonado corretamente."
    exit 1
fi

# Resources (fontes, i18n, ícones). Sem isso nenhum texto renderiza no PC.
echo "[2/4] Copiando resources do Borealis..."
./setup-resources.sh > /dev/null
echo "      ✓ resources/font, i18n, img, material"

# Docker — caminho padrão para gerar o .nro
echo "[3/4] Verificando Docker..."
if command -v docker &> /dev/null; then
    echo "      ✓ Docker disponível ($(docker --version | cut -d' ' -f3 | tr -d ','))"
else
    echo "      ⚠ Docker não encontrado. É o caminho padrão para 'make build'."
fi

# Dependências do build PC
echo "[4/4] Verificando dependências do build PC..."
missing=""
command -v cmake &> /dev/null || missing="$missing cmake"
command -v pkg-config &> /dev/null || missing="$missing pkg-config"

if [ -z "$missing" ]; then
    echo "      ✓ cmake e pkg-config disponíveis"
else
    echo "      ⚠ Faltando:$missing"
    echo "        Ubuntu/Debian: sudo apt install build-essential cmake pkg-config libglfw3-dev libglm-dev"
fi

echo ""
echo "=== Setup concluído ==="
echo ""
echo "  make pc       testa no PC"
echo "  make build    gera o .nro (Docker)"
echo "  make install  instala no Switch via FTP"
echo ""
echo "Antes de sincronizar, ajuste o IP do servidor em config.json."
