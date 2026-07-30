#!/bin/bash
# Script de build via Docker para ROM Manager NS (Borealis)
# Uso: ./build.sh

set -e

IMAGE_NAME="roms-manager-ns-builder"
CONTAINER_NAME="roms-manager-ns-build"

echo "=== ROMs Manager NS - Build (Borealis UI) ==="
echo ""

# Verificar se submodule está presente
if [ ! -f "library/library/borealis.mk" ]; then
    echo "[!] Submodule borealis não encontrado. Inicializando..."
    git submodule update --init --recursive
fi

# Limpar container anterior se existir
docker rm -f "$CONTAINER_NAME" > /dev/null 2>&1 || true

# Build da imagem
echo "[1/3] Construindo imagem Docker..."
docker build -t "$IMAGE_NAME" .

# Rodar container e copiar output
echo "[2/3] Extraindo .nro..."
docker create --name "$CONTAINER_NAME" "$IMAGE_NAME" > /dev/null 2>&1
docker cp "$CONTAINER_NAME:/app/build.nx/roms-manager-ns.nro" ./roms-manager-ns.nro
docker rm "$CONTAINER_NAME" > /dev/null 2>&1

echo "[3/3] Pronto!"
echo ""
echo "Output: ./roms-manager-ns.nro"
echo "Copie para: sdmc:/switch/roms-manager-ns/roms-manager-ns.nro"
