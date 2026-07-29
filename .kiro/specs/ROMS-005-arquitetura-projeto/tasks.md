# ROMS-005 — Tasks de Infraestrutura

## Concluídas

- [x] Setup do projeto com devkitPro/libnx
- [x] Integração Borealis como submodule (branch main)
- [x] Makefile unificado (Switch + PC)
- [x] meson.build para build PC
- [x] Dockerfile para build Switch
- [x] Hot reload (watch.sh + inotifywait)
- [x] Deploy via nxlink (Docker fallback)
- [x] Install via FTP
- [x] Servidor HTTP para sync (Python)
- [x] Target `make serve`
- [x] Abstração de plataforma (platform.hpp)
- [x] Inicialização de sockets no Switch
- [x] Workaround swkbd API (sed no Dockerfile)
- [x] Workaround cstdint/optional (GCC 13+ compat)
- [x] test_sd/ com dados fake

## Pendentes

- [ ] CI/CD (GitHub Actions: build + release .nro)
- [ ] .gitignore para server/data/ (dados de teste)
- [ ] Ícone customizado (icon.jpg 256x256)
- [ ] i18n (pt-BR no borealis)
- [ ] Versionamento semântico automatizado
- [ ] Documentação de contribuição (CONTRIBUTING.md)
