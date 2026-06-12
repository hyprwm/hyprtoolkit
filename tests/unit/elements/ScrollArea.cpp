#include <gtest/gtest.h>

#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Input.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

// the scrollbar thumb is sized and placed from the max scroll offset, so the
// clamp has to report the real content overflow. content 500 tall in a 100 tall
// viewport means 400 of scroll, no more, no less.
TEST(Element, scrollAreaClampsToContentOverflow) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 100}};

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 500}})->commence();

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // overscroll clamps to the overflow (500 - 100)
    scroll->setScroll({0, 1000});
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, 400.F);

    // negative clamps to zero
    scroll->setScroll({0, -50});
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, 0.F);

    // a valid offset is kept as-is
    scroll->setScroll({0, 120});
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, 120.F);

    // x never scrolls (scrollX is off)
    scroll->setScroll({300, 120});
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().x, 0.F);

    root.reset();
}

// the scrollbar is a real element in the tree now (so it can take input). its thumb
// must be sized to the visible fraction and slide from top to bottom as you scroll.
TEST(Element, scrollAreaThumbTracksScroll) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 100}};

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 500}})->commence(); // 5x overflow

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // children: [0] inner scrolled layer, [1] the shown vertical scrollbar strip
    ASSERT_GE(scroll->impl->children.size(), 2u);
    auto strip = scroll->impl->children.at(1);
    // strip children: [0] track (fills the strip), [1] thumb
    ASSERT_GE(strip->impl->children.size(), 2u);
    auto       thumb = strip->impl->children.at(1);

    const auto stripBox = strip->impl->position;
    const auto top      = thumb->impl->position;

    EXPECT_GT(top.h, 0.F);
    EXPECT_LT(top.h, stripBox.h);               // 5x content -> thumb shorter than the track
    EXPECT_NEAR(top.y, stripBox.y + 2.F, 1.0F); // at scroll 0 the thumb sits at the top (2px pad)

    // scroll to the bottom: the thumb's bottom edge reaches the bottom of the track
    scroll->setScroll({0, 400});
    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    const auto bottom = strip->impl->children.at(1)->impl->position;
    EXPECT_NEAR(bottom.y + bottom.h, stripBox.y + stripBox.h - 2.F, 1.5F);

    root.reset();
}

// a viewport shorter than the minimum thumb length used to abort in std::clamp(x, min, track)
// because track < min. the layout must survive it and keep the thumb within the track.
TEST(Element, scrollAreaTinyViewportDoesNotCrash) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 20}}; // track length (20 - 4) is below the 24px min thumb

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 500}})->commence();

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area); // must not abort
    g_positioner->positionChildren(root);

    ASSERT_GE(scroll->impl->children.size(), 2u);
    auto strip = scroll->impl->children.at(1);
    ASSERT_GE(strip->impl->children.size(), 2u);
    const auto thumb = strip->impl->children.at(1)->impl->position;

    EXPECT_GT(thumb.h, 0.F);
    EXPECT_LE(thumb.h, strip->impl->position.h); // thumb never exceeds the track

    root.reset();
}

// the scrollbar is in the tree so it can take input: grabbing the thumb and dragging it
// must move the scroll proportionally, and releasing must stop the drag.
TEST(Element, scrollAreaThumbDragScrolls) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 100}};

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 500}})->commence();

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    auto  strip = scroll->impl->children.at(1);
    auto& ev    = strip->impl->m_externalEvents;

    // geometry: track 96, thumb 24, thumb at top (offset 2) while scroll is 0
    // press on the thumb (y=10, grab = 10-2 = 8), then drag the cursor to y=50
    ev.mouseMove.emit(Vector2D{3, 10});
    ev.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, true);
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, 0.F); // pressing where the thumb already is does not jump

    ev.mouseMove.emit(Vector2D{3, 50});
    // thumbOff target = 50 - 8 = 42; frac = (42 - 2) / (96 - 24); scroll = frac * 400
    EXPECT_NEAR(scroll->getCurrentScroll().y, (40.F / 72.F) * 400.F, 1.0F);

    // release, then move again: a non-dragging move must not scroll
    ev.mouseButton.emit(Input::MOUSE_BUTTON_LEFT, false);
    const float held = scroll->getCurrentScroll().y;
    ev.mouseMove.emit(Vector2D{3, 90});
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, held);

    root.reset();
}

// regression: the scroll used to snap up and down while scrolling. the overflow was measured
// live from the content's laid-out box, which the reposition flips between a natural and a
// grown height within a frame. a layout child whose size tracks its parent then makes the
// measured overflow differ depending on which state it is sampled in, so an off-frame clamp
// (a wheel event) would yank the scroll to a wrong limit. the overflow is now cached from the
// grown pass, so a stray relayout of just the content must not move the clamp.
TEST(Element, scrollAreaMaxScrollIsStableAcrossContentRelayout) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 100}};

    auto       root   = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();

    // a column whose preferred height depends on its own laid-out box: childSize measures each
    // child against the column's stored position, so the percent-height child reports a
    // different height when the column was last laid out grown vs natural.
    auto content = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    content->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 400}})->commence());
    content->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {200.F, 0.25F}})->commence());

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // drive the scroll to its limit, record where the clamp lands
    scroll->setScroll({0.F, 1'000'000'000.F});
    const float bottom = scroll->getCurrentScroll().y;
    EXPECT_GT(bottom, 0.F);

    // a stray reflow of just the content layer (a window-level reflow can do this) leaves the
    // content's stored box in a different state. re-clamping at the same offset must keep the
    // scroll exactly where it was: the cached overflow does not follow the transient box.
    auto inner = scroll->impl->children.at(0);
    g_positioner->position(inner, inner->impl->position);

    scroll->setScroll(scroll->getCurrentScroll());
    EXPECT_FLOAT_EQ(scroll->getCurrentScroll().y, bottom);

    root.reset();
}

// regression: scrolling down and then letting go snapped the content back to the top. when a
// content child schedules a reflow, the window repositions the inner content layer on its own,
// and the scroll offset used to live only in a one-shot pass over inner's children, so that
// bare reposition dropped it (content jumped to y=0 while currentScroll stayed at the bottom).
// the offset now rides on inner's own position, so a stray relayout keeps the content scrolled.
TEST(Element, scrollAreaContentKeepsOffsetAcrossStrayRelayout) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {200, 100}};

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->showScrollbar(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    for (int i = 0; i < 6; ++i)
        content->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 80}})->commence()); // 480 tall, overflow 380

    scroll->addChild(content);
    root->addChild(scroll);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // scroll to the bottom: the content layer rides up by the overflow
    scroll->setScroll({0.F, 1000.F});
    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    const float scrolled = content->impl->position.y;
    EXPECT_LT(scrolled, -300.F); // shifted up by the ~380 overflow, not sitting at the top

    // a bare reposition of the content layer, exactly what the window does when a content child
    // schedules a reflow (a hover color rebuild, the gesture ending). the offset must survive it.
    g_positioner->repositionNeeded(content);
    EXPECT_FLOAT_EQ(content->impl->position.y, scrolled); // not snapped back to 0

    root.reset();
}
