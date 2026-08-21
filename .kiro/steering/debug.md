---
inclusion: auto
name: debug-switch
description: Use quando precisar diagnosticar o app no Switch — capturar logs, investigar crash, app fechando sozinho, ou entender os canais de log (arquivo no SD, nxlink em tempo real). Triggers: debug, logs, crash, app fecha, travou, diagnosticar, make logs, nxlink, debug.log.
---

# Diagnóstico no Switch

## Os dois canais de log

Ambos desligados por default, controlados pela seção `debug` do `config.json`:

```json
"debug": {
  "log_to_file": false,
  "log_path": "",
  "nxlink_host": ""
}
```

| Campo | Efeito |
|---|---|
| `log_to_file` | grava em `sdmc:/switch/roms-manager-ns/debug.log` |
| `log_path` | override do path; vazio = default da plataforma (`dbg::defaultLogPath()`) |
| `nxlink_host` | IP do PC que recebe os logs em tempo real na porta 28771 |

Implementação em `src/debug_log.hpp` / `src/debug_log.cpp` (namespace `dbg`).
`dbg::init()` é chamado em `main.cpp` **antes** de `brls::Application::init()`,
de propósito, para capturar também os logs de boot do Borealis.

## Fluxo de trabalho

```bash
make install-debug   # sobe .nro + config com debug ligado (detecta o IP local)
make logs-live       # escuta os logs em tempo real (nc -lk 28771)
make logs            # baixa debug.log e debug.log.1 para .logs/ e exibe
make logs-clean      # remove os logs locais
```

`make install-debug` gera o config patchado com `sed` em `/tmp` e envia por FTP;
não altera o `config.json` do repo.

## Por que existem dois canais

**nxlink** só entrega log quando **ele mesmo** lança o app, porque é o loader que
preenche `__nxlink_host`. Instalando por FTP e abrindo pelo hbmenu esse endereço
fica zerado e não há log nenhum.

Como `__nxlink_host` é `extern` público em `switch/runtime/nxlink.h`, o app
preenche o endereço a partir do config e chama `nxlinkConnectToHost()` na mão —
é isso que permite log em tempo real com o app aberto pelo hbmenu.

Se o app **for** lançado pelo nxlink, `connectManualNxlink()` detecta
`__nxlink_host != 0` e ignora o host do config, para não duplicar cada linha.
No log aparece `debug: lancado via nxlink, host manual ignorado`.

**Arquivo no SD** é o canal que sobrevive a crash e reboot, e o único disponível
quando não há PC escutando. Aberto com `_IONBF` (sem buffer) para não perder
linhas num `abort()`, e rotaciona para `.1` a cada boot — então o log do boot que
falhou continua acessível ao reabrir o app para investigar.

## Interpretando o encerramento do app

| Sintoma | Causa |
|---|---|
| `userAppExit` no log, mas `[main] App encerrado` **ausente** | `abort()` — exceção não capturada / `std::terminate` |
| Nada no log e crash report em `sdmc:/atmosphere/crash_reports/` | data abort / segfault real |
| `[main] App encerrado` presente | exit normal (`Application::quit()`) |

Exceção não capturada **não** gera crash report do Atmosphère. É por isso que o
handler de `std::set_terminate` em `main.cpp` existe: ele loga tipo e `what()` da
exceção nos dois canais. Foi ele que revelou o `ENOSYS` do `std::thread`.

O handler grava via `dbg::writeRaw()`, que usa `fprintf` direto em vez do
`Logger`. Chamar o `Logger` de dentro do handler pode dar deadlock, porque ele
toma um mutex que pode estar tomado pela mesma thread no momento da falha.

## Ordem dos logs não é ordem de execução

`printf` e `brls::Logger` usam buffers distintos. Já foi observado `userAppExit`
impresso **antes** de logs de código que rodou muito antes dele.

- Não concluir "morreu aqui" pelo último log impresso.
- Ao instrumentar, usar `printf(...)` + `fflush(stdout)` em todos os pontos.
- Usar marcadores numerados (`[net] c1`, `c2`, ...) em vez de texto solto: fica
  óbvio qual etapa não foi alcançada.

O fork do Borealis já tem `fflush` incondicional no `Logger` (antes era só
`__MINGW32__`), o que reduz muito o problema — mas `printf` cru continua sujeito
a buffer quando a saída não é um terminal.

## Validar caminhos de erro no PC primeiro

O build Switch leva minutos e depende do console disponível. Antes de gastar o
ciclo, compilar o módulo isolado e exercitar os casos de falha:

```bash
g++ -std=gnu++17 -o /tmp/t teste.cpp src/sync/http_client.cpp -I src
```

Cuidado: no PC (glibc) alguns bugs **não** reproduzem — `strerror` nunca retorna
NULL e `std::thread` funciona. Serve para validar lógica, não para provar que
funciona no Switch.
