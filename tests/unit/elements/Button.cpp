#include <gtest/gtest.h>

#include <hyprtoolkit/element/Button.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, buttonRebuildAppliesLabelLayout) {
    Tests::Tricks::createBackendSupport();

    const auto button  = CButtonBuilder::begin()->fontSize({CFontSize::HT_FONT_ABSOLUTE, 12.F})->commence();
    const auto element = SP<IElement>{button};
    const auto before  = element->preferredSize({500, 500});

    button->rebuild()->fontSize({CFontSize::HT_FONT_ABSOLUTE, 24.F})->alignText(HT_FONT_ALIGN_RIGHT)->ellipsize(true)->commence();

    const auto after = element->preferredSize({500, 500});
    const auto label = button->impl->children.at(0)->impl->children.at(0);
    ASSERT_TRUE(before.has_value());
    ASSERT_TRUE(after.has_value());
    EXPECT_GT(after->y, before->y);
    EXPECT_TRUE(label->impl->positionFlags & IElement::HT_POSITION_FLAG_RIGHT);
    EXPECT_TRUE(label->impl->positionFlags & IElement::HT_POSITION_FLAG_VCENTER);
}
