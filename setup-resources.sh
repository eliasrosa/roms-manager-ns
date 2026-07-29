#!/bin/bash
# Configura os resources do Borealis para build PC
# Cria symlinks dos assets do borealis em resources/
#
# Uso: ./setup-resources.sh (rodar após git submodule update --init --recursive)

set -e

BOREALIS_RES="library/resources"
APP_RES="resources"

echo "=== Setup de resources para build PC ==="
echo ""

# Verificar se borealis foi clonado
if [ ! -d "$BOREALIS_RES" ]; then
    echo "ERRO: Resources do Borealis não encontrados em $BOREALIS_RES"
    echo "Execute: git submodule update --init --recursive"
    exit 1
fi

# Linkar cada pasta de recursos do borealis
for dir in i18n img inter material xml; do
    src="$BOREALIS_RES/$dir"
    dest="$APP_RES/$dir"

    if [ -d "$src" ]; then
        # Remover link/pasta anterior se existir
        rm -rf "$dest"
        # Criar symlink relativo
        ln -sf "../$src" "$dest"
        echo "  ✓ $dir -> $src"
    else
        echo "  ⚠ $dir não encontrado em borealis (pode não ser necessário)"
    fi
done

# Garantir que pasta icon existe
mkdir -p "$APP_RES/icon"

# Verificar se icon.jpg existe, senão avisar
if [ ! -f "$APP_RES/icon/icon.jpg" ]; then
    echo ""
    echo "  ⚠ Nenhum icon.jpg encontrado em $APP_RES/icon/"
    echo "    O app vai usar o ícone padrão do borealis."
    echo "    Para customizar, coloque um icon.jpg 256x256 nessa pasta."
fi

echo ""
echo "=== Resources configurados! ==="
echo "Agora pode buildar com: ./build-pc.sh"
