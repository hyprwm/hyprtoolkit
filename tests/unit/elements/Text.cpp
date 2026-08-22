#include <gtest/gtest.h>

#include <element/text/Text.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Color.hpp>

#include <core/InternalBackend.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"
#include "hyprtoolkit/element/Element.hpp"
#include "hyprtoolkit/element/Null.hpp"
#include "hyprtoolkit/element/Rectangle.hpp"
#include "hyprtoolkit/element/RowLayout.hpp"
#include "hyprtoolkit/types/FontTypes.hpp"
#include "hyprtoolkit/types/SizeType.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, text) {
    Tests::Tricks::createBackendSupport();

    // pin the link color so the markup is deterministic regardless of the machine's theme
    g_palette->m_colors.linkText = CHyprColor(1.F, 0.F, 0.F, 1.F);

    auto text = CTextBuilder::begin()->text(R"(Hello <a href="https://hypr.land">link</a>! Hi <a href="https://hypr.land">link2</a>!)")->commence();

    EXPECT_EQ(text->m_impl->parsedText, "Hello <u><span foreground=\"#ff0000ff\">link</span></u>! Hi <u><span foreground=\"#ff0000ff\">link2</span></u>!");

    text.reset();
}

TEST(Element, textPreferredSize) {
    Tests::Tricks::createBackendSupport();

    auto       text = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->noEllipsize(true)->commence();

    const auto NATURAL_SIZE = text->preferredSize({0, 0});
    EXPECT_TRUE(NATURAL_SIZE.has_value());

    const auto CLAMPED_WIDTH = std::floor(NATURAL_SIZE->x * 3 / 4);

    const auto WRAPPED_SIZE = text->preferredSize({CLAMPED_WIDTH, NATURAL_SIZE->y * 3});
    EXPECT_TRUE(WRAPPED_SIZE.has_value());
    EXPECT_LE(WRAPPED_SIZE->x, CLAMPED_WIDTH);
    EXPECT_GT(WRAPPED_SIZE->y, NATURAL_SIZE->y);

    text->rebuild()->noEllipsize(false)->commence();
    const auto ELLIPSIZED_SIZE = text->preferredSize({CLAMPED_WIDTH, NATURAL_SIZE->y * 3});
    EXPECT_TRUE(ELLIPSIZED_SIZE.has_value());
    EXPECT_LE(ELLIPSIZED_SIZE->x, CLAMPED_WIDTH);
    EXPECT_EQ(ELLIPSIZED_SIZE->y, NATURAL_SIZE->y);

    const auto FINAL_NATURAL_SIZE = text->preferredSize({0, 0});
    EXPECT_TRUE(FINAL_NATURAL_SIZE.has_value());
    EXPECT_EQ(FINAL_NATURAL_SIZE->x, NATURAL_SIZE->x);
    EXPECT_EQ(FINAL_NATURAL_SIZE->y, NATURAL_SIZE->y);

    text.reset();
}

TEST(Element, textGrowsAgainstNeighbor) {
    Tests::Tricks::createBackendSupport();

    const CBox POSITION = {0, 0, 1000, 100};

    auto       layout = CRowLayoutBuilder::begin()->commence();
    auto       text   = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();
    auto       rect   = CRectangleBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 10}})->commence();
    rect->setGrow(true);

    layout->addChild(text);
    layout->addChild(rect);

    g_positioner->position(layout, POSITION);

    EXPECT_EQ(rect->impl->position.width + text->impl->position.width, POSITION.width);
    const auto PREVIOUS_TEXT_WIDTH = text->impl->position.width;

    text->rebuild()->text("Hello World Foo Bar Baz but longer")->commence();
    g_positioner->position(layout, POSITION);

    EXPECT_EQ(rect->impl->position.width + text->impl->position.width, POSITION.width);
    EXPECT_GT(text->impl->position.width, PREVIOUS_TEXT_WIDTH);

    text.reset();
    rect.reset();
    layout.reset();
}

TEST(Element, textSideBySide) {
    Tests::Tricks::createBackendSupport();

    const CBox POSITION = {0, 0, 300, 100};

    auto       text1 = CTextBuilder::begin()
                     ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.5, 1.0}})
                     ->text("First longish paragraph goes here. I love Hyprland it is the best.")
                     ->noEllipsize(true)
                     ->commence();
    auto text2 = CTextBuilder::begin()
                     ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.5, 1.0}})
                     ->text("Second longish paragraph goes here. Who needs GTK and Qt when you have Hyprtoolkit?")
                     ->noEllipsize(true)
                     ->commence();
    auto       layout = CRowLayoutBuilder::begin()->commence();

    const auto NATURAL_HEIGHT = text1->preferredSize({0, 0})->y;

    layout->addChild(text1);
    layout->addChild(text2);

    g_positioner->position(layout, POSITION);

    ASSERT_EQ(layout->impl->position.width, POSITION.width);
    ASSERT_EQ(text1->impl->position.width, POSITION.width / 2);
    ASSERT_EQ(text2->impl->position.width, POSITION.width / 2);
    ASSERT_EQ(text1->impl->position.width, text2->impl->position.x);
    ASSERT_GT(text1->impl->position.height, NATURAL_HEIGHT);
    ASSERT_GT(text2->impl->position.height, NATURAL_HEIGHT);

    text1.reset();
    text2.reset();
    layout.reset();
}

TEST(Element, textShrinkingThenGrowing) {
    Tests::Tricks::createBackendSupport();

    // This container is necessary because we need the text to auto size itself
    // if we position it directly, the text sizing code is never run
    auto container = CNullBuilder::begin()->commence();
    auto text      = CTextBuilder::begin()->text("First longish paragraph goes here. I love Hyprland it is the best.")->commence();

    container->addChild(text);

    const auto NATURAL_WIDTH = text->preferredSize({0, 0})->x;
    const auto CLAMPED_WIDTH = std::floor(NATURAL_WIDTH * 3 / 4);

    g_positioner->position(container, {0, 0, CLAMPED_WIDTH, 100});
    ASSERT_LE(text->impl->position.width, CLAMPED_WIDTH);

    g_positioner->position(container, {0, 0, NATURAL_WIDTH * 2, 100});
    ASSERT_EQ(text->impl->position.width, NATURAL_WIDTH);

    container.reset();
    text.reset();
}

TEST(Element, textChangingSize) {
    Tests::Tricks::createBackendSupport();

    auto       text = CTextBuilder::begin()->text("First longish paragraph goes here. I love Hyprland it is the best.")->commence();

    const auto FIRST_SIZE = text->preferredSize({0, 0});

    text->rebuild()->fontSize(CFontSize::HT_FONT_H1)->commence();

    const auto SECOND_SIZE = text->preferredSize({0, 0});

    ASSERT_GT(SECOND_SIZE->x, FIRST_SIZE->x);
    ASSERT_GT(SECOND_SIZE->y, FIRST_SIZE->y);

    text.reset();
}

TEST(Element, textChangingContent) {
    Tests::Tricks::createBackendSupport();

    auto       text = CTextBuilder::begin()->text("First longish paragraph goes here.")->commence();

    const auto FIRST_SIZE = text->preferredSize({0, 0});

    text->rebuild()->text("First longish paragraph goes here but event longer")->commence();

    const auto SECOND_SIZE = text->preferredSize({0, 0});

    ASSERT_GT(SECOND_SIZE->x, FIRST_SIZE->x);

    text.reset();
}

TEST(Element, textClampSize) {
    Tests::Tricks::createBackendSupport();

    auto       text = CTextBuilder::begin()->text("First longish paragraph goes here.")->commence();

    const auto NATURAL_SIZE  = text->preferredSize({0, 0});
    const auto CLAMPED_WIDTH = std::floor(NATURAL_SIZE->x * 3 / 4);

    text->rebuild()->clampSize({CLAMPED_WIDTH, -1.0})->commence();

    ASSERT_LE(text->preferredSize({0, 0})->x, CLAMPED_WIDTH);

    text.reset();
}

TEST(Element, textCharPosition) {
    Tests::Tricks::createBackendSupport();

    auto text = CTextBuilder::begin()->text("First longish paragraph goes here.")->commence();

    ASSERT_EQ(text->m_impl->getCursorPos(0), 0);
    ASSERT_GT(text->m_impl->getCursorPos(1), 0);
    ASSERT_GT(text->m_impl->getCursorPos(1000), 0);

    text->rebuild()->text("")->commence();

    ASSERT_EQ(text->m_impl->getCursorPos(0), 0);
    ASSERT_EQ(text->m_impl->getCursorPos(1), 0);

    text.reset();
}
