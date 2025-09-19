#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>

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

    auto rect2 = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.2F, .g = 1.F, .b = 0.2F},
        .rounding = 10,
        .size = {250, 250},
    });
    rect2->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_CENTER);
    window->m_rootElement->m_children.emplace_back(rect2);

    auto rect3 = makeShared<Hyprtoolkit::CRectangleElement>(Hyprtoolkit::CRectangleElement::SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.2F, .g = 0.2F, .b = 1.F},
        .rounding = 10,
        .size = {100, 100},
    });
    rect3->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_ABSOLUTE);
    rect3->setAbsolutePosition({75, 75});
    rect2->m_children.emplace_back(rect3);

    backend->enterLoop();

    return 0;
}