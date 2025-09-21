#pragma once

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SButtonImpl {
        SButtonData           data;

        WP<CButtonElement>    self;
        SP<CRectangleElement> background;
        SP<CTextElement>      label;

        bool                  labelChanged = true;
    };
}
