#pragma once

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SButtonData {
        std::string                                                            label    = "Click me";
        bool                                                                   noBorder = false;
        bool                                                                   noBg     = false;
        std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)> onMainClick;
        std::function<void(Hyprutils::Memory::CSharedPointer<CButtonElement>)> onRightClick;
        CDynamicSize                                                           size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}};
    };

    struct SButtonImpl {
        SButtonData           data;

        WP<CButtonElement>    self;
        SP<CRectangleElement> background;
        SP<CTextElement>      label;

        bool                  labelChanged = true;
    };
}
