/**
 * @file http_client.cpp
 * @brief Implementação HTTP GET com sockets BSD
 *
 * Funciona tanto no Switch (libnx sockets) quanto no PC (POSIX sockets).
 * Sem dependência de libcurl.
 */

#include "http_client.hpp"

#include <cstring>
#include <cstdio>
#include <sstream>
#include <fstream>

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

namespace netsync {

namespace {

// Tamanho máximo de buffer de recepção
constexpr size_t RECV_BUFFER_SIZE = 8192;

// Tamanho máximo de headers HTTP (proteção contra resposta malformada)
constexpr size_t MAX_HEADER_SIZE = 16384;

// Tamanho máximo de body para httpGet (proteção contra OOM)
constexpr size_t MAX_BODY_SIZE = 4 * 1024 * 1024; // 4MB

struct UrlParts
{
    std::string host;
    int port = 80;
    std::string path = "/";
    bool valid = false;
};

/**
 * Retorna mensagem de erro legível para errno atual
 *
 * IMPORTANTE: no newlib (devkitPro) strerror() pode retornar NULL para
 * valores de errno que ele não mapeia — e os erros de socket do libnx vêm
 * do serviço bsd:s, fora do range padrão. std::string(NULL) causa crash
 * imediato no Switch. Sempre validar o retorno antes de construir a string.
 */
std::string errnoMessage()
{
    int err = errno;
    const char* msg = strerror(err);
    if (msg == nullptr || *msg == '\0')
        return "errno " + std::to_string(err);
    return std::string(msg);
}

/**
 * Parseia URL no formato http://host:port/path
 */
UrlParts parseUrl(const std::string& url)
{
    UrlParts parts;

    if (url.empty())
    {
        parts.valid = false;
        return parts;
    }

    std::string work = url;

    // Remover protocolo
    size_t protoEnd = work.find("://");
    if (protoEnd != std::string::npos)
        work = work.substr(protoEnd + 3);

    if (work.empty())
    {
        parts.valid = false;
        return parts;
    }

    // Separar host:port/path
    size_t pathStart = work.find('/');
    std::string hostPort;
    if (pathStart != std::string::npos)
    {
        hostPort = work.substr(0, pathStart);
        parts.path = work.substr(pathStart);
    }
    else
    {
        hostPort = work;
        parts.path = "/";
    }

    if (hostPort.empty())
    {
        parts.valid = false;
        return parts;
    }

    // Separar host e port
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos)
    {
        parts.host = hostPort.substr(0, colonPos);
        std::string portStr = hostPort.substr(colonPos + 1);
        try
        {
            parts.port = std::stoi(portStr);
            if (parts.port <= 0 || parts.port > 65535)
            {
                parts.valid = false;
                return parts;
            }
        }
        catch (...)
        {
            parts.valid = false;
            return parts;
        }
    }
    else
    {
        parts.host = hostPort;
    }

    parts.valid = !parts.host.empty();
    return parts;
}

/**
 * Conecta ao host:port via TCP com timeout
 * Retorna socket fd ou -1 em caso de erro
 */
int connectToHost(const std::string& host, int port, std::string& errorOut)
{
    // Resolver endereço — IP direto (sem DNS)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_aton(host.c_str(), &addr.sin_addr) == 0)
    {
        struct hostent* he = gethostbyname(host.c_str());
        if (!he || !he->h_addr_list || !he->h_addr_list[0] || he->h_length <= 0)
        {
            errorOut = "DNS falhou para '" + host + "'";
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0],
               he->h_length > (int)sizeof(addr.sin_addr) ? sizeof(addr.sin_addr) : he->h_length);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        errorOut = "Falha ao criar socket: " + errnoMessage();
        return -1;
    }

    // Timeout para send/recv (3s)
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    std::string portStr = std::to_string(port);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        errorOut = "Conexao recusada em " + host + ":" + portStr + " (" + errnoMessage() + ")";
        close(sock);
        return -1;
    }

    return sock;
}

/**
 * Envia todos os bytes do buffer via socket
 */
bool sendAll(int sock, const std::string& data, std::string& errorOut)
{
    size_t total = 0;
    while (total < data.size())
    {
        ssize_t sent = send(sock, data.c_str() + total, data.size() - total, 0);
        if (sent < 0)
        {
            errorOut = "Erro ao enviar dados: " + errnoMessage();
            return false;
        }
        if (sent == 0)
        {
            errorOut = "Conexao fechada pelo servidor durante envio";
            return false;
        }
        total += sent;
    }
    return true;
}

/**
 * Monta request HTTP GET
 */
std::string buildRequest(const UrlParts& parts)
{
    return "GET " + parts.path + " HTTP/1.1\r\n"
         + "Host: " + parts.host + "\r\n"
         + "Connection: close\r\n"
         + "User-Agent: RomsManagerNS/0.2\r\n"
         + "\r\n";
}

/**
 * Extrai status code do header HTTP
 */
int parseStatusCode(const std::string& header)
{
    // Formato: "HTTP/1.1 200 OK\r\n..."
    size_t spacePos = header.find(' ');
    if (spacePos == std::string::npos || spacePos + 3 >= header.size())
        return 0;

    std::string codeStr = header.substr(spacePos + 1, 3);
    try
    {
        return std::stoi(codeStr);
    }
    catch (...)
    {
        return 0;
    }
}

/**
 * Extrai Content-Length do header HTTP (0 se não presente)
 */
size_t parseContentLength(const std::string& header)
{
    std::string key = "Content-Length: ";
    size_t pos = header.find(key);
    if (pos == std::string::npos)
    {
        // Tentar lowercase
        key = "content-length: ";
        pos = header.find(key);
        if (pos == std::string::npos)
            return 0;
    }

    size_t end = header.find("\r\n", pos);
    if (end == std::string::npos)
        return 0;

    std::string val = header.substr(pos + key.size(), end - pos - key.size());
    try
    {
        return std::stoull(val);
    }
    catch (...)
    {
        return 0;
    }
}

} // namespace anônimo

HttpResponse httpGet(const std::string& url)
{
    HttpResponse response;

    // Validar URL
    UrlParts parts = parseUrl(url);
    if (!parts.valid)
    {
        response.error = "URL invalida: '" + url + "'";
        return response;
    }

    // Conectar
    std::string connError;
    int sock = connectToHost(parts.host, parts.port, connError);
    if (sock < 0)
    {
        response.error = connError;
        return response;
    }

    // Enviar request
    std::string request = buildRequest(parts);
    std::string sendError;
    if (!sendAll(sock, request, sendError))
    {
        close(sock);
        response.error = sendError;
        return response;
    }

    // Ler response completa
    std::string raw;
    char buf[RECV_BUFFER_SIZE];
    while (true)
    {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                response.error = "Timeout ao receber resposta do servidor";
                close(sock);
                return response;
            }
            response.error = "Erro ao receber dados: " + errnoMessage();
            close(sock);
            return response;
        }
        if (n == 0) break; // conexão fechada (esperado com Connection: close)

        raw.append(buf, n);

        // Proteção contra resposta gigante
        if (raw.size() > MAX_HEADER_SIZE + MAX_BODY_SIZE)
        {
            response.error = "Resposta excede tamanho maximo permitido";
            close(sock);
            return response;
        }
    }
    close(sock);

    if (raw.empty())
    {
        response.error = "Resposta vazia do servidor";
        return response;
    }

    // Separar header e body
    size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
    {
        response.error = "Resposta HTTP malformada (sem fim de headers)";
        return response;
    }

    std::string header = raw.substr(0, headerEnd);
    response.body = raw.substr(headerEnd + 4);
    response.status_code = parseStatusCode(header);

    if (response.status_code == 0)
    {
        response.error = "Nao foi possivel parsear status HTTP";
    }

    return response;
}

HttpResponse httpDownloadFile(
    const std::string& url,
    const std::string& outputPath,
    ProgressCallback progress)
{
    HttpResponse response;

    // Validar URL
    UrlParts parts = parseUrl(url);
    if (!parts.valid)
    {
        response.error = "URL invalida: '" + url + "'";
        return response;
    }

    // Validar output path
    if (outputPath.empty())
    {
        response.error = "Path de saida vazio";
        return response;
    }

    // Conectar
    std::string connError;
    int sock = connectToHost(parts.host, parts.port, connError);
    if (sock < 0)
    {
        response.error = connError;
        return response;
    }

    // Enviar request
    std::string request = buildRequest(parts);
    std::string sendError;
    if (!sendAll(sock, request, sendError))
    {
        close(sock);
        response.error = sendError;
        return response;
    }

    // Ler headers (byte a byte até \r\n\r\n)
    std::string headerBuf;
    char c;
    while (headerBuf.size() < MAX_HEADER_SIZE)
    {
        ssize_t n = recv(sock, &c, 1, 0);
        if (n < 0)
        {
            response.error = "Timeout ao ler headers: " + errnoMessage();
            close(sock);
            return response;
        }
        if (n == 0)
        {
            response.error = "Conexao fechada antes de receber headers completos";
            close(sock);
            return response;
        }

        headerBuf += c;
        if (headerBuf.size() >= 4 &&
            headerBuf.substr(headerBuf.size() - 4) == "\r\n\r\n")
            break;
    }

    // Extrair status e content-length
    response.status_code = parseStatusCode(headerBuf);
    size_t totalSize = parseContentLength(headerBuf);

    if (response.status_code < 200 || response.status_code >= 300)
    {
        close(sock);
        response.error = "Servidor retornou HTTP " + std::to_string(response.status_code);
        return response;
    }

    // Abrir arquivo de saída
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open())
    {
        close(sock);
        response.error = "Nao foi possivel criar arquivo: " + outputPath + " (" + errnoMessage() + ")";
        return response;
    }

    // Download com progress
    size_t downloaded = 0;
    char buf[RECV_BUFFER_SIZE];
    while (true)
    {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                response.error = "Timeout durante download de " + parts.path;
                file.close();
                close(sock);
                return response;
            }
            response.error = "Erro ao receber dados: " + errnoMessage();
            file.close();
            close(sock);
            return response;
        }
        if (n == 0) break; // download completo

        file.write(buf, n);
        downloaded += n;

        if (progress)
        {
            if (!progress(downloaded, totalSize))
            {
                file.close();
                close(sock);
                response.error = "Download cancelado pelo usuario";
                response.status_code = 0;
                return response;
            }
        }
    }

    file.close();
    close(sock);

    // Verificar se baixou tudo (quando Content-Length conhecido)
    if (totalSize > 0 && downloaded != totalSize)
    {
        response.error = "Download incompleto: " + std::to_string(downloaded) +
                         "/" + std::to_string(totalSize) + " bytes";
        response.status_code = 0;
        return response;
    }

    response.body = std::to_string(downloaded) + " bytes";
    return response;
}

} // namespace netsync
