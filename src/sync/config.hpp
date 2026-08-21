#pragma once

/**
 * Config - Leitura e acesso à configuração do app (config.json)
 */

#include <string>
#include <vector>

namespace netsync {

struct ServerConfig
{
    std::string host = "192.168.0.100";
    int port = 8080;
    std::string protocol = "http";

    std::string baseUrl() const
    {
        return protocol + "://" + host + ":" + std::to_string(port);
    }
};

struct PathMapping
{
    std::string name;    // ex: "roms", "covers", "saves"
    std::string remote;  // ex: "/roms"
    std::string local;   // ex: "sdmc:/roms/"
};

struct SyncConfig
{
    bool auto_sync = false;
    int check_interval_minutes = 30;
    bool verify_hash = true;
    bool delete_removed = false;
    int max_concurrent_downloads = 2;
};

struct FilterConfig
{
    std::vector<std::string> extensions = {".nsp", ".xci", ".nro"};
    size_t max_file_size_mb = 0; // 0 = sem limite
    std::vector<std::string> exclude_patterns = {"._*", ".DS_Store", "Thumbs.db"};
};

struct DebugConfig
{
    // Grava os logs em arquivo no SD card (sobrevive a crash do app)
    bool log_to_file = false;

    // Path do arquivo de log. Vazio = default da plataforma.
    std::string log_path = "";

    // IP do PC que vai receber os logs em tempo real na porta 28771.
    // Necessário apenas quando o app NÃO é lançado pelo nxlink (ex: instalado
    // via FTP e aberto pelo hbmenu), pois nesse caso o loader não preenche
    // __nxlink_host. Vazio = desabilitado.
    std::string nxlink_host = "";
};

struct AppConfig
{
    ServerConfig server;
    SyncConfig sync;
    std::vector<PathMapping> paths;
    FilterConfig filters;
    DebugConfig debug;
};

/**
 * Carrega config.json do path especificado.
 * Retorna config padrão se o arquivo não existir.
 */
AppConfig loadConfig(const std::string& path);

/**
 * Salva configuração no path especificado.
 */
bool saveConfig(const AppConfig& config, const std::string& path);

/**
 * Retorna o path do config.json para a plataforma atual.
 */
std::string getConfigPath();

} // namespace netsync
