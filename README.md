# ROMs Manager NS

Gerenciador de ROMs homebrew para Nintendo Switch com interface gráfica nativa (Borealis) e sincronização via WiFi.

## Funcionalidades

- Interface gráfica estilo sistema do Switch (dark theme, animações)
- Navegador de arquivos do SD card
- **Sync via WiFi** — sincroniza ROMs com servidor na rede local
- Indicadores de espaço (microSD / System) no header
- Indicador de conexão WiFi com spinner
- Tabs: Arquivos, Sync, Configurações

## Controles

| Botão | Ação |
|-------|------|
| D-Pad / Stick | Navegar na lista |
| A | Selecionar / Abrir |
| B | Voltar |
| L / R | Trocar tab |
| + | Sair do app |

---

## Sync via WiFi

O app sincroniza ROMs com o [roms-manager-server](https://github.com/eliasrosa/roms-manager-server) — servidor Node.js + Express + MongoDB rodando no PC/NAS da sua rede local.

### Setup do Servidor

```bash
git clone https://github.com/eliasrosa/roms-manager-server ../roms-manager-server
cd ../roms-manager-server
cp .env.example .env
docker compose up -d --build
```

Coloque as ROMs em `data/<platform>/roms/` (ex: `data/gba/roms/`, `data/nes/roms/`).
Execute `POST /roms/sync` para indexar.

Veja a [documentação completa do servidor](https://github.com/eliasrosa/roms-manager-server#readme).

### Configuração no Switch

Edite `sdmc:/switch/roms-manager-ns/config.json`:

```json
{
  "server": {
    "host": "192.168.1.100",
    "port": 8080,
    "protocol": "http"
  },
  "sync": {
    "verify_hash": true,
    "delete_removed": false
  },
  "paths": {
    "roms": {
      "remote": "/roms",
      "local": "sdmc:/roms/"
    }
  },
  "filters": {
    "extensions": [".nsp", ".xci", ".nro", ".nes", ".snes", ".gba"],
    "max_file_size_mb": 0
  }
}
```

### Fluxo de sync

```
PC/NAS (servidor)                Switch (cliente)
─────────────────                ────────────────
Node.js + MongoDB       ←WiFi→   ROMs Manager NS
  ├─ GET /health                  1. Testa conexão
  ├─ GET /roms?platform=gba       2. Lista ROMs disponíveis
  ├─ GET /roms/:platform/:file    3. Baixa o que falta
  └─ POST /roms/sync              (admin: re-indexar)
```

---

## Build

### Testar no PC (Linux)

```bash
# Instalar dependências (uma vez)
sudo apt install build-essential cmake pkg-config libglfw3-dev libglm-dev

# Compilar e rodar
make pc
```

### Gerar .nro para Switch (via Docker)

```bash
make build
```

### Deploy/Install no Switch

```bash
make deploy      # envia via nxlink (modo dev)
make install     # copia via FTP (permanente)
```

### Todos os targets

```bash
make pc          # compila e roda no PC
make pc-build    # só compila PC
make watch       # hot reload (recompila ao salvar)
make serve       # inicia servidor (requer ../roms-manager-server)
make build       # gera .nro via Docker
make deploy      # envia via nxlink
make install     # copia via FTP
make clean-pc    # limpa build PC
make clean-all   # limpa tudo
```

---

## Instalação no Switch

1. Copie `roms-manager-ns.nro` para: `sdmc:/switch/roms-manager-ns/`
2. Copie `config.json` para: `sdmc:/switch/roms-manager-ns/config.json`
3. Edite o `config.json` com o IP do seu servidor
4. No Switch com CFW (Atmosphère), abra o **hbmenu**
5. Selecione **ROMs Manager NS**

---

## Estrutura do Projeto

```
roms-manager-ns/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── main_activity.cpp/hpp       # Activity principal (header, storage info)
│   ├── platform.hpp                # Abstração Switch/PC
│   ├── views/
│   │   ├── file_browser_tab.*      # Navegador de arquivos
│   │   └── sync_tab.*              # Tab de sincronização
│   └── sync/
│       ├── config.*                # Parser de config.json
│       ├── http_client.*           # HTTP GET/Download via sockets BSD
│       └── sync_manager.*          # Orquestrador de sync
├── resources/
│   └── xml/activity/main.xml       # Layout da UI
├── library/                        # Submodule: Borealis (fork)
├── config.json                     # Configuração padrão
├── Makefile                        # Build Switch + PC
├── CMakeLists.txt                  # Build PC (cmake)
├── Dockerfile                      # Build Switch via Docker
├── build.sh                        # Script build Docker
└── test_sd/                        # SD card fake (testes PC)
```

## Arquitetura

| Componente | Tecnologia |
|-----------|-----------|
| Linguagem | C++17 |
| UI | [Borealis](https://github.com/eliasrosa/borealis) (fork xfangfang) |
| GPU | deko3d (Switch) / OpenGL+GLFW (PC) |
| Rede | Sockets BSD (sem libcurl) |
| SDK | devkitPro / libnx |
| Build | Make + CMake + Docker |
| Servidor | [roms-manager-server](https://github.com/eliasrosa/roms-manager-server) (Node.js + MongoDB) |
| Target | Nintendo Switch (Atmosphère) |

## Roadmap

- [x] Navegador de arquivos com UI nativa
- [x] Sync HTTP via WiFi (manifest)
- [x] Configuração via JSON
- [x] Header com storage info (SD + System)
- [x] Indicador WiFi com spinner async
- [x] Servidor Node.js + MongoDB (repo separado)
- [x] Deploy automático no ZimaOS via GitHub Actions
- [ ] Migrar sync para usar novos endpoints (`GET /roms`)
- [ ] Verificação CRC32 pós-download
- [ ] Progress bar visual durante sync
- [ ] Filtro por plataforma/extensão na UI
- [ ] Scan de metadados (titleID, nome do jogo)

## Licença

MIT
