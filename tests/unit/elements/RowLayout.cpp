#include <gtest/gtest.h>

#include <element/rowLayout/RowLayout.hpp>
#include <hyprtoolkit/element/Null.hpp>

using namespace Hyprtoolkit;

TEST(Element, rowLayoutRebuild) {
    const auto layout  = CRowLayoutBuilder::begin()->gap(2)->commence();
    const auto element = SP<IElement>{layout};
    layout->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {10, 10}})->commence());
    layout->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {10, 10}})->commence());

    EXPECT_EQ(element->preferredSize({100, 100}), Hyprutils::Math::Vector2D(22, 10));

    const auto rebuilt = layout->rebuild()->gap(8)->commence();

    EXPECT_EQ(rebuilt, layout);
    EXPECT_EQ(element->preferredSize({100, 100}), Hyprutils::Math::Vector2D(28, 10));
}
