#include <gtest/gtest.h>

#include <array>

#include <core/platforms/WaylandPlatform.hpp>
#include <hyprtoolkit/element/Textbox.hpp>
#include <window/WaylandWindow.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;

TEST(WaylandPlatform, homeEndReachTextbox) {
    Tests::Tricks::createBackendSupport();

    CWaylandPlatform platform;
    auto&            seat = platform.m_waylandState.seatState;
    seat.repeatRate       = 0;
    seat.xkbContext       = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    ASSERT_NE(seat.xkbContext, nullptr);

    const xkb_rule_names names = {.layout = "us"};
    seat.xkbKeymap             = xkb_keymap_new_from_names(seat.xkbContext, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    ASSERT_NE(seat.xkbKeymap, nullptr);
    seat.xkbState = xkb_state_new(seat.xkbKeymap);
    ASSERT_NE(seat.xkbState, nullptr);

    auto       window = makeShared<CWaylandWindow>(SWindowCreationData{});

    const auto TEXT_CONTENT = "should not change";

    auto       textbox        = CTextboxBuilder::begin()->defaultText(TEXT_CONTENT)->multiline(false)->commence();
    window->m_keyboardFocus   = textbox;
    platform.m_keyboardWindow = window;

    size_t keyPresses = 0;
    window->m_events.keyboardKey.listenStatic([&](Input::SKeyboardKeyEvent e) {
        if (!e.down)
            return;
        ++keyPresses;
        EXPECT_TRUE(e.utf8.empty());
    });

    const std::array<const char*, 4> keys = {"END", "HOME", "KP1", "KP7"};
    for (size_t i = 0; i < keys.size(); ++i) {
        SCOPED_TRACE(keys[i]);
        const auto keycode = xkb_keymap_key_by_name(seat.xkbKeymap, keys[i]);
        ASSERT_NE(keycode, XKB_KEYCODE_INVALID);

        const int XKB_WAYLAND_KEYCODE_DIFF = 8;

        platform.onKey(keycode - XKB_WAYLAND_KEYCODE_DIFF, true);
        EXPECT_EQ(keyPresses, i + 1);
        // The test alternates between END and HOME
        const int expectedPosition = i % 2 == 0 ? textbox->currentText().size() : 0;
        EXPECT_EQ(textbox->cursorPos(), expectedPosition);
        EXPECT_EQ(seat.repeatKeyEvent.xkbKeysym, xkb_state_key_get_one_sym(seat.xkbState, keycode));
        EXPECT_EQ(textbox->currentText(), TEXT_CONTENT);
        platform.onKey(keycode - XKB_WAYLAND_KEYCODE_DIFF, false);
        EXPECT_TRUE(seat.pressedKeys.empty());
    }
}

TEST(WaylandPlatform, keyboardEnterReplacesPressedKeys) {
    CWaylandPlatform platform;

    platform.m_waylandState.seatState.pressedKeys  = {1, 2, 3};
    platform.m_waylandState.seatState.currentLayer = 2;
    platform.m_currentMods                         = Input::HT_MODIFIER_CTRL;

    std::array<uint32_t, 2> keysData = {28, 42};
    wl_array                keys     = {
        .size  = keysData.size() * sizeof(uint32_t),
        .alloc = keysData.size() * sizeof(uint32_t),
        .data  = keysData.data(),
    };

    platform.onKeyboardEnter(nullptr, &keys);

    EXPECT_EQ(platform.m_waylandState.seatState.pressedKeys, std::vector<uint32_t>({28, 42}));
    EXPECT_EQ(platform.m_waylandState.seatState.currentLayer, 0);
    EXPECT_EQ(platform.m_currentMods, 0);
}

TEST(WaylandPlatform, keyboardLeaveClearsKeyboardState) {
    CWaylandPlatform platform;

    platform.m_waylandState.seatState.pressedKeys    = {28};
    platform.m_waylandState.seatState.currentLayer   = 2;
    platform.m_waylandState.seatState.repeatKeyEvent = {
        .xkbKeysym = XKB_KEY_Return,
        .down      = true,
        .repeat    = true,
    };
    platform.m_currentMods = Input::HT_MODIFIER_SHIFT;

    platform.onKeyboardLeave();

    EXPECT_TRUE(platform.m_waylandState.seatState.pressedKeys.empty());
    EXPECT_EQ(platform.m_waylandState.seatState.currentLayer, 0);
    EXPECT_FALSE(platform.m_waylandState.seatState.repeatKeyEvent.down);
    EXPECT_FALSE(platform.m_waylandState.seatState.repeatKeyEvent.repeat);
    EXPECT_EQ(platform.m_waylandState.seatState.repeatKeyEvent.xkbKeysym, 0);
    EXPECT_EQ(platform.m_currentMods, 0);
}
