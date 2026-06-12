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

// regression: a text at the fit/elide boundary used to flicker between full and ellipsized
// every frame, and a transient zero-width box (before the layout settles) could lock it
// ellipsized forever. the clamp now compares against the stable natural size and ignores the
// not-yet-laid-out box, so the text does not lock and recovers when room returns.
TEST(Element, textEllipsizeRecoversAndDoesNotLock) {
    Tests::Tricks::createBackendSupport();

    auto        text    = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();
    const float NATURAL = text->preferredSize({}).value_or(Vector2D{}).x;
    EXPECT_GT(NATURAL, 20.F);

    // a transient zero-width box (layout not settled yet) must not clamp the text
    g_positioner->position(text, {{}, {0.F, 20.F}});
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    // a genuinely narrow box ellipsizes
    g_positioner->position(text, {{}, {NATURAL / 2.F, 20.F}});
    EXPECT_LT(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    // room returns: the text recovers to its full size, no permanent ellipsis
    g_positioner->position(text, {{}, {NATURAL + 80.F, 20.F}});
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    text.reset();
}