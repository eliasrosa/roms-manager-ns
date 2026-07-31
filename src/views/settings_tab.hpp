#pragma once

/**
 * SettingsTab - Tab de configurações do app
 *
 * Permite editar host, porta e protocolo do servidor de sync.
 */

#include <borealis.hpp>
#include "../sync/config.hpp"

class SettingsTab : public brls::Box
{
  public:
    SettingsTab();

    static brls::View* create();

  private:
    netsync::AppConfig config;
    std::string configPath;

    brls::Label* hostValueLabel = nullptr;
    brls::Label* portValueLabel = nullptr;
    brls::Label* statusLabel = nullptr;

    void buildUI();
    void onEditHost();
    void onEditPort();
    void saveAndUpdateUI();
};
