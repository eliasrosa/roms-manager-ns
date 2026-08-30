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
- **Fontes obrigatórias** para renderizar texto no PC:
  `resources/font/switch_font.ttf` e `switch_icons.ttf`. Sem elas **nenhum** texto
  aparece no desktop. O `setup-resources.sh` já copia `font i18n img material` de
  `library/resources/` (atenção: o path é esse, não `library/library/resources/`)
  e depois mescla as traduções do projeto de `i18n/`. Não editar `resources/i18n/`
  direto: é gitignored e o script apaga e recria o diretório a cada execução.
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
- Manter `#ifdef __SWITCH__` concentrado. Hoje aparece em: `main.cpp`,
  `main_activity.cpp`, `platform.hpp`, `debug_log.cpp`, `sync/http_client.cpp`,
  `sync/config.cpp`, `sync/sync_manager.cpp`. Antes de adicionar um novo,
  verificar se dá para resolver via `platform.hpp`
- **Configs editáveis pelo usuário NUNCA no romfs**. Ler do SD card (`sdmc:/switch/<app>/config.json`) com fallback para defaults hardcoded no código. Romfs é read-only e exige rebuild para alterar.
- **nsInitialize/nsExit**: centralizar chamada no `onContentAvailable()` da MainActivity, não dentro de cada função de platform

## Git

- Mensagens de commit em pt-BR com prefixo convencional
- Branch de dev separada, nunca push direto na main
- O `.gitignore` é a fonte da verdade do que não versionar. Cobre builds
  (`build-pc/`, `build.nx/`, `*.nro`, `*.nacp`, `*.elf`), assets copiados
  (`resources/font`, `i18n`, `img`, `inter`, `material`, `shaders/`), logs
  (`.logs/`, `debug.log*`), config local (`config.local.json`), `test_sd/` e
  `.kiro/specs/`
- ⚠️ Ao alterar o submodule `library/`, commitar e dar push **no fork** antes de
  rodar `make build` — o build roda `git submodule update` e descarta trabalho
  não commitado. Ver `borealis-fork.md`

## Debugging e Reprodução

- Para reproduzir bugs reportados pelo usuário, executar **exatamente o mesmo comando** que ele usou (ex: `make deploy`, não o comando Docker interno)
- Pode quebrar em partes para diagnosticar, mas o teste final deve ser pelo target do Make
- Nunca considerar o bug resolvido sem rodar o comando original e confirmar exit code 0

### Diagnóstico no Switch

Ver **`debug.md`** para os canais de log, como capturar (`make logs`,
`make logs-live`), como distinguir `abort()` de segfault, e por que a ordem dos
logs no nxlink não é a ordem de execução.

Dois pontos que valem repetir aqui, porque já custaram horas:

- Exceção C++ não capturada **não** gera crash report do Atmosphère e o log dá a
  impressão de ter morrido no lugar errado.
- Antes de gastar um ciclo de build Switch, exercitar o caminho de erro no PC
  compilando o módulo isolado. Mas lembrar que glibc esconde bugs que o newlib
  expõe (`strerror` retornando NULL, `std::thread` funcionando).

## Docker — artefatos com owner root

Todo build passa por `docker run -v`, e os arquivos que o container cria no
volume montado ficam com **owner root**. O usuário do host não consegue apagá-los
direto.

- Ao extrair um artefato, monte o container com o usuário do host:
  `docker run --rm --user $(id -u):$(id -g) -v "$PWD:/out" <img> cp ... /out/`
- Para limpar algo que já ficou root, use um container descartável em vez de
  sudo: `docker run --rm -v "$PWD:/w" alpine rm -rf /w/<path>`
- **Nunca** usar `sudo rm` — no host o sudo pede senha interativa e trava a
  sessão do agente.

## Docker — o source de `-v` é resolvido no host

Quem interpreta o source de um bind-mount é o **daemon**, no filesystem dele, não
o shell que monta o comando. As duas pontas divergem quando o Make roda de dentro
de um container:

| Caminho | Visível ao shell no container | Visível ao daemon (host) |
|---|---|---|
| `/home/<user-container>/...` | ✅ | ❌ → daemon cria dir vazio |
| caminho real do host | ❌ | ✅ |

Um source inexistente **não** dá erro: o Docker cria um diretório vazio e o
container efêmero recebe um diretório onde deveria haver o arquivo.

- Nos targets do Make, usar `$(HOST_CURDIR)` (via `host-path.sh`), nunca
  `$(CURDIR)`, em qualquer `-v`.
- Ao montar na mão a partir de um agente em container, traduzir o caminho:
  `./host-path.sh <caminho>`.
- `docker build` e `docker cp` **não** sofrem disso — o contexto e o arquivo
  trafegam pelo socket. É por isso que o `build.sh` sempre funcionou de dentro do
  container enquanto o `deploy` falhava.

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
