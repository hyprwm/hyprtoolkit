#include <hyprtoolkit/palette/Palette.hpp>

#include "../helpers/Memory.hpp"

using namespace Hyprtoolkit;

SP<CPalette> CPalette::palette() {
    auto CONFIG = configPalette();
    if (!CONFIG)
        return defaultPalette();
    return CONFIG;
}

SP<CPalette> CPalette::defaultPalette() {
    auto palette = SP<CPalette>(new CPalette());

    palette->m_colors.background      = {24 / 255.F, 24 / 255.F, 24 / 255.F, 1.F};
    palette->m_colors.base            = {32 / 255.F, 32 / 255.F, 32 / 255.F, 1.F};
    palette->m_colors.text            = {221 / 255.F, 221 / 255.F, 221 / 255.F, 1.F};
    palette->m_colors.alternateBase   = {39 / 255.F, 39 / 255.F, 39 / 255.F, 1.F};
    palette->m_colors.brightText      = {255 / 255.F, 221 / 255.F, 221 / 255.F, 1.F};
    palette->m_colors.accent          = {0 / 255.F, 225 / 255.F, 210 / 255.F, 1.F};
    palette->m_colors.accentSecondary = {0 / 255.F, 164 / 255.F, 247 / 255.F, 1.F};

    return palette;
}

SP<CPalette> CPalette::emptyPalette() {
    return SP<CPalette>(new CPalette());
}

SP<CPalette> CPalette::configPalette() {
    return nullptr; // FIXME:
}