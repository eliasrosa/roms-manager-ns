# ROMS-001 — Tasks

## Concluídas

- [x] Criar `config.json` com estrutura de configuração
- [x] Criar `server/serve.py` (HTTP server + manifest auto-gen)
- [x] Criar `src/sync/config.hpp/cpp` (parser JSON manual)
- [x] Criar `src/sync/http_client.hpp/cpp` (GET via sockets BSD)
- [x] Criar `src/sync/sync_manager.hpp/cpp` (orquestrador)
- [x] Criar `src/views/sync_tab.hpp/cpp` (UI com log terminal)
- [x] Registrar SyncTab no main.cpp e activity XML
- [x] Atualizar Makefile/meson com novos sources
- [x] Testar build PC (compila e roda)
- [x] Testar deploy no Switch via nxlink

## Pendentes (v2)

- [ ] Verificação MD5 pós-download
- [ ] Sync em thread separada (não bloquear UI)
- [ ] Progress bar visual (além do label)
- [ ] Resumo de download interrompido
- [ ] Cancelar sync em andamento
- [ ] Auto-sync periódico (baseado em config)
- [ ] Notificação sonora ao concluir
