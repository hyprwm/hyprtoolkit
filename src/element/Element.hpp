#pragma once

#include <hyprtoolkit/element/Element.hpp>

#include <functional>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IToolkitWindow;

    struct SElementInternalData {
        Hyprutils::Memory::CWeakPointer<IElement>                self;
        Hyprutils::Memory::CWeakPointer<IToolkitWindow>          window;
        Hyprutils::Math::CBox                                    position;

        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> children;

        IElement::ePositionMode                                  positionMode = IElement::HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                absoluteOffset;
        bool                                                     grow = false;

        WP<IElement>                                             parent;

        bool                                                     failedPositioning = false;

        //
        void bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn);
        void breadthfirst(const std::function<void(SP<IElement>)>& fn);
        void setWindow(SP<IToolkitWindow> w);
        void damageEntire();
    };

}
