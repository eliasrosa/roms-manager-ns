/**
 * Config - Implementação da leitura/escrita de config.json
 * Usa tinyxml2 para parsing JSON-like... na verdade usa parsing manual
 * simples já que tinyxml2 é XML. Vamos usar um parser JSON mínimo.
 *
 * Como o borealis já inclui nlohmann/json ou tinyxml2, e o projeto
 * precisa ser leve, usamos parsing manual com a lib padrão.
 */

#include "config.hpp"
#include "../platform.hpp"

#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/stat.h>

// Parser JSON mínimo (sem dependência externa)
// Suporta apenas o subset necessário para config.json
namespace {

std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string extractString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";

    pos = json.find("\"", pos + 1);
    if (pos == std::string::npos) return "";

    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return "";

    return json.substr(pos + 1, end - pos - 1);
}

int extractInt(const std::string& json, const std::string& key, int defaultVal = 0)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(":", pos);
    if (pos == std::string::npos) return defaultVal;

    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
        pos++;

    std::string num;
    while (pos < json.size() && (isdigit(json[pos]) || json[pos] == '-'))
        num += json[pos++];

    if (num.empty()) return defaultVal;
    return std::stoi(num);
}

bool extractBool(const std::string& json, const std::string& key, bool defaultVal = false)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(":", pos);
    if (pos == std::string::npos) return defaultVal;

    std::string rest = json.substr(pos + 1, 10);
    rest = trim(rest);

    if (rest.find("true") == 0) return true;
    if (rest.find("false") == 0) return false;
    return defaultVal;
}

std::vector<std::string> extractStringArray(const std::string& json, const std::string& key)
{
    std::vector<std::string> result;

    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return result;

    pos = json.find("[", pos);
    if (pos == std::string::npos) return result;

    size_t end = json.find("]", pos);
    if (end == std::string::npos) return result;

    std::string arr = json.substr(pos + 1, end - pos - 1);

    size_t p = 0;
    while (true)
    {
        size_t qs = arr.find("\"", p);
        if (qs == std::string::npos) break;
        size_t qe = arr.find("\"", qs + 1);
        if (qe == std::string::npos) break;
        result.push_back(arr.substr(qs + 1, qe - qs - 1));
        p = qe + 1;
    }

    return result;
}

std::string extractObject(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    pos = json.find("{", pos);
    if (pos == std::string::npos) return "";

    int depth = 0;
    size_t end = pos;
    for (size_t i = pos; i < json.size(); i++)
    {
        if (json[i] == '{') depth++;
        else if (json[i] == '}') depth--;
        if (depth == 0) { end = i; break; }
    }

    return json.substr(pos, end - pos + 1);
}

} // namespace anônimo

namespace netsync {

AppConfig loadConfig(const std::string& path)
{
    AppConfig config;

    // Garantir que o diretório do config existe
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos)
    {
        std::string dir = path.substr(0, lastSlash);
        mkdir(dir.c_str(), 0755);
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        // Se não existe, criar com defaults
        config.paths.push_back({"roms", "/roms", platform::sdRoot() + "roms/"});
        config.paths.push_back({"covers", "/covers", platform::sdRoot() + "roms/covers/"});
        config.paths.push_back({"saves", "/saves", platform::sdRoot() + "roms/saves/"});
        saveConfig(config, path);
        return config;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string json = ss.str();
    file.close();

    // Parse server
    std::string serverJson = extractObject(json, "server");
    if (!serverJson.empty())
    {
        config.server.host = extractString(serverJson, "host");
        config.server.port = extractInt(serverJson, "port", 8080);
        config.server.protocol = extractString(serverJson, "protocol");
        if (config.server.protocol.empty()) config.server.protocol = "http";
        if (config.server.host.empty()) config.server.host = "192.168.1.100";
    }

    // Parse sync
    std::string syncJson = extractObject(json, "sync");
    if (!syncJson.empty())
    {
        config.sync.auto_sync = extractBool(syncJson, "auto_sync", false);
        config.sync.check_interval_minutes = extractInt(syncJson, "check_interval_minutes", 30);
        config.sync.verify_hash = extractBool(syncJson, "verify_hash", true);
        config.sync.delete_removed = extractBool(syncJson, "delete_removed", false);
        config.sync.max_concurrent_downloads = extractInt(syncJson, "max_concurrent_downloads", 2);
    }

    // Parse paths
    std::string pathsJson = extractObject(json, "paths");
    if (!pathsJson.empty())
    {
        const char* pathNames[] = {"roms", "covers", "saves"};
        for (const char* name : pathNames)
        {
            std::string obj = extractObject(pathsJson, name);
            if (!obj.empty())
            {
                PathMapping mapping;
                mapping.name = name;
                mapping.remote = extractString(obj, "remote");
                mapping.local = extractString(obj, "local");
                if (!mapping.remote.empty() && !mapping.local.empty())
                    config.paths.push_back(mapping);
            }
        }
    }

    // Se nenhum path definido, usar defaults
    if (config.paths.empty())
    {
        config.paths.push_back({"roms", "/roms", platform::sdRoot() + "roms/"});
        config.paths.push_back({"covers", "/covers", platform::sdRoot() + "roms/covers/"});
        config.paths.push_back({"saves", "/saves", platform::sdRoot() + "roms/saves/"});
    }

    // Parse filters
    std::string filtersJson = extractObject(json, "filters");
    if (!filtersJson.empty())
    {
        auto exts = extractStringArray(filtersJson, "extensions");
        if (!exts.empty()) config.filters.extensions = exts;

        config.filters.max_file_size_mb = extractInt(filtersJson, "max_file_size_mb", 0);

        auto excludes = extractStringArray(filtersJson, "exclude_patterns");
        if (!excludes.empty()) config.filters.exclude_patterns = excludes;
    }

    return config;
}

bool saveConfig(const AppConfig& config, const std::string& path)
{
    std::ofstream file(path);
    if (!file.is_open())
        return false;

    file << "{\n";
    file << "  \"server\": {\n";
    file << "    \"host\": \"" << config.server.host << "\",\n";
    file << "    \"port\": " << config.server.port << ",\n";
    file << "    \"protocol\": \"" << config.server.protocol << "\"\n";
    file << "  },\n";

    file << "  \"sync\": {\n";
    file << "    \"auto_sync\": " << (config.sync.auto_sync ? "true" : "false") << ",\n";
    file << "    \"check_interval_minutes\": " << config.sync.check_interval_minutes << ",\n";
    file << "    \"verify_hash\": " << (config.sync.verify_hash ? "true" : "false") << ",\n";
    file << "    \"delete_removed\": " << (config.sync.delete_removed ? "true" : "false") << ",\n";
    file << "    \"max_concurrent_downloads\": " << config.sync.max_concurrent_downloads << "\n";
    file << "  },\n";

    file << "  \"paths\": {\n";
    for (size_t i = 0; i < config.paths.size(); i++)
    {
        auto& p = config.paths[i];
        file << "    \"" << p.name << "\": {\n";
        file << "      \"remote\": \"" << p.remote << "\",\n";
        file << "      \"local\": \"" << p.local << "\"\n";
        file << "    }";
        if (i < config.paths.size() - 1) file << ",";
        file << "\n";
    }
    file << "  },\n";

    file << "  \"filters\": {\n";
    file << "    \"extensions\": [";
    for (size_t i = 0; i < config.filters.extensions.size(); i++)
    {
        file << "\"" << config.filters.extensions[i] << "\"";
        if (i < config.filters.extensions.size() - 1) file << ", ";
    }
    file << "],\n";
    file << "    \"max_file_size_mb\": " << config.filters.max_file_size_mb << ",\n";
    file << "    \"exclude_patterns\": [";
    for (size_t i = 0; i < config.filters.exclude_patterns.size(); i++)
    {
        file << "\"" << config.filters.exclude_patterns[i] << "\"";
        if (i < config.filters.exclude_patterns.size() - 1) file << ", ";
    }
    file << "]\n";
    file << "  }\n";
    file << "}\n";

    file.close();
    return true;
}

std::string getConfigPath()
{
#ifdef __SWITCH__
    return "sdmc:/switch/roms-manager-ns/config.json";
#else
    // No PC: tentar config.local.json primeiro (gitignored), fallback para config.json
    std::ifstream localFile("./config.local.json");
    if (localFile.good())
    {
        localFile.close();
        return "./config.local.json";
    }
    return "./config.json";
#endif
}

} // namespace netsync
