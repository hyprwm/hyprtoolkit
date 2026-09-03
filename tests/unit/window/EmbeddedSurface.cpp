#include <gtest/gtest.h>

#include "core/BackendContext.hpp"
#include "element/Element.hpp"
#include "hyprtoolkit/element/Textbox.hpp"
#include "window/EmbeddedSurface.hpp"

#include "../tricks/Tricks.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

class CEmbeddedTestElement final : public IElement {
  public:
    static SP<CEmbeddedTestElement> create(bool always = false) {
        auto element        = makeShared<CEmbeddedTestElement>(always);
        element->impl->self = element;
        return element;
    }

    void paint() override {
        ;
    }

    Vector2D size() override {
        return impl->position.size();
    }

    bool alwaysGetMouseInput() override {
        return m_always;
    }

    explicit CEmbeddedTestElement(bool always = false) : m_always(always) {}

  private:
    bool m_always = false;
};

class CEmbeddedSurfaceTest : public testing::Test {
  protected:
    void SetUp() override {
        if (!g_backendServices) {
            g_backendServices   = makeUnique<SBackendServices>(makeDefaultBackendServices());
            m_installedServices = true;
        }

        m_surface = makeShared<CEmbeddedSurface>();
        m_surface->setSelf(m_surface, nullptr);
        m_surface->open();
    }

    void TearDown() override {
        m_surface->invalidate();
        m_surface.reset();
        if (m_installedServices)
            g_backendServices.reset();
    }

    SP<CEmbeddedSurface> m_surface;
    bool                 m_installedServices = false;
};

TEST_F(CEmbeddedSurfaceTest, frameRequestsAreCoalesced) {
    int requests = 0;
    m_surface->IEmbeddedSurface::m_events.frameRequested.listenStatic([&requests] { requests++; });

    m_surface->resize({100, 50}, {200, 100}, 2.F);
    m_surface->damageEntire();

    EXPECT_EQ(requests, 1);
    EXPECT_EQ(m_surface->pixelSize(), Vector2D(200, 100));
    EXPECT_EQ(m_surface->scale(), 2.F);
}

TEST_F(CEmbeddedSurfaceTest, mouseInputCanBeDisabled) {
    const auto element = CEmbeddedTestElement::create();
    int        moves   = 0;
    element->setMouseMove([&moves](const Vector2D&) { ++moves; });

    element->setReceivesMouse(true);
    element->impl->m_externalEvents.mouseMove.emit({1, 1});
    EXPECT_EQ(moves, 1);
    EXPECT_TRUE(element->acceptsMouseInput());

    element->setReceivesMouse(false);
    element->impl->m_externalEvents.mouseMove.emit({2, 2});
    EXPECT_EQ(moves, 1);
    EXPECT_FALSE(element->acceptsMouseInput());
}

TEST_F(CEmbeddedSurfaceTest, mouseAxisPreservesDelta) {
    const auto element  = CEmbeddedTestElement::create();
    float      received = 0.F;
    element->setReceivesMouse(true);
    element->setMouseAxis([&received](Input::eAxisAxis axis, float delta) {
        EXPECT_EQ(axis, Input::AXIS_AXIS_VERTICAL);
        received = delta;
    });

    element->impl->m_externalEvents.mouseAxis.emit(Input::AXIS_AXIS_VERTICAL, -2.75F);

    EXPECT_FLOAT_EQ(received, -2.75F);
}

TEST_F(CEmbeddedSurfaceTest, alwaysHoveredElementReceivesPointerEventsOnce) {
    const auto element = CEmbeddedTestElement::create(true);
    int        moves = 0, buttons = 0, axes = 0;
    element->setReceivesMouse(true);
    element->setMouseMove([&moves](const Vector2D&) { ++moves; });
    element->setMouseButton([&buttons](Input::eMouseButton, bool) { ++buttons; });
    element->setMouseAxis([&axes](Input::eAxisAxis, float) { ++axes; });
    element->reposition({10, 10, 20, 20});
    m_surface->rootElement()->addChild(element);

    m_surface->pointerEnter({15, 15});
    m_surface->pointerMotion({16, 16});
    m_surface->pointerButton(Input::MOUSE_BUTTON_LEFT, true);
    m_surface->pointerAxis(Input::AXIS_AXIS_VERTICAL, 1.F);

    EXPECT_EQ(moves, 2);
    EXPECT_EQ(buttons, 1);
    EXPECT_EQ(axes, 1);
}

TEST_F(CEmbeddedSurfaceTest, textboxFocusCanBeCleared) {
    Tests::Tricks::createBackendSupport();

    const auto textbox = CTextboxBuilder::begin()->commence();
    m_surface->rootElement()->addChild(textbox);

    textbox->focus();
    EXPECT_EQ(m_surface->m_keyboardFocus.lock().get(), textbox.get());

    textbox->focus(false);
    EXPECT_FALSE(m_surface->m_keyboardFocus);
}

TEST_F(CEmbeddedSurfaceTest, clearingTextboxFocusDoesNotAffectAnotherTextbox) {
    Tests::Tricks::createBackendSupport();

    const auto first  = CTextboxBuilder::begin()->commence();
    const auto second = CTextboxBuilder::begin()->commence();
    m_surface->rootElement()->addChild(first);
    m_surface->rootElement()->addChild(second);

    second->focus();
    first->focus(false);
    EXPECT_EQ(m_surface->m_keyboardFocus.lock().get(), second.get());

    second->focus();
    EXPECT_EQ(m_surface->m_keyboardFocus.lock().get(), second.get());
}

TEST_F(CEmbeddedSurfaceTest, textboxCallbackWaitsForIMCommit) {
    Tests::Tricks::createBackendSupport();

    size_t      edits = 0;
    std::string editedText;
    const auto  textbox = CTextboxBuilder::begin()
                              ->onTextEdited([&](SP<CTextboxElement>, const std::string& text) {
                                 ++edits;
                                 editedText = text;
                              })
                              ->commence();
    m_surface->rootElement()->addChild(textbox);
    textbox->focus();

    m_surface->imCommit("é");
    EXPECT_EQ(edits, 0);

    m_surface->imApply();
    EXPECT_EQ(edits, 1);
    EXPECT_EQ(editedText, "é");
}

TEST_F(CEmbeddedSurfaceTest, touchIsCapturedByInitialTarget) {
    const auto element = CEmbeddedTestElement::create();
    element->setReceivesTouch(true);
    element->reposition({10, 10, 20, 20});
    m_surface->rootElement()->addChild(element);

    std::vector<Input::STouchEvent> motions;
    element->setTouchMotion([&motions](const Input::STouchEvent& event) { motions.emplace_back(event); });

    m_surface->touchDown(7, {15, 15}, 10);
    m_surface->touchMotion(7, {80, 40}, 20);
    m_surface->touchUp(7, 30);

    ASSERT_EQ(motions.size(), 1);
    EXPECT_EQ(motions.front().id, 7);
    EXPECT_EQ(motions.front().local, Vector2D(70, 30));
    EXPECT_EQ(motions.front().timeMs, 20);
}

TEST_F(CEmbeddedSurfaceTest, cancelledPrimaryTouchDoesNotClick) {
    const auto element = CEmbeddedTestElement::create();
    element->setReceivesMouse(true);
    element->reposition({10, 10, 20, 20});
    m_surface->rootElement()->addChild(element);

    int buttons = 0;
    int leaves  = 0;
    element->setMouseButton([&buttons](Input::eMouseButton button, bool pressed) { buttons++; });
    element->setMouseLeave([&leaves] { leaves++; });

    m_surface->touchDown(4, {15, 15}, 10);
    m_surface->touchCancel(4, 20);

    EXPECT_EQ(buttons, 0);
    EXPECT_EQ(leaves, 1);
}

TEST_F(CEmbeddedSurfaceTest, primaryTouchTapKeepsInitialPointerTarget) {
    const auto first  = CEmbeddedTestElement::create();
    const auto second = CEmbeddedTestElement::create();
    first->setReceivesMouse(true);
    second->setReceivesMouse(true);
    first->reposition({10, 10, 20, 20});
    second->reposition({50, 10, 20, 20});
    m_surface->rootElement()->addChild(first);
    m_surface->rootElement()->addChild(second);

    int firstButtons  = 0;
    int secondButtons = 0;
    first->setMouseButton([&firstButtons](Input::eMouseButton button, bool pressed) { firstButtons++; });
    second->setMouseButton([&secondButtons](Input::eMouseButton button, bool pressed) { secondButtons++; });

    m_surface->touchDown(4, {15, 15}, 10);
    m_surface->touchMotion(4, {55, 15}, 20);
    m_surface->touchUp(4, 30);

    EXPECT_EQ(firstButtons, 2);
    EXPECT_EQ(secondButtons, 0);
}

TEST_F(CEmbeddedSurfaceTest, invalidatedSurfaceIgnoresHostCalls) {
    m_surface->invalidate();

    m_surface->resize({100, 50}, {200, 100}, 2.F);
    m_surface->pointerMotion({10, 10});
    m_surface->keyboardKey({});
    m_surface->touchDown(1, {10, 10}, 0);

    EXPECT_EQ(m_surface->pixelSize(), Vector2D{});
}
