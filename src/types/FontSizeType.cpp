#include <hyprtoolkit/types/FontTypes.hpp>

#include "../core/InternalBackend.hpp"
#include "../Macros.hpp"

using namespace Hyprtoolkit;

CFontSize::CFontSize(eSizingBase base, float mult) : m_base(base), m_value(mult) {
    ;
}

float CFontSize::ptSize() {
    switch (m_base) {
        case HT_FONT_H1: return g_palette->m_vars.h1Size * m_value;
        case HT_FONT_H2: return g_palette->m_vars.h2Size * m_value;
        case HT_FONT_H3: return g_palette->m_vars.h3Size * m_value;
        case HT_FONT_TEXT: return g_palette->m_vars.fontSize * m_value;
        case HT_FONT_SMALL: return g_palette->m_vars.smallFontSize * m_value;
        case HT_FONT_ABSOLUTE: return m_value;
        default: UNREACHABLE();
    }
    UNREACHABLE();
    return 1; // suppress warning
}