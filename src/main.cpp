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
#include <unistd.h>

#include <exception>

#include <borealis.hpp>
#include <string>

#include "main_activity.hpp"
#include "views/file_browser_tab.hpp"
#include "views/sync_tab.hpp"
#include "views/settings_tab.hpp"
#include "platform.hpp"

/**
 * Handler de std::terminate — captura exceções não tratadas.
 *
 * Sem isso, uma exceção não capturada chama abort() silenciosamente: os
 * atexit handlers rodam (userAppExit aparece no log) mas não há qualquer
 * indicação do que falhou. No Switch isso é indistinguível de um crash.
 */
static void onTerminate()
{
    printf("\n[FATAL] std::terminate chamado (excecao nao capturada)\n");
    fflush(stdout);

    if (std::exception_ptr ex = std::current_exception())
    {
        try
        {
            std::rethrow_exception(ex);
        }
        catch (const std::exception& e)
        {
            printf("[FATAL] tipo: std::exception | what(): %s\n", e.what());
        }
        catch (...)
        {
            printf("[FATAL] excecao de tipo desconhecido (nao derivada de std::exception)\n");
        }
    }
    else
    {
        printf("[FATAL] sem exception ativa (terminate direto)\n");
    }

    fflush(stdout);
    // Dar tempo do socket do nxlink drenar antes de abortar
    sleep(2);
    abort();
}

int main(int argc, char* argv[])
{
    std::set_terminate(onTerminate);

    // NOTA: No Switch, borealis já faz romfsInit(), socketInitializeDefault()
    // e nxlinkStdio() via userAppInit() em switch_wrapper.c
    // Não duplicar aqui!

    printf("[main] ROMs Manager NS iniciando...\n");

    // Log level
    brls::Logger::setLogLevel(brls::LogLevel::LOG_DEBUG);

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
    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);

    // Push da activity principal
    printf("[main] Criando MainActivity...\n");
    brls::Application::pushActivity(new MainActivity());

    printf("[main] Entrando no loop principal\n");

    // Loop principal
    while (brls::Application::mainLoop())
        ;

    printf("[main] App encerrado\n");

    return EXIT_SUCCESS;
}
