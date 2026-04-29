#include <gtest/gtest.h>

#include <element/slider/Slider.hpp>

using namespace Hyprtoolkit;

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
