#pragma once

/**
 * SyncManager - Orquestra a sincronização de arquivos com o servidor
 *
 * Fluxo:
 *   1. Lê config.json
 *   2. Busca manifest.json do servidor
 *   3. Compara com arquivos locais
 *   4. Baixa o que falta (ou está desatualizado)
 */

#include "config.hpp"
#include "http_client.hpp"

#include <string>
#include <vector>
#include <functional>

namespace netsync {

struct RemoteFile
{
    std::string path;   // ex: "/roms/zelda.nsp"
    size_t size = 0;
    std::string md5;
    std::string modified;
};

struct SyncResult
{
    int files_checked = 0;
    int files_downloaded = 0;
    int files_skipped = 0;
    int files_failed = 0;
    size_t bytes_downloaded = 0;
    std::string error;
    std::vector<std::string> downloaded_files;
    std::vector<std::string> failed_files;

    bool ok() const { return error.empty(); }
};

// Callbacks para a UI
struct SyncCallbacks
{
    // Status geral: "Conectando...", "Baixando manifest...", etc.
    std::function<void(const std::string& status)> onStatus;

    // Progresso de arquivo individual
    std::function<void(const std::string& filename, size_t downloaded, size_t total)> onFileProgress;

    // Arquivo concluído
    std::function<void(const std::string& filename, bool success)> onFileComplete;

    // Sync concluído
    std::function<void(const SyncResult& result)> onComplete;
};

class SyncManager
{
  public:
    SyncManager();

    /**
     * Carrega configuração do config.json
     */
    bool loadConfig();

    /**
     * Testa conexão com o servidor (GET /health)
     */
    bool testConnection(std::string& errorOut);

    /**
     * Executa sync completo
     */
    SyncResult runSync(SyncCallbacks callbacks = {});

    /**
     * Retorna a config carregada
     */
    const AppConfig& getConfig() const { return config; }

  private:
    AppConfig config;

    std::vector<RemoteFile> fetchManifest();
    bool shouldDownload(const RemoteFile& remote, const std::string& localPath);
    bool matchesFilters(const RemoteFile& file);
    std::string resolveLocalPath(const RemoteFile& file);
    void ensureDirectoryExists(const std::string& filepath);
};

} // namespace netsync
