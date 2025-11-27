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
            pav                                      = makeUnique<CAnimatedVariable<VarType>>();

            pav->create(EAVTYPE, static_cast<Hyprutils::Animation::CAnimationManager*>(this), pav, v);
            pav->setConfig(pConfig);
        }

        Hyprutils::Animation::CAnimationConfigTree m_animationTree;
    };

    inline SP<CHTAnimationManager> g_animationManager;
}
