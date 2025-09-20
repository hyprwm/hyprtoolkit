#include <hyprtoolkit/palette/Palette.hpp>

#include "../helpers/Memory.hpp"

using namespace Hyprtoolkit;

CToolkitColor::CToolkitColor(const Hyprgraphics::CColor& color, float a) : m_color(color), m_a(a) {
    ;
}

SP<CPalette> CPalette::palette() {
    auto CONFIG = configPalette();
    if (!CONFIG)
        return defaultPalette();
    return CONFIG;
}

SP<CPalette> CPalette::defaultPalette() {
    auto palette = SP<CPalette>(new CPalette());

    palette->m_colors.background      = {Hyprgraphics::CColor::SSRGB{.r = 24 / 255.F, .g = 24 / 255.F, .b = 24 / 255.F}, 1.F};
    palette->m_colors.base            = {Hyprgraphics::CColor::SSRGB{.r = 32 / 255.F, .g = 32 / 255.F, .b = 32 / 255.F}, 1.F};
    palette->m_colors.text            = {Hyprgraphics::CColor::SSRGB{.r = 221 / 255.F, .g = 221 / 255.F, .b = 221 / 255.F}, 1.F};
    palette->m_colors.alternateBase   = {Hyprgraphics::CColor::SSRGB{.r = 39 / 255.F, .g = 39 / 255.F, .b = 39 / 255.F}, 1.F};
    palette->m_colors.brightText      = {Hyprgraphics::CColor::SSRGB{.r = 255 / 255.F, .g = 221 / 255.F, .b = 221 / 255.F}, 1.F};
    palette->m_colors.accent          = {Hyprgraphics::CColor::SSRGB{.r = 0 / 255.F, .g = 225 / 255.F, .b = 210 / 255.F}, 1.F};
    palette->m_colors.accentSecondary = {Hyprgraphics::CColor::SSRGB{.r = 0 / 255.F, .g = 164 / 255.F, .b = 247 / 255.F}, 1.F};

    return palette;
}

SP<CPalette> CPalette::emptyPalette() {
    return SP<CPalette>(new CPalette());
}

SP<CPalette> CPalette::configPalette() {
    return nullptr; // FIXME:
}