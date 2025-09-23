#include "ConfigManager.hpp"

#include <hyprutils/path/Path.hpp>

#include "../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

CConfigManager::CConfigManager() {
    // Initialize the configuration
    // Read file from default location
    // or from an explicit location given by user

    const auto CFGPATH = Hyprutils::Path::findConfig("hyprtoolkit").first.value_or("");

    m_config = makeUnique<Hyprlang::CConfig>(CFGPATH.c_str(), Hyprlang::SConfigOptions{.allowMissingConfig = true});

    m_config->addConfigValue("background", Hyprlang::INT{0xFF181818});
    m_config->addConfigValue("base", Hyprlang::INT{0xFF202020});
    m_config->addConfigValue("text", Hyprlang::INT{0xFFDADADA});
    m_config->addConfigValue("alternate_base", Hyprlang::INT{0xFF272727});
    m_config->addConfigValue("bright_text", Hyprlang::INT{0xFFFFDEDE});
    m_config->addConfigValue("accent", Hyprlang::INT{0xFF00FFCC});
    m_config->addConfigValue("accent_secondary", Hyprlang::INT{0xFF0099F0});

    m_config->addConfigValue("h1Size", Hyprlang::INT{19});
    m_config->addConfigValue("h2Size", Hyprlang::INT{15});
    m_config->addConfigValue("h3Size", Hyprlang::INT{13});
    m_config->addConfigValue("fontSize", Hyprlang::INT{11});
    m_config->addConfigValue("smallFontSize", Hyprlang::INT{10});

    m_config->commence();
}

void CConfigManager::parse() {
    const auto ERROR = m_config->parse();

    if (ERROR.error)
        g_logger->log(HT_LOG_ERROR, "Error in config: {}", ERROR.getError());
}

SP<CPalette> CConfigManager::getPalette() {
    auto        p = CPalette::emptyPalette();

    static auto BACKGROUND      = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "background");
    static auto BASE            = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "base");
    static auto TEXT            = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "text");
    static auto ALTERNATEBASE   = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "alternate_base");
    static auto BRIGHTTEXT      = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "bright_text");
    static auto ACCENT          = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "accent");
    static auto ACCENTSECONDARY = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "accent_secondary");

    static auto H1SIZE        = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h1Size");
    static auto H2SIZE        = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h2Size");
    static auto H3SIZE        = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h3Size");
    static auto FONTSIZE      = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "fontSize");
    static auto SMALLFONTSIZE = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "smallFontSize");

    p->m_colors.background      = *BACKGROUND;
    p->m_colors.base            = *BASE;
    p->m_colors.text            = *TEXT;
    p->m_colors.alternateBase   = *ALTERNATEBASE;
    p->m_colors.brightText      = *BRIGHTTEXT;
    p->m_colors.accent          = *ACCENT;
    p->m_colors.accentSecondary = *ACCENTSECONDARY;

    p->m_vars.h1Size        = *H1SIZE;
    p->m_vars.h2Size        = *H2SIZE;
    p->m_vars.h3Size        = *H3SIZE;
    p->m_vars.fontSize      = *FONTSIZE;
    p->m_vars.smallFontSize = *SMALLFONTSIZE;

    return p;
}
