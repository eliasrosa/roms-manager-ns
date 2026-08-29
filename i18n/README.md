# i18n/ — traduções do app

Traduções mantidas por este projeto, **versionadas**.

O Borealis só traz `en-US`, `fr`, `ru` e `zh-Hans`. Idiomas adicionais (como
`pt-BR`) ficam aqui e o `setup-resources.sh` os copia para `resources/i18n/`
depois de copiar os do Borealis.

> Não editar `resources/i18n/` diretamente: aquele diretório é gitignored e o
> `setup-resources.sh` o apaga e recria a cada execução.

## Fallback

O Borealis carrega `en-US` como locale padrão e depois o locale atual. Uma chave
ausente na tradução cai automaticamente no texto em inglês, então um arquivo
parcial (só `hints.json`, por exemplo) é seguro.
