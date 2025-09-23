#include "ConfigManager.hpp"

#include <hyprutils/path/Path.hpp>

#include "../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

CConfigManager::CConfigManager() : m_inotifyFd(inotify_init()) {
    // Initialize the configuration
    // Read file from default location
    // or from an explicit location given by user

    const auto CFGPATH = Hyprutils::Path::findConfig("hyprtoolkit").first.value_or("");
    m_configPath       = CFGPATH;

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

    replantWatch();
}

void CConfigManager::replantWatch() {
    for (const auto& w : m_watches) {
        inotify_rm_watch(m_inotifyFd.get(), w);
    }

    m_watches.clear();

    m_watches.emplace_back(inotify_add_watch(m_inotifyFd.get(), m_configPath.c_str(), IN_MODIFY | IN_DONT_FOLLOW));
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

void CConfigManager::onInotifyEvent() {
    constexpr size_t                                     BUFFER_SIZE = sizeof(inotify_event) + NAME_MAX + 1;
    alignas(inotify_event) std::array<char, BUFFER_SIZE> buffer      = {};
    const ssize_t                                        bytesRead   = read(m_inotifyFd.get(), buffer.data(), buffer.size());
    if (bytesRead <= 0)
        return;

    for (size_t offset = 0; offset < sc<size_t>(bytesRead);) {
        const auto* ev = rc<const inotify_event*>(buffer.data() + offset);

        if (offset + sizeof(inotify_event) > sc<size_t>(bytesRead)) {
            // err
            break;
        }

        if (offset + sizeof(inotify_event) + ev->len > sc<size_t>(bytesRead)) {
            // err
            break;
        }

        offset += sizeof(inotify_event) + ev->len;
    }

    replantWatch();

    parse();
}
