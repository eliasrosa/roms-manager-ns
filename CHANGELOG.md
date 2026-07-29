# Changelog

Todas as mudanças notáveis do projeto serão documentadas aqui.

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.0.0/).

## [0.2.0] - 2026-07-29

### Adicionado
- Interface gráfica com Borealis (visual nativo Switch)
- Tab Sync com log estilo terminal
- Sincronização HTTP via WiFi com servidor local
- Servidor Python (`server/serve.py`) com manifest auto-gerado
- Configuração via `config.json` (servidor, paths, filtros)
- Build para PC via meson/ninja (teste sem console)
- Hot reload com `make watch`
- Deploy via nxlink (`make deploy`)
- Instalação via FTP (`make install`)
- Dockerfile para build Switch sem setup local
- Abstração de plataforma (Switch/PC)

### Técnico
- Borealis branch main (API nova: Activity + XML + Box)
- HTTP client com sockets BSD (sem libcurl)
- Workaround GCC 13+ (cstdint, optional)
- Workaround swkbd API (libnx atualizado)
- Namespace `netsync::` (evita conflito POSIX)

## [0.1.0] - 2026-07-29

### Adicionado
- Versão inicial com console text-mode
- Navegador de arquivos do SD card
- Navegação com D-pad
- Listagem de diretórios/arquivos
- Build via Docker
