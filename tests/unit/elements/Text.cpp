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

// A transient box constraint affects rendering, but not a later unconstrained measurement. Keeping
// those independent prevents the rendered result from becoming the next layout's input.
TEST(Element, textEllipsizeRecoversFromTransientClamp) {
    Tests::Tricks::createBackendSupport();

    auto text = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();

    // natural width: a generous box, no room hint
    g_positioner->position(text, {{}, {1000.F, 20.F}}, {-1.F, -1.F});
    const float NATURAL = text->preferredSize({}).value_or(Vector2D{}).x;
    EXPECT_GT(NATURAL, 40.F);

    // a transient narrow box on first layout clamps it
    g_positioner->position(text, {{}, {NATURAL * 0.3F, 20.F}}, {-1.F, -1.F});
    EXPECT_LT(text->m_impl->getTextSizePreferred().x, NATURAL);
    EXPECT_FLOAT_EQ(text->preferredSize({}).value_or(Vector2D{}).x, NATURAL);

    const auto PREF = text->preferredSize({}).value_or(Vector2D{});
    g_positioner->position(text, {{}, PREF}, {-1.F, -1.F});
    EXPECT_FLOAT_EQ(text->m_impl->getTextSizePreferred().x, NATURAL);

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
    const float CLAMPED = text->m_impl->getTextSizePreferred().x;
    EXPECT_LT(CLAMPED, NATURAL);

    // same room again does not flip the size
    g_positioner->position(text, widebox, tight);
    EXPECT_FLOAT_EQ(text->m_impl->getTextSizePreferred().x, CLAMPED);

    // room grows back: recover to full
    g_positioner->position(text, widebox, {NATURAL + 80.F, 20.F});
    EXPECT_FLOAT_EQ(text->m_impl->getTextSizePreferred().x, NATURAL);

    text.reset();
}

TEST(Element, textMeasurementUsesSuppliedConstraint) {
    Tests::Tricks::createBackendSupport();

    auto           text    = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();
    const Vector2D natural = text->preferredSize({}).value_or(Vector2D{});
    const Vector2D widthOnlyConstraint{natural.x * 0.4F, -1.F};
    const Vector2D boundedConstraint{widthOnlyConstraint.x, natural.y};
    const Vector2D wrapped = text->preferredSize(widthOnlyConstraint).value_or(Vector2D{});
    const Vector2D bounded = text->preferredSize(boundedConstraint).value_or(Vector2D{});

    EXPECT_LE(wrapped.x, widthOnlyConstraint.x);
    EXPECT_GT(wrapped.y, natural.y);
    EXPECT_LE(bounded.x, boundedConstraint.x);
    EXPECT_LE(bounded.y, boundedConstraint.y);

    auto unwrapped = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->noEllipsize(true)->commence();
    EXPECT_EQ(unwrapped->preferredSize(widthOnlyConstraint), unwrapped->preferredSize({}));

    text.reset();
    unwrapped.reset();
}

TEST(Element, textNoEllipsizeClearsDynamicConstraint) {
    Tests::Tricks::createBackendSupport();

    auto       text    = CTextBuilder::begin()->text("Hello World Foo Bar Baz")->commence();
    const auto NATURAL = text->preferredSize({}).value_or(Vector2D{});
    g_positioner->position(text, {{}, {NATURAL.x * 0.4F, NATURAL.y}}, {NATURAL.x * 0.4F, NATURAL.y});
    ASSERT_GT(text->m_impl->lastMaxSize.x, 0.F);

    text->rebuild()->noEllipsize(true)->commence();
    g_positioner->position(text, {{}, {NATURAL.x * 0.4F, NATURAL.y}}, {NATURAL.x * 0.4F, NATURAL.y});
    EXPECT_EQ(text->m_impl->lastMaxSize, Vector2D(-1, -1));
    EXPECT_EQ(text->m_impl->getTextSizePreferred(), NATURAL);
}

TEST(Element, synchronousTextChangesInvalidateTexture) {
    Tests::Tricks::createBackendSupport();

    auto text                     = CTextBuilder::begin()->text("Before")->async(false)->commence();
    text->m_impl->needsTexRefresh = false;
    text->setText("After");
    EXPECT_TRUE(text->m_impl->needsTexRefresh);

    text->m_impl->needsTexRefresh = false;
    text->rebuild()->align(HT_FONT_ALIGN_RIGHT)->commence();
    EXPECT_TRUE(text->m_impl->needsTexRefresh);
}
