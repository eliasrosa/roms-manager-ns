# ROMS-001 — Sync HTTP via WiFi

## Requisitos

### Descrição
Sincronizar ROMs, covers e saves entre um servidor HTTP na rede local e o Nintendo Switch via WiFi. O servidor roda no PC/NAS do usuário e o app no Switch conecta, compara e baixa o que falta.

### Requisitos Funcionais

1. **RF-01**: O app deve ler configuração de servidor (host, porta, protocolo) de um arquivo `config.json`
2. **RF-02**: O app deve buscar um `manifest.json` do servidor contendo lista de arquivos com path, tamanho e hash MD5
3. **RF-03**: O app deve comparar o manifest remoto com os arquivos locais no SD card
4. **RF-04**: O app deve baixar arquivos que não existem localmente ou que têm tamanho diferente
5. **RF-05**: O app deve exibir progresso de download (arquivo atual, porcentagem)
6. **RF-06**: O app deve permitir testar conexão com o servidor antes do sync
7. **RF-07**: O servidor deve gerar o manifest automaticamente a partir da estrutura de diretórios
8. **RF-08**: O app deve respeitar filtros de extensão (.nsp, .xci, .nro) configurados
9. **RF-09**: O app deve criar diretórios locais automaticamente se não existirem
10. **RF-10**: O app deve exibir log estilo terminal com histórico de operações

### Requisitos Não-Funcionais

1. **RNF-01**: Sem dependência de libcurl — usar sockets BSD (portável Switch/PC)
2. **RNF-02**: Servidor em Python 3 puro (sem dependências externas)
3. **RNF-03**: Timeout de conexão de 10 segundos
4. **RNF-04**: Suportar arquivos de até 16GB (ROMs XCI)
5. **RNF-05**: Não bloquear a UI durante sync (futuro: thread separada)
6. **RNF-06**: Config path: `sdmc:/switch/roms-manager-ns/config.json` no Switch, `./config.json` no PC

### Critérios de Aceite

- [ ] App conecta ao servidor e mostra "Conectado" na UI
- [ ] Sync baixa arquivos novos e ignora existentes
- [ ] Filtros de extensão funcionam (ignora arquivos fora da lista)
- [ ] Log mostra cada arquivo baixado com status OK/FALHA
- [ ] Servidor roda com `python3 serve.py` sem setup adicional
- [ ] Funciona tanto no Switch quanto no PC (teste local)

### Fora de Escopo (v1)

- Verificação MD5 pós-download
- Upload do Switch para o servidor
- Sync em thread separada (UI não-bloqueante)
- Autenticação/senha no servidor
- HTTPS/TLS
- Resumo de download interrompido
