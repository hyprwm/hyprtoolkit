#include <hyprtoolkit/core/Animation.hpp>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Combobox.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include <hyprutils/memory/SharedPtr.hpp>

#include <array>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;
using namespace Hyprutils::Memory;
using namespace std::chrono_literals;

#define SP CSharedPointer

static SP<IBackend>           backend;
static SP<CRectangleElement>  movingCard;
static SP<CRectangleElement>  colorCard;
static SP<CRectangleElement>  fadingCard;
static SP<CTextElement>       animatedText;
static bool                   alternate = false;

static const SBezierAnimation COLOR_CURVE{
    .duration = 650ms,
    .control1 = {0.16, 1.0},
    .control2 = {0.3, 1.0},
};

static const SBezierAnimation OPACITY_CURVE{
    .duration = 450ms,
    .control1 = {0.4, 0.0},
    .control2 = {0.2, 1.0},
};

static constexpr std::array GEOMETRY_SPRINGS{
    AnimationPresets::Slow, AnimationPresets::Medium, AnimationPresets::Fast, AnimationPresets::Snappy, AnimationPresets::Bouncy,
};

static void toggleAnimations() {
    alternate = !alternate;

    movingCard->setAbsolutePosition(alternate ? Vector2D{430, 115} : Vector2D{55, 115});
    movingCard->rebuild()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, alternate ? Vector2D{210, 145} : Vector2D{150, 100}})->commence();

    colorCard->rebuild()->color([alternate = alternate] { return alternate ? CHyprColor{0.94F, 0.35F, 0.28F, 1.F} : CHyprColor{0.20F, 0.62F, 0.86F, 1.F}; })->commence();

    fadingCard->setOpacity(alternate ? 0.15F : 1.F);

    animatedText->rebuild()->color([alternate = alternate] { return alternate ? CHyprColor{0.98F, 0.75F, 0.20F, 1.F} : CHyprColor{0.50F, 0.90F, 0.68F, 1.F}; })->commence();
}

static void selectGeometrySpring(size_t index) {
    if (index >= GEOMETRY_SPRINGS.size())
        return;

    movingCard->animateGeometry(GEOMETRY_SPRINGS[index]);
    toggleAnimations();
}

static void scheduleToggle() {
    backend->addTimer(
        1400ms,
        [](CAtomicSharedPointer<CTimer>, void*) {
            toggleAnimations();
            scheduleToggle();
        },
        nullptr);
}

static SP<CTextElement> label(std::string text, CFontSize size = {CFontSize::HT_FONT_TEXT}) {
    return CTextBuilder::begin()->text(std::move(text))->fontSize(std::move(size))->color([] { return CHyprColor{0.93F, 0.94F, 0.97F, 1.F}; })->commence();
}

int main() {
    backend     = IBackend::create();
    auto window = CWindowBuilder::begin()->preferredSize({720, 520})->appTitle("Animation playground")->appClass("hyprtoolkit-animations")->commence();

    auto background = CRectangleBuilder::begin()->color([] { return CHyprColor{0.055F, 0.065F, 0.09F, 1.F}; })->commence();
    window->m_rootElement->addChild(background);

    auto title = label("Animations!!! :D", {CFontSize::HT_FONT_H1});
    title->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    title->setAbsolutePosition({32, 24});
    background->addChild(title);

    movingCard = CRectangleBuilder::begin()
                     ->color([] { return CHyprColor{0.42F, 0.28F, 0.82F, 1.F}; })
                     ->rounding(18)
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {150, 100}})
                     ->commence();
    movingCard->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    movingCard->setAbsolutePosition({55, 115});
    movingCard->animateGeometry(AnimationPresets::Bouncy);
    auto movingLabel = label("Spring geometry\n(final input coords)");
    movingLabel->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    movingLabel->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    movingCard->addChild(movingLabel);
    background->addChild(movingCard);

    colorCard = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor{0.20F, 0.62F, 0.86F, 1.F}; })
                    ->rounding(16)
                    ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {250, 82}})
                    ->commence();
    colorCard->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    colorCard->setAbsolutePosition({55, 295});
    colorCard->animateColor(COLOR_CURVE);
    auto colorLabel = label("Bezier background color");
    colorLabel->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    colorLabel->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    colorCard->addChild(colorLabel);
    background->addChild(colorCard);

    fadingCard = CRectangleBuilder::begin()
                     ->color([] { return CHyprColor{0.18F, 0.72F, 0.55F, 1.F}; })
                     ->rounding(16)
                     ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {250, 82}})
                     ->commence();
    fadingCard->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    fadingCard->setAbsolutePosition({410, 295});
    fadingCard->animateOpacity(OPACITY_CURVE);
    auto opacityLabel = label("Inherited subtree opacity");
    opacityLabel->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    opacityLabel->setPositionFlag(IElement::HT_POSITION_FLAG_CENTER, true);
    fadingCard->addChild(opacityLabel);
    background->addChild(fadingCard);

    animatedText =
        CTextBuilder::begin()->text("Animated text foreground")->fontSize({CFontSize::HT_FONT_H2})->color([] { return CHyprColor{0.50F, 0.90F, 0.68F, 1.F}; })->commence();
    animatedText->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    animatedText->setAbsolutePosition({55, 410});
    animatedText->animateColor(COLOR_CURVE);
    background->addChild(animatedText);

    auto toggle = CButtonBuilder::begin()
                      ->label("Retarget now")
                      ->onMainClick([](SP<CButtonElement>) { toggleAnimations(); })
                      ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {140, 38}})
                      ->commence();
    toggle->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    toggle->setAbsolutePosition({525, 438});
    background->addChild(toggle);

    auto springSelectorLabel = label("Geometry spring");
    springSelectorLabel->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    springSelectorLabel->setAbsolutePosition({305, 410});
    background->addChild(springSelectorLabel);

    auto springSelector = CComboboxBuilder::begin()
                              ->items({"Slow", "Medium", "Fast", "Snappy", "Bouncy"})
                              ->currentItem(4)
                              ->onChanged([](SP<CComboboxElement>, size_t index) { selectGeometrySpring(index); })
                              ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {190, 38}})
                              ->commence();
    springSelector->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    springSelector->setAbsolutePosition({305, 438});
    background->addChild(springSelector);

    window->open();
    scheduleToggle();
    backend->enterLoop();
    return 0;
}
