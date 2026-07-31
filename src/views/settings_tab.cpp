/**
 * SettingsTab - Implementação da tab de configurações
 *
 * Permite editar host e porta do servidor via teclado virtual (IME).
 * Salva automaticamente no config.json ao confirmar.
 */

#include "settings_tab.hpp"

SettingsTab::SettingsTab()
{
    this->setAxis(brls::Axis::COLUMN);

    // Carregar config atual
    configPath = netsync::getConfigPath();
    config = netsync::loadConfig(configPath);

    this->buildUI();
}

void SettingsTab::buildUI()
{
    // === Header: Servidor ===
    auto* header = new brls::Header();
    header->setTitle("Servidor");
    header->setSubtitle("Configurações de conexão com o servidor de sync");
    this->addView(header);

    // --- Host ---
    auto* hostRow = new brls::Box();
    hostRow->setAxis(brls::Axis::ROW);
    hostRow->setAlignItems(brls::AlignItems::CENTER);
    hostRow->setMargins(12, 24, 8, 24);
    hostRow->setFocusable(true);

    auto* hostLabel = new brls::Label();
    hostLabel->setText("Host:");
    hostLabel->setFontSize(20);
    hostLabel->setWidth(100);
    hostRow->addView(hostLabel);

    hostValueLabel = new brls::Label();
    hostValueLabel->setText(config.server.host);
    hostValueLabel->setFontSize(20);
    hostValueLabel->setGrow(1.0f);
    hostValueLabel->setTextColor(nvgRGBA(100, 200, 255, 255));
    hostRow->addView(hostValueLabel);

    auto* hostHint = new brls::Label();
    hostHint->setText("[A] Editar");
    hostHint->setFontSize(14);
    hostHint->setTextColor(nvgRGBA(150, 150, 150, 255));
    hostRow->addView(hostHint);

    hostRow->registerAction("Editar", brls::BUTTON_A, [this](brls::View* view) {
        this->onEditHost();
        return true;
    });

    this->addView(hostRow);

    // --- Porta ---
    auto* portRow = new brls::Box();
    portRow->setAxis(brls::Axis::ROW);
    portRow->setAlignItems(brls::AlignItems::CENTER);
    portRow->setMargins(8, 24, 8, 24);
    portRow->setFocusable(true);

    auto* portLabel = new brls::Label();
    portLabel->setText("Porta:");
    portLabel->setFontSize(20);
    portLabel->setWidth(100);
    portRow->addView(portLabel);

    portValueLabel = new brls::Label();
    portValueLabel->setText(std::to_string(config.server.port));
    portValueLabel->setFontSize(20);
    portValueLabel->setGrow(1.0f);
    portValueLabel->setTextColor(nvgRGBA(100, 200, 255, 255));
    portRow->addView(portValueLabel);

    auto* portHint = new brls::Label();
    portHint->setText("[A] Editar");
    portHint->setFontSize(14);
    portHint->setTextColor(nvgRGBA(150, 150, 150, 255));
    portRow->addView(portHint);

    portRow->registerAction("Editar", brls::BUTTON_A, [this](brls::View* view) {
        this->onEditPort();
        return true;
    });

    this->addView(portRow);

    // --- URL atual (preview) ---
    auto* urlHeader = new brls::Header();
    urlHeader->setTitle("URL atual");
    this->addView(urlHeader);

    statusLabel = new brls::Label();
    statusLabel->setText(config.server.baseUrl());
    statusLabel->setFontSize(18);
    statusLabel->setMargins(8, 24, 8, 24);
    statusLabel->setTextColor(nvgRGBA(180, 180, 180, 255));
    this->addView(statusLabel);
}

void SettingsTab::onEditHost()
{
    auto* ime = brls::Application::getImeManager();
    if (!ime) return;

    ime->openForText(
        [this](std::string result) {
            if (!result.empty())
            {
                config.server.host = result;
                this->saveAndUpdateUI();
            }
        },
        "Host do servidor",
        "IP ou hostname (ex: 192.168.0.3)",
        64,
        config.server.host
    );
}

void SettingsTab::onEditPort()
{
    auto* ime = brls::Application::getImeManager();
    if (!ime) return;

    ime->openForNumber(
        [this](long result) {
            if (result > 0 && result <= 65535)
            {
                config.server.port = static_cast<int>(result);
                this->saveAndUpdateUI();
            }
        },
        "Porta do servidor",
        "1-65535 (padrao: 8080)",
        5,
        std::to_string(config.server.port)
    );
}

void SettingsTab::saveAndUpdateUI()
{
    // Salvar no arquivo
    netsync::saveConfig(config, configPath);

    // Atualizar labels
    hostValueLabel->setText(config.server.host);
    portValueLabel->setText(std::to_string(config.server.port));
    statusLabel->setText(config.server.baseUrl());

    brls::Logger::info("Config salva: {}", config.server.baseUrl());
}

brls::View* SettingsTab::create()
{
    return new SettingsTab();
}
