#include <gtest/gtest.h>

#include <hyprtoolkit/element/Combobox.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;

TEST(Element, comboboxNormalizesSelection) {
    Tests::Tricks::createBackendSupport();

    const auto combobox = CComboboxBuilder::begin()->items({"A", "B"})->currentItem(99)->commence();
    EXPECT_EQ(combobox->current(), 1);

    const auto rebuilt = combobox->rebuild()->items({"Only"})->commence();
    EXPECT_EQ(rebuilt, combobox);
    EXPECT_EQ(combobox->current(), 0);
}

TEST(Element, comboboxSupportsEmptyItems) {
    Tests::Tricks::createBackendSupport();

    const auto combobox = CComboboxBuilder::begin()->items({})->currentItem(99)->commence();
    EXPECT_EQ(combobox->current(), 0);

    combobox->setCurrent(10);
    combobox->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, true);
    combobox->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, false);
    EXPECT_EQ(combobox->current(), 0);

    combobox->rebuild()->items({"A", "B"})->currentItem(1)->commence();
    EXPECT_EQ(combobox->current(), 1);
    combobox->rebuild()->items({})->commence();
    EXPECT_EQ(combobox->current(), 0);
}
