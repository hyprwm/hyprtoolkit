#pragma once

#include <hyprtoolkit/element/Element.hpp>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SElementInternalData {
        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> children;

        IElement::ePositionMode                                  positionMode = IElement::HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                absoluteOffset;
        bool                                                     grow = false;

        WP<IElement>                                             parent;

        bool                                                     failedPositioning = false;
    };
}
