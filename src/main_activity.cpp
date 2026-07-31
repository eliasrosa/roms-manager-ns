#include "main_activity.hpp"

#include <borealis.hpp>
#include <cstdio>

#include "platform.hpp"

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
    auto sdInfo = platform::getSdInfo();
    auto sysInfo = platform::getSystemInfo();

    // Container principal (horizontal, lado a lado)
    auto* container = new brls::Box();
    container->setAxis(brls::Axis::ROW);
    container->setAlignItems(brls::AlignItems::CENTER);
    container->setJustifyContent(brls::JustifyContent::FLEX_END);

    // microSD (ícone sd_card U+E623)
    auto* sdRow = createStorageRow("\xEE\x98\xA3", sdInfo);
    container->addView(sdRow);

    // System (ícone memory U+E322)
    auto* sysRow = createStorageRow("\xEE\x8C\xA2", sysInfo);
    sysRow->setMargins(0, 0, 0, 16);
    container->addView(sysRow);

    // Seta como hintView do TabFrame → aparece no header direito
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
}
