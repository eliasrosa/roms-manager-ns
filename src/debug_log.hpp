#pragma once

/**
 * DebugLog - Canais de diagnóstico do app
 *
 * Dois canais independentes, ambos controlados pela seção "debug" do
 * config.json:
 *
 *  1. Arquivo no SD card — sobrevive a crash e a reboot do console, e pode ser
 *     baixado depois com `make logs`. É o canal confiável para investigar o app
 *     morrendo, já que exceção não capturada não gera crash report do
 *     Atmosphère.
 *
 *  2. nxlink em tempo real — o `nxlinkStdio()` do switch_wrapper só funciona
 *     quando o app é lançado PELO nxlink, porque é o loader que preenche
 *     `__nxlink_host`. Instalando via FTP e abrindo pelo hbmenu esse endereço
 *     fica zerado e não há log. Aqui preenchemos o host manualmente a partir do
 *     config e conectamos na porta 28771 do PC (`make logs-live` escuta).
 */

#include <string>

#include "sync/config.hpp"

namespace dbg {

/**
 * Inicializa os canais de debug conforme o config.
 *
 * Deve ser chamado no começo do main(), antes de brls::Application::init(),
 * para capturar também os logs de inicialização do Borealis.
 */
void init(const netsync::DebugConfig& cfg);

/**
 * Fecha o arquivo de log e a conexão nxlink manual (se abertos).
 */
void shutdown();

/**
 * Path do arquivo de log em uso. Vazio se o log em arquivo está desabilitado.
 */
const std::string& logFilePath();

/**
 * Escreve uma linha direto no arquivo de log, sem passar pelo Logger do
 * Borealis.
 *
 * Seguro para usar em handler de terminate/crash: o Logger toma um mutex e, se
 * a falha ocorreu com esse mutex já tomado pela mesma thread, chamá-lo daria
 * deadlock. Aqui é só fprintf num arquivo unbuffered.
 */
void writeRaw(const char* line);

/**
 * Path default do arquivo de log para a plataforma atual.
 */
std::string defaultLogPath();

} // namespace dbg
