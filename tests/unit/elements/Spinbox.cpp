#include <gtest/gtest.h>

#include <element/spinbox/Spinbox.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;

static SP<IElement> spinboxArrow(const SP<CSpinboxElement>& spinbox, bool right) {
    const auto outerLayout = spinbox->impl->children.at(0);
    const auto spinner     = outerLayout->impl->children.at(2);
    const auto spinnerRow  = spinner->impl->children.at(1);
    return spinnerRow->impl->children.at(right ? 3 : 1);
}

static void click(const SP<IElement>& element) {
    element->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, true);
    element->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, false);
}

TEST(Element, spinboxArrowsMoveAndNotify) {
    Tests::Tricks::createBackendSupport();

    size_t     selected = 99;
    int        changes  = 0;
    const auto spinbox  = CSpinboxBuilder::begin()
                              ->items({"A", "B", "C"})
                              ->currentItem(1)
                              ->onChanged([&](SP<CSpinboxElement>, size_t current) {
                                 selected = current;
                                 ++changes;
                              })
                              ->commence();

    click(spinboxArrow(spinbox, true));
    EXPECT_EQ(spinbox->current(), 2);
    EXPECT_EQ(selected, 2);

    click(spinboxArrow(spinbox, true));
    EXPECT_EQ(spinbox->current(), 0);

    click(spinboxArrow(spinbox, false));
    EXPECT_EQ(spinbox->current(), 2);
    EXPECT_EQ(changes, 3);

    spinbox->setCurrent(1);
    EXPECT_EQ(changes, 3);
}

TEST(Element, spinboxRebuildNormalizesSelection) {
    Tests::Tricks::createBackendSupport();

    int        changes = 0;
    const auto spinbox = CSpinboxBuilder::begin()->items({"A", "B", "C"})->currentItem(99)->onChanged([&](SP<CSpinboxElement>, size_t) { ++changes; })->commence();
    EXPECT_EQ(spinbox->current(), 2);

    const auto rebuilt = spinbox->rebuild()->items({"X", "Y"})->currentItem(1)->commence();
    EXPECT_EQ(rebuilt, spinbox);
    EXPECT_EQ(spinbox->current(), 1);

    spinbox->rebuild()->items({})->commence();
    EXPECT_EQ(spinbox->current(), 0);
    spinbox->setCurrent(10);
    click(spinboxArrow(spinbox, false));
    click(spinboxArrow(spinbox, true));
    EXPECT_EQ(spinbox->current(), 0);
    EXPECT_EQ(changes, 0);

    spinbox->rebuild()->items({"Only", "Again"})->commence();
    click(spinboxArrow(spinbox, true));
    EXPECT_EQ(spinbox->current(), 1);
    EXPECT_EQ(changes, 1);
}

TEST(Element, spinboxRebuildAppliesFill) {
    Tests::Tricks::createBackendSupport();

    const auto spinbox = CSpinboxBuilder::begin()->items({"A"})->commence();
    const auto element = SP<IElement>{spinbox};
    const auto before  = element->preferredSize({400, 100});

    spinbox->rebuild()->fill(true)->commence();
    const auto after = element->preferredSize({400, 100});

    ASSERT_TRUE(before.has_value());
    ASSERT_TRUE(after.has_value());
    EXPECT_GT(after->x, before->x);
    EXPECT_GE(after->x, 400.F);
}
