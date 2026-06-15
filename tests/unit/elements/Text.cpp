#include <gtest/gtest.h>

#include <element/text/Text.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Color.hpp>

#include <core/InternalBackend.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"

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

// an auto label gets no room hint and its box tracks its own clamped preferred, so the box feeds
// back into the next layout. a transient narrow box on first layout (e.g. before an async icon
// sibling reflows the row) must not lock the text ellipsized: once the box equals the clamped
// preferred the clamp has to release and the text recover. #94 compared the box against a cached
// natural size, so the box-feedback condition stayed true and the ellipsis locked forever. this is
// the hyprlauncher regression.
TEST(Element, textEllipsizeRecoversFromTransientClamp) {
    Tests::Tricks::createBackendSupport();

    auto text = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();

    // natural width: a generous box, no room hint
    g_positioner->position(text, {{}, {1000.F, 20.F}}, {-1.F, -1.F});
    const float NATURAL = text->preferredSize({}).value_or(Vector2D{}).x;
    EXPECT_GT(NATURAL, 40.F);

    // a transient narrow box on first layout clamps it
    g_positioner->position(text, {{}, {NATURAL * 0.3F, 20.F}}, {-1.F, -1.F});
    EXPECT_LT(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    // from here the layout hands the auto label a box equal to its own preferred. feed that back a
    // few frames: it must converge back to NATURAL, not stay locked at the clamped width.
    for (int i = 0; i < 4; ++i) {
        const auto PREF = text->preferredSize({}).value_or(Vector2D{});
        g_positioner->position(text, {{}, PREF}, {-1.F, -1.F});
    }
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    text.reset();
}

// a stable room hint (e.g. the combobox row layout) clamps to the room and stays put, no per-frame
// flicker at the fit/elide boundary.
TEST(Element, textEllipsizeStableUnderRoomHint) {
    Tests::Tricks::createBackendSupport();

    auto        text    = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();
    const float NATURAL = text->preferredSize({}).value_or(Vector2D{}).x;
    EXPECT_GT(NATURAL, 20.F);

    const CBox     widebox = {{}, {NATURAL, 20.F}};
    const Vector2D tight   = {NATURAL * 0.4F, 20.F};

    g_positioner->position(text, widebox, tight);
    const float CLAMPED = text->preferredSize({}).value_or(Vector2D{}).x;
    EXPECT_LT(CLAMPED, NATURAL);

    // same room again does not flip the size
    g_positioner->position(text, widebox, tight);
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, CLAMPED);

    // room grows back: recover to full
    g_positioner->position(text, widebox, {NATURAL + 80.F, 20.F});
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    text.reset();
}