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
- **Build**: Make (Switch) / CMake (PC) / Docker (Switch)
- **Servidor**: Node.js 24 + Express + MongoDB (repo separado: [roms-manager-server](https://github.com/eliasrosa/roms-manager-server))

## Estrutura principal

- `src/` — código C++ do app
- `src/views/` — componentes de UI (herdam de brls::Box)
- `src/sync/` — módulo de sync HTTP (config, http_client, sync_manager)
- `resources/xml/` — layouts XML do Borealis
- `library/` — submodule Borealis (fork com patches locais)
- `test_sd/` — SD card fake para testes no PC
- `config.json` — configuração padrão do app

## Server — Endpoints (roms-manager-server)

| Método | Path | Descrição |
|--------|------|-----------|
| `GET` | `/health` | Status do servidor |
| `GET` | `/roms` | Lista ROMs (filtros: `?platform=`, `?crc32=`, `?md5=`) |
| `GET` | `/roms/:platform/:filename` | Download direto da ROM |
| `POST` | `/roms/sync?platform=gba` | Re-indexa ROMs no disco |

### Fluxo de sync (planejado — migração pendente)

1. App faz `GET /roms?platform=gba` → recebe lista com `filename`, `size`, `crc32`
2. Compara com storage local (por filename + crc32)
3. Baixa apenas ROMs ausentes ou divergentes via `GET /roms/:platform/:filename`
4. Verifica CRC32 localmente após download

### Fluxo de sync (atual — legado, será substituído)

1. App faz `GET /manifest.json` → recebe lista com `path`, `size`, `md5`
2. Compara por tamanho de arquivo
3. Baixa via URL direta (`baseUrl + path`)

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
- Porta 28771 do nxlink pode ficar presa após timeout — matar processo antes de re-deploy
- resources/xml NÃO pode ser symlink — deve ser diretório real com nosso main.xml
- **hbmenu injeta evento + no startup** — `setGlobalQuit(true)` só no PC; no Switch usar `setIgnoreExitRequest` com delay de 1s

## Variáveis de ambiente

- `SWITCH_IP` — IP do Switch na rede (default: 192.168.0.2)
- `SWITCH_FTP_PORT` — porta do ftpd (default: 5000)
- `DEVKITPRO` — path do devkitPro (só para build local sem Docker)

## Targets do Makefile

```bash
make pc          # testa no PC (cmake)
make watch       # hot reload
make serve       # servidor sync (requer ../roms-manager-server clonado)
make build       # gera .nro (Docker)
make deploy      # envia via nxlink
make install     # copia via FTP
```

## Troubleshooting — Ordem de Investigação

- **Problemas com deploy/install/build**: investigar `Makefile` + `build.sh` PRIMEIRO, não o código C++ do app. O app não controla como é copiado/instalado — isso é responsabilidade dos scripts de build.
- **Problemas de runtime** (crash, UI, lógica): aí sim investigar `src/`

## APIs libnx — Notas Importantes

### Espaço de armazenamento (SD / NAND)

- **`statvfs()` NÃO funciona** com paths scheme do Switch (`sdmc:/`, `save:/`, etc.)
- Usar `nsGetTotalSpaceSize()` / `nsGetFreeSpaceSize()` com `NcmStorageId`:
  - `NcmStorageId_SdCard` — microSD
  - `NcmStorageId_BuiltInUser` — NAND (system)
- **Requer `nsInitialize()` antes e `nsExit()` depois** — centralizar no `onContentAvailable()` da MainActivity
- No PC, usar `statvfs` normalmente (paths reais funcionam)

### Material Icons (Borealis)

- Fonte carregada como fallback: `library/resources/material/MaterialIcons-Regular.ttf`
- Usar codepoints UTF-8 diretamente no texto do Label (ex: `"\xEE\x98\xA3"` para U+E623)
- Nem todos os codepoints do range renderizam — testar com `fc-query --format='%{charset}\n'`
- Codepoints confirmados funcionais:
  - `U+E623` (`\xEE\x98\xA3`) — sd_card
  - `U+E322` (`\xEE\x8C\xA2`) — memory (chip)
  - `U+E63E` (`\xEE\x98\xBE`) — wifi
