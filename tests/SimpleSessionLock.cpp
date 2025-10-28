#include "hyprtoolkit/core/SessionLock.hpp"
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Output.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <print>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

static SP<IBackend>             backend;
static SP<ISessionLockState>    lockState;
static std::vector<SP<IWindow>> windows;

static void                     addTimer(SP<CRectangleElement> rect) {
    backend->addTimer(
        std::chrono::seconds(1),
        [rect](Hyprutils::Memory::CAtomicSharedPointer<CTimer> timer, void* data) {
            rect->rebuild()->color([] { return CHyprColor{rand() % 1000 / 1000.F, rand() % 1000 / 1000.F, rand() % 1000 / 1000.F, 1.F}; })->commence();

            addTimer(rect);
        },
        nullptr);
}

static void layout(const SP<IWindow>& window) {
    window->m_rootElement->addChild(CRectangleBuilder::begin()->color([] { return CHyprColor{0.1F, 0.1F, 0.1F}; })->commence());

    auto layout = CRowLayoutBuilder::begin()->commence();

    window->m_rootElement->addChild(layout);

    auto rect3 = CRectangleBuilder::begin() //
                     ->color([] { return CHyprColor{0.2F, 0.4F, 0.4F}; })
                     ->rounding(10)
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {150, 150}})
                     ->commence();

    auto rect4 = CRectangleBuilder::begin() //
                     ->color([] { return CHyprColor{0.4F, 0.2F, 0.4F}; })
                     ->rounding(0)
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 50}})
                     ->commence();

    auto layout2 = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.5F, 1.F}})->commence();

    auto image = CImageBuilder::begin() //
                     ->path("/home/max/media/picture/avatar.png")
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {447, 447}})
                     ->commence();

    auto text = CTextBuilder::begin()->text("never give up")->color([] { return CHyprColor{0.4F, 0.4F, 0.4F}; })->commence();

    auto button = CButtonBuilder::begin()
                      ->label("Click to unlock")
                      ->onMainClick([](SP<CButtonElement> el) {
                          el->rebuild()->label("Unlocking...")->commence();
                          lockState->unlock();
                      })
                      ->onRightClick([](SP<CButtonElement> el) { el->rebuild()->label("Reset")->commence(); })
                      ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                      ->commence();

    text->setGrow(true);
    rect4->setGrow(true);

    addTimer(rect4);

    layout2->addChild(image);
    layout2->addChild(button);
    layout2->addChild(text);
    layout->addChild(layout2);
    layout->addChild(rect3);
    layout->addChild(rect4);

    window->open();
}

static void closeWindow(WP<IWindow> window) {
    std::println("Remove surface {}!", (uintptr_t)windows.back().get());
    auto windowsIt = std::ranges::find_if(windows, [&window](const auto& w) { return w.get() == window.get(); });
    if (windowsIt == windows.end())
        return;

    (*windowsIt)->close();
    windows.erase(windowsIt);
    if (windows.empty()) {
        std::println("It's over!!!");
        backend->destroy();
    }
}

static void createLockSurface(SP<IOutput> output) {
    windows.emplace_back(CWindowBuilder::begin()->type(HT_WINDOW_LOCK_SURFACE)->prefferedOutput(output)->commence());
    std::println("New surface {}!", (uintptr_t)windows.back().get());
    WP<IWindow> weakWindow = windows.back();
    weakWindow->m_events.closeRequest.listenStatic([weakWindow]() { closeWindow(weakWindow); });

    layout(weakWindow.lock());
}

int main(int argc, char** argv, char** envp) {
    int unlockSecs = 10;
    if (argc == 2)
        unlockSecs = atoi(argv[1]);

    backend = IBackend::create();
    if (!backend) {
        std::println("Backend create failed!");
        return 1;
    }

    auto sessionLockState = backend->aquireSessionLock();
    if (sessionLockState.has_value())
        lockState = sessionLockState.value();

    if (!lockState) {
        std::println("Cloudn't lock");
        return 1;
    }

    lockState->m_events.finished.listenStatic([] {
        std::println("Compositor kicked us");
        for (const auto& w : windows) {
            closeWindow(w);
        }
    });

    backend->m_events.outputAdded.listenStatic(createLockSurface);

    for (const auto& o : backend->getOutputs()) {
        createLockSurface(o);
    }

    backend->addTimer(std::chrono::seconds(unlockSecs), [](auto, auto) { lockState->unlock(); }, nullptr);

    backend->enterLoop();

    return 0;
}
