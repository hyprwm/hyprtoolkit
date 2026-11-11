#include <gtest/gtest.h>

#include <format>

#include <core/platforms/WaylandPlatform.hpp>
#include <window/WaylandWindow.hpp>

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

class CMouseRecorder : public CWaylandWindow {
  public:
    CMouseRecorder() : CWaylandWindow(SWindowCreationData{}) {
        ;
    }

    void mouseEnter(const Vector2D& local) override {
        m_log.emplace_back(std::format("enter {} {}", local.x, local.y));
    }

    void mouseMove(const Vector2D& local) override {
        m_log.emplace_back(std::format("move {} {}", local.x, local.y));
    }

    void mouseButton(const Input::eMouseButton button, bool state) override {
        m_log.emplace_back(std::format("{} {}", button == Input::MOUSE_BUTTON_LEFT ? "left" : "other", state ? "down" : "up"));
    }

    void mouseLeave() override {
        m_log.emplace_back("leave");
    }

    std::vector<std::string> m_log;
};

TEST(WaylandPlatform, touchDrivesLeftButton) {
    Tests::Tricks::createBackendSupport();

    CWaylandPlatform platform;
    auto             window = makeShared<CMouseRecorder>();

    platform.onTouchDown(window, 3, {10, 20});
    // a second finger while the first is held is ignored, down, motion and up alike
    platform.onTouchDown(window, 4, {50, 50});
    platform.onTouchMotion(4, {60, 60});
    platform.onTouchMotion(3, {15, 25});
    platform.onTouchUp(4);
    platform.onTouchUp(3);
    platform.onTouchMotion(3, {99, 99});

    const std::vector<std::string> EXPECTED = {"enter 10 20", "left down", "move 15 25", "left up", "leave"};
    EXPECT_EQ(window->m_log, EXPECTED);
}

TEST(WaylandPlatform, touchCancelReleasesAndFreesTheSlot) {
    Tests::Tricks::createBackendSupport();

    CWaylandPlatform platform;
    auto             window = makeShared<CMouseRecorder>();

    platform.onTouchDown(window, 1, {5, 5});
    platform.onTouchCancel();
    // the compositor sends no up for a cancelled point, a late one must not release twice
    platform.onTouchUp(1);
    platform.onTouchDown(window, 2, {7, 7});

    const std::vector<std::string> EXPECTED = {"enter 5 5", "left down", "left up", "leave", "enter 7 7", "left down"};
    EXPECT_EQ(window->m_log, EXPECTED);
}

TEST(WaylandPlatform, touchOnUnknownSurfaceIsIgnored) {
    Tests::Tricks::createBackendSupport();

    CWaylandPlatform platform;
    auto             window = makeShared<CMouseRecorder>();

    platform.onTouchDown(nullptr, 1, {5, 5});
    platform.onTouchDown(window, 2, {7, 7});
    platform.onTouchUp(2);

    const std::vector<std::string> EXPECTED = {"enter 7 7", "left down", "left up", "leave"};
    EXPECT_EQ(window->m_log, EXPECTED);
}
