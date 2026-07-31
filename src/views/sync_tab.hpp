#pragma once

/**
 * SyncTab - Tab de sincronização com servidor
 *
 * Mostra status da conexão, botão de sync, progresso e log estilo terminal.
 */

#include <borealis.hpp>
#include "../sync/sync_manager.hpp"

class SyncTab : public brls::Box
{
  public:
    SyncTab();

    static brls::View* create();

  private:
    netsync::SyncManager syncManager;

    brls::Label* statusLabel = nullptr;
    brls::Label* connectionIcon = nullptr;
    brls::Label* serverLabel = nullptr;
    brls::Label* progressLabel = nullptr;
    brls::Box* logContainer = nullptr;
    brls::ScrollingFrame* logScroll = nullptr;
    brls::Button* syncButton = nullptr;
    brls::Button* testButton = nullptr;

    bool isSyncing = false; // impede re-entrada durante sync

    std::vector<std::string> logLines;
    static const int MAX_LOG_LINES = 15;

    void buildUI();
    void onTestConnection();
    void onStartSync();
    void appendLog(const std::string& text);
    void rebuildLogView();
};
