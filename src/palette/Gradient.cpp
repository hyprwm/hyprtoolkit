#include <hyprtoolkit/palette/Gradient.hpp>

using namespace Hyprtoolkit;

bool CGradientValueData::operator==(const CGradientValueData& rhs) const {
    return m_fAngle == rhs.m_fAngle && m_vColors == rhs.m_vColors;
}
