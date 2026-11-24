#include "Icons.hpp"
#include "../core/InternalBackend.hpp"

#include <hyprgraphics/resource/resources/ImageResource.hpp>
#include <algorithm>

using namespace Hyprtoolkit;

static std::optional<SIconCandidate> readCandidate(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec) || ec)
        return std::nullopt;

    // Validate artwork once, and use its actual pixel dimensions rather than
    // assuming that a directory name describes the image it contains.
    Hyprgraphics::CImageResource resource(path.string(), {32, 32});
    resource.render();
    if (!resource.m_asset.cairoSurface)
        return std::nullopt;

    return SIconCandidate{path, sc<int>(std::max(resource.m_asset.pixelSize.x, resource.m_asset.pixelSize.y)), path.extension() == ".svg"};
}

std::vector<SIconCandidate> Hyprtoolkit::findIconCandidates(const std::vector<SIconDirectory>& directories, const std::string& name) {
    if (name.empty())
        return {};
    if (std::filesystem::path(name).is_absolute()) {
        const auto CANDIDATE = readCandidate(name);
        return CANDIDATE ? std::vector<SIconCandidate>{*CANDIDATE} : std::vector<SIconCandidate>{};
    }
    // Icon names must not escape the selected theme directory.
    if (name.find('/') != std::string::npos)
        return {};

    std::vector<SIconCandidate> candidates;
    std::string                 selectedTheme;
    for (const auto& directory : directories) {
        if (!candidates.empty() && directory.theme != selectedTheme)
            continue;
        for (const auto* extension : {".svg", ".png"}) {
            const auto CANDIDATE = readCandidate(directory.path / (name + extension));
            if (!CANDIDATE)
                continue;
            selectedTheme = directory.theme;
            candidates.emplace_back(*CANDIDATE);
        }
    }
    // Inherited themes must not replace an icon supplied by the selected theme.
    return candidates;
}

std::filesystem::path Hyprtoolkit::selectIconPath(const std::vector<SIconCandidate>& candidates, int pixelSize) {
    pixelSize = std::max(1, pixelSize);
    auto best = candidates.end();
    for (auto candidate = candidates.begin(); candidate != candidates.end(); ++candidate) {
        if (candidate->scalable)
            return candidate->path;
        if (best == candidates.end() || (candidate->pixelSize >= pixelSize && (best->pixelSize < pixelSize || candidate->pixelSize < best->pixelSize)) ||
            (candidate->pixelSize < pixelSize && best->pixelSize < pixelSize && candidate->pixelSize > best->pixelSize))
            best = candidate;
    }
    return best != candidates.end() ? best->path : std::filesystem::path{};
}

CSystemIconDescription::CSystemIconDescription() {
    ;
}

CSystemIconDescription::CSystemIconDescription(const std::string& name) {
    if (name.empty())
        return;
    if (const auto CK = g_iconFactory->getCached(name); CK) {
        m_bestPath   = CK->badIcon ? "" : CK->path;
        m_scalable   = CK->scalable;
        m_candidates = CK->candidates;
        return;
    }

    m_candidates = findIconCandidates(g_iconFactory->m_lookupPaths, name);
    if (m_candidates.empty() && !std::filesystem::path(name).is_absolute())
        m_candidates = findIconCandidates({{"/usr/share/pixmaps", "pixmaps"}}, name);
    m_bestPath = selectIconPath(m_candidates, 32).string();
    m_scalable = std::ranges::any_of(m_candidates, [](const auto& candidate) { return candidate.scalable; });
    g_iconFactory->cacheEntry(name, {.badIcon = m_bestPath.empty(), .path = m_bestPath, .scalable = m_scalable, .candidates = m_candidates});
}

std::string CSystemIconDescription::pathForSize(int pixelSize) const {
    return selectIconPath(m_candidates, pixelSize).string();
}

bool CSystemIconDescription::exists() {
    return !m_bestPath.empty();
}

bool CSystemIconDescription::scalable() {
    return m_scalable;
}
