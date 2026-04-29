#include <gtest/gtest.h>

#include <layout/Positioner.hpp>
#include <element/Element.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Positioner, main) {
    auto root = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1000, 1000}})->commence();
    root->setMargin(4);
    auto rowLayoutMain = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(10)->commence();
    root->addChild(rowLayoutMain);

    auto columnLayoutLeft = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(10)->commence();
    auto childLeft        = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 200}})->commence();

    auto nullRight = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    nullRight->setGrow(true);

    auto columnLayoutRight = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(10)->commence();
    auto childRight        = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 300}})->commence();
    auto childRight2       = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 300}})->commence();

    childRight->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    childRight2->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    columnLayoutRight->setPositionMode(IElement::HT_POSITION_ABSOLUTE);

    nullRight->addChild(columnLayoutRight);
    rowLayoutMain->addChild(columnLayoutLeft);
    rowLayoutMain->addChild(nullRight);

    columnLayoutLeft->addChild(childLeft);

    columnLayoutRight->addChild(childRight);
    columnLayoutRight->addChild(childRight2);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(root->impl->position, CBox(4, 4, 992, 992));
    EXPECT_EQ(rowLayoutMain->impl->position, CBox(4, 4, 992, 992));
    EXPECT_EQ(columnLayoutLeft->impl->position, CBox(4, 4, 100, 992));
    EXPECT_EQ(columnLayoutRight->impl->position, CBox(114, 4, 200, 610));
    EXPECT_EQ(childRight2->impl->position, CBox(114, 314, 200, 300));
}

TEST(Positioner, align) {
    auto root  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1000, 1000}})->commence();
    auto child = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {10, 10}})->commence();
    child->setPositionMode(IElement::HT_POSITION_ABSOLUTE);

    root->addChild(child);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(0, 0, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(0, 495, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(495, 495, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_VCENTER, false);
    child->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(495, 0, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, false);
    child->setPositionFlag(IElement::HT_POSITION_FLAG_BOTTOM, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(495, 990, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_HCENTER, false);
    child->setPositionFlag(IElement::HT_POSITION_FLAG_RIGHT, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(990, 990, 10, 10));

    child->setPositionFlag(IElement::HT_POSITION_FLAG_RIGHT, false);
    child->setPositionFlag(IElement::HT_POSITION_FLAG_LEFT, true);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(child->impl->position, CBox(0, 990, 10, 10));
}

// fixed-size sibling + setGrow sibling should split parent height: fixed gets its size, grow gets the rest
TEST(Positioner, columnLayoutGrowFillsRemaining) {
    auto root = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1000, 1000}})->commence();

    auto col = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(0)->commence();
    root->addChild(col);

    auto fixed = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1000, 30}})->commence();
    auto grow  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    grow->setGrow(true);

    col->addChild(fixed);
    col->addChild(grow);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(fixed->impl->position.h, 30);
    EXPECT_EQ(grow->impl->position.h, 970);
    EXPECT_EQ(grow->impl->position.y, 30);
}

TEST(Positioner, rowLayoutGrowFillsRemaining) {
    auto root = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1000, 1000}})->commence();

    auto row = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(0)->commence();
    root->addChild(row);

    auto fixed = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {30, 1000}})->commence();
    auto grow  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    grow->setGrow(true);

    row->addChild(fixed);
    row->addChild(grow);

    g_positioner->position(root, {{}, {1000, 1000}});
    g_positioner->positionChildren(root);

    EXPECT_EQ(fixed->impl->position.w, 30);
    EXPECT_EQ(grow->impl->position.w, 970);
    EXPECT_EQ(grow->impl->position.x, 30);
}

// when shrinking previous siblings frees up some space, the running `used`
// must reflect that before deciding whether/how to expand the last visible
// sibling. before the fix, used was stale (still tracked the pre-shrink sum)
// so the expand-last math used the wrong gap. with non-shrinkable b and
// failed c, b should remain exactly at its absolute width and not balloon
// from a stale-used arithmetic path.
TEST(Positioner, rowLayoutShrinkRecomputesUsedBeforeExpand) {
    auto root = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence();

    auto row = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(0)->commence();
    root->addChild(row);

    // a: shrinkable to 0 (PERCENT, no min)
    // b: fixed 50 wide (ABSOLUTE; min == max == 50)
    // c: 100 wide; cannot fit at all in the remainder, will be disabled
    auto a = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto b = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 100}})->commence();
    auto c = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence();

    row->addChild(a);
    row->addChild(b);
    row->addChild(c);

    g_positioner->position(root, {{}, {100, 100}});
    g_positioner->positionChildren(root);

    // b must respect its absolute size: no stale-used arithmetic should
    // grow it past the declared 50.
    EXPECT_LE(b->impl->position.w, 50);
    // total realised must not exceed the parent
    const auto totalRealised = a->impl->position.w + b->impl->position.w + c->impl->position.w;
    EXPECT_LE(totalRealised, 100);
}

// overflow path: a child that doesn't fit must not size_t-underflow when
// shrinking earlier siblings. before the fix, sizes[j] -= needs - sizes[j]
// could wrap to ~2^64 and the layout would explode.
TEST(Positioner, rowLayoutOverflowShrinkDoesNotUnderflow) {
    auto root = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200, 100}})->commence();

    auto row = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->gap(0)->commence();
    root->addChild(row);

    // three 100-px children, parent only has 200, last one cannot fit
    auto a = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence();
    auto b = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence();
    auto c = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence();

    row->addChild(a);
    row->addChild(b);
    row->addChild(c);

    g_positioner->position(root, {{}, {200, 100}});
    g_positioner->positionChildren(root);

    // every realised width must be sane (<= parent width). underflow would
    // produce nonsense in the gigabytes.
    EXPECT_LE(a->impl->position.w, 200);
    EXPECT_LE(b->impl->position.w, 200);
    EXPECT_LE(c->impl->position.w, 200);
}