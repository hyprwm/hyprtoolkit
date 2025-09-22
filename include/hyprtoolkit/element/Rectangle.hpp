#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SRectangleImpl;

    struct SRectangleData {
        CHyprColor   color           = CHyprColor{1.F, 1.F, 1.F, 1.F};
        int          rounding        = 0;
        CHyprColor   borderColor     = CHyprColor{1.F, 1.F, 1.F, 1.F};
        int          borderThickness = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {}}; // 0,0 means no size, automatic, fits parent
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
