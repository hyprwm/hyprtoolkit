#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>

namespace Hyprtoolkit {

    struct SRectangleData {
        Hyprgraphics::CColor color    = Hyprgraphics::CColor{{.r = 1.F, .g = 1.F, .b = 1.F}};
        float                a        = 1.F;
        int                  rounding = 0;
        CDynamicSize         size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CRectangleElement : public IElement {
      public:
        CRectangleElement(const SRectangleData& data = {});
        virtual ~CRectangleElement() = default;

      private:
        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                size();
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);

        SRectangleData                                   m_data;

        Hyprutils::Math::CBox                            m_position;
    };
};
