#!/bin/bash
# Configura os resources do Borealis para o projeto
# Copia assets do borealis para resources/ (merge com nossos XMLs)
#
# Uso: ./setup-resources.sh (rodar após git submodule update --init --recursive)

set -e

BOREALIS_RES="library/resources"
APP_RES="resources"
APP_I18N="i18n"

echo "=== Setup de resources ==="
echo ""

# Verificar se borealis foi clonado
if [ ! -d "$BOREALIS_RES" ]; then
    echo "ERRO: Resources do Borealis não encontrados em $BOREALIS_RES"
    echo "Execute: git submodule update --init --recursive"
    exit 1
fi

# Copiar cada pasta de recursos do borealis (sem sobrescrever nossos XMLs)
#
# 'font' é obrigatório: sem switch_font.ttf e switch_icons.ttf nenhum texto
# renderiza no PC. Antes ficava de fora e a cópia era manual.
# 'inter' saiu da lista — não existe mais no Borealis e só gerava aviso falso.
for dir in font i18n img material; do
    src="$BOREALIS_RES/$dir"
    dest="$APP_RES/$dir"

    if [ -d "$src" ]; then
        # Remover symlink se existir
        [ -L "$dest" ] && rm "$dest"
        # Copiar
        rm -rf "$dest"
        cp -r "$src" "$dest"
        echo "  ✓ $dir copiado"
    else
        echo "  ⚠ $dir não encontrado em borealis"
    fi
done

# Garantir que pasta icon existe
mkdir -p "$APP_RES/icon"

# Traduções mantidas pelo projeto (i18n/), copiadas DEPOIS das do Borealis.
#
# O Borealis só traz en-US, fr, ru e zh-Hans. Idiomas adicionais vivem em
# i18n/ (versionado) porque o loop acima faz 'rm -rf' em resources/i18n e
# apagaria qualquer coisa colocada lá manualmente.
if [ -d "$APP_I18N" ]; then
    echo ""
    for locale_dir in "$APP_I18N"/*/; do
        [ -d "$locale_dir" ] || continue
        locale=$(basename "$locale_dir")
        mkdir -p "$APP_RES/i18n/$locale"
        cp -r "$locale_dir." "$APP_RES/i18n/$locale/"
        echo "  ✓ i18n/$locale (do projeto) mesclado"
    done
fi

# Verificar se icon.jpg existe
if [ ! -f "$APP_RES/icon/icon.jpg" ]; then
    echo ""
    echo "  ⚠ Nenhum icon.jpg encontrado em $APP_RES/icon/"
    echo "    O app vai usar o ícone padrão."
fi

echo ""
echo "=== Resources configurados! ==="
