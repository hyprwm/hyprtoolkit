#include "AnimationManager.hpp"

#include "../Macros.hpp"

#include <hyprtoolkit/palette/Gradient.hpp>

#include <algorithm>
#include <format>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CHTAnimationManager::CHTAnimationManager() {
    addBezierWithName("linear", {0, 0}, {1, 1});
    addBezierWithName("easeOutQuint", {0.23F, 1.F}, {0.32F, 1.F});

    m_animationTree.createNode("global");

    m_animationTree.createNode("fast", "global");
    m_animationTree.createNode("indeterminate", "global");

    m_animationTree.setConfigForNode("fast", 1, 3.3F, "easeOutQuint");
    m_animationTree.setConfigForNode("indeterminate", 1, 0.7F, "linear");
}

template <Animable VarType>
static void updateVariable(CAnimatedVariable<VarType>& av, const float POINTY, bool warp = false) {
    if (warp || !av.enabled() || av.value() == av.goal()) {
        av.warp(true, false);
        return;
    }

    const auto DELTA = av.goal() - av.begun();
    av.value()       = av.begun() + DELTA * POINTY;
}

static void updateColorVariable(CAnimatedVariable<CHyprColor>& av, const float POINTY, bool warp = false) {
    if (warp || !av.enabled() || av.value() == av.goal()) {
        av.warp(true, false);
        return;
    }

    // convert both to OkLab, then lerp that, and convert back.
    // This is not as fast as just lerping rgb, but it's WAY more precise...
    // Use the CHyprColor cache for OkLab

    const auto&                L1 = av.begun().asOkLab();
    const auto&                L2 = av.goal().asOkLab();

    static const auto          lerp = [](const float one, const float two, const float progress) -> float { return one + ((two - one) * progress); };

    const Hyprgraphics::CColor lerped = Hyprgraphics::CColor::SOkLab{
        .l = lerp(L1.l, L2.l, POINTY),
        .a = lerp(L1.a, L2.a, POINTY),
        .b = lerp(L1.b, L2.b, POINTY),
    };
    const auto RGB = lerped.asRgb();

    av.value() = {
        std::clamp(sc<float>(RGB.r), 0.F, 1.F),
        std::clamp(sc<float>(RGB.g), 0.F, 1.F),
        std::clamp(sc<float>(RGB.b), 0.F, 1.F),
        std::clamp(lerp(av.begun().a, av.goal().a, POINTY), 0.F, 1.F),
    };
}

static void updateGradientVariable(CAnimatedVariable<CGradientValueData>& av, const float POINTY, bool warp = false) {
    if (warp || !av.enabled() || av.value() == av.goal()) {
        av.warp(true, false);
        return;
    }

    if (av.goal().m_vColors.empty() || av.begun().m_vColors.empty()) {
        av.warp(true, false);
        return;
    }

    av.value().m_vColors.resize(av.goal().m_vColors.size(), av.goal().m_vColors.back());

    static const auto lerp = [](const float one, const float two, const float progress) -> float { return one + ((two - one) * progress); };

    for (size_t i = 0; i < av.value().m_vColors.size(); ++i) {
        const CHyprColor&          sourceCol = (i < av.begun().m_vColors.size()) ? av.begun().m_vColors[i] : av.begun().m_vColors.back();
        const CHyprColor&          targetCol = av.goal().m_vColors[i];

        const auto&                L1 = sourceCol.asOkLab();
        const auto&                L2 = targetCol.asOkLab();

        const Hyprgraphics::CColor lerped = Hyprgraphics::CColor::SOkLab{
            .l = lerp(L1.l, L2.l, POINTY),
            .a = lerp(L1.a, L2.a, POINTY),
            .b = lerp(L1.b, L2.b, POINTY),
        };

        const auto RGB          = lerped.asRgb();
        av.value().m_vColors[i] = {
            std::clamp(sc<float>(RGB.r), 0.F, 1.F),
            std::clamp(sc<float>(RGB.g), 0.F, 1.F),
            std::clamp(sc<float>(RGB.b), 0.F, 1.F),
            std::clamp(lerp(sourceCol.a, targetCol.a, POINTY), 0.F, 1.F),
        };
    }

    const float DELTA   = av.goal().m_fAngle - av.begun().m_fAngle;
    av.value().m_fAngle = av.begun().m_fAngle + DELTA * POINTY;
}

static void updateBoxVariable(CAnimatedVariable<CBox>& av, const float POINTY, bool warp = false) {
    if (warp || !av.enabled() || av.value() == av.goal()) {
        av.warp(true, false);
        return;
    }

    const auto lerp = [POINTY](const double from, const double to) { return from + ((to - from) * POINTY); };
    av.value()      = CBox{
        lerp(av.begun().x, av.goal().x),
        lerp(av.begun().y, av.goal().y),
        lerp(av.begun().w, av.goal().w),
        lerp(av.begun().h, av.goal().h),
    };
}

SP<CHTAnimationManager::SAnimationPropertyConfig> CHTAnimationManager::configFor(const SAnimation& animation) {
    const auto EXISTING = std::ranges::find_if(m_dynamicConfigs, [&animation](const auto& entry) { return entry.first == animation; });
    if (EXISTING != m_dynamicConfigs.end())
        return EXISTING->second;

    auto config = makeShared<SAnimationPropertyConfig>();

    config->overridden       = true;
    config->internalEnabled  = 1;
    config->pValues          = config;
    config->pParentAnimation = config;

    if (std::holds_alternative<SNoAnimation>(animation)) {
        config->internalEnabled = 0;
        config->internalSpeed   = 1.F;
        m_dynamicConfigs.emplace_back(animation, config);
        return config;
    }

    const auto NAME = std::format("hyprtoolkit-{}", m_nextCurveID++);

    if (const auto* bezier = std::get_if<SBezierAnimation>(&animation)) {
        addBezierWithName(NAME, bezier->control1, bezier->control2);
        config->internalBezier = NAME;
        config->internalSpeed  = std::max(0.001F, bezier->duration.count() / 100.F);
    } else {
        const auto& spring = std::get<SSpringAnimation>(animation);
        addSpringWithName(NAME,
                          Hyprutils::Animation::SSpringCurve{
                              .stiffness       = std::max(0.001F, spring.stiffness),
                              .damping         = std::max(0.F, spring.damping),
                              .mass            = std::max(0.001F, spring.mass),
                              .valueEpsilon    = std::max(0.000001F, spring.valueEpsilon),
                              .velocityEpsilon = std::max(0.000001F, spring.velocityEpsilon),
                          });
        config->internalBezier = std::format("spring:{}", NAME);
        config->internalSpeed  = 1.F;
    }

    m_dynamicConfigs.emplace_back(animation, config);
    return config;
}

void CHTAnimationManager::tick() {
    // Callbacks are allowed to retarget animations, which mutates the manager's active list.
    const auto ACTIVE = m_vActiveAnimatedVariables;
    for (const auto& PAV : ACTIVE) {
        if (!PAV || !PAV->ok())
            continue;

        const auto STEP   = PAV->getCurveStep();
        const auto POINTY = STEP.value;
        const bool WARP   = STEP.finished;

        switch (PAV->m_Type) {
            case AVARTYPE_FLOAT: {
                auto pTypedAV = dynamic_cast<CAnimatedVariable<float>*>(PAV.get());
                RASSERT(pTypedAV, "Failed to upcast animated float");
                updateVariable(*pTypedAV, POINTY, WARP);
            } break;
            case AVARTYPE_VECTOR: {
                auto pTypedAV = dynamic_cast<CAnimatedVariable<Vector2D>*>(PAV.get());
                RASSERT(pTypedAV, "Failed to upcast animated Vector2D");
                updateVariable(*pTypedAV, POINTY, WARP);
            } break;
            case AVARTYPE_COLOR: {
                auto pTypedAV = dynamic_cast<CAnimatedVariable<CHyprColor>*>(PAV.get());
                RASSERT(pTypedAV, "Failed to upcast animated CHyprColor");
                updateColorVariable(*pTypedAV, POINTY, WARP);
            } break;
            case AVARTYPE_GRADIENT: {
                auto pTypedAV = dynamic_cast<CAnimatedVariable<CGradientValueData>*>(PAV.get());
                RASSERT(pTypedAV, "Failed to upcast animated CGradientValueData");
                updateGradientVariable(*pTypedAV, POINTY, WARP);
            } break;
            case AVARTYPE_BOX: {
                auto pTypedAV = dynamic_cast<CAnimatedVariable<CBox>*>(PAV.get());
                RASSERT(pTypedAV, "Failed to upcast animated CBox");
                updateBoxVariable(*pTypedAV, POINTY, WARP);
            } break;
            default: continue;
        }

        PAV->onUpdate();
    }

    tickDone();
}

void CHTAnimationManager::scheduleTick() {
    ;
}

void CHTAnimationManager::onTicked() {
    ;
}
