#pragma once

#include <hyprtoolkit/element/Element.hpp>
#include <hyprutils/signal/Signal.hpp>

#include <functional>

#include "../helpers/Memory.hpp"
#include "../core/Input.hpp"

#include <hyprutils/math/Box.hpp>

namespace Hyprtoolkit {
    class IToolkitWindow;
    struct SPositionerData;

    struct SElementInternalData {
        Hyprutils::Memory::CWeakPointer<IElement>                self;
        Hyprutils::Memory::CWeakPointer<IToolkitWindow>          window;
        Hyprutils::Math::CBox                                    position;

        UP<SPositionerData>                                      positionerData;

        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> children;

        IElement::ePositionMode                                  positionMode = IElement::HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                absoluteOffset;
        bool                                                     growV   = false;
        bool                                                     growH   = false;
        float                                                    margin = 0;

        WP<IElement>                                             parent;

        bool                                                     failedPositioning = false;

        struct {
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> mouseEnter; // local coords
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> mouseMove;  // local coords
            Hyprutils::Signal::CSignalT<Input::eMouseButton, bool> mouseButton;
            Hyprutils::Signal::CSignalT<>                          mouseLeave;
        } m_externalEvents;

        //
        void bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn);
        void breadthfirst(const std::function<void(SP<IElement>)>& fn);
        void setWindow(SP<IToolkitWindow> w);
        void damageEntire();
        void setPosition(const Hyprutils::Math::CBox& box);
    };

}
