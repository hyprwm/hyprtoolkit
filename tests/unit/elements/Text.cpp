#include <gtest/gtest.h>

#include <element/text/Text.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Color.hpp>

#include <core/InternalBackend.hpp>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;

TEST(Element, text) {
    Tests::Tricks::createBackendSupport();

    // pin the link color so the markup is deterministic regardless of the machine's theme
    g_palette->m_colors.linkText = CHyprColor(1.F, 0.F, 0.F, 1.F);

    auto text = CTextBuilder::begin()->text(R"(Hello <a href="https://hypr.land">link</a>! Hi <a href="https://hypr.land">link2</a>!)")->commence();

    EXPECT_EQ(text->m_impl->parsedText, "Hello <u><span foreground=\"#ff0000ff\">link</span></u>! Hi <u><span foreground=\"#ff0000ff\">link2</span></u>!");

    text.reset();
}