/**
 * ROMs Manager NS - Versão UI com Borealis
 *
 * Interface gráfica estilo Nintendo Switch para navegar
 * e gerenciar ROMs no SD card.
 *
 * Compila para Switch (devkitPro) e PC (meson/glfw) para testes.
 */

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <borealis.hpp>
#include <string>

#include "main_activity.hpp"
#include "views/file_browser_tab.hpp"
#include "views/sync_tab.hpp"
#include "platform.hpp"

int main(int argc, char* argv[])
{
#ifdef __SWITCH__
    // Inicializar sockets para rede (necessário para HTTP e nxlink logs)
    socketInitializeDefault();

    // Redirecionar stdout/stderr para nxlink (permite ver logs no PC)
    nxlinkStdio();
#endif

    printf("[main] ROMs Manager NS iniciando...\n");

    // Log level
    brls::Logger::setLogLevel(brls::LogLevel::DEBUG);

    printf("[main] Inicializando Borealis...\n");

    // Inicializar Borealis
    if (!brls::Application::init())
    {
        printf("[main] ERRO: Falha ao inicializar Borealis\n");
        brls::Logger::error("Falha ao inicializar Borealis");
#ifdef __SWITCH__
        socketExit();
#endif
        return EXIT_FAILURE;
    }

    printf("[main] Borealis inicializado OK\n");
    printf("[main] Criando janela...\n");

    brls::Application::createWindow("ROMs Manager NS");

    printf("[main] Janela criada\n");

    // Habilitar quit global com botão +
    brls::Application::setGlobalQuit(true);

    // Registrar views customizadas
    printf("[main] Registrando views...\n");
    brls::Application::registerXMLView("FileBrowserTab", FileBrowserTab::create);
    brls::Application::registerXMLView("SyncTab", SyncTab::create);

    // Push da activity principal
    printf("[main] Criando MainActivity...\n");
    brls::Application::pushActivity(new MainActivity());

    printf("[main] Entrando no loop principal\n");

    // Loop principal
    while (brls::Application::mainLoop())
        ;

    printf("[main] Loop encerrado, saindo...\n");

#ifdef __SWITCH__
    socketExit();
#endif

    return EXIT_SUCCESS;
}
