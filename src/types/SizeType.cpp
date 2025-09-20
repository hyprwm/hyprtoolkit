#include <hyprtoolkit/types/SizeType.hpp>

using namespace Hyprtoolkit;

CDynamicSize::CDynamicSize(eSizingType typeX, eSizingType typeY, const Hyprutils::Math::Vector2D& size) : m_typeX(typeX), m_typeY(typeY), m_value(size) {
    ;
}

Hyprutils::Math::Vector2D CDynamicSize::calculate(Hyprutils::Math::Vector2D elSize) {
    Hyprutils::Math::Vector2D size;
    switch (m_typeX) {
        case HT_SIZE_ABSOLUTE: size.x = m_value.x; break;
        case HT_SIZE_PERCENT: size.x = m_value.x * elSize.x; break;
        case HT_SIZE_AUTO: size.x = -1; break;
        default: break;
    }
    switch (m_typeY) {
        case HT_SIZE_ABSOLUTE: size.y = m_value.y; break;
        case HT_SIZE_PERCENT: size.y = m_value.y * elSize.y; break;
        case HT_SIZE_AUTO: size.y = -1; break;
        default: break;
    }

    return size;
}