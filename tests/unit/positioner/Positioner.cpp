
// FIXME: These tests aren't very comprehensive.

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