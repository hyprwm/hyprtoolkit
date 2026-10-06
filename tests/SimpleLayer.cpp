#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Button.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

int main(int argc, char** argv, char** envp) {
    auto backend = IBackend::create();

    auto window = CWindowBuilder::begin()
                      ->type(HT_WINDOW_LAYER)
                      ->anchor(1 | 4 | 8)
                      ->layer(1)
                      ->preferredSize({0, 40})
                      ->exclusiveZone(40)
                      ->appTitle("Hello World!")
                      ->appClass("hyprtoolkit")
                      ->commence();

    window->m_rootElement->addChild(CButtonBuilder::begin()->label("Hello, world")->commence());

    window->open();

    backend->enterLoop();

    return 0;
}
