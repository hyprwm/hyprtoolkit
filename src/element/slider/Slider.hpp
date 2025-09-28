#pragma once

#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    struct SSliderData {
        float                                                                         min = 0, max = 100, current = 50;
        bool                                                                          snapInt = true;
        std::function<void(Hyprutils::Memory::CSharedPointer<CSliderElement>, float)> onChanged;
        CDynamicSize                                                                  size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {}};
    };

    struct SSliderImpl {
        SSliderData               data;

        WP<CSliderElement>        self;

        void                      updateValue();

        std::string               valueAsText();
        void                      valueChanged(float perc);
        float                     maxLabelSize();

        SP<CRowLayoutElement>     layout;
        SP<CRectangleElement>     background;
        SP<CRectangleElement>     foreground;
        SP<CTextElement>          valueText;
        SP<CNullElement>          textContainer;

        bool                      dragging = false;
        Hyprutils::Math::Vector2D lastPosLocal;

        bool                      labelChanged = true;
    };
}
