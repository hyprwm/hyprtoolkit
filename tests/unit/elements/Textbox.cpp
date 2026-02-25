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