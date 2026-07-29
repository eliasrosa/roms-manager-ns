#pragma once

/**
 * HttpClient - Wrapper simples sobre sockets/curl para GET HTTP
 *
 * No Switch: usa sockets BSD do libnx
 * No PC: usa libcurl (mais fácil de linkar)
 *
 * Para manter portabilidade, usa implementação com sockets raw
 * que funciona em ambas as plataformas.
 */

#include <string>
#include <functional>
#include <vector>
#include <cstdint>

namespace netsync {

// Callback de progresso: (bytes_baixados, bytes_total) -> continuar?
using ProgressCallback = std::function<bool(size_t downloaded, size_t total)>;

struct HttpResponse
{
    int status_code = 0;
    std::string body;
    std::string error;
    bool ok() const { return status_code >= 200 && status_code < 300; }
};

/**
 * Faz GET HTTP e retorna response body como string.
 */
HttpResponse httpGet(const std::string& url);

/**
 * Faz GET HTTP e salva response body em arquivo.
 * Suporta callback de progresso.
 */
HttpResponse httpDownloadFile(
    const std::string& url,
    const std::string& outputPath,
    ProgressCallback progress = nullptr
);

} // namespace netsync
