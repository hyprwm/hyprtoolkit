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

static SP<CBackend> backend;
static size_t       buttonClicks = 1;

static void         addTimer(SP<CRectangleElement> rect) {
    backend->addTimer(
        std::chrono::seconds(1),
        [rect](Hyprutils::Memory::CAtomicSharedPointer<CTimer> timer, void* data) {
            auto rectData  = rect->dataCopy();
            rectData.color = Hyprgraphics::CColor::SSRGB{.r = rand() % 1000 / 1000.F, .g = rand() % 1000 / 1000.F, .b = rand() % 1000 / 1000.F};
            rect->replaceData(rectData);

            addTimer(rect);
        },
        nullptr);
}

int main(int argc, char** argv, char** envp) {
    backend = CBackend::create();

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
        .path = "/home/vaxry/Documents/born to C++.png",
        .size = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {447, 447}},
    });

    auto text = CTextElement::create(STextData{
        .text  = "world is a fuck",
        .color = Hyprgraphics::CColor::SSRGB{.r = 0.4F, .g = 0.4F, .b = 0.4F},
    });

    auto button = CButtonElement::create(SButtonData{
        .label = "Click me bitch",
        .onMainClick =
            [](SP<CButtonElement> el) {
                auto data  = el->dataCopy();
                data.label = std::format("Clicked {} times bitch", buttonClicks++);
                el->replaceData(data);
            },
        .onRightClick =
            [](SP<CButtonElement> el) {
                auto data    = el->dataCopy();
                data.label   = std::format("Reset bitch");
                buttonClicks = 0;
                el->replaceData(data);
            },
        .size = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    text->setGrow(true);
    rect4->setGrow(true);

    addTimer(rect4);

    layout2->addChild(image);
    layout2->addChild(button);
    layout2->addChild(text);
    layout->addChild(layout2);
    layout->addChild(rect3);
    layout->addChild(rect4);

    backend->enterLoop();

    return 0;
}