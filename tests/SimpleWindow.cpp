#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

int main(int argc, char** argv, char** envp) {
    SP<CBackend> backend = CBackend::create();

    //
    auto window = backend->openWindow(SWindowCreationData{
        .preferredSize = Vector2D{640, 480},
        .title         = "Hello World!",
        .class_        = "hyprtoolkit",
    });

    window->m_rootElement->addChild(CRectangleElement::create(SRectangleData{
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.1F, .g = 0.1F, .b = 0.1F},
    }));

    auto layout = CRowLayoutElement::create();

    window->m_rootElement->addChild(layout);

    auto rect3 = CRectangleElement::create(SRectangleData{
        .color    = Hyprgraphics::CColor::SSRGB{.r = 0.2F, .g = 0.4F, .b = 0.4F},
        .rounding = 10,
        .size     = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {150, 150}},
    });

    auto rect4 = CRectangleElement::create(SRectangleData{
        .color    = Hyprgraphics::CColor::SSRGB{.r = 0.4F, .g = 0.2F, .b = 0.4F},
        .rounding = 10,
        .size     = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {50, 50}},
    });

    auto layout2 = CColumnLayoutElement::create(SColumnLayoutData{.size = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.5F, 1.F}}});

    auto image = CImageElement::create(SImageData{
        .path = "/home/vaxry/Documents/Hypr/thats a clip.jpg",
        .size = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {250, 250}},
    });

    auto text = CTextElement::create(STextData{
        .text  = "That's a clip!!!!",
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.4F, .g = 0.4F, .b = 0.4F},
    });

    text->setGrow(true);
    rect4->setGrow(true);

    layout2->addChild(image);
    layout2->addChild(text);
    layout->addChild(layout2);
    layout->addChild(rect3);
    layout->addChild(rect4);

    backend->enterLoop();

    return 0;
}