#include "Icons.hpp"
#include "../core/InternalBackend.hpp"

#include <filesystem>

using namespace Hyprtoolkit;

CSystemIconDescription::CSystemIconDescription() {
    ;
}

CSystemIconDescription::CSystemIconDescription(const std::string& name) {
    if (g_iconFactory->m_lookupPaths.empty() || name.empty())
        return;

    if (const auto CK = g_iconFactory->getCached(name); CK) {
        m_bestPath = CK->badIcon ? "" : CK->path;
        return;
    }

    bool found = false;

    for (const auto& lookupDir : g_iconFactory->m_lookupPaths) {
        std::filesystem::path fullDirPath = lookupDir + "/";

        auto                  iconPath = fullDirPath / (name + ".svg");
        std::error_code       ec;

        if (!std::filesystem::exists(iconPath, ec) || ec) {
            iconPath = fullDirPath / (name + ".png");
            continue;
        }

        if (!std::filesystem::exists(iconPath, ec) || ec)
            continue;

        m_bestPath = iconPath;
        found      = true;
        break;
    }

    if (!found) {
        // try /usr/share/pixmaps
        std::error_code ec;
        if (std::filesystem::exists("/usr/share/pixmaps/" + name + ".svg", ec) && !ec) {
            found      = true;
            m_bestPath = "/usr/share/pixmaps/" + name + ".svg";
        }

        if (!found && std::filesystem::exists("/usr/share/pixmaps/" + name + ".png", ec) && !ec) {
            found      = true;
            m_bestPath = "/usr/share/pixmaps/" + name + ".png";
        }
    }

    if (found)
        g_iconFactory->cacheEntry(name, CSystemIconFactory::SIconCacheResult{.badIcon = false, .path = m_bestPath});
    else
        g_iconFactory->cacheEntry(name, CSystemIconFactory::SIconCacheResult{});
}

bool CSystemIconDescription::exists() {
    return !m_bestPath.empty();
}

bool CSystemIconDescription::scalable() {
    return m_scalable;
}