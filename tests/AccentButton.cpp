#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Button.hpp>

#include <hyprutils/memory/SharedPtr.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer

static SP<IBackend> backend;

int main() {
    backend = IBackend::create();

    auto window = CWindowBuilder::begin() //
                      ->preferredSize({320, 260})
                      ->appTitle("AccentButton")
                      ->appClass("hyprtoolkit-accentbutton")
                      ->commence();

    auto bg = CRectangleBuilder::begin() //
                  ->color([] { return backend->getPalette()->m_colors.background; })
                  ->commence();
    window->m_rootElement->addChild(bg);

    auto col = CColumnLayoutBuilder::begin() //
                   ->gap(8)
                   ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                   ->commence();
    col->setMargin(16);
    bg->addChild(col);

    col->addChild(CButtonBuilder::begin()->label("Normal")->commence());
    col->addChild(CButtonBuilder::begin()->label("Accent")->accent(true)->commence());
    col->addChild(CButtonBuilder::begin()->label("Accent disabled")->accent(true)->enabled(false)->commence());
    col->addChild(CButtonBuilder::begin()->label("Accent noBorder")->accent(true)->noBorder(true)->commence());

    window->open();
    backend->enterLoop();
    return 0;
}
