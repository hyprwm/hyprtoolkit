#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"

#include <hyprgraphics/color/Color.hpp>
#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SRectangleImpl;

    struct SRectangleData {
        Hyprgraphics::CColor color           = Hyprgraphics::CColor{{.r = 1.F, .g = 1.F, .b = 1.F}};
        float                a               = 1.F;
        int                  rounding        = 0;
        Hyprgraphics::CColor borderColor     = Hyprgraphics::CColor{{.r = 1.F, .g = 1.F, .b = 1.F}};
        int                  borderThickness = 0;
        CDynamicSize         size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
    };

    class CRectangleElement : public IElement {
      public:
        static Hyprutils::Memory::CSharedPointer<CRectangleElement> create(const SRectangleData& data = {});
        virtual ~CRectangleElement() = default;

        SRectangleData dataCopy();
        void           replaceData(const SRectangleData& data);

      private:
        CRectangleElement(const SRectangleData& data);

        virtual void                                      paint();
        virtual void                                      reposition(const Hyprutils::Math::CBox& box);
        virtual Hyprutils::Math::Vector2D                 size();
        virtual std::optional<Hyprutils::Math::Vector2D>  preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  maximumSize(const Hyprutils::Math::Vector2D& parent);

        Hyprutils::Memory::CUniquePointer<SRectangleImpl> m_impl;
    };
};
