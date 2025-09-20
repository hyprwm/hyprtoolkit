#pragma once

#include <hyprtoolkit/element/Element.hpp>

namespace Hyprtoolkit {
    struct SElementInternalData {
        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> children;

        IElement::ePositionMode                                  positionMode = IElement::HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                absoluteOffset;
        bool                                                     grow = false;

        bool                                                     failedPositioning = false;
    };
}
