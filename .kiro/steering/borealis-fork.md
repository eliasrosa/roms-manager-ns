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

Diff contra `upstream/wiliwili`: **2 arquivos** em vigor.

### `9f467c44` — fflush incondicional no Logger
`library/include/borealis/core/logger.hpp`

Remove o `#ifdef __MINGW32__` em volta do `fflush(logOut)`.

Sem isso, assim que a saída deixa de ser um terminal (socket do nxlink, redirect
para arquivo, pipe) o stdio vira full-buffered e as linhas se perdem — exatamente
no cenário de diagnosticar um crash. Medido no PC: 0 linhas capturadas antes do
patch, 36 depois.

**Manter.** É a base do modo debug.

### `30e64f8e` — bateria/wifi forçados no desktop
`library/lib/platforms/desktop/desktop_platform.cpp`

`canShowBatteryLevel()` e `canShowWirelessLevel()` retornam `true` no ramo Linux
em vez de `false`, com comentário `// forçado para debug visual`.

Conveniência para ver os indicadores do header ao rodar no PC. Não afeta o
Switch. **Candidato a virar flag de build** em vez de patch no fork.

### Revertido — `ignoreExitRequest` (`ab18b2ca`)

Houve um patch que adicionava `setIgnoreExitRequest()` para ignorar o
`AppletHookType_OnExitRequest`. Foi **revertido**: a flag nascia `true` e nada em
`src/` a liberava, então o `OnExitRequest` ficava permanentemente ignorado e o app
não respondia a pedido do OS para encerrar.

Tinha sido criado para investigar o app fechando sozinho no startup, mas a causa
real era o `std::thread` lançando `ENOSYS`. O quit pelo botão + nunca dependeu
desse hook — vem de `Activity::registerExitAction` / dialog do `AppletFrame`.

Registrado aqui porque a ideia parece atraente ao reencontrar o sintoma: **não
reintroduzir sem antes descartar exceção não capturada** (ver `debug.md`).

## Atualizar a partir do upstream

```bash
cd library
git fetch upstream
git rebase upstream/wiliwili     # conflitos esperados nos 3 arquivos acima
git push --force-with-lease
cd .. && git add library && git commit
```

Depois do rebase, conferir que os patches sobreviveram — especialmente o
`fflush`, cuja ausência não quebra o build, só faz o log desaparecer
silenciosamente.
