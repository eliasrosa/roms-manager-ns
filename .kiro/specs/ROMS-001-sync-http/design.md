# ROMS-001 — Design Técnico: Sync HTTP

## Arquitetura

```
┌─────────────────────────────────┐     ┌──────────────────────────────┐
│         PC / NAS                │     │       Nintendo Switch         │
│                                 │     │                              │
│  server/serve.py                │     │  ROMs Manager NS             │
│  ├─ HTTPServer (porta 8080)     │     │  ├─ SyncTab (UI)             │
│  ├─ manifest.json (auto-gen)    │◄───►│  ├─ SyncManager              │
│  └─ data/                       │WiFi │  ├─ HttpClient (sockets)     │
│     ├─ roms/*.nsp,*.xci        │     │  └─ Config (config.json)     │
│     ├─ covers/*.jpg             │     │                              │
│     └─ saves/*                  │     │  SD Card:                    │
│                                 │     │  └─ sdmc:/roms/              │
└─────────────────────────────────┘     └──────────────────────────────┘
```

## Componentes

### 1. Config (`src/sync/config.hpp/cpp`)
- Parser JSON manual (sem dependência externa)
- Lê/escreve `config.json`
- Struct `AppConfig` com server, sync, paths, filters

### 2. HttpClient (`src/sync/http_client.hpp/cpp`)
- Sockets BSD (POSIX) — portável Switch/PC
- `httpGet(url)` → retorna body como string
- `httpDownloadFile(url, path, progressCallback)` → salva em arquivo
- Resolve DNS via `getaddrinfo`
- Timeout 10s em recv/send

### 3. SyncManager (`src/sync/sync_manager.hpp/cpp`)
- Orquestrador principal
- `fetchManifest()` → GET /manifest.json → parse → vector<RemoteFile>
- `shouldDownload(remote, localPath)` → compara tamanho
- `matchesFilters(file)` → extensões, tamanho máximo, exclude
- `resolveLocalPath(file)` → mapeia remote path → local path via config
- `runSync(callbacks)` → executa sync completo com callbacks para UI

### 4. SyncTab (`src/views/sync_tab.hpp/cpp`)
- UI Borealis (Box-based)
- Botões: Testar Conexão, Iniciar Sync
- Labels: servidor, status, progresso
- Log terminal: fundo escuro, texto verde, 15 linhas visíveis, timestamp

### 5. Servidor (`server/serve.py`)
- Python 3 stdlib puro
- `SimpleHTTPRequestHandler` customizado
- Gera `manifest.json` com: path, size, md5, modified
- Regenera a cada request de /manifest.json
- Endpoint /health para teste de conectividade

## Fluxo de Sync

```
1. SyncTab → onStartSync()
2. SyncManager.runSync(callbacks)
3.   → httpGet(baseUrl + "/manifest.json")
4.   → parseManifest(json) → vector<RemoteFile>
5.   → para cada arquivo:
6.       → matchesFilters(file) → skip se não passa
7.       → resolveLocalPath(file) → mapeia para path local
8.       → shouldDownload(file, localPath) → skip se já existe com mesmo tamanho
9.       → ensureDirectoryExists(localPath)
10.      → httpDownloadFile(url, localPath, progressCb)
11.      → callback onFileComplete
12. → callback onComplete com SyncResult
```

## Formato do manifest.json

```json
{
  "version": 1,
  "generated_at": "2024-01-15T14:30:00-0300",
  "total_files": 6,
  "total_size": 266240,
  "files": [
    {
      "path": "/roms/zelda_totk.nsp",
      "size": 102400,
      "md5": "ad0eab4a...",
      "modified": "2024-01-15T10:00:00-0300"
    }
  ]
}
```

## Mapeamento de Paths

| Remote (servidor) | Local (Switch) | Configurável em |
|-------------------|----------------|-----------------|
| /roms/* | sdmc:/roms/* | config.paths.roms |
| /covers/* | sdmc:/roms/covers/* | config.paths.covers |
| /saves/* | sdmc:/roms/saves/* | config.paths.saves |

## Decisões Técnicas

| Decisão | Escolha | Motivo |
|---------|---------|--------|
| HTTP client | Sockets BSD raw | Sem libcurl = menos deps, portável Switch/PC |
| JSON parser | Manual (substring) | Sem nlohmann/json = binário menor |
| Servidor | Python stdlib | Zero deps, qualquer PC roda |
| Protocolo | HTTP simples | Switch não precisa TLS em rede local |
| Comparação | Tamanho do arquivo | Rápido, suficiente para v1 (MD5 futuro) |
| Namespace | netsync:: | Evita conflito com POSIX sync() |
