# ROMS-005 — Design de Arquitetura

## Stack Tecnológico

| Camada | Tecnologia | Motivo |
|--------|-----------|--------|
| UI Framework | Borealis (natinusala) | Visual nativo Switch, hardware-accelerated, suporta PC |
| GPU (Switch) | deko3d + nanovg | API nativa Tegra X1, performático |
| GPU (PC) | OpenGL + GLFW + nanovg | Compatível para testes |
| Layout Engine | Yoga (flexbox) | Layouts responsivos, XML declarativo |
| Linguagem | C++17 | Padrão do devkitPro, bom balanço perf/produtividade |
| SDK | devkitPro / libnx | SDK oficial para homebrew Switch |
| Rede | Sockets BSD | Portável, sem deps, funciona em ambas plataformas |
| Build (Switch) | Make + devkitA64 | Padrão devkitPro |
| Build (PC) | Meson + Ninja | Rápido, suporta subprojects |
| Build (Docker) | devkitpro/devkita64 | CI/CD sem setup local |
| Servidor | Python 3 stdlib | Zero deps, qualquer máquina roda |

## Estrutura de Diretórios

```
roms-manager-ns/
├── .kiro/
│   └── specs/              # Especificações do projeto
├── src/
│   ├── main.cpp            # Entry point (init sockets, borealis, loop)
│   ├── main_activity.hpp   # Activity principal (carrega XML)
│   ├── platform.hpp        # Abstração Switch/PC (#ifdef)
│   ├── views/
│   │   ├── file_browser_tab.*   # Navegador de arquivos
│   │   └── sync_tab.*           # Tab de sincronização
│   └── sync/
│       ├── config.*         # Leitura/escrita config.json
│       ├── http_client.*    # HTTP GET via sockets
│       └── sync_manager.*   # Orquestrador de sync
├── server/
│   ├── serve.py            # Servidor HTTP de sync
│   └── data/               # Dados de teste
├── resources/
│   └── xml/
│       └── activity/main.xml  # Layout principal (tabs)
├── test_sd/                # SD card fake (testes PC)
├── config.json             # Config padrão
├── Makefile                # Build unificado (Switch + PC)
├── meson.build             # Build PC
├── Dockerfile              # Build Switch via Docker
├── build.sh               # Script Docker
├── build-pc.sh            # Script PC
├── watch.sh               # Hot reload PC
└── library/                # Submodule: borealis
```

## Padrão de UI (Borealis nova API)

```
Application
  └── Activity (MainActivity)
        └── View tree (carregado de XML)
              └── TabFrame
                    ├── Tab "Sync" → SyncTab (custom Box)
                    └── Tab "Sobre" → inline XML
```

- **Activity**: "tela" do app — tem um content view (XML ou programático)
- **View**: qualquer componente (Box, Label, Button, Header, ScrollingFrame)
- **Custom View**: herda de `brls::Box`, registrada via `registerXMLView`
- **XML**: define layout declarativo, referencia custom views por nome

## Plataforma: Abstração Switch/PC

```cpp
// src/platform.hpp
namespace platform {
#ifdef __SWITCH__
    inline std::string sdRoot() { return "sdmc:/"; }
#else
    inline std::string sdRoot() { return "./test_sd/"; }
#endif
}
```

Todo código que acessa filesystem usa `platform::sdRoot()` como base.

## Rede: Inicialização

```cpp
// Switch precisa init explícito de sockets
#ifdef __SWITCH__
    socketInitializeDefault();  // antes de qualquer operação de rede
    nxlinkStdio();             // redireciona stdout para nxlink (debug)
    // ... app ...
    socketExit();              // cleanup
#endif
```

## Build System

### Targets do Makefile

| Target | Ação | Requer |
|--------|------|--------|
| `make pc` | Compila + roda no PC | meson, ninja, glfw, glm |
| `make pc-build` | Só compila PC | idem |
| `make watch` | Hot reload PC | inotify-tools |
| `make build` | Gera .nro via Docker | Docker |
| `make deploy` | Envia .nro via nxlink | Switch em modo nxlink |
| `make deploy-fresh` | Build + deploy | Docker + nxlink |
| `make install` | Copia via FTP | ftpd no Switch |
| `make install-fresh` | Build + install FTP | Docker + ftpd |
| `make serve` | Inicia servidor sync | Python 3 |
| `make clean-all` | Limpa tudo | — |

### Workflow de Desenvolvimento

```
1. make watch          (terminal 1 - hot reload PC)
2. make serve          (terminal 2 - servidor de sync)
3. Editar código → salvar → app rebuilda e reabre sozinho
4. Testar funcionalidade no PC
5. make deploy-fresh   (quando quiser testar no Switch)
```

## Decisões Técnicas Registradas

### ADR-001: Borealis main vs legacy
- **Decisão**: Usar branch `main` (nova API com XML + Activity)
- **Motivo**: API mais moderna, suporte a XML declarativo, melhor manutenção
- **Consequência**: Mais verboso que legacy (sem ListItem/Dialog prontos)

### ADR-002: Sockets BSD vs libcurl
- **Decisão**: Sockets BSD puro
- **Motivo**: Menos uma dependência, binário menor, portável Switch/PC
- **Consequência**: HTTP básico apenas (sem HTTPS, sem redirect following)

### ADR-003: JSON parser manual vs nlohmann/json
- **Decisão**: Parser manual por substring
- **Motivo**: Binário menor, config.json é simples e controlado por nós
- **Consequência**: Parser frágil — funciona apenas para o formato esperado

### ADR-004: Build directory `build.nx` (não `build`)
- **Decisão**: Renomear dir de build Switch para `build.nx`
- **Motivo**: Conflito com target `build` do Makefile (Docker)
- **Consequência**: Nenhuma negativa

### ADR-005: Namespace `netsync` (não `sync`)
- **Decisão**: Renomear namespace de `sync` para `netsync`
- **Motivo**: Conflito com `void sync()` de `<unistd.h>` (POSIX)
- **Consequência**: Nenhuma negativa
