#include <gtest/gtest.h>

#include "core/BackendContext.hpp"
#include "core/InternalBackend.hpp"
#include "element/Element.hpp"
#include "hyprtoolkit/core/Timer.hpp"
#include "hyprtoolkit/element/Textbox.hpp"
#include "hyprtoolkit/element/ScrollArea.hpp"
#include "hyprtoolkit/element/ColumnLayout.hpp"
#include "hyprtoolkit/element/Null.hpp"
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

// only hands out timers that never fire, which is all tooltips need
class CTimerOnlyBackend final : public IBackend {
  public:
    void destroy() override {
        ;
    }

    void setLogFn(LogFn&& fn) override {
        ;
    }

    void addFd(int fd, std::function<void()>&& callback) override {
        ;
    }

    void removeFd(int fd) override {
        ;
    }

    SP<ISystemIconFactory> systemIcons() override {
        return nullptr;
    }

    ASP<CTimer> addTimer(const TimerDuration& timeout, std::function<void(ASP<CTimer> self, void* data)> cb, void* data, bool force) override {
        return makeAtomicShared<CTimer>(timeout, std::move(cb), data, force);
    }

    void addIdle(const std::function<void()>& fn) override {
        ;
    }

    void enterLoop() override {
        ;
    }

    SP<CPalette> getPalette() override {
        return nullptr;
    }

    std::vector<SP<IOutput>> getOutputs() override {
        return {};
    }

    std::expected<SP<ISessionLockState>, eSessionLockError> aquireSessionLock() override {
        return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);
    }
};

class CEmbeddedSurfaceTooltipTest : public CEmbeddedSurfaceTest {
  protected:
    void SetUp() override {
        CEmbeddedSurfaceTest::SetUp();
        g_backend = makeShared<CTimerOnlyBackend>();
    }

    void TearDown() override {
        CEmbeddedSurfaceTest::TearDown();
        g_backend.reset();
    }
};

// Hovers A, moves onto the adjacent B, then moves within B. Returns the enter / leave log.
static std::vector<std::string> hoverAcross(SP<CEmbeddedSurface> surface, bool firstAlways, bool secondAlways) {
    // the elements outlive this function, so the log has to as well
    const auto log = makeShared<std::vector<std::string>>();

    const auto track = [&surface, &log](SP<CEmbeddedTestElement> el, const std::string& name, const CBox& box) {
        el->setReceivesMouse(true);
        el->setMouseEnter([log, name](const Vector2D&) { log->emplace_back(name + " enter"); });
        el->setMouseLeave([log, name] { log->emplace_back(name + " leave"); });
        el->reposition(box);
        surface->rootElement()->addChild(el);
    };

    track(CEmbeddedTestElement::create(firstAlways), "A", {10, 10, 20, 20});
    track(CEmbeddedTestElement::create(secondAlways), "B", {30, 10, 20, 20});

    surface->pointerEnter({15, 15});
    surface->pointerMotion({35, 15});
    surface->pointerMotion({40, 15});

    return *log;
}

TEST_F(CEmbeddedSurfaceTest, frameRequestsAreCoalesced) {
    int requests = 0;
    m_surface->IEmbeddedSurface::m_events.frameRequested.listenStatic([&requests] { requests++; });

    m_surface->resize({100, 50}, {200, 100}, 2.F);
    m_surface->damageEntire();

    EXPECT_EQ(requests, 1);
    EXPECT_EQ(m_surface->pixelSize(), Vector2D(200, 100));
    EXPECT_EQ(m_surface->scale(), 2.F);
}

TEST_F(CEmbeddedSurfaceTest, rootReflowPreservesConfiguredBounds) {
    const auto child = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    m_surface->rootElement()->addChild(child);

    for (const float scale : {1.F, 1.25F, 4.F / 3.F, 1.5F, 2.F}) {
        for (const auto size : {Vector2D{400, 260}, Vector2D{401, 259}, Vector2D{398, 261}}) {
            const auto pixels = (size * scale).floor();
            // Embedded hosts supply logical bounds explicitly; Wayland uses the drawable extent.
            for (const auto logical : {size, pixels / scale}) {
                SCOPED_TRACE(::testing::Message() << scale << ", " << logical.x << "x" << logical.y);
                m_surface->resize(logical, pixels, scale);
                EXPECT_EQ(child->size(), logical);

                for (int i = 0; i < 3; ++i) {
                    m_surface->rootElement()->forceReposition();
                    EXPECT_EQ(m_surface->rootElement()->size(), logical);
                    EXPECT_EQ(child->size(), logical);
                    if (logical != pixels / scale)
                        continue;
                    EXPECT_EQ((CBox{{}, child->size()}.scale(scale).round().size()), pixels);
                }
            }
        }
    }
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

TEST_F(CEmbeddedSurfaceTest, hoverLeavesBeforeEntering) {
    EXPECT_EQ(hoverAcross(m_surface, false, false), (std::vector<std::string>{"A enter", "A leave", "B enter"}));
}

TEST_F(CEmbeddedSurfaceTest, alwaysHoverLeavesBeforeEntering) {
    for (const auto& [firstAlways, secondAlways] : {std::pair{true, false}, std::pair{false, true}, std::pair{true, true}}) {
        SCOPED_TRACE(::testing::Message() << "A always: " << firstAlways << ", B always: " << secondAlways);

        // start every combination from a clean, unhovered surface
        m_surface->pointerLeave();
        m_surface->rootElement()->clearChildren();

        EXPECT_EQ(hoverAcross(m_surface, firstAlways, secondAlways), (std::vector<std::string>{"A enter", "A leave", "B enter"}));
    }
}

TEST_F(CEmbeddedSurfaceTest, windowSeesMouseButtonsWhateverIsHovered) {
    using SButton = std::pair<Input::eMouseButton, bool>;

    std::vector<SButton> windowButtons;
    m_surface->IWindow::m_events.mouseButton.listenStatic([&windowButtons](Input::eMouseButton button, bool down) { windowButtons.emplace_back(button, down); });

    int        elementButtons = 0;
    const auto element        = CEmbeddedTestElement::create();
    element->setReceivesMouse(true);
    element->setMouseButton([&elementButtons](Input::eMouseButton, bool) { ++elementButtons; });
    element->reposition({10, 10, 20, 20});
    m_surface->rootElement()->addChild(element);

    // nothing under the cursor
    m_surface->pointerEnter({50, 50});
    m_surface->pointerButton(Input::MOUSE_BUTTON_BACK, true);
    m_surface->pointerButton(Input::MOUSE_BUTTON_BACK, false);

    // over an element, which still gets it as well
    m_surface->pointerMotion({15, 15});
    m_surface->pointerButton(Input::MOUSE_BUTTON_FORWARD, true);

    EXPECT_EQ(windowButtons, (std::vector<SButton>{{Input::MOUSE_BUTTON_BACK, true}, {Input::MOUSE_BUTTON_BACK, false}, {Input::MOUSE_BUTTON_FORWARD, true}}));
    EXPECT_EQ(elementButtons, 1);
}

TEST_F(CEmbeddedSurfaceTooltipTest, tooltipComesFromParentOfHoveredElement) {
    const auto parent = CEmbeddedTestElement::create();
    const auto child  = CEmbeddedTestElement::create();
    parent->setTooltip("parent tooltip");
    child->setReceivesMouse(true);
    parent->reposition({10, 10, 40, 40});
    child->reposition({20, 20, 20, 20});
    parent->addChild(child);
    m_surface->rootElement()->addChild(parent);

    m_surface->pointerEnter({25, 25});

    ASSERT_TRUE(m_surface->m_mainHoverElement);
    EXPECT_EQ(m_surface->m_mainHoverElement->m_el.lock().get(), child.get());
    EXPECT_EQ(m_surface->m_tooltip.hoverElement.lock().get(), parent.get());
    EXPECT_TRUE(m_surface->m_tooltip.hoverTooltipTimer);

    m_surface->pointerLeave();

    EXPECT_FALSE(m_surface->m_tooltip.hoverElement);
    EXPECT_FALSE(m_surface->m_tooltip.hoverTooltipTimer);
}

TEST_F(CEmbeddedSurfaceTooltipTest, tooltipDoesNotCaptureMouseInput) {
    std::vector<std::string> log;

    const auto               parent = CEmbeddedTestElement::create();
    const auto               child  = CEmbeddedTestElement::create();
    parent->setReceivesMouse(true);
    parent->setMouseEnter([&log](const Vector2D&) { log.emplace_back("enter"); });
    parent->setMouseButton([&log](Input::eMouseButton, bool down) { log.emplace_back(down ? "down" : "up"); });
    child->setTooltip("child tooltip");
    parent->reposition({10, 10, 40, 40});
    child->reposition({20, 20, 20, 20});
    parent->addChild(child);
    m_surface->rootElement()->addChild(parent);

    m_surface->pointerEnter({25, 25});
    m_surface->pointerButton(Input::MOUSE_BUTTON_LEFT, true);
    m_surface->pointerButton(Input::MOUSE_BUTTON_LEFT, false);

    EXPECT_EQ(log, (std::vector<std::string>{"enter", "down", "up"}));
    EXPECT_EQ(m_surface->m_tooltip.hoverElement.lock().get(), child.get());
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

TEST_F(CEmbeddedSurfaceTest, textboxKnowsWhetherItIsFocused) {
    Tests::Tricks::createBackendSupport();

    const auto first  = CTextboxBuilder::begin()->commence();
    const auto second = CTextboxBuilder::begin()->commence();
    m_surface->rootElement()->addChild(first);
    m_surface->rootElement()->addChild(second);
    EXPECT_FALSE(first->focused());

    first->focus();
    EXPECT_TRUE(first->focused());
    EXPECT_FALSE(second->focused());

    second->focus();
    EXPECT_FALSE(first->focused());
    EXPECT_TRUE(second->focused());

    second->focus(false);
    EXPECT_FALSE(second->focused());
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

TEST_F(CEmbeddedSurfaceTest, repositionScheduledDuringPreRenderIsKept) {
    Tests::Tricks::createBackendSupport();
    m_surface->resize({200, 100}, {200, 100}, 1.F);

    const auto fixed = [] { return CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {100, 100}})->commence(); };

    auto       scroll  = CScrollAreaBuilder::begin()->scrollY(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
    auto       content = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    for (int i = 0; i < 10; ++i) {
        content->addChild(CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 30}})->commence());
    }
    scroll->addChild(content);
    auto scrollHolder = fixed();
    scrollHolder->addChild(scroll);
    m_surface->rootElement()->addChild(scrollHolder);

    auto leaf       = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {10, 10}})->commence();
    auto leafHolder = fixed();
    leafHolder->addChild(leaf);
    m_surface->rootElement()->addChild(leafHolder);

    m_surface->onPreRender();
    m_surface->onPreRender();

    m_surface->m_needsReposition.clear();
    m_surface->m_needsReposition.shrink_to_fit();
    m_surface->m_needsReposition.reserve(2);
    m_surface->scheduleReposition(scroll);
    m_surface->scheduleReposition(leaf);
    ASSERT_EQ(m_surface->m_needsReposition.size(), m_surface->m_needsReposition.capacity());

    m_surface->onPreRender();

    EXPECT_FALSE(m_surface->m_needsReposition.empty());
}

TEST_F(CEmbeddedSurfaceTest, detachedElementDoesNotDropRootReposition) {
    Tests::Tricks::createBackendSupport();
    m_surface->resize({200, 100}, {200, 100}, 1.F);

    auto child = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 50}})->commence();
    m_surface->rootElement()->addChild(child);
    m_surface->onPreRender();
    ASSERT_EQ(child->size(), (Vector2D{50, 50}));

    auto detached = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {10, 10}})->commence();
    m_surface->scheduleReposition(detached);
    m_surface->scheduleReposition(m_surface->rootElement());
    child->rebuild()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {80, 50}})->commence();

    m_surface->onPreRender();

    EXPECT_EQ(child->size(), (Vector2D{80, 50}));
}
