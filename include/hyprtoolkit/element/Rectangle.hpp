#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SRectangleImpl;
    struct SRectangleData;
    class CRectangleElement;

    class CRectangleBuilder {
      public:
        ~CRectangleBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CRectangleBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CRectangleBuilder>        color(colorFn&&);
        Hyprutils::Memory::CSharedPointer<CRectangleBuilder>        borderColor(colorFn&&);
        Hyprutils::Memory::CSharedPointer<CRectangleBuilder>        rounding(int);
        Hyprutils::Memory::CSharedPointer<CRectangleBuilder>        borderThickness(int);
        Hyprutils::Memory::CSharedPointer<CRectangleBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CRectangleElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CRectangleBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SRectangleData>  m_data;
        Hyprutils::Memory::CWeakPointer<CRectangleElement> m_element;

        CRectangleBuilder() = default;

        friend class CRectangleElement;
    };

    class CRectangleElement : public IElement {
      public:
        virtual ~CRectangleElement() = default;

        Hyprutils::Memory::CSharedPointer<CRectangleBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D                 size();

      private:
        static Hyprutils::Memory::CSharedPointer<CRectangleElement> create(const SRectangleData& data);
        CRectangleElement(const SRectangleData& data);

        void replaceData(const SRectangleData& data);

        virtual void                                      paint();
        virtual void                                      reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D>  preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D>  maximumSize(const Hyprutils::Math::Vector2D& parent);

        virtual void                                      recheckColor();

        Hyprutils::Memory::CUniquePointer<SRectangleImpl> m_impl;

        friend class CRectangleBuilder;
    };
};
