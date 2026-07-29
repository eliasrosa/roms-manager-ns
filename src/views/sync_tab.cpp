/**
 * SyncTab - Implementação da tab de sincronização
 * Log com visual de terminal (fundo escuro, font mono, auto-scroll)
 */

#include "sync_tab.hpp"
#include <ctime>

namespace {

std::string timestamp()
{
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    return std::string(buf);
}

} // namespace

SyncTab::SyncTab()
{
    this->setAxis(brls::Axis::COLUMN);

    // Carregar config
    syncManager.loadConfig();

    this->buildUI();
    this->appendLog("ROMs Manager NS v0.2.0 - Sync");
    this->appendLog("Servidor: " + syncManager.getConfig().server.baseUrl());
    this->appendLog("Aguardando comando...");
}

void SyncTab::buildUI()
{
    // === Seção superior: info + botões ===

    // Info do servidor
    auto& cfg = syncManager.getConfig();
    serverLabel = new brls::Label();
    serverLabel->setText("Servidor: " + cfg.server.baseUrl());
    serverLabel->setFontSize(18);
    serverLabel->setMargins(12, 24, 4, 24);
    this->addView(serverLabel);

    // Status
    statusLabel = new brls::Label();
    statusLabel->setText("Status: Aguardando...");
    statusLabel->setFontSize(18);
    statusLabel->setMargins(4, 24, 8, 24);
    this->addView(statusLabel);

    // Progresso
    progressLabel = new brls::Label();
    progressLabel->setText("");
    progressLabel->setFontSize(16);
    progressLabel->setMargins(2, 24, 8, 24);
    this->addView(progressLabel);

    // Botões lado a lado
    brls::Box* buttonRow = new brls::Box();
    buttonRow->setAxis(brls::Axis::ROW);
    buttonRow->setMargins(4, 24, 8, 24);

    testButton = new brls::Button();
    testButton->setText("Testar Conexao");
    testButton->setMarginRight(12);
    testButton->registerAction("Testar", brls::BUTTON_A, [this](brls::View* view) {
        this->onTestConnection();
        return true;
    });
    buttonRow->addView(testButton);

    syncButton = new brls::Button();
    syncButton->setText("Iniciar Sync");
    syncButton->registerAction("Sync", brls::BUTTON_A, [this](brls::View* view) {
        this->onStartSync();
        return true;
    });
    buttonRow->addView(syncButton);

    this->addView(buttonRow);

    // === Seção inferior: terminal de log (tamanho fixo, estilo tail) ===

    // Container do terminal (fundo escuro, altura fixa)
    logScroll = new brls::ScrollingFrame();
    logScroll->setMargins(8, 24, 12, 24);
    logScroll->setHeight(300);

    // Box interna do log
    logContainer = new brls::Box();
    logContainer->setAxis(brls::Axis::COLUMN);
    logContainer->setPadding(12, 16, 12, 16);
    logContainer->setBackgroundColor(nvgRGBA(15, 15, 20, 250));
    logContainer->setCornerRadius(6.0f);

    logScroll->setContentView(logContainer);
    this->addView(logScroll);
}

void SyncTab::onTestConnection()
{
    statusLabel->setText("Status: Testando...");
    appendLog("$ test connection " + syncManager.getConfig().server.baseUrl());

    std::string error;
    bool ok = syncManager.testConnection(error);

    if (ok)
    {
        statusLabel->setText("Status: Conectado!");
        appendLog("  -> OK (servidor acessivel)");
    }
    else
    {
        statusLabel->setText("Status: FALHA");
        appendLog("  -> ERRO: " + error);
    }
}

void SyncTab::onStartSync()
{
    statusLabel->setText("Status: Sincronizando...");
    progressLabel->setText("Iniciando...");
    appendLog("$ sync start");

    netsync::SyncCallbacks callbacks;

    callbacks.onStatus = [this](const std::string& status) {
        statusLabel->setText("Status: " + status);
    };

    callbacks.onFileProgress = [this](const std::string& filename, size_t downloaded, size_t total) {
        std::string progress;
        if (total > 0)
        {
            int pct = (int)((downloaded * 100) / total);
            progress = filename + " [" + std::to_string(pct) + "%]";
        }
        else
        {
            progress = filename + " [" + std::to_string(downloaded / 1024) + " KB]";
        }
        progressLabel->setText(progress);
    };

    callbacks.onFileComplete = [this](const std::string& filename, bool success) {
        if (success)
            appendLog("  + " + filename + " ... OK");
        else
            appendLog("  ! " + filename + " ... FALHA");
    };

    callbacks.onComplete = [this](const netsync::SyncResult& result) {
        std::string summary = "  -> " +
            std::to_string(result.files_downloaded) + " baixados, " +
            std::to_string(result.files_skipped) + " ignorados";
        if (result.files_failed > 0)
            summary += ", " + std::to_string(result.files_failed) + " falharam";

        appendLog(summary);
        appendLog("$ sync complete");
        progressLabel->setText(summary);
        statusLabel->setText("Status: Concluido");
    };

    syncManager.runSync(callbacks);
}

void SyncTab::appendLog(const std::string& text)
{
    std::string line = "[" + timestamp() + "] " + text;
    logLines.push_back(line);

    // Limitar linhas
    while ((int)logLines.size() > MAX_LOG_LINES)
        logLines.erase(logLines.begin());

    rebuildLogView();
}

void SyncTab::rebuildLogView()
{
    // Limpar container
    auto children = logContainer->getChildren();
    while (!children.empty())
    {
        logContainer->removeView(children[children.size() - 1]);
        children = logContainer->getChildren();
    }

    // Adicionar cada linha como Label
    for (auto& line : logLines)
    {
        brls::Label* label = new brls::Label();
        label->setText(line);
        label->setFontSize(14);
        label->setTextColor(nvgRGBA(0, 255, 128, 255)); // verde terminal
        label->setMargins(1, 0, 1, 0);
        logContainer->addView(label);
    }

    // O ScrollingFrame vai naturalmente mostrar o conteúdo novo
    // já que o logContainer cresce conforme labels são adicionados
}

brls::View* SyncTab::create()
{
    return new SyncTab();
}
