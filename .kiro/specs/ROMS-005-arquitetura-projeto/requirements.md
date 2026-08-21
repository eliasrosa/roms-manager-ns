# Requirements Document

## Introduction

ROMs Manager NS é um app homebrew para Nintendo Switch (Atmosphère/CFW) que oferece interface gráfica nativa para gerenciar ROMs no SD card e sincronizar arquivos via WiFi com um servidor local no PC. O sistema é composto por dois componentes: o app C++ rodando no Switch e um servidor Python rodando no PC do usuário.

Este documento cobre os requisitos de arquitetura e infraestrutura que sustentam todas as funcionalidades do app: build system, plataformas suportadas, ciclo de desenvolvimento e manutenção do projeto.

---

## Glossary

- **Homebrew**: Software não oficial que roda em consoles com firmware modificado, como o Nintendo Switch com Atmosphère
- **CFW (Custom Firmware)**: Firmware modificado para permitir execução de software não licenciado pela Nintendo; neste projeto, refere-se ao Atmosphère
- **Atmosphère**: CFW open-source para Nintendo Switch, necessário para rodar homebrew
- **ROM**: Arquivo de imagem de jogo (ex: `.nsp`, `.xci`, `.nro` para homebrew)
- **SD card**: Cartão microSD inserido no Switch, usado para armazenar ROMs e apps homebrew
- **libnx**: Biblioteca C/C++ que provê bindings para as APIs do HorizonOS (SO do Switch)
- **devkitPro**: Suite de toolchains para cross-compilar código para plataformas homebrew
- **devkitA64**: Toolchain GCC para ARM64 (aarch64), usada para compilar o app para o Switch
- **nxlink**: Ferramenta do devkitPro que envia um `.nro` via rede para o Switch e o executa em modo dev
- **nro**: Formato de executável homebrew do Switch (Nintendo Relocatable Object)
- **romfs**: Sistema de arquivos read-only empacotado no `.nro`; contém shaders, fontes e layouts XML
- **ftpd**: Servidor FTP que roda no Switch (homebrew); permite instalar arquivos via rede
- **deko3d**: API gráfica nativa do Switch (Tegra X1), equivalente ao Vulkan; usada pelo Borealis no Switch
- **Borealis**: Framework de UI para homebrew Switch/PC, provê sistema de views, layouts XML e navegação por gamepad
- **HorizonOS**: Sistema operacional do Nintendo Switch
- **Tegra X1**: SoC ARM (CPU Cortex-A57 + GPU Maxwell) presente no Nintendo Switch
- **netsync**: Namespace C++ do módulo de sincronização do ROMs Manager NS (evita conflito com POSIX `sync`)
- **platform.hpp**: Header de abstração de plataforma que resolve paths Switch (`sdmc:/`) vs PC (`./test_sd/`)
- **hot reload**: Reconstrução e re-execução automática do app ao detectar mudanças no código-fonte

---

## Requirements

---

### Requirement 1: Interface Gráfica Nativa

**User Story:** Como usuário do Switch, eu quero que o app tenha interface visual estilo Switch nativo, para que a experiência seja natural e consistente com outros apps homebrew de qualidade.

#### Acceptance Criteria

1. THE App SHALL renderizar interface gráfica usando o framework Borealis (branch `main`, nova API com XML + Activity)
2. THE App SHALL estruturar a navegação em Activities empilhadas via `Application::pushActivity()`
3. THE App SHALL carregar layouts declarativos a partir de arquivos XML em `resources/xml/activity/`
4. THE App SHALL registrar views customizadas via `Application::registerXMLView()` antes de carregar qualquer XML que as referencie
5. WHEN rodando no Switch, THE App SHALL usar deko3d + nanovg como backend de renderização
6. WHEN rodando no PC, THE App SHALL usar OpenGL + GLFW + nanovg como backend de renderização
7. THE App SHALL responder à navegação por gamepad (D-pad, botões A/B) conforme convenções do Switch

---

### Requirement 2: Compilação Multiplataforma

**User Story:** Como desenvolvedor, eu quero compilar o app tanto para Switch quanto para PC, para que eu possa iterar no código sem precisar do hardware a cada mudança.

#### Acceptance Criteria

1. THE Build_System SHALL compilar o app para Switch usando o toolchain devkitA64 via `make build` (Docker)
2. THE Build_System SHALL compilar o app para PC usando CMake + GCC/Clang via `make pc`
3. WHEN compilando para Switch, THE Build_System SHALL produzir um arquivo `roms-manager-ns.nro` válido com romfs embutido
4. WHEN compilando para PC, THE Build_System SHALL produzir um executável nativo que corre sem hardware Switch
5. THE Platform_Abstraction SHALL resolver `sdmc:/` no Switch e `./test_sd/` no PC via `platform::sdRoot()`
6. THE Platform_Abstraction SHALL isolar código específico de plataforma em `platform.hpp` usando `#ifdef __SWITCH__`
7. THE Build_System SHALL compilar com C++17 (`-std=gnu++1z`) em ambas as plataformas

---

### Requirement 3: Workflow de Desenvolvimento Rápido

**User Story:** Como desenvolvedor, eu quero um workflow de desenvolvimento com hot reload no PC e deploy fácil no Switch, para que o ciclo de iteração seja curto e produtivo.

#### Acceptance Criteria

1. WHEN o desenvolvedor executa `make watch`, THE Dev_Environment SHALL recompilar e re-executar o app automaticamente ao detectar alterações em `src/`
2. THE Dev_Environment SHALL suportar execução do app no PC sem Switch físico conectado
3. WHEN o desenvolvedor executa `make deploy`, THE Build_System SHALL enviar o `.nro` para o Switch via nxlink na porta padrão (28771)
4. WHEN `nxlink` não está disponível localmente, THE Build_System SHALL usar a imagem Docker `devkitpro/devkita64` como fallback para o envio
5. WHEN o desenvolvedor executa `make deploy`, THE Build_System SHALL instalar o `.nro` em `sdmc:/switch/roms-manager-ns/roms-manager-ns.nro` (não na raiz de `/switch/`)
6. WHEN o desenvolvedor executa `make install`, THE Build_System SHALL copiar o `.nro` e `config.json` para o Switch via FTP usando `curl`
7. WHEN o desenvolvedor executa `make serve`, THE Dev_Environment SHALL iniciar o servidor Python de sync em `server/serve.py`
8. THE Dev_Environment SHALL incluir dados de teste em `test_sd/` para simular o SD card no PC

---

### Requirement 4: Módulo de Sincronização via WiFi

**User Story:** Como usuário, eu quero sincronizar ROMs, covers e saves entre o PC e o Switch via WiFi, para que eu possa gerenciar minha coleção sem cabos ou cartões SD.

#### Acceptance Criteria

1. THE Sync_Module SHALL comunicar-se com o servidor Python via HTTP usando sockets BSD puros (sem libcurl ou libssl)
2. THE Sync_Module SHALL residir no namespace `netsync::` para evitar conflito com `sync()` POSIX
3. WHEN rodando no Switch, THE App SHALL inicializar sockets via `socketInitializeDefault()` antes de qualquer operação de rede
4. WHEN rodando no Switch, THE App SHALL chamar `socketExit()` ao encerrar para liberar recursos de rede
5. THE HTTP_Client SHALL aplicar timeout de 10 segundos em operações de socket
6. THE Sync_Server SHALL ser implementado em Python 3 usando apenas a biblioteca padrão (zero dependências externas)
7. IF `path` estiver vazio ao tentar acessar um diretório ou URL, THEN THE Sync_Module SHALL registrar um warning via `brls::Logger::warning()` e abortar a operação sem crash

---

### Requirement 5: Binário Leve e Sem Dependências Externas Pesadas

**User Story:** Como desenvolvedor, eu quero manter o binário leve e com o mínimo de dependências, para que o app seja fácil de compilar em qualquer máquina e ocupe pouco espaço no SD card.

#### Acceptance Criteria

1. THE App SHALL usar sockets BSD puros para comunicação HTTP (sem libcurl)
2. THE App SHALL parsear `config.json` com código C++ próprio (sem nlohmann/json ou similar)
3. THE Build_System SHALL construir o app Switch via Docker usando a imagem `devkitpro/devkita64` sem dependências instaladas no host além do Docker
4. THE Server_Component SHALL rodar com Python 3 stdlib sem necessidade de `pip install`
5. THE App SHALL usar Borealis como único framework de UI, sem bibliotecas de widgets adicionais

---

### Requirement 6: CI/CD Automatizado

**User Story:** Como desenvolvedor, eu quero um pipeline de CI/CD no GitHub Actions, para que cada push valide o build e releases gerem o `.nro` automaticamente.

#### Acceptance Criteria

1. WHEN um pull request é aberto ou atualizado, THE CI_Pipeline SHALL compilar o app para Switch via Docker e reportar sucesso ou falha
2. WHEN uma tag semântica (ex: `v1.2.3`) é criada no repositório, THE CI_Pipeline SHALL gerar o `.nro` e publicá-lo como asset no GitHub Release
3. THE CI_Pipeline SHALL executar usando a imagem Docker `devkitpro/devkita64` sem dependências adicionais instaladas
4. IF o build falhar, THEN THE CI_Pipeline SHALL reportar o erro com logs suficientes para diagnóstico

---

### Requirement 7: Identidade Visual do App

**User Story:** Como desenvolvedor, eu quero que o app tenha ícone customizado no hbmenu do Switch, para que ele seja reconhecível entre os outros homebrew instalados.

#### Acceptance Criteria

1. THE App SHALL incluir um arquivo `icon.jpg` com dimensões 256×256 pixels na raiz do projeto
2. THE Build_System SHALL empacotar o `icon.jpg` no `.nro` via flag `--icon=$(APP_ICON)` no `elf2nro`
3. WHEN o `.nro` é instalado no Switch, THE App SHALL exibir o ícone customizado no hbmenu (Homebrew Launcher)

---

### Requirement 8: Internacionalização (pt-BR)

**User Story:** Como usuário brasileiro, eu quero que a interface do app esteja em português, para que a experiência seja mais natural no meu idioma.

#### Acceptance Criteria

1. THE App SHALL incluir strings de interface em português brasileiro usando o sistema de i18n do Borealis
2. THE i18n_System SHALL carregar os arquivos de tradução a partir de `resources/i18n/pt-BR/` no romfs
3. WHEN o idioma do sistema do Switch está definido como português, THE App SHALL exibir a interface em pt-BR automaticamente
4. WHEN o idioma do sistema não tem tradução disponível, THE App SHALL usar inglês como fallback

---

### Requirement 9: Documentação de Contribuição

**User Story:** Como desenvolvedor externo, eu quero encontrar documentação clara sobre como contribuir com o projeto, para que eu possa configurar o ambiente e submeter mudanças sem precisar de ajuda.

#### Acceptance Criteria

1. THE Project SHALL incluir um arquivo `CONTRIBUTING.md` na raiz do repositório
2. THE CONTRIBUTING.md SHALL descrever os pré-requisitos para compilar o app (Docker, devkitPro ou CMake)
3. THE CONTRIBUTING.md SHALL descrever os targets do Makefile disponíveis e seu propósito
4. THE CONTRIBUTING.md SHALL descrever o workflow de desenvolvimento recomendado (`make watch` + `make serve`)
5. THE CONTRIBUTING.md SHALL descrever o processo de envio de pull requests e padrões de commit
