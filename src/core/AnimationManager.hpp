#pragma once

#include <hyprutils/animation/AnimationManager.hpp>
#include <hyprutils/animation/AnimatedVariable.hpp>

#include "AnimatedVariable.hpp"

namespace Hyprtoolkit {
    class CHTAnimationManager : public Hyprutils::Animation::CAnimationManager {
      public:
        CHTAnimationManager();

        void         tick();
        virtual void scheduleTick();
        virtual void onTicked();

        using SAnimationPropertyConfig = Hyprutils::Animation::SAnimationPropertyConfig;

        template <Animable VarType>
        void createAnimation(const VarType& v, PHLANIMVAR<VarType>& pav, SP<SAnimationPropertyConfig> pConfig) {
            constexpr const eAnimatedVarType EAVTYPE = typeToeAnimatedVarType<VarType>;
            const auto                       PAV     = makeShared<CAnimatedVariable<VarType>>();

            PAV->create(EAVTYPE, static_cast<Hyprutils::Animation::CAnimationManager*>(this), PAV, v);
            PAV->setConfig(pConfig);

            pav = std::move(PAV);
        }

        Hyprutils::Animation::CAnimationConfigTree m_animationTree;
    };

    inline SP<CHTAnimationManager> g_animationManager;
}
