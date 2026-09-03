#include <gtest/gtest.h>

#include <element/checkbox/Checkbox.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;

TEST(Element, checkboxRebuildAppliesStyle) {
    Tests::Tricks::createBackendSupport();

    const auto checkbox   = CCheckboxBuilder::begin()->toggled(true)->commence();
    const auto background = checkbox->impl->children.at(0);
    ASSERT_EQ(background->impl->children.size(), 1);
    EXPECT_NE(dynamic_cast<CCheckmarkElement*>(background->impl->children.at(0).get()), nullptr);

    checkbox->rebuild()->style(HT_CHECKBOX_STYLE_RADIO)->commence();

    ASSERT_EQ(background->impl->children.size(), 1);
    EXPECT_NE(dynamic_cast<CRectangleElement*>(background->impl->children.at(0).get()), nullptr);
    EXPECT_TRUE(checkbox->state());

    checkbox->rebuild()->style(HT_CHECKBOX_STYLE_CHECKMARK)->commence();

    ASSERT_EQ(background->impl->children.size(), 1);
    EXPECT_NE(dynamic_cast<CCheckmarkElement*>(background->impl->children.at(0).get()), nullptr);
}
