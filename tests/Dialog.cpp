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
                        .text  = "Hello World",
                        .color = backend->getPalette()->m_colors.text,
    });

    auto content = CTextElement::create(STextData{
                        .text  = "This is an example dialog. This first line is long on purpose, so that we overflow.<br/><br/>Woo!",
                        .color = backend->getPalette()->m_colors.text,
    });

    auto button1 = CButtonElement::create(SButtonData{
                        .label = "Exit",
                        .onMainClick =
            [](SP<CButtonElement> el) {
                ; // TODO:
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
    layout->addChild(content);
    layout->addChild(null1);

    layout2->addChild(null2);
    layout2->addChild(button2);
    layout2->addChild(button1);

    layout->addChild(layout2);

    backend->enterLoop();

    return 0;
}