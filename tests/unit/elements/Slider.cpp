#include <gtest/gtest.h>

#include <element/slider/Slider.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

// SSliderImpl::maxLabelSize is a heuristic but it had two real bugs:
// (a) floor(log10(N)) undercounts digits by 1 (log10(100)=2 but "100" is 3 chars)
// (b) UB when data.max <= 0 (log10(0)=-inf, log10(neg)=NaN, then size_t cast).
// these tests pin behavior so future tweaks notice if either regresses.

TEST(SliderLabel, threeDigitMaxIsThreeChars) {
    SSliderImpl s;
    s.data.min     = 0;
    s.data.max     = 100;
    s.data.snapInt = true;
    EXPECT_FLOAT_EQ(s.maxLabelSize(), 3 * 10.F);
}

TEST(SliderLabel, twoDigitMaxIsTwoChars) {
    SSliderImpl s;
    s.data.min     = 0;
    s.data.max     = 50;
    s.data.snapInt = true;
    EXPECT_FLOAT_EQ(s.maxLabelSize(), 2 * 10.F);
}

TEST(SliderLabel, snapFloatAddsTwoForFraction) {
    SSliderImpl s;
    s.data.min     = 0;
    s.data.max     = 100;
    s.data.snapInt = false;
    EXPECT_FLOAT_EQ(s.maxLabelSize(), (3 + 2) * 10.F);
}

TEST(SliderLabel, negativeMinAddsSign) {
    SSliderImpl s;
    s.data.min     = -100;
    s.data.max     = 100;
    s.data.snapInt = true;
    EXPECT_FLOAT_EQ(s.maxLabelSize(), (3 + 1) * 10.F);
}

TEST(SliderLabel, smallRangeDoesNotUnderflow) {
    // log10(1)=0; we still need at least one char to show "0" or "1".
    SSliderImpl s;
    s.data.min     = 0;
    s.data.max     = 1;
    s.data.snapInt = true;
    EXPECT_FLOAT_EQ(s.maxLabelSize(), 1 * 10.F);
}

TEST(SliderLabel, zeroMaxDoesNotCrash) {
    // pre-fix this hit log10(0) = -inf -> floor -> cast to size_t = UB.
    SSliderImpl s;
    s.data.min     = 0;
    s.data.max     = 0;
    s.data.snapInt = true;
    // contract: returns at least one char's width, no crash.
    EXPECT_GE(s.maxLabelSize(), 1 * 10.F);
}

TEST(SliderLabel, negativeMaxDoesNotCrash) {
    // log10(<0) = NaN. defensive: shouldn't blow up.
    SSliderImpl s;
    s.data.min     = -100;
    s.data.max     = -1;
    s.data.snapInt = true;
    EXPECT_GE(s.maxLabelSize(), 1 * 10.F);
}

static std::pair<SP<IElement>, SP<IElement>> sliderTrack(const SP<CSliderElement>& slider) {
    const auto layout     = slider->impl->children.at(0);
    const auto background = layout->impl->children.at(0);
    return {background, background->impl->children.at(0)};
}

TEST(Element, sliderUsesConfiguredRange) {
    Tests::Tricks::createBackendSupport();

    const auto slider                   = CSliderBuilder::begin()->min(-10)->max(10)->val(0)->snapInt(false)->commence();
    const auto [background, foreground] = sliderTrack(slider);

    ASSERT_TRUE(foreground->preferredSize({100, 10}).has_value());
    EXPECT_FLOAT_EQ(foreground->preferredSize({100, 10})->x, 50.F);

    slider->rebuild()->max(30)->commence();
    EXPECT_FLOAT_EQ(foreground->preferredSize({100, 10})->x, 25.F);
}

TEST(Element, sliderReportsRangeValue) {
    Tests::Tricks::createBackendSupport();

    float      changed                  = 0.F;
    const auto slider                   = CSliderBuilder::begin()
                                              ->min(10)
                                              ->max(30)
                                              ->val(10)
                                              ->snapInt(false)
                                              ->onChanged([&changed](SP<CSliderElement>, float value) { changed = value; })
                                              ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {300, 30}})
                                              ->commence();
    const auto [background, foreground] = sliderTrack(slider);
    g_positioner->position(slider, {{}, {300, 30}});

    const auto LOCAL_X = background->impl->position.x - slider->impl->position.x + background->impl->position.w * 0.25;
    slider->impl->m_externalEvents.mouseMove.emit(Vector2D{LOCAL_X, 10.0});
    slider->impl->m_externalEvents.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, true);
    g_positioner->position(slider, {{}, {300, 30}});

    EXPECT_FLOAT_EQ(changed, 15.F);
    EXPECT_NEAR(foreground->impl->position.w / background->impl->position.w, 0.25, 0.001);
}

TEST(Element, sliderDegenerateRangeIsFinite) {
    Tests::Tricks::createBackendSupport();

    float      changed                  = 10.F;
    const auto slider                   = CSliderBuilder::begin()->min(5)->max(5)->val(20)->onChanged([&changed](SP<CSliderElement>, float value) { changed = value; })->commence();
    const auto [background, foreground] = sliderTrack(slider);

    const auto size = foreground->preferredSize({100, 10});
    ASSERT_TRUE(size.has_value());
    EXPECT_TRUE(std::isfinite(size->x));
    EXPECT_FLOAT_EQ(size->x, 0.F);

    slider->rebuild()->val(30)->commence();
    EXPECT_FLOAT_EQ(changed, 10.F);
}
