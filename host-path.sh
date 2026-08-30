#!/bin/bash
# Traduz um caminho do container para o caminho equivalente no host.
#
# Por que existe: o daemon Docker resolve o source de '-v' no filesystem do
# HOST. Rodando de dentro de um container, $(CURDIR) é um caminho que só existe
# ali — o daemon não o encontra e cria um diretório vazio no lugar. O container
# efêmero recebe então um diretório onde deveria haver o arquivo. Foi assim que
# um deploy enviou um .nro inexistente (nxlink reportou INT64_MAX de tamanho) e
# travou o netloader do Switch, com o erro aparecendo do lado do console.
#
# Uso: ./host-path.sh [caminho]        (default: diretório atual)
#
# Fora de um container, sem acesso ao daemon, ou se a tradução falhar, imprime o
# caminho inalterado — o comportamento no host continua idêntico ao de antes.

set -u

target="${1:-$PWD}"

# Fora de container não há nada a traduzir.
[ -f /.dockerenv ] || { printf '%s\n' "$target"; exit 0; }

# Sem o CLI/socket do Docker não há como descobrir os mounts.
command -v docker >/dev/null 2>&1 || { printf '%s\n' "$target"; exit 0; }

id="$(tr -d '[:space:]' < /etc/hostname 2>/dev/null)"
[ -n "$id" ] || { printf '%s\n' "$target"; exit 0; }

mounts="$(docker inspect "$id" \
    --format '{{range .Mounts}}{{.Destination}}|{{.Source}}{{"\n"}}{{end}}' 2>/dev/null)" \
    || { printf '%s\n' "$target"; exit 0; }

# Escolhe o mount cujo Destination é o prefixo MAIS LONGO do alvo. Com mounts
# aninhados (ex.: ./data em /home/kirocrew e ./ em .../dev/<repo>) apenas o mais
# específico mapeia para o lugar certo.
best_dest=""
best_src=""

while IFS='|' read -r dest src; do
    [ -n "$dest" ] || continue
    case "$target" in
        "$dest" | "$dest"/*)
            if [ "${#dest}" -gt "${#best_dest}" ]; then
                best_dest="$dest"
                best_src="$src"
            fi
            ;;
    esac
done <<EOF
$mounts
EOF

[ -n "$best_dest" ] || { printf '%s\n' "$target"; exit 0; }

printf '%s%s\n' "$best_src" "${target#"$best_dest"}"
