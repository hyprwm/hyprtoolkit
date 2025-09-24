#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Spinbox.hpp>

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
                        .preferredSize = Vector2D{480, 480},
                        .minSize       = Vector2D{480, 480},
                        .maxSize       = Vector2D{1280, 720},
                        .title         = "Controls",
                        .class_        = "hyprtoolkit-controls",
    });

    window->m_rootElement->addChild(CRectangleElement::create(SRectangleData{
                        .color = [] { return backend->getPalette()->m_colors.background; },
    }));

    auto layout = CColumnLayoutElement::create(SColumnLayoutData{
                        .size = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.7F, 1.F}},
                        .gap  = 3,

    });
    layout->setMargin(3);
    layout->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_CENTER);

    window->m_rootElement->addChild(layout);

    auto title = CTextElement::create(STextData{
                        .text     = "Controls",
                        .fontSize = CFontSize{CFontSize::HT_FONT_H2},
                        .color    = [] { return backend->getPalette()->m_colors.text; },
    });

    auto hr = CRectangleElement::create(SRectangleData{
                        .color = [] { return CHyprColor{backend->getPalette()->m_colors.text.darken(0.65)}; },
                        .size  = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 9.F}},
    });

    hr->setMargin(4);

    auto button1 = CButtonElement::create(SButtonData{
                        .label       = "Button",
                        .onMainClick = [](SP<CButtonElement> el) { std::println("Hello world!"); },
                        .size        = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    auto checkbox = CCheckboxElement::create(SCheckboxData{
                        .label     = "Checkbox",
                        .onToggled = [](SP<CCheckboxElement> el, bool state) { std::println("Toggled to {}", state); },
                        .size      = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    auto checkbox2 = CCheckboxElement::create(SCheckboxData{
                        .label     = "Wide checkbox",
                        .onToggled = [](SP<CCheckboxElement> el, bool state) { std::println("Toggled wide to {}", state); },
                        .size      = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
                        .fill      = true,
    });

    auto spinbox = CSpinboxElement::create(SSpinboxData{
                        .label     = "Spinbox",
                        .items     = {"Hello", "World", "Amongus"},
                        .onChanged = [](SP<CSpinboxElement> el, size_t idx) { std::println("Toggled spin to {}", idx); },
                        .size      = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
                        .fill      = true,
    });

    auto null1 = CNullElement::create({});

    null1->setGrow(true);

    layout->addChild(title);
    layout->addChild(hr);
    layout->addChild(button1);
    layout->addChild(checkbox);
    layout->addChild(checkbox2);
    layout->addChild(spinbox);
    layout->addChild(null1);

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}