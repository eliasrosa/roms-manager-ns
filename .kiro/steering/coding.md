# Convenções de Código — ROMs Manager NS

## C++

- Standard: C++17 (`-std=gnu++1z`)
- Indentação: 4 espaços
- Braces: Allman style (abertura na linha seguinte para funções, mesma linha para if/for)
- Naming:
  - Classes: PascalCase (`SyncManager`, `FileBrowserTab`)
  - Métodos/funções: camelCase (`loadDirectory`, `httpGet`)
  - Variáveis locais: camelCase (`currentPath`, `fileSize`)
  - Membros privados: sem prefixo, acessados via `this->`
  - Constantes: UPPER_SNAKE (`MAX_LOG_LINES`)
  - Namespaces: lowercase (`netsync`, `platform`)
- Headers: usar `#pragma once`
- Ponteiros: raw pointers para views do Borealis (framework gerencia lifecycle)

## Borealis (UI)

- Custom views herdam de `brls::Box`
- Implementar `static brls::View* create()` para registro XML
- Registrar em main.cpp via `Application::registerXMLView("Nome", Classe::create)`
- Não destruir views dentro de callbacks de action (causa segfault)
- Para navegação: usar `Application::pushActivity()` (stack-based)
- XML de activities em `resources/xml/activity/`
- Referenciar resources com path relativo ao BRLS_RESOURCES (sem prefixo `xml/` no código)

## Rede

- Sempre inicializar sockets no Switch: `socketInitializeDefault()`
- Sempre limpar: `socketExit()`
- Timeout de 10s em operações de socket
- Verificar `path.empty()` antes de abrir diretórios/URLs
- Log warnings via `brls::Logger::warning()`

## Platform

- Nunca usar `sdmc:/` direto no código
- Sempre usar `platform::sdRoot()` ou `platform::romsPath()`
- Usar `#ifdef __SWITCH__` apenas em main.cpp e platform.hpp
- **Configs editáveis pelo usuário NUNCA no romfs**. Ler do SD card (`sdmc:/switch/<app>/config.json`) com fallback para defaults hardcoded no código. Romfs é read-only e exige rebuild para alterar.

## Git

- Mensagens de commit em pt-BR com prefixo convencional
- Branch de dev separada, nunca push direto na main
- Não commitar: build/, build-pc/, build.nx/, *.nro, test_sd/, server/data/

## Migrações de Dependência

- Antes de trocar submodule de lib, verificar build system da nova versão (Makefile? CMake? xmake?)
- Testar build PC localmente primeiro — só depois tentar Docker/Switch
- Verificar se a API mudou (classes renomeadas, métodos removed/protected)
- Manter branch/tag estável da versão anterior até confirmar que a nova funciona
