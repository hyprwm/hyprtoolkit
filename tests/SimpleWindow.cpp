#include <hyprtoolkit/core/Backend.hpp>
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

static SP<IBackend> backend;
static size_t       buttonClicks = 1;

static void         addTimer(SP<CRectangleElement> rect) {
    backend->addTimer(
        std::chrono::seconds(1),
        [rect](Hyprutils::Memory::CAtomicSharedPointer<CTimer> timer, void* data) {
            rect->rebuild()->color([] { return CHyprColor{rand() % 1000 / 1000.F, rand() % 1000 / 1000.F, rand() % 1000 / 1000.F, 1.F}; })->commence();

            addTimer(rect);
        },
        nullptr);
}

int main(int argc, char** argv, char** envp) {
    backend = IBackend::create();

    //
    auto window = CWindowBuilder::begin()->preferredSize({640, 480})->appTitle("Hello World!")->appClass("hyprtoolkit")->commence();

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
                     ->rounding(10)
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 50}})
                     ->commence();

    auto layout2 = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.5F, 1.F}})->commence();

    auto image = CImageBuilder::begin() //
                     ->path("/home/vaxry/Documents/born to C++.png")
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {447, 447}})
                     ->commence();

    auto text = CTextBuilder::begin()->text("world is a fuck")->color([] { return CHyprColor{0.4F, 0.4F, 0.4F}; })->commence();

    auto button = CButtonBuilder::begin()
                      ->label("Click me bitch")
                      ->onMainClick([](SP<CButtonElement> el) { el->rebuild()->label(std::format("Clicked {} times bitch", buttonClicks++))->commence(); })
                      ->onRightClick([](SP<CButtonElement> el) { el->rebuild()->label("Reset bitch")->commence(); })
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

    backend->enterLoop();

    return 0;
}