#include "Icons.hpp"
#include "../helpers/Memory.hpp"
#include "../core/InternalBackend.hpp"

#include <optional>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include <hyprutils/string/String.hpp>
#include <hyprutils/string/ConstVarList.hpp>
#include <hyprutils/string/VarList2.hpp>
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

static const std::array<const char*, 4> ICON_THEME_DIRS = {"/usr/share/icons", "/usr/local/share/icons", "~/.icons", "~/.local/share/icons"};

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

static std::optional<std::vector<std::string>> getThemeDir(const std::string& name) {
    std::vector<std::string> results;

    for (const auto& rawDir : ICON_THEME_DIRS) {
        auto            path = fromDir(rawDir) + "/" + name;

        std::error_code ec;
        if (!std::filesystem::exists(path + "/index.theme", ec) || ec) {
            g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping theme dir {} (no index.theme)", path);
            continue;
        }

        results.emplace_back(path);
    }

    return results.empty() ? std::nullopt : std::optional<std::vector<std::string>>(results);
}

static std::optional<std::vector<std::string>> findAnyTheme() {
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

        std::vector<std::string> themeDirs;

        for (const auto& p : std::filesystem::directory_iterator(path)) {
            if (!std::filesystem::exists(p.path().string() + "/index.theme", ec) || ec) {
                g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: skipping theme dir {} (no index.theme)", p.path().string());
                continue;
            }

            themeDirs.emplace_back(p.path().string());
        }

        if (!themeDirs.empty())
            return themeDirs;
    }

    return std::nullopt;
}

static std::optional<std::vector<std::string>> getIconThemeDirs() {
    static std::optional<std::vector<std::string>> paths;
    static bool                                    once = true;

    if (!once)
        return paths;

    once = false;

    // if we have an explicit one, try to find it.
    if (!g_palette->m_vars.iconTheme.empty())
        paths = getThemeDir(g_palette->m_vars.iconTheme);

    if (paths) {
        g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: Using theme dirs set explicitly by the user");
        return paths;
    }

    // we haven't found one, or it's unset. Use the first one we can.
    paths = findAnyTheme();

    g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: getIconThemeDirs returning {} base dirs", paths ? paths->size() : 0);

    return paths;
}

void CSystemIconFactory::parseTheme(const std::string& themeDir) {
    const auto THEME_DATA = readFileAsString(themeDir + "/index.theme");

    if (!THEME_DATA)
        return;

    size_t directoriesEntry = THEME_DATA->find("\nDirectories=");
    if (directoriesEntry == std::string::npos)
        directoriesEntry = THEME_DATA->find("\nDirectories =");

    if (directoriesEntry != std::string::npos) {
        const auto BASE_PATH = std::filesystem::path{themeDir};

        size_t     directoriesLineEnd = THEME_DATA->find('\n', directoriesEntry + 2);
        directoriesEntry              = THEME_DATA->find('=', directoriesEntry + 2) + 1;

        std::string   directoriesList = THEME_DATA->substr(directoriesEntry, directoriesLineEnd - directoriesEntry);

        CConstVarList dirs(directoriesList, 0, ',', true);

        for (const auto& d : dirs) {
            m_lookupPaths.emplace_back(BASE_PATH / d);
        }

        g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: theme {} has {} dirs", themeDir, dirs.size());
    }

    size_t inheritsEntry = THEME_DATA->find("\nInherits=");
    if (inheritsEntry == std::string::npos)
        inheritsEntry = THEME_DATA->find("\nInherits =");

    if (inheritsEntry != std::string::npos) {
        size_t inheritsLineEnd = THEME_DATA->find('\n', inheritsEntry + 2);
        inheritsEntry          = THEME_DATA->find('=', inheritsEntry + 2) + 1;

        std::string inheritsList = THEME_DATA->substr(inheritsEntry, inheritsLineEnd - inheritsEntry);

        CVarList2   inherits(std::move(inheritsList), 0, ',', true);

        g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: theme {} inherits {} themes", themeDir, inherits.size());

        for (const auto& inheritName : inherits) {
            if (inheritName == "hicolor")
                m_hicolorAdded = true;

            auto inheritTheme = getThemeDir(trim(std::string{inheritName}));
            if (inheritTheme) {
                g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: parsing inherited theme {}", *inheritTheme);
                parseThemes(*inheritTheme);
            } else
                g_logger->log(HT_LOG_WARNING, "CSystemIconFactory: inherited theme {} not found", std::string{inheritName});
        }
    }
}

void CSystemIconFactory::parseThemes(const std::vector<std::string>& themeDirs) {
    for (const auto& td : themeDirs) {
        parseTheme(td);
    }
}

CSystemIconFactory::CSystemIconFactory() {
    auto baseThemeDir = getIconThemeDirs();

    if (!baseThemeDir)
        return;

    parseThemes(baseThemeDir.value());

    if (!m_hicolorAdded) {
        const auto DIRS = getThemeDir("hicolor");
        if (DIRS)
            parseThemes(*DIRS);
    }

    g_logger->log(HT_LOG_TRACE, "CSystemIconFactory: initialized with {} lookup paths", m_lookupPaths.size());
}

SP<ISystemIconDescription> CSystemIconFactory::lookupIcon(const std::string& iconName) {
    if (m_lookupPaths.empty())
        return makeShared<CSystemIconDescription>();

    return makeShared<CSystemIconDescription>(iconName);
}

void CSystemIconFactory::cacheEntry(const std::string& iconName, SIconCacheResult&& result) {
    m_pathCache.set(iconName, std::move(result));
}

std::optional<CSystemIconFactory::SIconCacheResult> CSystemIconFactory::getCached(const std::string& name) {
    if (const auto* HIT = m_pathCache.get(name))
        return *HIT;
    return std::nullopt;
}
