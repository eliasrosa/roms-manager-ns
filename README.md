# ROMs Manager NS

Gerenciador de ROMs homebrew para Nintendo Switch com interface gráfica nativa (Borealis) e sincronização via WiFi.

## Funcionalidades

- Interface gráfica estilo sistema do Switch (dark theme, animações)
- Navegador de arquivos do SD card
- **Sync via WiFi** — sincroniza ROMs, covers e saves com servidor na rede local
- Detalhes de arquivo (nome, caminho, tamanho, extensão)
- Tabs: Arquivos, Sync, Sobre

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

O app sincroniza arquivos com um servidor HTTP rodando no PC/NAS da sua rede local.

### Setup do Servidor (PC)

```bash
cd server/

# Colocar ROMs/covers na estrutura:
# data/roms/    → .nsp, .xci, .nro
# data/covers/  → .jpg, .png
# data/saves/   → qualquer

# Iniciar servidor
python3 serve.py --port 8080 --dir ./data
```

O servidor gera automaticamente o `manifest.json` com hash MD5 de cada arquivo.

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
    },
    "covers": {
      "remote": "/covers",
      "local": "sdmc:/roms/covers/"
    }
  },
  "filters": {
    "extensions": [".nsp", ".xci", ".nro"],
    "max_file_size_mb": 0
  }
}
```

### Uso

1. Inicie o servidor no PC: `python3 server/serve.py`
2. No Switch, abra **ROMs Manager NS**
3. Vá na tab **Sync**
4. Clique em **Testar Conexao** para verificar
5. Clique em **Iniciar Sync** para baixar arquivos novos

### Como funciona

```
PC (servidor)                    Switch (cliente)
─────────────                    ────────────────
python3 serve.py        ←WiFi→   ROMs Manager NS
  ├─ /manifest.json              1. GET /manifest.json
  ├─ /roms/*.nsp                 2. Compara com arquivos locais
  ├─ /covers/*.jpg               3. Baixa o que falta
  └─ /health                     4. Salva em sdmc:/roms/
```

- Apenas baixa arquivos **novos ou modificados** (compara tamanho)
- Respeita filtros de extensão e tamanho máximo
- Não deleta arquivos locais por padrão (`delete_removed: false`)

---

## Build

### Testar no PC (Linux)

```bash
# Instalar dependências (uma vez)
sudo apt install build-essential meson ninja-build pkg-config libglfw3-dev libglm-dev

# Setup inicial
./setup.sh

# Compilar e rodar
make pc
```

### Gerar .nro para Switch (via Docker)

```bash
make build
```

Output: `roms-manager-ns.nro`

### Todos os targets

```bash
make pc        # compila e roda no PC
make pc-build  # só compila PC
make build     # gera .nro via Docker
make clean-pc  # limpa build PC
make clean-all # limpa tudo
```

## Instalação no Switch

1. Copie `roms-manager-ns.nro` para: `sdmc:/switch/roms-manager-ns/`
2. Copie `config.json` para: `sdmc:/switch/roms-manager-ns/config.json`
3. Edite o `config.json` com o IP do seu servidor
4. No Switch com CFW (Atmosphère), abra o **hbmenu**
5. Selecione **ROMs Manager NS**

## Estrutura do Projeto

```
roms-manager-ns/
├── src/
│   ├── main.cpp                    # Entry point
│   ├── main_activity.hpp           # Activity principal
│   ├── platform.hpp                # Abstração Switch/PC
│   ├── views/
│   │   ├── file_browser_tab.*      # Navegador de arquivos
│   │   └── sync_tab.*              # Tab de sincronização
│   └── sync/
│       ├── config.*                # Parser de config.json
│       ├── http_client.*           # HTTP GET via sockets BSD
│       └── sync_manager.*          # Orquestrador de sync
├── server/
│   └── serve.py                    # Servidor HTTP + manifest
├── resources/
│   └── xml/activity/main.xml       # Layout da UI
├── config.json                     # Configuração padrão
├── Makefile                        # Build Switch + PC
├── meson.build                     # Build PC (meson/ninja)
├── Dockerfile                      # Build Switch via Docker
├── build.sh                        # Script build Docker
├── build-pc.sh                     # Script build PC
└── test_sd/                        # SD card fake (testes PC)
```

## Arquitetura

| Componente | Tecnologia |
|-----------|-----------|
| Linguagem | C++17 |
| UI | Borealis (natinusala) |
| GPU | deko3d / nanovg |
| Rede | Sockets BSD (sem libcurl) |
| SDK | devkitPro / libnx |
| Build | Make + meson + Docker |
| Servidor | Python 3 (stdlib) |
| Target | Nintendo Switch (Atmosphère) |

## Roadmap

- [x] Navegador de arquivos com UI nativa
- [x] Sync HTTP via WiFi
- [x] Configuração via JSON
- [x] Servidor Python standalone
- [ ] Verificação MD5 pós-download
- [ ] Sync em thread separada (não bloquear UI)
- [ ] Progress bar visual
- [ ] Filtro por extensão na lista de arquivos
- [ ] Copiar/mover ROMs entre pastas
- [ ] Scan de metadados (titleID, nome do jogo)
- [ ] Temas customizáveis

## Licença

MIT
