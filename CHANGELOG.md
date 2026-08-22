# Changelog

Todas as mudanças notáveis do projeto serão documentadas aqui.

Formato baseado em [Keep a Changelog](https://keepachangelog.com/pt-BR/1.0.0/).

## [Não lançado]

### Adicionado
- Modo debug com dois canais de log: arquivo no SD card
  (`sdmc:/switch/roms-manager-ns/debug.log`, com rotação por sessão) e envio em
  tempo real para o PC na porta 28771 — este último funciona mesmo com o app
  instalado por FTP e aberto pelo hbmenu, o que o nxlink sozinho não permite
- Targets `make install-debug`, `make logs`, `make logs-live` e `make logs-clean`
- Handler de `std::set_terminate` que loga tipo e `what()` da exceção nos dois
  canais antes de abortar
- Header com espaço livre do microSD e do sistema, e indicador de WiFi com
  spinner durante o teste de conexão
- Tab de configurações

### Corrigido
- App morria ao testar conexão ou iniciar sync: `std::thread` lança
  `std::system_error(ENOSYS)` no devkitA64, que usa os stubs single-thread de
  gthreads. Substituído por `brls::async`, que enfileira na task loop thread do
  Borealis
- `std::string(strerror(errno))` podia receber `NULL` no newlib e derrubar o
  processo; os erros de socket do Switch vêm do serviço `bsd:s`, com códigos fora
  da tabela padrão
- `gethostbyname` sem validar `h_addr_list`/`h_length` antes do `memcpy`
- Default de `server.host` divergia: o parser caía em `192.168.1.100` quando o
  campo vinha vazio, enquanto o default declarado era `192.168.0.100`
- `Falha ao buscar manifest:` aparecia sem motivo quando o servidor respondia
  404, porque nesse caso o HTTP teve sucesso e a mensagem de erro fica vazia
- Logs do nxlink chegavam fora de ordem ou não chegavam: o `fflush` do Logger do
  Borealis só existia em `__MINGW32__`

### Alterado
- Servidor de sync movido para repositório próprio e reescrito em Node.js +
  Express + MongoDB: [roms-manager-server](https://github.com/eliasrosa/roms-manager-server).
  O `server/` local e o `serve.py` deixaram de existir aqui
- Submodule do Borealis passou a apontar para o fork
  [eliasrosa/borealis](https://github.com/eliasrosa/borealis) (branch `wiliwili`),
  que carrega os patches locais
- Build do PC migrado de meson/ninja para CMake

### Conhecido
- O sync ainda chama `GET /manifest.json`, endpoint que **não existe** no
  servidor Node.js — a migração para `GET /roms` está pendente
- `sync.verify_hash` é lido e ignorado: `shouldDownload()` compara apenas
  existência e tamanho do arquivo
- O build Switch pelo `Makefile`/devkitPro está morto (depende de `borealis.mk`,
  ausente nesta branch do fork). O caminho válido é Docker → CMake, e por isso
  `make clean`/`clean-all` não funcionam no uso normal

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
