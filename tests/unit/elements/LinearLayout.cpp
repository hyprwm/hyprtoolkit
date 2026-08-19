#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <element/Element.hpp>
#include <element/columnLayout/ColumnLayout.hpp>
#include <element/rowLayout/RowLayout.hpp>
#include <element/text/Text.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <layout/Positioner.hpp>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, columnTextWrapsAndExpandsAutoHeight) {
    Tests::Tricks::createBackendSupport();

    auto column = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {100.F, 1.F}})->commence();
    auto text   = CTextBuilder::begin()->text("This text is long enough to wrap onto several lines in a narrow column")->commence();
    column->setMargin(5.F);
    column->addChild(text);

    const SP<IElement> COLUMN_ELEMENT   = column;
    const auto         NATURAL          = text->preferredSize({}).value_or(Vector2D{});
    const auto         WRAPPED          = text->preferredSize({90.F, -1.F}).value_or(Vector2D{});
    const auto         COLUMN_PREFERRED = COLUMN_ELEMENT->preferredSize({500.F, 500.F}).value_or(Vector2D{});
    ASSERT_FLOAT_EQ(COLUMN_PREFERRED.y, WRAPPED.y + 10.F);
    ASSERT_GT(WRAPPED.y, NATURAL.y);

    const CBox BOX{{}, {100.F, COLUMN_PREFERRED.y}};
    g_positioner->position(column, BOX);

    const auto CONSTRAINT = text->m_impl->lastMaxSize;
    EXPECT_FLOAT_EQ(CONSTRAINT.x, 90.F);
    EXPECT_LT(CONSTRAINT.y, 0.F);
    EXPECT_GT(text->m_impl->getTextSizePreferred().y, NATURAL.y);

    g_positioner->position(column, BOX);
    EXPECT_EQ(text->m_impl->lastMaxSize, CONSTRAINT);
    EXPECT_FLOAT_EQ(text->impl->position.h, WRAPPED.y);
}

TEST(Element, rowTextEllipsizesOnlyWhenSqueezed) {
    Tests::Tricks::createBackendSupport();

    auto row  = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {80.F, 1.F}})->commence();
    auto text = CTextBuilder::begin()->text("This row text is much wider than eighty pixels")->commence();
    row->addChild(text);

    const SP<IElement> ROW_ELEMENT   = row;
    const auto         NATURAL       = text->preferredSize({}).value_or(Vector2D{});
    const auto         ROW_PREFERRED = ROW_ELEMENT->preferredSize({80.F, 100.F}).value_or(Vector2D{});
    ASSERT_FLOAT_EQ(ROW_PREFERRED.y, NATURAL.y);

    const CBox BOX{{}, {80.F, ROW_PREFERRED.y}};
    g_positioner->position(row, BOX);

    const auto CONSTRAINT = text->m_impl->lastMaxSize;
    EXPECT_GT(CONSTRAINT.x, 0.F);
    EXPECT_FLOAT_EQ(CONSTRAINT.y, ROW_PREFERRED.y);
    EXPECT_LT(text->m_impl->getTextSizePreferred().x, NATURAL.x);
    EXPECT_FLOAT_EQ(text->m_impl->getTextSizePreferred().y, NATURAL.y);

    g_positioner->position(row, BOX);
    EXPECT_EQ(text->m_impl->lastMaxSize, CONSTRAINT);
}

TEST(Element, rowUnsqueezedLabelsRemainNatural) {
    Tests::Tricks::createBackendSupport();

    auto                          row = CRowLayoutBuilder::begin()->gap(8)->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {300.F, 1.F}})->commence();
    std::vector<SP<CTextElement>> labels;
    for (const auto* label : {"first", "second", "third"}) {
        row->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {12.F, 12.F}})->commence());
        labels.emplace_back(CTextBuilder::begin()->text(std::string{label})->commence());
        row->addChild(labels.back());
    }

    const SP<IElement> ROW_ELEMENT   = row;
    const auto         ROW_PREFERRED = ROW_ELEMENT->preferredSize({300.F, 100.F}).value_or(Vector2D{});
    const CBox         BOX{{}, {300.F, ROW_PREFERRED.y}};
    g_positioner->position(row, BOX);

    for (const auto& label : labels) {
        const auto NATURAL = label->preferredSize({}).value_or(Vector2D{});
        EXPECT_LT(label->m_impl->lastMaxSize.x, 0.F);
        EXPECT_EQ(label->m_impl->getTextSizePreferred(), NATURAL);
        EXPECT_FALSE(label->impl->failedPositioning);
    }

    g_positioner->position(row, BOX);
    for (const auto& label : labels)
        EXPECT_LT(label->m_impl->lastMaxSize.x, 0.F);
}

TEST(Element, growSpacerKeepsLeadingTextNatural) {
    Tests::Tricks::createBackendSupport();

    auto row     = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {200.F, 1.F}})->commence();
    auto text    = CTextBuilder::begin()->text("Label")->commence();
    auto spacer  = CNullBuilder::begin()->commence();
    auto control = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50.F, 20.F}})->commence();
    spacer->setGrow(true);
    row->addChild(text);
    row->addChild(spacer);
    row->addChild(control);

    g_positioner->position(row, {{}, {200.F, 20.F}});

    EXPECT_LT(text->m_impl->lastMaxSize.x, 0.F);
    EXPECT_EQ(text->m_impl->getTextSizePreferred(), text->preferredSize({}).value_or(Vector2D{}));
    EXPECT_FLOAT_EQ(control->impl->position.x + control->impl->position.w, 200.F);
}

TEST(Element, linearLayoutTextConstraintsPreservePercentageChildren) {
    Tests::Tricks::createBackendSupport();

    auto row      = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {200.F, 40.F}})->commence();
    auto rowChild = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 20.F}})->commence();
    row->addChild(rowChild);
    g_positioner->position(row, {{}, {200.F, 40.F}});
    EXPECT_FLOAT_EQ(rowChild->impl->position.w, 100.F);

    auto column      = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {40.F, 200.F}})->commence();
    auto columnChild = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {20.F, 0.5F}})->commence();
    column->addChild(columnChild);
    g_positioner->position(column, {{}, {40.F, 200.F}});
    EXPECT_FLOAT_EQ(columnChild->impl->position.h, 100.F);
}
