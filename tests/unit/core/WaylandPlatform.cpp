#include <gtest/gtest.h>

#include <array>

#include <core/platforms/WaylandPlatform.hpp>
#include <xkbcommon/xkbcommon-keysyms.h>

using namespace Hyprtoolkit;

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
