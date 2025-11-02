#include "Icons.hpp"
#include "../helpers/Memory.hpp"
#include "../core/InternalBackend.hpp"

#include <optional>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <hyprutils/string/String.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/utils/ScopeGuard.hpp>

extern "C" {
#include "iniparser.h"
}

using namespace Hyprtoolkit;
using namespace Hyprutils::String;
using namespace Hyprutils::Utils;

static std::optional<std::string> readFileAsString(const std::string& path) {
    std::error_code ec;

    if (!std::filesystem::exists(path, ec) || ec)
        return std::nullopt;

    std::ifstream file(path);
    if (!file.good())
        return std::nullopt;

    return trim(std::string((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>())));
}

static const std::array<const char*, 3> ICON_THEME_DIRS = {"/usr/share/icons", "/usr/local/share/icons", "~/.icons"};

static std::string                      fromDir(const char* p) {
    static auto HOME_ENV = getenv("HOME");

    if (p[0] == '~') {
        if (!HOME_ENV) {
            g_logger->log(HT_LOG_ERROR, "CSystemIconFactory: can't resolve user path without $HOME");
            return p;
        }

        return std::string{HOME_ENV} + std::string{p}.substr(1);
    }

    return p;
}

static std::optional<std::string> getThemeDir(const std::string& name) {
    for (const auto& rawDir : ICON_THEME_DIRS) {
        auto            path = fromDir(rawDir) + "/" + name;

        std::error_code ec;
        if (!std::filesystem::exists(path + "/index.theme", ec) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping theme dir {} (no index.theme)", path);
            continue;
        }

        return path;
    }

    return std::nullopt;
}

static std::optional<std::string> findAnyTheme() {
    // first, try to find the default system theme
    for (const auto& rawDir : ICON_THEME_DIRS) {
        auto            path = fromDir(rawDir) + "/default/index.theme";

        std::error_code ec;
        if (!std::filesystem::exists(path) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping file {} (no access)", path);
            continue;
        }

        if (!std::filesystem::is_regular_file(path, ec) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping file {} (not a file)", path);
            continue;
        }

        dictionary* ini = iniparser_load(path.c_str());
        CScopeGuard x([ini] { iniparser_freedict(ini); });

        if (!ini) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping file {} (iniparser failed)", path);
            continue;
        }

        auto iconTheme = iniparser_getstring(ini, "Icon Theme:Inherits", nullptr);

        if (iconTheme) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Theme {} has valid data", path);
            // try to find it
            auto themeDir = getThemeDir(iconTheme);
            if (themeDir && !themeDir->empty()) {
                g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Found {} as default fallback", themeDir.value());
                return themeDir;
            }

            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Skipping finding default theme, as {} inherits a non-existent theme", path);
            break;
        } else
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Skipping {}, doesn't inherit an icon theme", path);
    }

    for (const auto& rawDir : ICON_THEME_DIRS) {
        auto            path = fromDir(rawDir) + "/";

        std::error_code ec;
        if (!std::filesystem::exists(path) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping dir {} (no access)", path);
            continue;
        }

        if (!std::filesystem::is_directory(path, ec) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping dir {} (not a dir)", path);
            continue;
        }

        for (const auto& p : std::filesystem::directory_iterator(path)) {
            if (!std::filesystem::exists(p.path().string() + "/index.theme", ec) || ec) {
                g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping theme dir {} (no index.theme)", p.path().string());
                continue;
            }

            return path;
        }
    }

    return std::nullopt;
}

static std::optional<std::string> getIconThemeDir() {
    static std::optional<std::string> path;
    static bool                       once = true;

    if (!once)
        return path;

    once = false;

    // if we have an explicit one, try to find it.
    if (!g_palette->m_vars.iconTheme.empty())
        path = getThemeDir(g_palette->m_vars.iconTheme);

    if (path) {
        g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Using theme dir {}, explicit by user", *path);
        return path;
    }

    // we haven't found one, or it's unset. Use the first one we can.
    path = findAnyTheme();
    if (path)
        g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Using theme dir {} (default fallback)", *path);
    else
        g_logger->log(HT_LOG_ERROR, "CSystemIconFactory: No theme found, icons will be broken");

    return path;
}

CSystemIconFactory::CSystemIconFactory() : m_themeDir(getIconThemeDir()) {
    // Parse the index.theme if applicable and find Directories
    if (!m_themeDir)
        return;

    const auto THEME_DATA = readFileAsString(*m_themeDir + "/index.theme");

    if (!THEME_DATA)
        return;

    size_t directoriesEntry = THEME_DATA->find("\nDirectories=");
    if (directoriesEntry == std::string::npos)
        directoriesEntry = THEME_DATA->find("\nDirectories =");

    if (directoriesEntry == std::string::npos) {
        g_logger->log(HT_LOG_ERROR, "CSystemIconFactory: index.theme has no Directories?");
        return;
    }

    size_t directoriesLineEnd = THEME_DATA->find('\n', directoriesEntry + 2);
    directoriesEntry          = THEME_DATA->find('=', directoriesEntry + 2) + 1;

    std::string   directoriesList = THEME_DATA->substr(directoriesEntry, directoriesLineEnd - directoriesEntry);

    CConstVarList dirs(directoriesList, 0, ',', true);

    m_iconDirs.reserve(dirs.size());

    for (const auto& d : dirs) {
        m_iconDirs.emplace_back(trim(std::string{d}));
    }

    g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: index.theme parsed: {} dirs", dirs.size());
}

SP<ISystemIconDescription> CSystemIconFactory::lookupIcon(const std::string& iconName) {
    if (!m_themeDir)
        return makeShared<CSystemIconDescription>();

    return makeShared<CSystemIconDescription>(iconName);
}