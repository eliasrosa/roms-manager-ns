/**
 * SyncManager - Implementação do sync HTTP
 */

#include "sync_manager.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <borealis/core/logger.hpp>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace netsync {

namespace {

// Parser simples de manifest.json (array de files)
std::vector<RemoteFile> parseManifest(const std::string& json)
{
    std::vector<RemoteFile> files;

    // Encontrar array "files"
    size_t filesPos = json.find("\"files\"");
    if (filesPos == std::string::npos)
        return files;

    size_t arrStart = json.find("[", filesPos);
    if (arrStart == std::string::npos)
        return files;

    // Iterar por cada objeto no array
    size_t pos = arrStart;
    while (true)
    {
        size_t objStart = json.find("{", pos + 1);
        if (objStart == std::string::npos) break;

        size_t objEnd = json.find("}", objStart);
        if (objEnd == std::string::npos) break;

        // Verificar se ainda dentro do array
        size_t arrEnd = json.find("]", arrStart);
        if (objStart > arrEnd) break;

        std::string obj = json.substr(objStart, objEnd - objStart + 1);

        RemoteFile file;

        // Extrair campos
        auto extractStr = [&obj](const std::string& key) -> std::string {
            std::string search = "\"" + key + "\"";
            size_t p = obj.find(search);
            if (p == std::string::npos) return "";
            p = obj.find("\"", p + search.size() + 1);
            if (p == std::string::npos) return "";
            size_t e = obj.find("\"", p + 1);
            if (e == std::string::npos) return "";
            return obj.substr(p + 1, e - p - 1);
        };

        auto extractSize = [&obj](const std::string& key) -> size_t {
            std::string search = "\"" + key + "\"";
            size_t p = obj.find(search);
            if (p == std::string::npos) return 0;
            p = obj.find(":", p);
            if (p == std::string::npos) return 0;
            p++;
            while (p < obj.size() && obj[p] == ' ') p++;
            std::string num;
            while (p < obj.size() && isdigit(obj[p]))
                num += obj[p++];
            return num.empty() ? 0 : std::stoull(num);
        };

        file.path = extractStr("path");
        file.size = extractSize("size");
        file.md5 = extractStr("md5");
        file.modified = extractStr("modified");

        if (!file.path.empty())
            files.push_back(file);

        pos = objEnd;
    }

    return files;
}

bool fileExists(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

size_t fileSize(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        return st.st_size;
    return 0;
}

} // namespace anônimo

SyncManager::SyncManager()
{
}

bool SyncManager::loadConfig()
{
    this->config = netsync::loadConfig(netsync::getConfigPath());
    return true;
}

bool SyncManager::testConnection(std::string& errorOut)
{
    std::string url = config.server.baseUrl() + "/health";
    brls::Logger::info("Testando conexao: {}", url);

    HttpResponse resp = httpGet(url);
    if (!resp.ok())
    {
        errorOut = resp.error.empty()
            ? "HTTP " + std::to_string(resp.status_code)
            : resp.error;
        return false;
    }

    return true;
}

SyncResult SyncManager::runSync(SyncCallbacks cb)
{
    SyncResult result;

    // 1. Buscar manifest
    if (cb.onStatus) cb.onStatus("Conectando ao servidor...");

    std::vector<RemoteFile> remoteFiles = this->fetchManifest();
    if (remoteFiles.empty())
    {
        result.error = "Nao foi possivel obter manifest do servidor";
        if (cb.onComplete) cb.onComplete(result);
        return result;
    }

    if (cb.onStatus)
        cb.onStatus("Manifest recebido: " + std::to_string(remoteFiles.size()) + " arquivos");

    // 2. Comparar e baixar
    for (auto& remote : remoteFiles)
    {
        result.files_checked++;

        // Verificar filtros
        if (!this->matchesFilters(remote))
        {
            result.files_skipped++;
            continue;
        }

        // Resolver path local
        std::string localPath = this->resolveLocalPath(remote);
        if (localPath.empty())
        {
            result.files_skipped++;
            continue;
        }

        // Verificar se precisa baixar
        if (!this->shouldDownload(remote, localPath))
        {
            result.files_skipped++;
            continue;
        }

        // Baixar arquivo
        if (cb.onStatus)
            cb.onStatus("Baixando: " + remote.path);

        this->ensureDirectoryExists(localPath);

        std::string downloadUrl = config.server.baseUrl() + remote.path;

        HttpResponse resp = httpDownloadFile(
            downloadUrl,
            localPath,
            [&cb, &remote](size_t downloaded, size_t total) -> bool {
                if (cb.onFileProgress)
                    cb.onFileProgress(remote.path, downloaded, total);
                return true; // continuar
            }
        );

        if (resp.ok())
        {
            result.files_downloaded++;
            result.bytes_downloaded += remote.size;
            result.downloaded_files.push_back(remote.path);
            if (cb.onFileComplete)
                cb.onFileComplete(remote.path, true);
        }
        else
        {
            result.files_failed++;
            result.failed_files.push_back(remote.path);
            brls::Logger::error("Falha ao baixar {}: {}", remote.path, resp.error);
            if (cb.onFileComplete)
                cb.onFileComplete(remote.path, false);
        }
    }

    if (cb.onStatus)
    {
        std::string msg = "Sync concluido: " +
            std::to_string(result.files_downloaded) + " baixados, " +
            std::to_string(result.files_skipped) + " ignorados";
        if (result.files_failed > 0)
            msg += ", " + std::to_string(result.files_failed) + " falharam";
        cb.onStatus(msg);
    }

    if (cb.onComplete) cb.onComplete(result);
    return result;
}

std::vector<RemoteFile> SyncManager::fetchManifest()
{
    std::string url = config.server.baseUrl() + "/manifest.json";
    brls::Logger::info("Buscando manifest: {}", url);

    HttpResponse resp = httpGet(url);
    if (!resp.ok())
    {
        // Quando o servidor responde (ex: 404), o HTTP teve sucesso e 'error'
        // fica vazio — logar só ele produzia "Falha ao buscar manifest:" sem
        // nenhuma pista. Cair para o status code nesse caso.
        std::string reason = resp.error.empty()
            ? "HTTP " + std::to_string(resp.status_code)
            : resp.error;
        brls::Logger::error("Falha ao buscar manifest: {}", reason);
        return {};
    }

    return parseManifest(resp.body);
}

bool SyncManager::shouldDownload(const RemoteFile& remote, const std::string& localPath)
{
    // Se arquivo não existe, baixar
    if (!fileExists(localPath))
        return true;

    // Se tamanho diferente, baixar
    size_t localSize = fileSize(localPath);
    if (localSize != remote.size)
        return true;

    // TODO: se verify_hash habilitado, comparar MD5
    // Por enquanto, se tamanho igual assume que está ok
    return false;
}

bool SyncManager::matchesFilters(const RemoteFile& file)
{
    // Verificar extensão
    if (!config.filters.extensions.empty())
    {
        std::string ext;
        size_t dotPos = file.path.find_last_of('.');
        if (dotPos != std::string::npos)
            ext = file.path.substr(dotPos);

        // Covers e saves não filtram por extensão de ROM
        bool isRom = (file.path.find("/roms") != std::string::npos);
        if (isRom && !ext.empty())
        {
            bool found = false;
            for (auto& e : config.filters.extensions)
            {
                if (e == ext) { found = true; break; }
            }
            if (!found) return false;
        }
    }

    // Verificar tamanho máximo
    if (config.filters.max_file_size_mb > 0)
    {
        size_t maxBytes = config.filters.max_file_size_mb * 1024 * 1024;
        if (file.size > maxBytes)
            return false;
    }

    // Verificar exclude patterns
    for (auto& pattern : config.filters.exclude_patterns)
    {
        // Simplificado: verifica se filename contém o pattern (sem wildcard)
        std::string filename = file.path.substr(file.path.find_last_of('/') + 1);
        if (pattern.front() == '.' && filename.find(pattern) != std::string::npos)
            return false;
        if (filename == pattern)
            return false;
    }

    return true;
}

std::string SyncManager::resolveLocalPath(const RemoteFile& file)
{
    // Mapear path remoto para path local baseado na config
    for (auto& mapping : config.paths)
    {
        if (file.path.find(mapping.remote) == 0)
        {
            // Substituir prefixo remoto pelo local
            std::string relativePath = file.path.substr(mapping.remote.size());
            return mapping.local + relativePath;
        }
    }

    return ""; // Não mapeado
}

void SyncManager::ensureDirectoryExists(const std::string& filepath)
{
    // Encontrar último separador e criar diretórios
    size_t lastSlash = filepath.find_last_of('/');
    if (lastSlash == std::string::npos) return;

    std::string dir = filepath.substr(0, lastSlash);

    // mkdir -p equivalente
    std::string current;
    for (char c : dir)
    {
        current += c;
        if (c == '/')
        {
            mkdir(current.c_str(), 0755);
        }
    }
    mkdir(dir.c_str(), 0755);
}

} // namespace netsync
