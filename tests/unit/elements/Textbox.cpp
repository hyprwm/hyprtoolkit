#include <gtest/gtest.h>

#include <element/textbox/Textbox.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <layout/Positioner.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"
#include "hyprtoolkit/core/Input.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

TEST(Element, textboxNavigation) {
    Tests::Tricks::createBackendSupport();

    auto textbox = CTextboxBuilder::begin()->defaultText("is 🧑‍🌾 the same as 🧑🌾?")->commence();

    EXPECT_EQ(textbox->cursorPos(), 0);
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    EXPECT_EQ(textbox->cursorPos(), 1);
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    EXPECT_EQ(textbox->cursorPos(), 7);

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_End});
    EXPECT_EQ(textbox->cursorPos(), 36);

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Left});
    EXPECT_EQ(textbox->cursorPos(), 35);
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Left});
    EXPECT_EQ(textbox->cursorPos(), 31);

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Left, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->cursorPos(), 27);
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Left, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->cursorPos(), 24);

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Home});
    EXPECT_EQ(textbox->cursorPos(), 0);

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->cursorPos(), 2);
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->cursorPos(), 14);

    textbox.reset();
}

TEST(Element, textboxEditing) {
    Tests::Tricks::createBackendSupport();

    auto textbox = CTextboxBuilder::begin()->defaultText("is 🧑‍🌾 the same as 🧑🌾?")->commence();

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete});
    EXPECT_EQ(textbox->currentText(), "🌾 the same as 🧑🌾?");

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->currentText(), " the same as 🧑🌾?");
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Delete, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->currentText(), " same as 🧑🌾?");

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_End});

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_BackSpace});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_BackSpace});
    EXPECT_EQ(textbox->currentText(), " same as 🧑");

    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_BackSpace, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->currentText(), " same as ");
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_BackSpace, .modMask = Input::HT_MODIFIER_CTRL});
    EXPECT_EQ(textbox->currentText(), " same ");

    textbox.reset();
}

TEST(Element, textBoxNewLines) {
    Tests::Tricks::createBackendSupport();

    auto textbox = CTextboxBuilder::begin()->defaultText("I am on the first line!")->multiline(true)->commence();
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_End});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.utf8 = "\r"});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.utf8 = "\n"});
    EXPECT_EQ(textbox->currentText(), "I am on the first line!\r\n");
    textbox.reset();

    textbox = CTextboxBuilder::begin()->defaultText("I am on the first line!")->multiline(false)->commence();
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_End});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.utf8 = "\r"});
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.utf8 = "\n"});
    EXPECT_EQ(textbox->currentText(), "I am on the first line!");
    textbox.reset();
}

// a single-line textbox vertically centers its text, so the selection highlight must be
// centered too. regression test for the highlight sitting above the glyphs.
TEST(Element, textboxSingleLineSelectionVCentered) {
    Tests::Tricks::createBackendSupport();

    const CBox area = {{}, {300, 40}};

    auto       root    = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {area.w, area.h}})->commence();
    auto       textbox = CTextboxBuilder::begin()->defaultText("hunter2pw")->multiline(false)->commence();
    root->addChild(textbox);

    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // select everything, then re-run the positioner so the freshly added highlight gets a box
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_A, .modMask = Input::HT_MODIFIER_CTRL});
    g_positioner->position(root, area);
    g_positioner->positionChildren(root);

    // walk to the highlight: textbox -> bg -> bgInnerCont -> selectBgCont -> selectBg.
    // selectBgCont is the inner child that itself holds children (the text element has none).
    auto         bg      = textbox->impl->children.at(0);
    auto         bgInner = bg->impl->children.at(0);

    SP<IElement> selectCont;
    for (auto& c : bgInner->impl->children) {
        if (!c->impl->children.empty())
            selectCont = c;
    }
    ASSERT_TRUE(selectCont);
    ASSERT_FALSE(selectCont->impl->children.empty());

    const auto selBox = selectCont->impl->children.at(0)->impl->position;
    const auto tbBox  = textbox->impl->position;

    EXPECT_GT(selBox.w, 0.F); // the highlight spans the selected text
    EXPECT_GT(selBox.h, 0.F);
    // the highlight's vertical center lands on the textbox's vertical center
    EXPECT_NEAR(selBox.y + selBox.h / 2.F, tbBox.y + tbBox.h / 2.F, 1.5F);

    textbox.reset();
}