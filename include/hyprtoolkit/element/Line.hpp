#pragma once

#include "Element.hpp"
#include "../types/SizeType.hpp"
#include "../palette/Color.hpp"

#include <hyprutils/memory/UniquePtr.hpp>

namespace Hyprtoolkit {

    struct SLineImpl;
    struct SLineData;
    class CLineElement;

    class CLineBuilder {
      public:
        ~CLineBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CLineBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CLineBuilder>        color(colorFn&&);
        Hyprutils::Memory::CSharedPointer<CLineBuilder>        thick(int);
        Hyprutils::Memory::CSharedPointer<CLineBuilder>        points(std::vector<Hyprutils::Math::Vector2D>&&); // [0, 0] - [1, 1]
        Hyprutils::Memory::CSharedPointer<CLineBuilder>        size(CDynamicSize&&);

        Hyprutils::Memory::CSharedPointer<CLineElement>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CLineBuilder> m_self;
        Hyprutils::Memory::CUniquePointer<SLineData>  m_data;
        Hyprutils::Memory::CWeakPointer<CLineElement> m_element;

        CLineBuilder() = default;

        friend class CLineElement;
    };

    class CLineElement : public IElement {
      public:
        virtual ~CLineElement() = default;

        Hyprutils::Memory::CSharedPointer<CLineBuilder> rebuild();
        virtual Hyprutils::Math::Vector2D               size();

      private:
        static Hyprutils::Memory::CSharedPointer<CLineElement> create(const SLineData& data);
        CLineElement(const SLineData& data);

        void                                             replaceData(const SLineData& data);

        virtual void                                     paint();
        virtual void                                     reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual bool                                     positioningDependsOnChild();
        virtual void                                     recheckColor();

        Hyprutils::Memory::CUniquePointer<SLineImpl>     m_impl;

        friend class CLineBuilder;
    };
};
