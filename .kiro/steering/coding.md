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
- **Fontes obrigatórias** para renderizar texto no PC: `resources/font/switch_font.ttf` e `switch_icons.ttf` (copiar de `library/resources/font/`). Sem elas nenhum texto aparece no desktop.
- `AppletFrame` com `TabFrame` interno: não usar `iconInterpolation` no AppletFrame — causa textos invisíveis

## Rede

- Sempre inicializar sockets no Switch: `socketInitializeDefault()`
- Sempre limpar: `socketExit()`
- Timeout de 10s em operações de socket
- Verificar `path.empty()` antes de abrir diretórios/URLs
- Log warnings via `brls::Logger::warning()`
- **Operações de rede devem rodar em `brls::async()`** + `brls::sync()` para callback na UI thread
- Nunca bloquear a UI thread com httpGet/httpDownloadFile
- ⚠️ **NUNCA usar `std::thread` no Switch** — ver seção "Threading" abaixo
- **`strerror()` pode retornar NULL no newlib** para errno fora da tabela dele (os erros de socket vêm do serviço `bsd:s` com códigos não mapeados). `std::string(strerror(errno))` é UB e mata o processo. Sempre validar o ponteiro antes de construir a string.
- Validar retorno de `gethostbyname` a fundo: `he`, `he->h_addr_list`, `he->h_addr_list[0]` e `he->h_length` antes do `memcpy`

## Threading

- ⚠️ **NUNCA usar `std::thread` no Switch.** O devkitA64 vem com os stubs
  single-thread de gthreads: criar uma `std::thread` lança
  `std::system_error` com `ENOSYS` ("Function not implemented"). A exceção
  não capturada chama `std::terminate` → `abort()` e o app morre.
- Usar **`brls::async(fn)`** para trabalho fora da UI thread e
  **`brls::sync(fn)`** para voltar à UI thread.
- O Borealis não cria thread por chamada: ele mantém **uma** task loop thread
  (criada com `pthread_create` direto, justamente por causa dessa limitação)
  e `brls::async` enfileira nela. Tasks longas em sequência serializam.
- Sintoma típico do erro: app fecha ao acionar a ação, `userAppExit` aparece
  no log mas o `printf` final do `main` **não** — sinal de `abort()`, não de
  retorno normal do loop.
- `main.cpp` registra um handler de `std::set_terminate` que loga tipo e
  `what()` da exceção. Manter — é a única forma de ver esse tipo de falha.

## Platform

- Nunca usar `sdmc:/` direto no código
- Sempre usar `platform::sdRoot()` ou `platform::romsPath()`
- Usar `#ifdef __SWITCH__` apenas em main.cpp, platform.hpp e main_activity.cpp
- **Configs editáveis pelo usuário NUNCA no romfs**. Ler do SD card (`sdmc:/switch/<app>/config.json`) com fallback para defaults hardcoded no código. Romfs é read-only e exige rebuild para alterar.
- **nsInitialize/nsExit**: centralizar chamada no `onContentAvailable()` da MainActivity, não dentro de cada função de platform

## Git

- Mensagens de commit em pt-BR com prefixo convencional
- Branch de dev separada, nunca push direto na main
- Não commitar: build/, build-pc/, build.nx/, *.nro, test_sd/

## Debugging e Reprodução

- Para reproduzir bugs reportados pelo usuário, executar **exatamente o mesmo comando** que ele usou (ex: `make deploy`, não o comando Docker interno)
- Pode quebrar em partes para diagnosticar, mas o teste final deve ser pelo target do Make
- Nunca considerar o bug resolvido sem rodar o comando original e confirmar exit code 0

### Ordem dos logs no nxlink NÃO é ordem de execução

- `printf` e `brls::Logger` usam buffers diferentes. No output do nxlink as
  linhas aparecem **fora de ordem** — já foi visto `userAppExit` impresso
  antes de logs de código que rodou muito antes dele.
- **Não concluir "morreu aqui" pelo último log impresso.** Para instrumentar
  um caminho suspeito, usar `printf(...)` seguido de `fflush(stdout)` em
  todos os pontos, garantindo ordem real.
- Usar marcadores numerados (`[net] c1`, `c2`, ...) em vez de texto solto:
  fica óbvio qual etapa não foi alcançada.

### Distinguir crash de exceção

| Sintoma | Causa provável |
|---|---|
| `userAppExit` no log, mas printf final do `main` ausente | `abort()` — exceção não capturada / `std::terminate` |
| Nada no log, crash report em `sdmc:/atmosphere/crash_reports/` | data abort / segfault real |
| App fecha e printf final do `main` aparece | exit normal (`Application::quit()`) |

- Exceção não capturada **não** gera crash report do Atmosphère — por isso o
  handler de `std::set_terminate` no `main.cpp` é essencial.

### Validar caminhos de erro no PC antes de gastar ciclo de build Switch

- O build Switch via Docker leva minutos e depende do nxlink aberto. Antes de
  enviar, compilar o módulo isolado no PC com um `main` de teste
  (`g++ -std=gnu++17 -o /tmp/t teste.cpp src/sync/http_client.cpp -I src`)
  e exercitar os casos de falha (porta fechada, host inexistente, DNS
  inválido, URL malformada).
- Atenção: no PC (glibc) alguns bugs **não** reproduzem — `strerror` nunca
  retorna NULL e `std::thread` funciona. Serve para validar lógica, não para
  provar que funciona no Switch.

## nxlink (deploy para Switch)

- `nxlink` envia o `.nro` para o Switch e executa imediatamente (modo dev)
- **Comportamento padrão**: copia o .nro para `sdmc:/switch/<nome>.nro` (raiz de /switch/) — isso polui o hbmenu com entrada duplicada
- **Flag `-p`**: define o path de destino no SD card (ex: `-p /switch/roms-manager-ns/roms-manager-ns.nro`)
- **`deploy`** = execução temporária via nxlink (dev/debug)
- **`install`** = instalação permanente via FTP no path correto
- Porta 28771 pode ficar presa após timeout — matar processo antes de re-deploy

## Migrações de Dependência

- Antes de trocar submodule de lib, verificar build system da nova versão (Makefile? CMake? xmake?)
- Testar build PC localmente primeiro — só depois tentar Docker/Switch
- Verificar se a API mudou (classes renomeadas, métodos removed/protected)
- Manter branch/tag estável da versão anterior até confirmar que a nova funciona
