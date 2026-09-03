#include <gtest/gtest.h>

#include <element/checkbox/Checkbox.hpp>
#include <hyprtoolkit/element/RadioGroup.hpp>

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

TEST(Element, radioGroupSurvivesCallbackRebuild) {
    Tests::Tricks::createBackendSupport();

    const auto first  = CCheckboxBuilder::begin()->style(HT_CHECKBOX_STYLE_RADIO)->toggled(true)->commence();
    const auto second = CCheckboxBuilder::begin()->style(HT_CHECKBOX_STYLE_RADIO)->commence();
    const auto group  = CRadioGroup::create();
    group->add(first);
    group->add(second);

    int toggles = 0, selections = 0;
    second->rebuild()->onToggled([&toggles](SP<CCheckboxElement>, bool) { ++toggles; })->commence();
    group->onSelected([&selections](SP<CCheckboxElement>) { ++selections; });

    second->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, true);
    second->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, false);

    EXPECT_EQ(toggles, 1);
    EXPECT_EQ(selections, 1);
    EXPECT_FALSE(first->state());
    EXPECT_TRUE(second->state());
    EXPECT_EQ(group->selected(), second);
}
