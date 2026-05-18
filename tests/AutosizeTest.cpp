#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>

#include <hyprutils/memory/SharedPtr.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer

static SP<IBackend>          backend;
static SP<CRectangleElement> anchor;
static int                   step = 0;

static void schedule();
static void tick(Hyprutils::Memory::CAtomicSharedPointer<CTimer>, void*) {
    const float sizes[][2] = {{240, 80}, {300, 200}, {500, 350}, {180, 120}, {900, 700}};
    const auto& s          = sizes[step++ % 5];
    anchor->rebuild()
        ->color([] { return CHyprColor{0.F, 0.F, 0.F, 0.F}; })
        ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {s[0], s[1]}})
        ->commence();
    schedule();
}
static void schedule() {
    backend->addTimer(std::chrono::seconds(3), tick, nullptr);
}

int main() {
    backend = IBackend::create();

    auto window = CWindowBuilder::begin()
                      ->preferredSize({240, 80})
                      ->minSize({100, 60})
                      ->maxSize({1600, 1200})
                      ->autosize(true)
                      ->appTitle("Autosize Demo")
                      ->appClass("hyprtoolkit-autosize")
                      ->commence();

    // green fill covers whatever size the window ends up at, including fullscreen
    auto fill = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor{0.3F, 0.7F, 0.4F, 1.F}; })
                    ->rounding(6)
                    ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                    ->commence();
    window->m_rootElement->addChild(fill);

    // invisible ABSOLUTE-sized anchor drives autosize via its preferredSize
    anchor = CRectangleBuilder::begin()
                 ->color([] { return CHyprColor{0.F, 0.F, 0.F, 0.F}; })
                 ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {240.F, 80.F}})
                 ->commence();
    window->m_rootElement->addChild(anchor);

    schedule();

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] { w->close(); backend->destroy(); });
    window->open();

    backend->enterLoop();
    return 0;
}
