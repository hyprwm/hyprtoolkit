#include <gtest/gtest.h>

#include <element/textbox/Textbox.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../tricks/Tricks.hpp"
#include "element/Element.hpp"
#include "hyprtoolkit/core/Input.hpp"

using namespace Hyprtoolkit;

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

// imDeleteSurroundingText models the text-input-v3 delete_surrounding_text
// action: chars before/after the cursor get removed, then a commit/preedit
// can land on the freed space. before/after are byte counts, clamped to the
// available text on each side.
TEST(Element, textboxImDeleteSurroundingText) {
    Tests::Tricks::createBackendSupport();

    // delete two chars to the left of the cursor
    auto textbox = CTextboxBuilder::begin()->defaultText("hello world")->commence();
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_End});
    EXPECT_EQ(textbox->cursorPos(), 11);
    static_cast<IElement*>(textbox.get())->imDeleteSurroundingText(2, 0);
    EXPECT_EQ(textbox->currentText(), "hello wor");
    EXPECT_EQ(textbox->cursorPos(), 9);
    textbox.reset();

    // delete one char on each side of the cursor
    textbox = CTextboxBuilder::begin()->defaultText("abcde")->commence();
    for (int i = 0; i < 3; ++i)
        textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    EXPECT_EQ(textbox->cursorPos(), 3);
    static_cast<IElement*>(textbox.get())->imDeleteSurroundingText(1, 1);
    EXPECT_EQ(textbox->currentText(), "abe");
    EXPECT_EQ(textbox->cursorPos(), 2);
    textbox.reset();

    // before and after both clamp to available text
    textbox = CTextboxBuilder::begin()->defaultText("hi")->commence();
    textbox->impl->m_externalEvents.key.emit(Input::SKeyboardKeyEvent{.xkbKeysym = XKB_KEY_Right});
    EXPECT_EQ(textbox->cursorPos(), 1);
    static_cast<IElement*>(textbox.get())->imDeleteSurroundingText(100, 100);
    EXPECT_EQ(textbox->currentText(), "");
    EXPECT_EQ(textbox->cursorPos(), 0);
    textbox.reset();

    // (0, 0) is a no-op
    textbox = CTextboxBuilder::begin()->defaultText("untouched")->commence();
    static_cast<IElement*>(textbox.get())->imDeleteSurroundingText(0, 0);
    EXPECT_EQ(textbox->currentText(), "untouched");
    textbox.reset();
}