---
inclusion: auto
name: borealis-fork
description: Use ao mexer no submodule library/, atualizar ou sincronizar o Borealis, resolver conflito no fork, ou investigar comportamento que parece bug do framework. Triggers: submodule, borealis, fork, upstream, xfangfang, library/, submodule update, atualizar borealis.
---

# Fork do Borealis — patches locais

O submodule `library/` **não** é o Borealis upstream. É o fork
[eliasrosa/borealis](https://github.com/eliasrosa/borealis), branch `wiliwili`,
com patches que o app depende para funcionar.

```
origin    → github.com/eliasrosa/borealis  (nosso fork)
upstream  → github.com/xfangfang/borealis  (original)
branch    → wiliwili
```

## ⚠️ Risco de perder os patches

`build.sh` roda `git submodule update --init --recursive` quando não encontra
`library/library/borealis.mk` — arquivo que **não existe** nesta branch, então a
condição é sempre verdadeira e **todo `make build` executa esse update**.

Isso funciona hoje porque os patches estão commitados e pushados no fork. Mas
qualquer alteração **não commitada** dentro de `library/` é descartada
silenciosamente no próximo build.

Ao mexer no submodule: commitar e dar push no fork **antes** de rodar `make build`.

## Os patches

Diff contra `upstream/wiliwili`: 3 arquivos, 3 commits.

### `9f467c44` — fflush incondicional no Logger
`library/include/borealis/core/logger.hpp`

Remove o `#ifdef __MINGW32__` em volta do `fflush(logOut)`.

Sem isso, assim que a saída deixa de ser um terminal (socket do nxlink, redirect
para arquivo, pipe) o stdio vira full-buffered e as linhas se perdem — exatamente
no cenário de diagnosticar um crash. Medido no PC: 0 linhas capturadas antes do
patch, 36 depois.

**Manter.** É a base do modo debug.

### `14aaaa3e` — `extern "C"` em `setIgnoreExitRequest`
Corrige name mangling do símbolo introduzido no commit abaixo.

### `30e64f8e` — patches iniciais
Dois patches não relacionados no mesmo commit:

**1. `switch_platform.cpp` — `ignoreExitRequest`**

Adiciona `static bool ignoreExitRequest = true` e `setIgnoreExitRequest(bool)`;
o hook `AppletHookType_OnExitRequest` só chama `Application::quit()` se a flag
for falsa.

⚠️ **Nada em `src/` chama `setIgnoreExitRequest(false)`.** A flag nasce `true` e
nunca é liberada, então no Switch o `OnExitRequest` fica **permanentemente
ignorado** — o app não responde a pedidos do OS para encerrar.

O quit pelo botão + continua funcionando porque vem de outro caminho
(`Activity::registerExitAction` / dialog do `AppletFrame`), não desse hook.

Esse patch foi criado para investigar o app fechando sozinho no startup, mas a
causa real era outra (`std::thread` lançando `ENOSYS`). Ficou como resíduo:
**candidato a reverter.**

**2. `desktop_platform.cpp` — bateria/wifi forçados**

`canShowBatteryLevel()` e `canShowWirelessLevel()` retornam `true` no ramo Linux
em vez de `false`, com comentário `// forçado para debug visual`.

Patch de conveniência para ver os indicadores do header no PC. Não afeta o
Switch. **Candidato a virar flag de build** em vez de patch no fork.

## Atualizar a partir do upstream

```bash
cd library
git fetch upstream
git rebase upstream/wiliwili     # conflitos esperados nos 3 arquivos acima
git push --force-with-lease
cd .. && git add library && git commit
```

Depois do rebase, conferir que os 3 patches sobreviveram — especialmente o
`fflush`, cuja ausência não quebra o build, só faz o log desaparecer
silenciosamente.
