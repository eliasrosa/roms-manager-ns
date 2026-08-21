/**
 * DebugLog - Implementação dos canais de diagnóstico
 */

#include "debug_log.hpp"

#include <borealis/core/logger.hpp>

#include <cstdio>
#include <ctime>
#include <sys/stat.h>

#ifdef __SWITCH__
#include <switch.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace dbg {

namespace {

std::FILE* logFile = nullptr;
std::string activeLogPath;
int manualNxlinkSock = -1;
bool subscribed = false;

const char* levelName(brls::LogLevel level)
{
    switch (level)
    {
        case brls::LogLevel::LOG_ERROR:   return "ERROR";
        case brls::LogLevel::LOG_WARNING: return "WARN";
        case brls::LogLevel::LOG_INFO:    return "INFO";
        case brls::LogLevel::LOG_DEBUG:   return "DEBUG";
        case brls::LogLevel::LOG_VERBOSE: return "VERB";
        default:                          return "?";
    }
}

/**
 * Cria os diretórios do path do arquivo (equivalente a mkdir -p no dirname).
 */
void ensureParentDir(const std::string& path)
{
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos || lastSlash == 0)
        return;

    std::string current;
    for (size_t i = 0; i < lastSlash; i++)
    {
        current += path[i];
        if (path[i] == '/')
            mkdir(current.c_str(), 0755);
    }
    mkdir(path.substr(0, lastSlash).c_str(), 0755);
}

/**
 * Abre o arquivo de log, rotacionando o anterior para .1
 *
 * A rotação existe para o caso mais importante: o app morreu, você reabre para
 * investigar e o log do boot que falhou continua disponível em .1.
 */
bool openLogFile(const std::string& path)
{
    ensureParentDir(path);

    std::string previous = path + ".1";
    std::remove(previous.c_str());
    std::rename(path.c_str(), previous.c_str());

    logFile = std::fopen(path.c_str(), "w");
    if (!logFile)
        return false;

    // Sem buffer: garante que cada linha chega ao SD antes de um possível
    // abort(). Custa I/O, mas é debug — perder o log é pior.
    std::setvbuf(logFile, nullptr, _IONBF, 0);

    activeLogPath = path;
    return true;
}

/**
 * Escreve uma linha no arquivo de log.
 *
 * Roda dentro do lock do Logger do Borealis (o fire do evento é feito com o
 * mutex tomado), então não precisa de sincronização própria. Usa apenas
 * fprintf para não lançar exceção — uma exceção aqui cairia no catch do
 * Logger e seria reportada como erro de formato.
 */
void writeToFile(brls::Logger::TimePoint when, brls::LogLevel level, const std::string& msg)
{
    if (!logFile)
        return;

    std::time_t tt = std::chrono::system_clock::to_time_t(when);
    std::tm tm = *std::localtime(&tt);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  when.time_since_epoch()).count() % 1000;

    std::fprintf(logFile, "%02d:%02d:%02d.%03d [%s] %s\n",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, (int)ms,
                 levelName(level), msg.c_str());
}

#ifdef __SWITCH__
/**
 * Conecta ao host de log em tempo real quando o app NÃO foi lançado pelo nxlink.
 *
 * Só age se __nxlink_host estiver zerado. Se o app veio do nxlink, o endereço
 * já está preenchido e o switch_wrapper (sob -DDEBUG) já redirecionou a saída —
 * conectar de novo duplicaria as linhas.
 */
void connectManualNxlink(const std::string& host)
{
    if (host.empty())
        return;

    if (__nxlink_host.s_addr != 0)
    {
        brls::Logger::info("debug: lancado via nxlink, host manual ignorado");
        return;
    }

    struct in_addr addr;
    if (inet_aton(host.c_str(), &addr) == 0)
    {
        brls::Logger::warning("debug: nxlink_host invalido: {}", host);
        return;
    }

    __nxlink_host = addr;
    manualNxlinkSock = nxlinkConnectToHost(true, true);

    if (manualNxlinkSock < 0)
    {
        // Sem listener do outro lado. Não é erro fatal: o app segue normal.
        __nxlink_host.s_addr = 0;
        return;
    }

    brls::Logger::info("debug: log em tempo real -> {}:{}", host, NXLINK_CLIENT_PORT);
}
#endif

} // namespace anônimo

std::string defaultLogPath()
{
#ifdef __SWITCH__
    return "sdmc:/switch/roms-manager-ns/debug.log";
#else
    return "./debug.log";
#endif
}

void init(const netsync::DebugConfig& cfg)
{
    // O arquivo é aberto ANTES de conectar o nxlink, para que o próprio
    // resultado dessa conexão (ativada ou ignorada) fique registrado nele.
    // Caso contrário o arquivo não diz nada sobre o canal em tempo real.
    if (cfg.log_to_file)
    {
        std::string path = cfg.log_path.empty() ? defaultLogPath() : cfg.log_path;

        if (openLogFile(path))
        {
            if (!subscribed)
            {
                brls::Logger::getLogEvent()->subscribe(writeToFile);
                subscribed = true;
            }

            // Header de sessão: sem isso, ao abrir um log rotacionado não há
            // como saber de quando ele é.
            std::time_t now = std::time(nullptr);
            std::tm tm = *std::localtime(&now);
            std::fprintf(logFile,
                         "=== ROMs Manager NS | sessao %04d-%02d-%02d %02d:%02d:%02d ===\n",
                         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                         tm.tm_hour, tm.tm_min, tm.tm_sec);

            brls::Logger::info("debug: log em arquivo -> {}", path);
        }
        else
        {
            brls::Logger::warning("debug: nao foi possivel abrir log em {}", path);
        }
    }

#ifdef __SWITCH__
    connectManualNxlink(cfg.nxlink_host);
#endif
}

void shutdown()
{
    if (logFile)
    {
        std::fprintf(logFile, "=== fim da sessao ===\n");
        std::fclose(logFile);
        logFile = nullptr;
    }

#ifdef __SWITCH__
    if (manualNxlinkSock >= 0)
    {
        close(manualNxlinkSock);
        manualNxlinkSock = -1;
    }
#endif
}

const std::string& logFilePath()
{
    return activeLogPath;
}

void writeRaw(const char* line)
{
    if (!logFile || !line)
        return;

    std::fprintf(logFile, "%s\n", line);
}

} // namespace dbg
