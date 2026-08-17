#include <gtest/gtest.h>

#include <element/spinbox/Spinbox.hpp>
#include <element/text/Text.hpp>
#include <element/Element.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, spinboxSelectedValueConstraintIsStable) {
    Tests::Tricks::createBackendSupport();

    auto       spinbox = CSpinboxBuilder::begin()->label("Label")->items({"A selected value that is much wider than the spinbox"})->fill(true)->commence();

    const auto outerLayout   = spinbox->impl->children.at(0);
    const auto caption       = dynamicPointerCast<CTextElement>(outerLayout->impl->children.at(0));
    const auto spinner       = outerLayout->impl->children.at(2);
    const auto spinnerLayout = spinner->impl->children.at(1);
    const auto value         = dynamicPointerCast<CTextElement>(spinnerLayout->impl->children.at(2));
    ASSERT_TRUE(caption);
    ASSERT_TRUE(value);

    const Vector2D natural = value->preferredSize({}).value_or(Vector2D{});
    ASSERT_GT(natural.x, 140.F);

    constexpr float SPINNER_CHROME_WIDTH  = 52.F;
    const auto      naturalSpinner        = spinner->preferredSize({}).value_or(Vector2D{});
    const auto      constrainedPreference = spinner->preferredSize({100.F, -1.F}).value_or(Vector2D{});
    const auto      boundedPreference     = spinner->preferredSize({100.F, 100.F}).value_or(Vector2D{});
    EXPECT_FLOAT_EQ(naturalSpinner.x, natural.x + SPINNER_CHROME_WIDTH);
    EXPECT_EQ(constrainedPreference, naturalSpinner);
    EXPECT_EQ(boundedPreference, naturalSpinner);

    const CBox constrainedBox{{}, {140.F, 30.F}};
    g_positioner->position(spinbox, {{}, {20.F, 30.F}});
    ASSERT_TRUE(spinner->impl->failedPositioning);

    constexpr float MINIMUM_SPINBOX_WIDTH = 58.F;
    g_positioner->position(spinbox, {{}, {MINIMUM_SPINBOX_WIDTH, 30.F}});
    ASSERT_FALSE(spinner->impl->failedPositioning);
    ASSERT_LE(spinner->impl->position.x + spinner->impl->position.w, MINIMUM_SPINBOX_WIDTH);
    for (const auto& child : spinnerLayout->impl->children) {
        ASSERT_FALSE(child->impl->failedPositioning);
        ASSERT_LE(child->impl->position.x + child->impl->position.w, spinner->impl->position.x + spinner->impl->position.w);
    }

    g_positioner->position(spinbox, constrainedBox);

    ASSERT_FALSE(spinner->impl->failedPositioning);
    ASSERT_LE(spinner->impl->position.x + spinner->impl->position.w, spinbox->impl->position.x + spinbox->impl->position.w);
    ASSERT_GT(value->m_impl->lastMaxSize.x, 0.F);
    ASSERT_LT(value->m_impl->lastMaxSize.x, natural.x);
    ASSERT_LE(value->m_impl->lastMaxSize.x, value->impl->position.w);
    ASSERT_LE(value->m_impl->getTextSizePreferred().x, value->m_impl->lastMaxSize.x);

    const Vector2D constraint = value->m_impl->lastMaxSize;
    for (size_t i = 0; i < 8; ++i) {
        g_positioner->position(spinbox, constrainedBox);
        EXPECT_FALSE(spinner->impl->failedPositioning);
        EXPECT_LE(spinner->impl->position.x + spinner->impl->position.w, spinbox->impl->position.x + spinbox->impl->position.w);
        EXPECT_EQ(value->m_impl->lastMaxSize, constraint);
        EXPECT_LE(value->m_impl->lastMaxSize.x, value->impl->position.w);
    }

    g_positioner->position(spinbox, {{}, {natural.x + 100.F, 30.F}});
    EXPECT_FALSE(spinner->impl->failedPositioning);
    EXPECT_LT(value->m_impl->lastMaxSize.x, 0.F);
    EXPECT_EQ(value->m_impl->getTextSizePreferred(), natural);
    EXPECT_LT(caption->m_impl->lastMaxSize.x, 0.F);
}
