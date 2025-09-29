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
    struct SToolkitWindowData;
    class CDynamicSize;

    struct SElementInternalData {
        Hyprutils::Memory::CWeakPointer<IElement>                self;
        Hyprutils::Memory::CWeakPointer<IToolkitWindow>          window;
        Hyprutils::Math::CBox                                    position;

        UP<SPositionerData>                                      positionerData;
        UP<SToolkitWindowData>                                   toolkitWindowData;

        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> children;

        IElement::ePositionMode                                  positionMode = IElement::HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                absoluteOffset;
        bool                                                     growV                   = false;
        bool                                                     growH                   = false;
        float                                                    margin                  = 0;
        bool                                                     userRequestedMouseInput = false;

        // rendering: clip children to parent box
        bool         clipChildren = false;

        WP<IElement> parent;

        bool         failedPositioning = false;

        struct {
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> mouseEnter; // local coords
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> mouseMove;  // local coords
            Hyprutils::Signal::CSignalT<Input::eMouseButton, bool> mouseButton;
            Hyprutils::Signal::CSignalT<>                          mouseLeave;
            Hyprutils::Signal::CSignalT<Input::eAxisAxis, float>   mouseAxis;
            Hyprutils::Signal::CSignalT<Input::SKeyboardKeyEvent>  key;
            Hyprutils::Signal::CSignalT<>                          keyboardEnter;
            Hyprutils::Signal::CSignalT<>                          keyboardLeave;
        } m_externalEvents;

        struct {
            std::function<void(const Hyprutils::Math::Vector2D&)> mouseEnter;
            std::function<void()>                                 mouseLeave;
            std::function<void(const Hyprutils::Math::Vector2D&)> mouseMove;
            std::function<void(Input::eMouseButton, bool)>        mouseButton;
            std::function<void(Input::eAxisAxis, float)>          mouseAxis;
        } userFns;

        //
        void                      bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn);
        void                      breadthfirst(const std::function<void(SP<IElement>)>& fn);
        void                      setWindow(SP<IToolkitWindow> w);
        void                      damageEntire();
        void                      setPosition(const Hyprutils::Math::CBox& box);
        void                      setFailedPositioning(bool set);
        Hyprutils::Math::Vector2D maxChildSize(const Hyprutils::Math::Vector2D& parent);
        Hyprutils::Math::Vector2D getPreferredSizeGeneric(const CDynamicSize& size, const Hyprutils::Math::Vector2D& parent);
    };

}
