#include "ConfigManager.hpp"

#include <hyprutils/path/Path.hpp>

#include "../core/InternalBackend.hpp"

#include <unistd.h>
#include <glob.h>
#include <filesystem>
#include <cstring>
#include <climits>

using namespace Hyprtoolkit;

static Hyprlang::CParseResult handleSource(const char* c, const char* v) {
    const std::string      VALUE   = v;
    const std::string      COMMAND = c;

    const auto             RESULT = g_config->handleSource(COMMAND, VALUE);

    Hyprlang::CParseResult result;
    if (RESULT.has_value())
        result.setError(RESULT.value().c_str());
    return result;
}

static std::string absolutePath(const std::string& rawpath, const std::string& currentDir) {
    std::filesystem::path path(rawpath);

    // Handling where rawpath starts with '~'
    if (!rawpath.empty() && rawpath[0] == '~') {
        static const char* const ENVHOME = getenv("HOME");
        path                             = std::filesystem::path(ENVHOME) / path.relative_path().string().substr(2);
    }

    // Handling e.g. ./, ../
    if (path.is_relative())
        return std::filesystem::weakly_canonical(std::filesystem::path(currentDir) / path);
    else
        return std::filesystem::weakly_canonical(path);
}

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
    m_config->addConfigValue("link_text", Hyprlang::INT{0xFF4EECF8});
    m_config->addConfigValue("accent", Hyprlang::INT{0xFF00FFCC});
    m_config->addConfigValue("accent_secondary", Hyprlang::INT{0xFF0099F0});
    m_config->addConfigValue("rounding_large", Hyprlang::INT{10});
    m_config->addConfigValue("rounding_small", Hyprlang::INT{5});

    m_config->addConfigValue("h1_size", Hyprlang::INT{19});
    m_config->addConfigValue("h2_size", Hyprlang::INT{15});
    m_config->addConfigValue("h3_size", Hyprlang::INT{13});
    m_config->addConfigValue("font_size", Hyprlang::INT{11});
    m_config->addConfigValue("small_font_size", Hyprlang::INT{10});
    m_config->addConfigValue("icon_theme", Hyprlang::STRING{""});
    m_config->addConfigValue("font_family", Hyprlang::STRING{"Sans Serif"});
    m_config->addConfigValue("font_family_monospace", Hyprlang::STRING{"monospace"});

    m_config->registerHandler(&::handleSource, "source", {.allowFlags = false});

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
    auto p = CPalette::emptyPalette();

    auto BACKGROUND      = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "background");
    auto BASE            = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "base");
    auto TEXT            = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "text");
    auto ALTERNATEBASE   = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "alternate_base");
    auto BRIGHTTEXT      = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "bright_text");
    auto LINK            = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "link_text");
    auto ACCENT          = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "accent");
    auto ACCENTSECONDARY = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "accent_secondary");
    auto ROUNDINGLARGE   = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "rounding_large");
    auto ROUNDINGSMALL   = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "rounding_small");

    auto H1SIZE         = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h1_size");
    auto H2SIZE         = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h2_size");
    auto H3SIZE         = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "h3_size");
    auto FONTSIZE       = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "font_size");
    auto SMALLFONTSIZE  = Hyprlang::CSimpleConfigValue<Hyprlang::INT>(m_config.get(), "small_font_size");
    auto ICONTHEME      = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(m_config.get(), "icon_theme");
    auto FONTFAMILY     = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(m_config.get(), "font_family");
    auto FONTFAMILYMONO = Hyprlang::CSimpleConfigValue<Hyprlang::STRING>(m_config.get(), "font_family_monospace");

    p->m_colors.background      = *BACKGROUND;
    p->m_colors.base            = *BASE;
    p->m_colors.text            = *TEXT;
    p->m_colors.alternateBase   = *ALTERNATEBASE;
    p->m_colors.brightText      = *BRIGHTTEXT;
    p->m_colors.linkText        = *LINK;
    p->m_colors.accent          = *ACCENT;
    p->m_colors.accentSecondary = *ACCENTSECONDARY;

    p->m_vars.bigRounding   = *ROUNDINGLARGE;
    p->m_vars.smallRounding = *ROUNDINGSMALL;

    p->m_vars.h1Size              = *H1SIZE;
    p->m_vars.h2Size              = *H2SIZE;
    p->m_vars.h3Size              = *H3SIZE;
    p->m_vars.fontSize            = *FONTSIZE;
    p->m_vars.smallFontSize       = *SMALLFONTSIZE;
    p->m_vars.iconTheme           = *ICONTHEME;
    p->m_vars.fontFamily          = *FONTFAMILY;
    p->m_vars.fontFamilyMonospace = *FONTFAMILYMONO;

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

std::optional<std::string> CConfigManager::handleSource(const std::string& command, const std::string& rawpath) {
    if (rawpath.length() < 2) {
        g_logger->log(HT_LOG_ERROR, "source= path garbage");
        return "source path " + rawpath + " bogus!";
    }
    std::unique_ptr<glob_t, void (*)(glob_t*)> glob_buf{new glob_t, [](glob_t* g) { globfree(g); }};
    memset(glob_buf.get(), 0, sizeof(glob_t));

    const auto CURRENTDIR = std::filesystem::path(m_configCurrentPath).parent_path().string();

    if (auto r = glob(absolutePath(rawpath, CURRENTDIR).c_str(), GLOB_TILDE, nullptr, glob_buf.get()); r != 0) {
        std::string err = std::format("source= globbing error: {}", r == GLOB_NOMATCH ? "found no match" : GLOB_ABORTED ? "read error" : "out of memory");
        g_logger->log(HT_LOG_ERROR, "{}", err);
        return err;
    }

    for (size_t i = 0; i < glob_buf->gl_pathc; i++) {
        const auto PATH = absolutePath(glob_buf->gl_pathv[i], CURRENTDIR);

        if (PATH.empty() || PATH == m_configCurrentPath) {
            g_logger->log(HT_LOG_WARNING, "source= skipping invalid path");
            continue;
        }

        if (!std::filesystem::is_regular_file(PATH)) {
            if (std::filesystem::exists(PATH)) {
                g_logger->log(HT_LOG_WARNING, "source= skipping non-file {}", PATH);
                continue;
            }

            g_logger->log(HT_LOG_ERROR, "source= file doesnt exist");
            return "source file " + PATH + " doesn't exist!";
        }

        // allow for nested config parsing
        auto backupConfigPath = m_configCurrentPath;
        m_configCurrentPath   = PATH;

        m_config->parseFile(PATH.c_str());

        m_configCurrentPath = backupConfigPath;
    }

    return {};
}
