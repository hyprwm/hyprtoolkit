#include <hyprtoolkit/palette/Palette.hpp>

#include "ConfigManager.hpp"

#include "../helpers/Memory.hpp"
#include "../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

SP<CPalette> CPalette::palette() {
    return g_config->getPalette();
}

SP<CPalette> CPalette::emptyPalette() {
    return SP<CPalette>(new CPalette());
}
