#pragma once

#include <hyprtoolkit/element/RadioGroup.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

#include <functional>

namespace Hyprtoolkit {

    struct SRadioGroupData {
        std::vector<std::string>                                                        items;
        int                                                                             selected = 0;
        float                                                                           gap      = 4.F;
        std::function<void(Hyprutils::Memory::CSharedPointer<CRadioGroupElement>, int)> onSelected;
        CDynamicSize                                                                    size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.F, 1.F}};
    };

    struct SRadioGroupImpl {
        SRadioGroupData                    data;

        WP<CRadioGroupElement>             self;

        SP<CColumnLayoutElement>           layout;

        // per-item indicator dot, kept so we can flip its alpha on selection
        std::vector<SP<CRectangleElement>> dots;
        std::vector<SP<IElement>>          rows;

        void                               applySelection();
        void                               select(int idx);
    };
}
