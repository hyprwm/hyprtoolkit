#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/system/Icons.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Checkbox.hpp>
#include <hyprtoolkit/element/Spinbox.hpp>
#include <hyprtoolkit/element/Slider.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Textbox.hpp>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

#include <print>

using namespace Hyprutils::Memory;
using namespace Hyprutils::Math;
using namespace Hyprtoolkit;

#define SP CSharedPointer
#define WP CWeakPointer
#define UP CUniquePointer

static SP<CBackend>             backend;
static SP<CSliderElement>       hiddenSlider;
static SP<CColumnLayoutElement> mainLayout;
static SP<IWindow>              window;
static SP<IWindow>              popup;

constexpr float                 SLIDER_HEIGHT = 10.F;

//
static void toggleVisibilityOfSecretSlider() {
    static bool visible = false;

    if (!visible)
        mainLayout->addChild(hiddenSlider);
    else
        mainLayout->removeChild(hiddenSlider);

    visible = !visible;
}

static void openPopup() {
    popup = window->openPopup(SPopupCreationData{
        .pos  = {200, 200},
        .size = {350, 600},
    });

    auto popbg = CRectangleBuilder::begin()
                     ->rounding(10)
                     ->color([] { return backend->getPalette()->m_colors.background.brighten(0.5); })
                     ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                     ->commence();

    auto poptext = CTextBuilder::begin() //
                       ->text("THEY CALL ME MR BOOMBASTIC")
                       ->fontSize({CFontSize::HT_FONT_H2})
                       ->color([] { return backend->getPalette()->m_colors.text; })
                       ->commence();

    poptext->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_CENTER);

    popup->m_rootElement->addChild(popbg);
    popbg->addChild(poptext);

    popup->open();

    popup->m_events.popupClosed.listenStatic([] { popup.reset(); });
}

static SP<IElement> stretchLayout(std::string&& label, SP<IElement> control) {
    auto bg = CRectangleBuilder::begin()
                  ->color([] { return backend->getPalette()->m_colors.alternateBase; })
                  ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                  ->rounding(4)
                  ->commence();
    auto layoutE = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    auto labelE  = CTextBuilder::begin()->text(std::move(label))->color([] { return backend->getPalette()->m_colors.text; })->commence();
    auto nullE   = CNullBuilder::begin()->commence();
    nullE->setGrow(true);

    auto container = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    container->setMargin(4);

    bg->addChild(container);
    container->addChild(layoutE);

    layoutE->addChild(labelE);
    layoutE->addChild(nullE);
    layoutE->addChild(control);

    return bg;
}

int main(int argc, char** argv, char** envp) {
    backend = CBackend::create();

    //
    window = backend->openWindow(SWindowCreationData{
        .preferredSize = Vector2D{480, 480},
        .minSize       = Vector2D{480, 480},
        .maxSize       = Vector2D{1280, 720},
        .title         = "Controls",
        .class_        = "hyprtoolkit-controls",
    });

    auto bg = CRectangleBuilder::begin()->color([] { return backend->getPalette()->m_colors.background; })->commence();

    window->m_rootElement->addChild(bg);

    auto scroll = CScrollAreaBuilder::begin()->scrollY(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();

    mainLayout = CColumnLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.7F, 1.F}})->commence();

    mainLayout->setMargin(3);
    mainLayout->setPositionMode(Hyprtoolkit::IElement::HT_POSITION_HCENTER);

    bg->addChild(scroll);
    scroll->addChild(mainLayout);

    auto title = CTextBuilder::begin() //
                     ->text("Controls")
                     ->fontSize({CFontSize::HT_FONT_H2})
                     ->color([] { return backend->getPalette()->m_colors.text; })
                     ->commence();

    auto hr = CRectangleBuilder::begin() //
                  ->color([] { return CHyprColor{backend->getPalette()->m_colors.text.darken(0.65)}; })
                  ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, 9.F}})
                  ->commence();

    hr->setMargin(4);

    auto button1 = CButtonBuilder::begin()
                       ->label("Secret")
                       ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                       ->onMainClick([](SP<CButtonElement>) { toggleVisibilityOfSecretSlider(); })
                       ->commence();

    auto button2 = CButtonBuilder::begin()
                       ->label("Popup")
                       ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                       ->onMainClick([](SP<CButtonElement>) { openPopup(); })
                       ->commence();

    auto checkbox  = CCheckboxBuilder::begin()->label("Checkbox")->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();
    auto checkbox2 = CCheckboxBuilder::begin()->label("Checkbox")->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->fill(true)->commence();

    auto spinbox = CSpinboxBuilder::begin()
                       ->label("Spinbox")
                       ->items({"Hello", "World", "Amongus"})
                       ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                       ->fill(true)
                       ->commence();

    auto slider = stretchLayout("Slider", CSliderBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, SLIDER_HEIGHT}})->commence());

    auto slider2 = stretchLayout(
        "Big Slider", CSliderBuilder::begin()->max(10000)->val(2500)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.5F, SLIDER_HEIGHT}})->commence());

    auto combo = stretchLayout(
        "Combo",
        CComboboxBuilder::begin()
            ->items({"According", "to",  "all",  "known", "laws",   "of",    "aviation", "there",   "is",   "no",    "way",  "that", "a",      "bee",   "should", "be",
                     "able",      "to",  "fly.", "its",   "wings",  "are",   "too",      "small",   "to",   "get",   "its",  "fat",  "little", "body",  "off",    "the",
                     "ground.",   "the", "bee",  "of",    "course", "flies", "anyways,", "because", "bees", "don't", "care", "what", "humans", "think", "is",     "impossible."})
            ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.3F, 25.F}})
            ->commence());

    auto textbox = stretchLayout(
        "Textbox",
        CTextboxBuilder::begin()->defaultText("")->placeholder("placeholder")->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {150.F, 24.F}})->commence());

    hiddenSlider = CSliderBuilder::begin()->max(100)->val(69)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, SLIDER_HEIGHT}})->commence();

    mainLayout->addChild(title);
    mainLayout->addChild(hr);
    mainLayout->addChild(button1);
    mainLayout->addChild(button2);
    mainLayout->addChild(checkbox);
    mainLayout->addChild(checkbox2);
    mainLayout->addChild(spinbox);
    mainLayout->addChild(slider);
    mainLayout->addChild(slider2);
    mainLayout->addChild(combo);
    mainLayout->addChild(textbox);

    auto iconDesc = backend->systemIcons()->lookupIcon("action-unavailable");
    if (iconDesc->exists()) {
        mainLayout->addChild(CImageBuilder::begin()->icon(iconDesc)->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {40, 40}})->commence());
    }

    window->m_events.closeRequest.listenStatic([w = WP<IWindow>{window}] {
        w->close();
        backend->destroy();
    });

    window->open();

    backend->enterLoop();

    return 0;
}