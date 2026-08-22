# ROMs Manager NS — Contexto do Projeto

## O que é

App homebrew para Nintendo Switch (Atmosphère) que gerencia ROMs no SD card e sincroniza via WiFi com um servidor local.

- **Repositório (app)**: github.com/eliasrosa/roms-manager-ns
- **Repositório (server)**: github.com/eliasrosa/roms-manager-server
- **Borealis (fork)**: github.com/eliasrosa/borealis
- **Autor**: elfarelo (eliasrosa)
- **Licença**: MIT

## Stack

- **Linguagem**: C++17
- **UI**: Borealis (fork xfangfang, branch `wiliwili`)
- **GPU**: deko3d (Switch) / OpenGL+GLFW (PC)
- **Rede**: Sockets BSD (sem libcurl)
- **SDK**: devkitPro / devkitA64 / libnx
- **Build**: Docker + CMake (Switch) / CMake (PC). O `Makefile` é só orquestração.
- **Servidor**: Node.js 24 + Express + MongoDB (repo separado: [roms-manager-server](https://github.com/eliasrosa/roms-manager-server))

## Estrutura principal

Código:

- `src/main.cpp` — entry point, handler de `std::set_terminate`, init do debug
- `src/main_activity.*` — activity raiz (header com storage e indicador WiFi)
- `src/platform.hpp` — abstração Switch/PC (`sdRoot()`, `romsPath()`, storage)
- `src/debug_log.*` — canais de diagnóstico (namespace `dbg`), ver `debug.md`
- `src/views/` — tabs de UI: `file_browser_tab`, `sync_tab`, `settings_tab`
- `src/sync/` — módulo HTTP: `config`, `http_client`, `sync_manager`

Build e recursos:

- `CMakeLists.txt` — build real (PC e Switch)
- `cmake/SwitchToolchain.cmake` — toolchain de cross-compile
- `Dockerfile` + `build.sh` — build Switch containerizado (`make build`)
- `watch.sh` — hot reload no PC (`make watch`)
- `setup.sh` — setup inicial (submodule + resources + checagem de deps)
- `setup-resources.sh` — copia os assets do Borealis para `resources/`
- `resources/xml/` — layouts XML do Borealis
- `library/` — submodule Borealis (**fork com patches**, ver `borealis-fork.md`)
- `icon.jpg` — ícone do .nro, referenciado pelo `elf2nro`

Não versionado (mas necessário):

- `resources/font`, `i18n`, `img`, `material` — gitignorados. Um clone limpo
  **não** tem esses assets e sem `font/` nenhum texto renderiza. Resolvido por
  `./setup.sh` (ou `./setup-resources.sh` direto).
- `config.local.json` — override do config no PC. `getConfigPath()` tenta esse
  arquivo antes do `config.json`. Útil para não sujar o config versionado.
- `test_sd/` — SD card fake para testes no PC

## Server — Endpoints (roms-manager-server)

| Método | Path | Descrição | Usado pelo app? |
|--------|------|-----------|-----------------|
| `GET` | `/health` | Status do servidor | ✅ `testConnection()` |
| `GET` | `/roms` | Lista ROMs (`?platform=`, `?crc32=`, `?md5=`) | ❌ ainda não |
| `GET` | `/roms/:platform/:filename` | Download direto da ROM | ❌ ainda não |
| `POST` | `/roms/sync?platform=gba` | Re-indexa ROMs no disco | ❌ ainda não |

### Fluxo de sync atual (legado — desalinhado do servidor)

`SyncManager::fetchManifest()` chama **`GET /manifest.json`**, que **não existe**
no servidor Node.js. Na prática o sync está quebrado: o log mostra
`Falha ao buscar manifest:` com erro vazio, porque o HTTP em si teve sucesso
(404) mas o parse não encontra nada.

1. `GET /manifest.json` → esperava lista com `path`, `size`, `md5`, `modified`
2. `shouldDownload()` compara **só existência e tamanho** do arquivo local
3. Download por URL direta (`baseUrl + path`)

⚠️ `sync.verify_hash` no config **não tem efeito** — há um
`// TODO: comparar MD5` em `sync_manager.cpp`. O campo existe mas é ignorado.

### Fluxo de sync planejado (migração pendente)

1. `GET /roms?platform=gba` → lista com `filename`, `size`, `crc32`
2. Comparar com storage local por filename + crc32
3. Baixar ausentes/divergentes via `GET /roms/:platform/:filename`
4. Verificar CRC32 após o download

## Convenções de código

- Nomes de variáveis/funções em inglês (camelCase para C++)
- Comentários e docs em português
- Includes: borealis primeiro, depois std, depois nossos headers
- Namespace do sync: `netsync::` (não `sync` — conflita com POSIX)
- Plataforma: usar `platform::sdRoot()` em vez de hardcoded `sdmc:/`
- Views customizadas: herdar de `brls::Box`, implementar `static View* create()`

## Build Switch (CMake cross-compile via Docker)

Cadeia de build completa para gerar .nro:

1. **Dockerfile**: imagem `devkitpro/devkita64:latest` + cmake (com retry no pacman)
2. **Toolchain**: `cmake/SwitchToolchain.cmake` (aarch64-none-elf-gcc, flags -D__SWITCH__, specs libnx)
3. **CMake flags obrigatórias**: `-DPLATFORM_SWITCH=ON -DPLATFORM_DESKTOP=OFF -DBOREALIS_USE_DEKO3D=ON -DBRLS_UNITY_BUILD=OFF`
4. **switch_wrapper.c**: DEVE ser incluído manualmente no target (CMakeLists do borealis só faz GLOB de *.cpp, ignora *.c). Contém `userAppInit()` que faz `romfsInit()`, `socketInitialize()`, `plInitialize()`, etc.
5. **Shaders**: compilar GLSL → DKSH com `uam` antes do build CMake. Output em `resources/shaders/`. O nanovg/deko3d busca em `romfs:/shaders/`.
6. **elf2nro**: após compilar, gerar .nro com `--romfsdir=resources` para empacotar shaders + i18n + material fonts.
7. **ENV PATH**: `/opt/devkitpro/devkitA64/bin:/opt/devkitpro/tools/bin` deve estar no PATH do Docker.
8. **nxlink logs**: definir `-DDEBUG` para `switch_wrapper.c` habilitar `nxlinkStdio()`.

### Problemas conhecidos

- `BRLS_UNITY_BUILD` deve ser OFF (variável não definida causa erro no CMake)
- `resources/xml` NÃO pode ser symlink — diretório real com nosso `main.xml`
- **`APP_SOURCES` no `CMakeLists.txt` é lista explícita, não GLOB.** Criar um
  `.cpp` novo em `src/` e esquecer de adicioná-lo ali resulta em erro de link
  sem pista óbvia.
- **O build Switch via `Makefile`/devkitPro está morto.** `Makefile` faz
  `include .../borealis.mk`, arquivo que não existe nesta branch do fork. Todo o
  bloco `ifeq ($(strip $(DEVKITPRO)),)`...`else` (~200 linhas de regras devkitPro)
  é código morto que quebra se alguém exportar `DEVKITPRO`. O caminho válido é
  Docker → CMake. Consequência prática: `make clean` e `make clean-all` só
  existem quando `DEVKITPRO` está definido, ou seja **falham no uso normal**.
- Porta 28771 do nxlink pode ficar presa após timeout. O target `deploy` já faz
  `pkill -f nxlink` automaticamente; ao rodar o nxlink na mão, matar antes.

## Variáveis do Makefile

| Variável | Default | Para que serve |
|---|---|---|
| `SWITCH_IP` | `192.168.0.150` | IP do Switch (nxlink e FTP) |
| `SWITCH_FTP_PORT` | `5000` | porta do ftpd no Switch |
| `HOST_IP` | autodetectado (`ip route`) | IP desta máquina, para o Switch saber onde mandar os logs. Override: `make install-debug HOST_IP=192.168.0.4` |
| `NXLINK_LOG_PORT` | `28771` | porta que recebe os logs (`NXLINK_CLIENT_PORT` do libnx) |
| `APP_DIR` | `/switch/roms-manager-ns` | destino no SD card |
| `DEVKITPRO` | — | só para o bloco de build legado (morto, ver acima) |

## Targets do Makefile

```bash
# PC
make pc            # compila e executa
make pc-build      # só compila
make pc-setup      # roda o cmake -B build-pc (implícito nos anteriores)
make watch         # hot reload
make clean-pc      # remove build-pc

# Switch
make build         # gera o .nro via Docker
make deploy        # envia e executa via nxlink (temporário, dev)
make install       # instala via FTP (permanente)
make deploy-fresh  # build + deploy
make install-fresh # build + install

# Debug — ver debug.md
make install-debug # instala via FTP com logs habilitados
make logs-live     # escuta logs em tempo real
make logs          # baixa debug.log do SD
make logs-clean    # remove logs locais

# Servidor
make serve         # sobe o roms-manager-server (requer ../roms-manager-server)
```

`make clean` / `make clean-all` existem, mas só com `DEVKITPRO` definido —
na prática falham. Para limpar: `make clean-pc` e `rm -f *.nro`.

## Steerings deste projeto

| Arquivo | Assunto |
|---|---|
| `project.md` | este — contexto geral, build, estrutura |
| `coding.md` | convenções de código, threading, git |
| `borealis.md` | API do Borealis (carregado ao mexer em `src/**`) |
| `debug.md` | capturar logs e investigar crash no Switch |
| `borealis-fork.md` | patches no submodule e risco de perdê-los |

## Troubleshooting — Ordem de Investigação

- **Problemas com deploy/install/build**: investigar `Makefile` + `build.sh` PRIMEIRO, não o código C++ do app. O app não controla como é copiado/instalado — isso é responsabilidade dos scripts de build.
- **Problemas de runtime** (crash, UI, lógica): aí sim investigar `src/`
- **App fecha sozinho ou morre numa ação**: ver `debug.md`. Atenção: exceção não
  capturada não gera crash report do Atmosphère, e a ordem dos logs no nxlink não
  é a ordem de execução.
- **Comportamento estranho do framework**: conferir `borealis-fork.md` antes de
  culpar o upstream — o submodule tem patches locais.

## APIs libnx — Notas Importantes

### Espaço de armazenamento (SD / NAND)

- **`statvfs()` NÃO funciona** com paths scheme do Switch (`sdmc:/`, `save:/`, etc.)
- Usar `nsGetTotalSpaceSize()` / `nsGetFreeSpaceSize()` com `NcmStorageId`:
  - `NcmStorageId_SdCard` — microSD
  - `NcmStorageId_BuiltInUser` — NAND (system)
- **Requer `nsInitialize()` antes e `nsExit()` depois** — centralizar no `onContentAvailable()` da MainActivity
- No PC, usar `statvfs` normalmente (paths reais funcionam)

### Material Icons (Borealis)

- O Borealis resolve via `BRLS_ASSET("material/MaterialIcons-Regular.ttf")`, ou
  seja relativo ao `BRLS_RESOURCES` — `./resources/` no PC, `romfs:/` no Switch.
  O arquivo efetivamente carregado é `resources/material/MaterialIcons-Regular.ttf`;
  `library/resources/material/` é apenas a origem de onde o `setup-resources.sh`
  copia.
- Usar codepoints UTF-8 diretamente no texto do Label (ex: `"\xEE\x98\xA3"` para U+E623)
- Nem todos os codepoints do range renderizam — testar com `fc-query --format='%{charset}\n'`
- Codepoints confirmados funcionais:
  - `U+E623` (`\xEE\x98\xA3`) — sd_card
  - `U+E322` (`\xEE\x8C\xA2`) — memory (chip)
  - `U+E63E` (`\xEE\x98\xBE`) — wifi
