/**
 * SyncTab - Implementação da tab de sincronização
 * Log com visual de terminal (fundo escuro, font mono, auto-scroll)
 */

#include "sync_tab.hpp"
#include <borealis/core/thread.hpp>
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
    : alive(std::make_shared<std::atomic<bool>>(true))
{
    this->setAxis(brls::Axis::COLUMN);

    // Carregar config
    syncManager.loadConfig();

    this->buildUI();
    this->appendLog("ROMs Manager NS v0.2.0 - Sync");
    this->appendLog("Servidor: " + syncManager.getConfig().server.baseUrl());
}

SyncTab::~SyncTab()
{
    // Sinaliza para todas as threads/lambdas pendentes que a view foi destruída
    *alive = false;
}

void SyncTab::buildUI()
{
    // === Seção superior: info + botões ===

    // Status de conexão (ícone wifi + texto)
    auto* connectionRow = new brls::Box();
    connectionRow->setAxis(brls::Axis::ROW);
    connectionRow->setAlignItems(brls::AlignItems::CENTER);
    connectionRow->setMargins(12, 24, 4, 24);

    // Ícone WiFi (U+E63E = signal_wifi_4_bar)
    connectionIcon = new brls::Label();
    connectionIcon->setText("\xEE\x98\xBE");
    connectionIcon->setFontSize(24);
    connectionIcon->setTextColor(nvgRGBA(150, 150, 150, 255));
    connectionIcon->setMarginRight(8);
    connectionRow->addView(connectionIcon);

    // Status da conexão
    statusLabel = new brls::Label();
    statusLabel->setText("Nao testado");
    statusLabel->setFontSize(18);
    statusLabel->setTextColor(nvgRGBA(150, 150, 150, 255));
    connectionRow->addView(statusLabel);

    this->addView(connectionRow);

    // Info do servidor
    auto& cfg = syncManager.getConfig();
    serverLabel = new brls::Label();
    serverLabel->setText("Servidor: " + cfg.server.baseUrl());
    serverLabel->setFontSize(16);
    serverLabel->setMargins(4, 24, 8, 24);
    serverLabel->setTextColor(nvgRGBA(140, 140, 140, 255));
    this->addView(serverLabel);

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
    if (isSyncing) return;
    isSyncing = true;

    statusLabel->setText("Testando...");
    statusLabel->setTextColor(nvgRGBA(200, 200, 100, 255));
    connectionIcon->setTextColor(nvgRGBA(200, 200, 100, 255));
    appendLog("$ test connection " + syncManager.getConfig().server.baseUrl());

    // Rodar em thread separada para não bloquear a UI
    auto guard = alive;
    std::thread([this, guard]() {
        std::string error;
        bool ok = syncManager.testConnection(error);

        // Devolver resultado para a UI thread
        brls::sync([this, guard, ok, error]() {
            if (!*guard) return; // view já foi destruída

            if (ok)
            {
                statusLabel->setText("Conectado");
                statusLabel->setTextColor(nvgRGBA(76, 175, 80, 255));
                connectionIcon->setTextColor(nvgRGBA(76, 175, 80, 255));
                appendLog("  -> OK (servidor acessivel)");
            }
            else
            {
                statusLabel->setText("Sem conexao");
                statusLabel->setTextColor(nvgRGBA(244, 67, 54, 255));
                connectionIcon->setTextColor(nvgRGBA(244, 67, 54, 255));
                appendLog("  -> ERRO: " + error);
            }

            isSyncing = false;
        });
    }).detach();
}

void SyncTab::onStartSync()
{
    if (isSyncing) return;
    isSyncing = true;

    statusLabel->setText("Status: Sincronizando...");
    progressLabel->setText("Iniciando...");
    appendLog("$ sync start");

    netsync::SyncCallbacks callbacks;

    auto guard = alive;

    callbacks.onStatus = [this, guard](const std::string& status) {
        brls::sync([this, guard, status]() {
            if (!*guard) return;
            statusLabel->setText("Status: " + status);
        });
    };

    callbacks.onFileProgress = [this, guard](const std::string& filename, size_t downloaded, size_t total) {
        brls::sync([this, guard, filename, downloaded, total]() {
            if (!*guard) return;
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
        });
    };

    callbacks.onFileComplete = [this, guard](const std::string& filename, bool success) {
        brls::sync([this, guard, filename, success]() {
            if (!*guard) return;
            if (success)
                appendLog("  + " + filename + " ... OK");
            else
                appendLog("  ! " + filename + " ... FALHA");
        });
    };

    callbacks.onComplete = [this, guard](const netsync::SyncResult& result) {
        brls::sync([this, guard, result]() {
            if (!*guard) return;
            std::string summary = "  -> " +
                std::to_string(result.files_downloaded) + " baixados, " +
                std::to_string(result.files_skipped) + " ignorados";
            if (result.files_failed > 0)
                summary += ", " + std::to_string(result.files_failed) + " falharam";

            appendLog(summary);
            appendLog("$ sync complete");
            progressLabel->setText(summary);
            statusLabel->setText("Status: Concluido");
            isSyncing = false;
        });
    };

    // Rodar sync em thread separada para não bloquear a UI
    std::thread([this, guard, callbacks]() {
        syncManager.runSync(callbacks);
    }).detach();
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
