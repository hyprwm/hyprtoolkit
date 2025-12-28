#include "Icons.hpp"
#include "../core/InternalBackend.hpp"

#include <filesystem>

using namespace Hyprtoolkit;

CSystemIconDescription::CSystemIconDescription() {
    ;
}

CSystemIconDescription::CSystemIconDescription(const std::string& name) {
    if (g_iconFactory->m_iconDirs.empty())
        return;

    const auto THEME_DIR = g_iconFactory->m_themeDir.value();

    bool       found = false;

    for (const auto& sd : g_iconFactory->m_iconDirs) {
        auto fullDirPath = THEME_DIR + "/";
        fullDirPath += sd;

        std::error_code ec;
        if (!std::filesystem::exists(fullDirPath, ec) || ec)
            continue;

        auto iconPath = fullDirPath + "/";
        iconPath += name + ".svg"; // FUCK them raster themes in the year of our Lord 2025

        if (!std::filesystem::exists(iconPath, ec) || ec)
            continue;

        m_bestPath = iconPath;
        found      = true;
        break;
    }

    if (!found) {
        // try /usr/share/pixmaps
        std::error_code ec;
        if (std::filesystem::exists("/usr/share/pixmaps/" + name + ".svg", ec) || ec) {
            found      = true;
            m_bestPath = "/usr/share/pixmaps/" + name + ".svg";
        }
    }
}

bool CSystemIconDescription::exists() {
    return !m_bestPath.empty();
}

bool CSystemIconDescription::scalable() {
    return m_scalable;
}