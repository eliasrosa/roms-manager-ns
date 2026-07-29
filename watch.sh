#!/bin/bash
# Hot reload para ROMs Manager NS
# Monitora src/ e resources/ — ao detectar mudança, rebuilda e reinicia o app
#
# Dependência: sudo apt install inotify-tools
# Uso: ./watch.sh

set -e

BUILD_DIR="build-pc"
BIN="./$BUILD_DIR/roms-manager-ns"
PID=""

# Cores
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m'

cleanup() {
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        kill "$PID" 2>/dev/null
        wait "$PID" 2>/dev/null
    fi
    echo -e "\n${YELLOW}[watch] Encerrado.${NC}"
    exit 0
}

trap cleanup EXIT INT TERM

# Garantir que build-pc existe
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}[watch] Configurando meson...${NC}"
    meson setup "$BUILD_DIR"
fi

build_and_run() {
    # Matar processo anterior
    if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
        kill "$PID" 2>/dev/null
        wait "$PID" 2>/dev/null
    fi

    echo -e "${YELLOW}[watch] Compilando...${NC}"

    if ninja -C "$BUILD_DIR" 2>&1 | tail -5; then
        echo -e "${GREEN}[watch] Build OK — iniciando app...${NC}"
        $BIN &
        PID=$!
    else
        echo -e "${RED}[watch] Build FALHOU — aguardando correção...${NC}"
        PID=""
    fi
}

echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
echo -e "${GREEN}║   ROMs Manager NS — Watch Mode      ║${NC}"
echo -e "${GREEN}╠══════════════════════════════════════╣${NC}"
echo -e "${GREEN}║  Monitorando: src/ resources/        ║${NC}"
echo -e "${GREEN}║  Ctrl+C para parar                  ║${NC}"
echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
echo ""

# Build inicial
build_and_run

# Monitorar mudanças
while true; do
    inotifywait -r -q -e modify,create,delete \
        --include '\.(cpp|hpp|h|c|xml|json)$' \
        src/ resources/ config.json 2>/dev/null

    echo -e "\n${YELLOW}[watch] Mudança detectada!${NC}"
    sleep 0.3  # debounce
    build_and_run
done
