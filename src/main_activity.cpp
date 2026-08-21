#include "main_activity.hpp"

#include <borealis.hpp>
#include <borealis/core/thread.hpp>
#include <cstdio>
#include <thread>

#include "platform.hpp"
#include "sync/config.hpp"
#include "sync/http_client.hpp"

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace {

/**
 * Cria uma linha com ícone + barra de progresso.
 * icon: codepoint Material Icons em UTF-8 (ex: "\xEE\x87\x80")
 */
brls::Box* createStorageRow(const char* icon, const platform::StorageInfo& info)
{
    auto* row = new brls::Box();
    row->setAxis(brls::Axis::COLUMN);
    row->setWidth(160);

    // Label com ícone Material + espaço
    char buf[64];
    snprintf(buf, sizeof(buf), "%.1f / %.1f GB", info.usedGB, info.totalGB);
    std::string text = std::string(icon) + " " + buf;

    auto* label = new brls::Label();
    label->setText(text);
    label->setFontSize(15);
    label->setSingleLine(true);
    label->setTextColor(nvgRGBA(180, 180, 180, 255));
    row->addView(label);

    // Barra de progresso (fundo)
    auto* barBg = new brls::Box();
    barBg->setWidthPercentage(100.0f);
    barBg->setHeight(5);
    barBg->setCornerRadius(2.5f);
    barBg->setBackgroundColor(nvgRGBA(60, 60, 60, 255));
    barBg->setMargins(5, 0, 0, 0);

    // Barra de progresso (preenchimento)
    auto* barFill = new brls::Box();
    float fillPct = static_cast<float>(info.usagePercent);
    barFill->setWidthPercentage(fillPct > 0.0f ? fillPct : 1.0f);
    barFill->setHeight(5);
    barFill->setCornerRadius(2.5f);

    // Cor: verde (<60%), amarelo (60-80%), vermelho (>80%)
    NVGcolor barColor;
    if (info.usagePercent < 60)
        barColor = nvgRGBA(76, 175, 80, 255);
    else if (info.usagePercent < 80)
        barColor = nvgRGBA(255, 193, 7, 255);
    else
        barColor = nvgRGBA(244, 67, 54, 255);

    barFill->setBackgroundColor(barColor);
    barBg->addView(barFill);
    row->addView(barBg);

    return row;
}

} // namespace

void MainActivity::onContentAvailable()
{
    brls::Logger::info("onContentAvailable: inicio");

#ifdef __SWITCH__
    brls::Logger::info("onContentAvailable: nsInitialize");
    nsInitialize();
#endif
    brls::Logger::info("onContentAvailable: getSdInfo");
    auto sdInfo = platform::getSdInfo();
    brls::Logger::info("onContentAvailable: getSystemInfo");
    auto sysInfo = platform::getSystemInfo();
#ifdef __SWITCH__
    brls::Logger::info("onContentAvailable: nsExit");
    nsExit();
#endif

    brls::Logger::info("onContentAvailable: criando container");

    // Container principal (horizontal, lado a lado)
    auto* container = new brls::Box();
    container->setAxis(brls::Axis::ROW);
    container->setAlignItems(brls::AlignItems::CENTER);
    container->setJustifyContent(brls::JustifyContent::FLEX_END);

    // WiFi status (ícone signal_wifi_4_bar U+E63E)
    auto* wifiRow = new brls::Box();
    wifiRow->setAxis(brls::Axis::ROW);
    wifiRow->setAlignItems(brls::AlignItems::CENTER);
    wifiRow->setMarginRight(20);

    wifiIcon = new brls::Label();
    wifiIcon->setText("\xEE\x98\xBE");
    wifiIcon->setFontSize(18);
    wifiIcon->setTextColor(nvgRGBA(150, 150, 150, 255));
    wifiRow->addView(wifiIcon);

    wifiStatus = new brls::Label();
    wifiStatus->setText(" ...");
    wifiStatus->setFontSize(14);
    wifiStatus->setTextColor(nvgRGBA(150, 150, 150, 255));
    wifiRow->addView(wifiStatus);

    container->addView(wifiRow);

    // microSD (ícone sd_card U+E623)
    auto* sdRow = createStorageRow("\xEE\x98\xA3", sdInfo);
    container->addView(sdRow);

    // System (ícone memory U+E322)
    auto* sysRow = createStorageRow("\xEE\x8C\xA2", sysInfo);
    sysRow->setMargins(0, 0, 0, 16);
    container->addView(sysRow);

    // Setar como hintView do TabFrame → aparece no header direito
    brls::Logger::info("onContentAvailable: setHintView");
    auto* contentView = this->getView("applet");
    if (contentView)
    {
        auto* applet = dynamic_cast<brls::AppletFrame*>(contentView);
        if (applet && applet->getContentView())
        {
            applet->getContentView()->getAppletFrameItem()->setHintView(container);
            applet->updateAppletFrameItem();
        }
    }

    // Conexão será testada manualmente pelo usuário na tab Sync
    brls::Logger::info("onContentAvailable: fim");
}

void MainActivity::checkConnection()
{
    // Iniciar spinner
    spinner = new SpinnerTask(wifiIcon);
    spinner->start();
    wifiIcon->setTextColor(nvgRGBA(150, 150, 150, 255));
    wifiStatus->setText(" ...");
    wifiStatus->setTextColor(nvgRGBA(150, 150, 150, 255));

    auto config = netsync::loadConfig(netsync::getConfigPath());
    std::string url = config.server.baseUrl() + "/health";

    std::thread([this, url]() {
        netsync::HttpResponse resp = netsync::httpGet(url);

        brls::sync([this, resp]() {
            // Parar e destruir spinner
            spinner->stop();
            delete spinner;
            spinner = nullptr;

            // Restaurar ícone wifi
            wifiIcon->setText("\xEE\x98\xBE");

            if (resp.ok())
            {
                wifiIcon->setTextColor(nvgRGBA(76, 175, 80, 255));
                wifiStatus->setText(" ON");
                wifiStatus->setTextColor(nvgRGBA(76, 175, 80, 255));
            }
            else
            {
                wifiIcon->setTextColor(nvgRGBA(244, 67, 54, 255));
                wifiStatus->setText(" OFF");
                wifiStatus->setTextColor(nvgRGBA(244, 67, 54, 255));
            }
        });
    }).detach();
}
