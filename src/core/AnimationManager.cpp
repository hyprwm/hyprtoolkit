#include "AnimationManager.hpp"

#include "../Macros.hpp"

#include <hyprtoolkit/palette/Gradient.hpp>

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

    av.value() = {lerped, lerp(av.begun().a, av.goal().a, POINTY)};
}

static void updateGradientVariable(CAnimatedVariable<CGradientValueData>& av, const float POINTY, bool warp = false) {
    if (warp || !av.enabled() || av.value() == av.goal()) {
        av.warp(true, false);
        return;
    }

    if (av.goal().m_vColors.empty()) {
        av.warp(true, false);
        return;
    }

    av.value().m_vColors.resize(av.goal().m_vColors.size(), av.goal().m_vColors.back());

    static const auto lerp = [](const float one, const float two, const float progress) -> float { return one + ((two - one) * progress); };

    for (size_t i = 0; i < av.value().m_vColors.size(); ++i) {
        const CHyprColor& sourceCol = (i < av.begun().m_vColors.size()) ? av.begun().m_vColors[i] : av.begun().m_vColors.back();
        const CHyprColor& targetCol = av.goal().m_vColors[i];

        const auto&                L1 = sourceCol.asOkLab();
        const auto&                L2 = targetCol.asOkLab();

        const Hyprgraphics::CColor lerped = Hyprgraphics::CColor::SOkLab{
            .l = lerp(L1.l, L2.l, POINTY),
            .a = lerp(L1.a, L2.a, POINTY),
            .b = lerp(L1.b, L2.b, POINTY),
        };

        av.value().m_vColors[i] = {lerped, lerp(sourceCol.a, targetCol.a, POINTY)};
    }

    const float DELTA   = av.goal().m_fAngle - av.begun().m_fAngle;
    av.value().m_fAngle = av.begun().m_fAngle + DELTA * POINTY;
}

void CHTAnimationManager::tick() {
    for (const auto& PAV : m_vActiveAnimatedVariables) {
        if (!PAV || !PAV->ok())
            continue;

        const auto SPENT   = PAV->getPercent();
        const auto PBEZIER = getBezier(PAV->getBezierName());
        const auto POINTY  = PBEZIER->getYForPoint(SPENT);
        const bool WARP    = SPENT >= 1.f;

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
