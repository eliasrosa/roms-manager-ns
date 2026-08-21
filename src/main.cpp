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
extern "C" void setIgnoreExitRequest(bool ignore);
#endif

#include <stdio.h>
#include <stdlib.h>

#include <borealis.hpp>
#include <string>

#include "main_activity.hpp"
#include "views/file_browser_tab.hpp"
#include "views/sync_tab.hpp"
#include "views/settings_tab.hpp"
#include "platform.hpp"

int main(int argc, char* argv[])
{
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

    // Habilitar quit global com botão + (apenas no PC)
    // No Switch via nxlink o hbmenu injeta um evento + ao abrir,
    // o que fecharia o app imediatamente
#ifndef __SWITCH__
    brls::Application::setGlobalQuit(true);
#endif

    // Registrar views customizadas
    printf("[main] Registrando views...\n");
    brls::Application::registerXMLView("FileBrowserTab", FileBrowserTab::create);
    brls::Application::registerXMLView("SyncTab", SyncTab::create);
    brls::Application::registerXMLView("SettingsTab", SettingsTab::create);

    // Push da activity principal
    printf("[main] Criando MainActivity...\n");
    brls::Application::pushActivity(new MainActivity());

    printf("[main] Entrando no loop principal\n");

#ifdef __SWITCH__
    // Liberar o OnExitRequest após 1s — ignora o request espúrio do hbmenu no startup
    brls::delay(1000, []() {
        setIgnoreExitRequest(false);
        brls::Logger::info("OnExitRequest habilitado");
    });
#endif

    // Loop principal
    int frames = 0;
    while (brls::Application::mainLoop())
    {
        frames++;
        if (frames <= 5)
            brls::Logger::info("frame {}", frames);
    }

    printf("[main] App encerrado após %d frames\n", frames);

    return EXIT_SUCCESS;
}
