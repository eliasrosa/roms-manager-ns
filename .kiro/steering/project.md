# ROMs Manager NS — Contexto do Projeto

## O que é

App homebrew para Nintendo Switch (Atmosphère) que gerencia ROMs no SD card e sincroniza via WiFi com um servidor local.

## Stack

- **Linguagem**: C++17
- **UI**: Borealis (branch main, API nova com XML + Activity)
- **GPU**: deko3d (Switch) / OpenGL+GLFW (PC)
- **Rede**: Sockets BSD (sem libcurl)
- **SDK**: devkitPro / devkitA64 / libnx
- **Build**: Make (Switch + PC) / Meson (PC) / Docker (Switch)
- **Servidor**: Python 3 stdlib

## Estrutura principal

- `src/` — código C++ do app
- `src/views/` — componentes de UI (herdam de brls::Box)
- `src/sync/` — módulo de sync HTTP (config, http_client, sync_manager)
- `server/` — servidor Python de sync
- `resources/xml/` — layouts XML do Borealis
- `library/` — submodule Borealis
- `test_sd/` — SD card fake para testes no PC
- `config.json` — configuração do app

## Convenções de código

- Nomes de variáveis/funções em inglês (camelCase para C++)
- Comentários e docs em português
- Includes: borealis primeiro, depois std, depois nossos headers
- Namespace do sync: `netsync::` (não `sync` — conflita com POSIX)
- Plataforma: usar `platform::sdRoot()` em vez de hardcoded `sdmc:/`
- Views customizadas: herdar de `brls::Box`, implementar `static View* create()`

## Build

```bash
make pc          # testa no PC
make watch       # hot reload
make serve       # servidor sync
make build       # gera .nro (Docker)
make deploy      # envia via nxlink
make install     # copia via FTP
```

## Variáveis de ambiente

- `SWITCH_IP` — IP do Switch na rede (default: 192.168.0.2)
- `SWITCH_FTP_PORT` — porta do ftpd (default: 5000)
- `DEVKITPRO` — path do devkitPro (só para build local sem Docker)
