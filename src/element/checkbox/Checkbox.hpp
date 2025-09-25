#pragma once

#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SCheckboxData {
        std::string                                                                    label   = "Checkbox";
        bool                                                                           toggled = false;
        std::function<void(Hyprutils::Memory::CSharedPointer<CCheckboxElement>, bool)> onToggled;
        CDynamicSize                                                                   size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
        bool                                                                           fill = false; // FIXME: better layouting needed...
    };

    struct SCheckboxImpl {
        SCheckboxData               data;

        WP<CCheckboxElement>        self;
        SP<CRowLayoutElement>       layout;
        SP<CRectangleElement>       background;
        SP<CRectangleElement>       foreground;
        SP<CTextElement>            label;
        SP<CNullElement>            spacer;

        bool                        labelChanged = true;

        bool                        primedForUp = false;

        std::function<CHyprColor()> getFgColor();
    };
}
