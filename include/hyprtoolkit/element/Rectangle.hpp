#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SRectangleImpl;

    struct SRectangleData {
        colorFn      color           = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        int          rounding        = 0;
        colorFn      borderColor     = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        int          borderThickness = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
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
        virtual void                                      reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual Hyprutils::Math::Vector2D                 size();
        virtual std::optional<Hyprutils::Math::Vector2D>  preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  maximumSize(const Hyprutils::Math::Vector2D& parent);

        virtual void                                      recheckColor();

        Hyprutils::Memory::CUniquePointer<SRectangleImpl> m_impl;
    };
};
