#include <gtest/gtest.h>

#include <hyprtoolkit/core/Animation.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>

#include <element/Element.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;
using namespace std::chrono_literals;

TEST(Animation, opacityPolicyAppliesToFutureChanges) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    element->setOpacity(0.8F);
    EXPECT_FLOAT_EQ(element->impl->opacity, 0.8F);

    element->animateOpacity(SBezierAnimation{.duration = 250ms});
    element->setOpacity(0.25F);

    ASSERT_TRUE(element->impl->animatedOpacity);
    EXPECT_FLOAT_EQ(element->impl->animatedOpacity->value(), 0.8F);
    EXPECT_FLOAT_EQ(element->impl->animatedOpacity->goal(), 0.25F);
    EXPECT_TRUE(element->impl->animatedOpacity->isBeingAnimated());
}

TEST(Animation, geometryUsesFinalLayoutAndPreviousPresentation) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 80}})->commence();
    g_positioner->position(element, {10, 20, 100, 80});
    element->animateGeometry(SSpringAnimation{});
    element->impl->hasBeenPresented = true;

    g_positioner->position(element, {210, 120, 160, 100});

    EXPECT_EQ(element->impl->position, CBox(210, 120, 160, 100));
    ASSERT_TRUE(element->impl->animatedGeometry);
    EXPECT_EQ(element->impl->animatedGeometry->value(), CBox(10, 20, 100, 80));
    EXPECT_EQ(element->impl->animatedGeometry->goal(), CBox(210, 120, 160, 100));
    EXPECT_EQ(element->impl->presentationBox(element->impl->position), CBox(10, 20, 100, 80));
    EXPECT_TRUE(element->impl->animatedGeometry->isSpringCurve());
}

TEST(Animation, unconfiguredGeometryChangesImmediately) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    g_positioner->position(element, {10, 20, 100, 80});
    g_positioner->position(element, {210, 120, 160, 100});

    EXPECT_FALSE(element->impl->animatedGeometry);
    EXPECT_EQ(element->impl->presentationBox(element->impl->position), CBox(210, 120, 160, 100));
}

TEST(Animation, nestedGeometryUsesNearestConfiguredElement) {
    Tests::Tricks::createBackendSupport();

    auto parent = CRectangleBuilder::begin()->commence();
    auto child  = CRectangleBuilder::begin()->commence();
    parent->addChild(child);

    parent->impl->setPosition({0, 0, 200, 200});
    child->impl->setPosition({10, 10, 50, 50});
    parent->animateGeometry(SSpringAnimation{});
    child->animateGeometry(SSpringAnimation{});
    parent->impl->hasBeenPresented = true;
    child->impl->hasBeenPresented  = true;

    parent->impl->setPosition({100, 100, 300, 300});
    child->impl->setPosition({120, 120, 75, 75});

    EXPECT_EQ(child->impl->presentationBox(child->impl->position), CBox(10, 10, 50, 50));
}

TEST(Animation, configuredChildComposesWithParentGeometry) {
    Tests::Tricks::createBackendSupport();

    auto parent = CRectangleBuilder::begin()->commence();
    auto child  = CRectangleBuilder::begin()->commence();
    parent->addChild(child);

    parent->impl->setPosition({0, 0, 200, 200});
    child->impl->setPosition({10, 10, 50, 50});
    parent->animateGeometry(SSpringAnimation{});
    child->animateGeometry(SSpringAnimation{});
    parent->impl->hasBeenPresented = true;
    child->impl->hasBeenPresented  = true;

    parent->impl->setPosition({100, 100, 300, 300});
    child->impl->setPosition({110, 110, 50, 50});

    EXPECT_EQ(child->impl->presentationBox(child->impl->position), CBox(10, 10, 50, 50));
}

TEST(Animation, replacingPolicyOnlyAffectsNextChange) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    element->animateOpacity(SBezierAnimation{.duration = 250ms});
    element->setOpacity(0.5F);
    EXPECT_FALSE(element->impl->animatedOpacity->isSpringCurve());

    element->animateOpacity(SSpringAnimation{});
    EXPECT_FALSE(element->impl->animatedOpacity->isSpringCurve());

    element->setOpacity(0.25F);
    EXPECT_TRUE(element->impl->animatedOpacity->isSpringCurve());
    EXPECT_FLOAT_EQ(element->impl->animatedOpacity->goal(), 0.25F);
}

TEST(Animation, noAnimationRestoresImmediateOpacityChanges) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    element->animateOpacity(SSpringAnimation{});
    element->setOpacity(0.25F);
    element->animateOpacity(SNoAnimation{});

    EXPECT_FALSE(element->impl->animatedOpacity);
    EXPECT_FLOAT_EQ(element->impl->opacity, 0.25F);

    element->setOpacity(0.75F);
    EXPECT_FLOAT_EQ(element->impl->opacity, 0.75F);
}

TEST(Animation, noAnimationRemovesPresentationGeometry) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    g_positioner->position(element, {10, 20, 100, 80});
    element->animateGeometry(SSpringAnimation{});
    element->impl->hasBeenPresented = true;
    g_positioner->position(element, {210, 120, 160, 100});
    element->animateGeometry(SNoAnimation{});

    EXPECT_FALSE(element->impl->animatedGeometry);
    EXPECT_EQ(element->impl->presentationBox(element->impl->position), CBox(210, 120, 160, 100));
}

TEST(Animation, geometryIgnoresLayoutsBeforeFirstPresentation) {
    Tests::Tricks::createBackendSupport();

    auto element = CRectangleBuilder::begin()->commence();
    element->animateGeometry(SSpringAnimation{});

    g_positioner->position(element, {10, 20, 100, 80});
    g_positioner->position(element, {210, 120, 160, 100});

    EXPECT_FALSE(element->impl->animatedGeometry->isBeingAnimated());
    EXPECT_EQ(element->impl->animatedGeometry->value(), CBox(210, 120, 160, 100));

    element->impl->hasBeenPresented = true;
    g_positioner->position(element, {310, 220, 180, 120});

    EXPECT_TRUE(element->impl->animatedGeometry->isBeingAnimated());
    EXPECT_EQ(element->impl->animatedGeometry->value(), CBox(210, 120, 160, 100));
    EXPECT_EQ(element->impl->animatedGeometry->goal(), CBox(310, 220, 180, 120));
}
