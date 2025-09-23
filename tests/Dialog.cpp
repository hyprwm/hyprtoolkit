#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>

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

int                 main(int argc, char** argv, char** envp) {
    backend = CBackend::create();

    //
    auto window = backend->openWindow(SWindowCreationData{
                        .preferredSize = Vector2D{480, 180},
                        .minSize       = Vector2D{480, 180},
                        .maxSize       = Vector2D{480, 180},
                        .title         = "Dialog",
                        .class_        = "hyprtoolkit-dialog",
    });

    window->m_rootElement->addChild(CRectangleElement::create(SRectangleData{
                        .color = backend->getPalette()->m_colors.background,
    }));

    auto layout = CColumnLayoutElement::create();
    layout->setMargin(3);

    window->m_rootElement->addChild(layout);

    auto title = CTextElement::create(STextData{
                        .text     = "Hello World",
                        .fontSize = CFontSize{CFontSize::HT_FONT_H2},
                        .color    = backend->getPalette()->m_colors.text,
    });

    auto hr = CRectangleElement::create(SRectangleData{
                        .color = CHyprColor{backend->getPalette()->m_colors.text.darken(0.65)},
                        .size  = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 9.F}},
    });

    hr->setMargin(4);

    auto content = CTextElement::create(STextData{
                        .text  = "This is an example dialog. This first line is long on purpose, so that we overflow.\n\nWoo!",
                        .color = backend->getPalette()->m_colors.text,
    });

    auto button1 = CButtonElement::create(SButtonData{
                        .label = "Exit",
                        .onMainClick =
            [w = WP<IWindow>{window}](SP<CButtonElement> el) {
                w->close();
                backend->destroy();
            },
                        .size = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    auto button2 = CButtonElement::create(SButtonData{
                        .label       = "Do something",
                        .onMainClick = [](SP<CButtonElement> el) { std::println("Did something!"); },
                        .size        = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    auto null1 = CNullElement::create({});
    auto null2 = CNullElement::create({});

    auto layout2 = CRowLayoutElement::create({.gap = 3});

    null1->setGrow(true);
    null2->setGrow(true);

    layout->addChild(title);
    layout->addChild(hr);
    layout->addChild(content);
    layout->addChild(null1);

    layout2->addChild(null2);
    layout2->addChild(button2);
    layout2->addChild(button1);

    layout->addChild(layout2);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}