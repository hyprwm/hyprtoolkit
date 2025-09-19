#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

int main(int argc, char** argv, char** envp) {
    SP<Hyprtoolkit::CBackend> backend = Hyprtoolkit::CBackend::create();

    //
    auto window = backend->openWindow(Hyprtoolkit::SWindowCreationData{
        .preferredSize = Vector2D{640, 480},
        .minSize       = Vector2D{640, 480},
        .maxSize       = Vector2D{640, 480},
        .title         = "Hello World!",
        .class_        = "hyprtoolkit",
    });

    window->m_rootElement->m_children.emplace_back(makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 1.F, .g = 0.2F, .b = 0.2F},
    }));

    auto layout = makeShared<Hyprtoolkit::CRowLayoutElement>();

    window->m_rootElement->m_children.emplace_back(layout);

    auto rect3 = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.2F, .g = 0.7F, .b = 0.7F},
        .rounding = 10,
        .size = {150, 150},
    });

    auto rect4 = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.7F, .g = 0.2F, .b = 0.7F},
        .rounding = 10,
        .size = {50, 50},
    });

    auto layout2 = makeShared<Hyprtoolkit::CColumnLayoutElement>();

    layout2->setGrow(true);

    auto rect2a = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.2F, .g = 1.F, .b = 0.2F},
        .rounding = 10,
        .size = {250, 350},
    });

    auto rect2b = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.7F, .g = 0.7F, .b = 0.2F},
        .rounding = 10,
        .size = {50, 50},
    });

    rect2b->setGrow(true);

    layout2->m_children.emplace_back(rect2a);
    layout2->m_children.emplace_back(rect2b);
    layout->m_children.emplace_back(layout2);
    layout->m_children.emplace_back(rect3);
    layout->m_children.emplace_back(rect4);

    backend->enterLoop();

    return 0;
}