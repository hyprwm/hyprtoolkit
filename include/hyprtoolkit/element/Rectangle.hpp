#pragma once

#include "Element.hpp"

#include <hyprgraphics/color/Color.hpp>

namespace Hyprtoolkit {
    class CRectangleElement : public IElement {
      public:
        struct SRectangleData {
            Hyprgraphics::CColor      color    = Hyprgraphics::CColor{{.r = 1.F, .g = 1.F, .b = 1.F}};
            float                     a        = 1.F;
            int                       rounding = 0;
            Hyprutils::Math::Vector2D size     = {}; // 0,0 means no size, automatic, fits parent
        };

        CRectangleElement(const SRectangleData& data);
        virtual ~CRectangleElement() = default;

      private:
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize();
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize();

        SRectangleData                                   m_data;

        Hyprutils::Math::CBox                            m_position;
    };
};
